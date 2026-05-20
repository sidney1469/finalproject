import asyncio
import json
import time
from collections import defaultdict, deque
from contextlib import asynccontextmanager
from math import atan2

import logging

logger = logging.getLogger("uvicorn.error")

import psycopg
from fastapi import APIRouter, FastAPI
from pydantic import BaseModel
from serial import Serial, SerialException
from serial.threaded import LineReader, ReaderThread

import random
import string

def _random_flight_name() -> str:
    letters = ''.join(random.choices(string.ascii_uppercase, k=3))
    digits  = ''.join(random.choices(string.digits, k=3))
    return f"{letters}{digits}"

# ---------------------------------------------------------------------------
# Config
# ---------------------------------------------------------------------------

PORT = "/dev/ttyACM0"
BAUD = 115200
POLL_INTERVAL = 1.0  # seconds between plane print commands

conn_info = "dbname=wheat_weywot user=postgres password=... host=localhost"

# ---------------------------------------------------------------------------
# Models
# ---------------------------------------------------------------------------

class Plane(BaseModel):
    flightName: str
    long: float
    lat: float

class Coords(BaseModel):
    long: float
    lat: float

# ---------------------------------------------------------------------------
# DB helpers
# ---------------------------------------------------------------------------

def get_coords() -> list[Coords]:
    try:
        with psycopg.connect(conn_info) as conn:
            with conn.cursor() as cur:
                cur.execute("SELECT long, lat FROM flights;")
                return [Coords(long=row[0], lat=row[1]) for row in cur.fetchall()]
    except Exception as e:
        print(f"[db] get_coords error: {e}")
        return []

# ---------------------------------------------------------------------------
# Serial reader
# ---------------------------------------------------------------------------

def parse_plane_json(raw: str) -> list[Plane]:
    """
    Parse the JSON emitted by `plane print`.
    Expected format: [{"f":"AAA000","lo":92.1,"la":83.2,"ll":94.1,"lt":81.3}, ...]
    Strips shell echo, prompt noise, and partial lines defensively.
    """
    raw = raw.strip()

    # Find the outermost [...] in case there's shell echo prefix
    start = raw.find("[")
    end   = raw.rfind("]")

    if start == -1 or end == -1 or end <= start:
        return []

    try:
        entries = json.loads(raw[start:end + 1])
    except json.JSONDecodeError as e:
        print(f"[serial] JSON parse error: {e}")
        return []

    planes = []
    for entry in entries:
        try:
            planes.append(Plane(
                flightName=entry["f"],
                long=entry["lo"],
                lat=entry["la"],
            ))
        except (KeyError, ValueError) as e:
            print(f"[serial] bad entry {entry}: {e}")

    return planes



# ---------------------------------------------------------------------------
# Router
# ---------------------------------------------------------------------------

router = APIRouter(prefix="/planes", tags=["planes"])

# ---------------------------------------------------------------------------
# Geometry helpers (unchanged)
# ---------------------------------------------------------------------------

def cross(o: Coords, a: Coords, b: Coords) -> float:
    return (
        (a.long - o.long) * (b.lat - o.lat)
        - (a.lat - o.lat) * (b.long - o.long)
    )


def distance_sq(a: Coords, b: Coords) -> float:
    return (a.long - b.long) ** 2 + (a.lat - b.lat) ** 2


def graham_scan(points: list[Coords]) -> list[Coords]:
    unique_points = list({(p.long, p.lat): p for p in points}.values())

    if len(unique_points) < 3:
        return unique_points

    start = min(unique_points, key=lambda p: (p.lat, p.long))

    sorted_points = sorted(
        unique_points,
        key=lambda p: (
            atan2(p.lat - start.lat, p.long - start.long),
            distance_sq(start, p),
        ),
    )

    hull: list[Coords] = []

    for point in sorted_points:
        while len(hull) >= 2 and cross(hull[-2], hull[-1], point) <= 0:
            hull.pop()
        hull.append(point)

    return hull


def cluster_h3_cells(
    cell_to_coords: dict[str, list[Coords]],
    max_gap: int = 3,
) -> list[list[str]]:
    import h3

    occupied_cells = set(cell_to_coords.keys())
    visited: set[str] = set()
    clusters: list[list[str]] = []

    for start_cell in occupied_cells:
        if start_cell in visited:
            continue

        cluster: list[str] = []
        queue: deque[str] = deque([start_cell])
        visited.add(start_cell)

        while queue:
            current_cell = queue.popleft()
            cluster.append(current_cell)

            nearby_occupied = set(h3.grid_disk(current_cell, max_gap)).intersection(occupied_cells)

            for next_cell in nearby_occupied:
                if next_cell not in visited:
                    visited.add(next_cell)
                    queue.append(next_cell)

        clusters.append(cluster)

    return clusters

# ---------------------------------------------------------------------------
# Endpoints
# ---------------------------------------------------------------------------

@router.get("/coords", response_model=list[list[Coords]])
def get_polygon_coords():
    import h3

    coords = get_coords()

    res = 7
    max_hex_gap = 3
    min_points_per_polygon = 3

    cell_to_coords: dict[str, list[Coords]] = defaultdict(list)

    for coord in coords:
        cell = h3.latlng_to_cell(coord.lat, coord.long, res)
        cell_to_coords[cell].append(coord)

    h3_clusters = cluster_h3_cells(cell_to_coords=cell_to_coords, max_gap=max_hex_gap)

    polygons: list[list[Coords]] = []

    for cluster in h3_clusters:
        cluster_points: list[Coords] = []

        for cell in cluster:
            cluster_points.extend(cell_to_coords[cell])

        if len(cluster_points) < min_points_per_polygon:
            continue

        polygon = graham_scan(cluster_points)

        if len(polygon) >= 3:
            polygons.append(polygon)

    return polygons


@router.get("/list")
def get_planes_list():
    """Returns the most recent batch of planes from the DB."""
    try:
        return insert_planes()
    except Exception as e:
        print(f"[db] get_planes_list error: {e}")
        return []


@router.get("/")
def planes_health():
    return {200: "ok"}

# ---------------------------------------------------------------------------
# Serial
# ---------------------------------------------------------------------------

def parse_plane_json(raw: str | bytes) -> list[Plane]:
    if isinstance(raw, bytes):
        raw = raw.decode(errors="replace")

    raw = raw.strip()

    # Find JSON array. Prefer plane-style JSON: [{"f":...}]
    start = raw.find("[{")
    if start == -1:
        start = raw.find("[]")
    if start == -1:
        start = raw.find("[")

    end = raw.rfind("]")

    if start == -1 or end == -1 or end <= start:
        logger.warning(f"[serial] no JSON array found in: {raw!r}")
        return []

    json_text = raw[start:end + 1]

    try:
        entries = json.loads(json_text)
    except json.JSONDecodeError as e:
        logger.warning(f"[serial] JSON parse error: {e}")
        logger.warning(f"[serial] raw JSON text: {json_text!r}")
        return []

    planes: list[Plane] = []

    for entry in entries:
        try:
            planes.append(
                Plane(
                    flightName=str(entry["f"]),
                    long=float(entry["lo"]),
                    lat=float(entry["la"]),
                )
            )
        except (KeyError, TypeError, ValueError) as e:
            logger.warning(f"[serial] bad entry {entry}: {e}")

    return planes


def read_json_from_serial(port, timeout: float = 2.0) -> str | None:
    """
    Reads serial lines until a JSON array is found or timeout expires.
    Handles noisy Zephyr logs before/after the JSON.
    """
    deadline = time.monotonic() + timeout
    buffer = ""

    while time.monotonic() < deadline:
        line = port.readline()

        if not line:
            continue

        decoded = line.decode(errors="replace").strip()
        logger.info(f"[serial RX] {decoded}")

        buffer += decoded

        # Once we have something that looks like a full JSON array, return it
        if "[{" in buffer and "]" in buffer:
            return buffer

        # Also allow empty JSON array
        if "[]" in buffer:
            return buffer

    return None


def generate_dummy_planes(n: int = 30):
    planes = []

    for _ in range(n):
        flight = (
            ''.join(random.choices(string.ascii_uppercase, k=3)) +
            ''.join(random.choices(string.digits, k=3))
        )

        # Brisbane-ish dummy range
        longitude = round(random.uniform(152.5, 153.5), 4)
        latitude = round(random.uniform(-27.8, -27.1), 4)

        # Base/location point
        loclong = 153.0100
        loclat = -27.4600

        planes.append((flight, longitude, latitude, loclong, loclat))

    return planes

def insert_planes() -> None:
    try:
        with Serial(
            PORT,
            BAUD,
            timeout=0.2,
            write_timeout=1,
            rtscts=False,
            dsrdtr=False,
            xonxoff=False,
        ) as port:

            time.sleep(2)  # board reset / serial settle

            port.reset_input_buffer()
            port.reset_output_buffer()

            port.write(b"plane print\r\n")
            port.flush()

            raw_json = read_json_from_serial(port, timeout=2.0)

            if raw_json is None:
                logger.warning("[serial] no JSON received")
                return

            planes = parse_plane_json(raw_json)

            logger.info(f"[serial] decoded planes: {planes}")
            return planes

    except SerialException as e:
        print(f"Could not open {PORT}: {e}")
        print("Check the board is connected and the port is correct.")

# ---------------------------------------------------------------------------
# Dummy data
# ---------------------------------------------------------------------------

def _random_flight_name() -> str:
    letters = ''.join(random.choices(string.ascii_uppercase, k=3))
    digits  = ''.join(random.choices(string.digits, k=3))
    return f"{letters}{digits}"


def _seed_dummy_data() -> None:
    planes = [
        Plane(
            flightName=_random_flight_name(),
            long=random.uniform(152.9, 153.2),
            lat=random.uniform(-27.6, -27.3),
        )
        for _ in range(30)
    ]
    logger.info(f"[db] seeded {len(planes)} dummy planes")

