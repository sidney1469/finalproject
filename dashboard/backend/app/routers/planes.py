from fastapi import APIRouter
from pydantic import BaseModel, EmailStr, PositiveInt
import random
import psycopg
import h3
from collections import defaultdict, deque
from math import atan2

router = APIRouter(prefix="/planes", tags=['planes'])
conn_info = "dbname=wheat_weywot user=postgres password=... host=localhost"

class Plane(BaseModel):
    flightName: str
    long: float
    lat: float

class Coords(BaseModel):
    long: float
    lat: float

def get_coords() -> list[Coords]:
    try:
        with psycopg.connect(conn_info) as conn:
            with conn.cursor() as cur:
                select_query = "SELECT id, flightname, long, lat FROM flights;"
                cur.execute(select_query)

                rows = cur.fetchall()
                coord_list = []

                for row in rows:
                    coord_list.append(Coords(long=row[2], lat=row[3]))

                return coord_list

    except Exception as e:
        print(f"An error occurred: {e}")
        return []


def cross(o: Coords, a: Coords, b: Coords) -> float:
    """
    2D cross product of OA and OB.
    Positive = counter-clockwise turn.
    Negative = clockwise turn.
    Zero = collinear.
    """
    return (
        (a.long - o.long) * (b.lat - o.lat)
        - (a.lat - o.lat) * (b.long - o.long)
    )


def distance_sq(a: Coords, b: Coords) -> float:
    return (a.long - b.long) ** 2 + (a.lat - b.lat) ** 2


def graham_scan(points: list[Coords]) -> list[Coords]:
    """
    Returns the convex hull of a group of points.
    Uses long as x and lat as y.
    """

    # Remove duplicate coordinates
    unique_points = list({
        (p.long, p.lat): p
        for p in points
    }.values())

    if len(unique_points) < 3:
        return unique_points

    # Pick lowest point, then left-most if tie
    start = min(unique_points, key=lambda p: (p.lat, p.long))

    # Sort by angle around start
    sorted_points = sorted(
        unique_points,
        key=lambda p: (
            atan2(p.lat - start.lat, p.long - start.long),
            distance_sq(start, p)
        )
    )

    hull: list[Coords] = []

    for point in sorted_points:
        while len(hull) >= 2 and cross(hull[-2], hull[-1], point) <= 0:
            hull.pop()

        hull.append(point)

    return hull


def cluster_h3_cells(
    cell_to_coords: dict[str, list[Coords]],
    max_gap: int = 3
) -> list[list[str]]:
    """
    Groups H3 cells into clusters.

    If two occupied cells are within max_gap H3 hexagons,
    they belong to the same cluster.
    """

    occupied_cells = set(cell_to_coords.keys())
    visited = set()
    clusters: list[list[str]] = []

    for start_cell in occupied_cells:
        if start_cell in visited:
            continue

        cluster = []
        queue = deque([start_cell])
        visited.add(start_cell)

        while queue:
            current_cell = queue.popleft()
            cluster.append(current_cell)

            # All cells within max_gap hexes
            nearby_cells = set(h3.grid_disk(current_cell, max_gap))

            # Only care about cells that actually contain planes
            nearby_occupied_cells = nearby_cells.intersection(occupied_cells)

            for next_cell in nearby_occupied_cells:
                if next_cell not in visited:
                    visited.add(next_cell)
                    queue.append(next_cell)

        clusters.append(cluster)

    return clusters


@router.get("/coords", response_model=list[list[Coords]])
def get_polygon_coords():
    coords = get_coords()

    res = 7
    max_hex_gap = 3
    min_points_per_polygon = 3

    cell_to_coords: dict[str, list[Coords]] = defaultdict(list)

    # 1. Put every coordinate into an H3 cell
    for coord in coords:
        cell = h3.latlng_to_cell(coord.lat, coord.long, res)
        cell_to_coords[cell].append(coord)

    # 2. Split into separate H3 clusters
    h3_clusters = cluster_h3_cells(
        cell_to_coords=cell_to_coords,
        max_gap=max_hex_gap
    )

    polygons: list[list[Coords]] = []

    # 3. For each H3 cluster, collect points and Graham scan them
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


def insert_planes(planes: list[Plane]) -> None:
    planes_data = [(p.flightName, p.long, p.lat) for p in planes]

    try:
        with psycopg.connect(conn_info) as conn:
            with conn.cursor() as cur:
                insert_query = """
                    INSERT INTO flights (flightname, long, lat) 
                    VALUES (%s, %s, %s);
                """
                cur.executemany(insert_query, planes_data)
                
                conn.commit()
                print(f"Successfully added {len(planes)} flights!")
    except Exception as e:
        print(f"An error occurred: {e}")

long1 = 0.22
lat1 = -0.33
long2 = 1.22
lat2 = -1.33

@router.get("/list")
def get_planes_list():
    global long1, lat1, long2, lat2

    long1 += random.uniform(-1, 1)
    lat1 += random.uniform(-0.1, 0.1)
    long2 -= random.uniform(-0.1, 0.1)
    lat2 += random.uniform(-0.1, 0.1)

    planes = [
        Plane(flightName="ABCDEF", long=long1, lat=lat1),
        Plane(flightName="LKJHGD", long=long2, lat=lat2),
    ]

    insert_planes(planes)

    return planes



@router.get("/")
def planes_health():
    return {200: 'ok'}
