#!/usr/bin/env python3
"""
ads_b.py

Reads SBS-format ADS-B data from dump1090 on localhost:30003,
aggregates per-aircraft state, and forwards compact CSV packets
to a serial device (e.g. nRF52840 on /dev/ttyACM0).

"""

import socket
import serial
import sys
import time
from collections import defaultdict

# DUMP1090 Configurations
DUMP1090_HOST = "127.0.0.1"
DUMP1090_PORT = 30003

# UART configurations
SERIAL_PORT   = "/dev/ttyACM0"
BAUD_RATE     = 115200

# Timing
STALE_TIMEOUT = 60      # seconds before removing an aircraft from state
RECONNECT_DELAY = 5     # seconds to wait before reconnecting on error

# SBS-1 field indices (0-based)
SBS_MSG_TYPE = 1
SBS_ICAO = 4
SBS_TIME = 7
SBS_ALT  = 11
SBS_SPEED = 12
SBS_HEADING = 13
SBS_LAT = 14
SBS_LON = 15
SBS_VERT_RATE = 16

# SBS-1 Message Types
ES_IDENT_AND_CATEGORY = "1" # Contains aircraft callsign and Category.
ES_SURFACE_POS = "2" # Contains surface position data, ground speed, while the aircraft is on the ground.
ES_AIRBORNE_POS = "3" # Contains airborne position data, altitude, and an emergency/spi (Special Position Indicator) flag.
ES_AIRBORNE_VEL = "4" # Contains airborne velocity data, aircraft heading, track angle, and vertical climb/sink rate.
SURFACE_POS_ALT = "5" # Contains surface position data and altitude information.
AG_STATUS = "6" # Contains air-to-ground status messages detailing airborne and ground states.
AIR_ALT = "7" # Contains altitude data and squawk codes transmitted from air interrogations.
AIR_IDENT = "8" # Contains the aircraft identity and callsign from air interrogations.

# Required fields to emit a packet
REQUIRED = ("alt", "lat", "lon", "spd", "tim")


def log(msg):
    print(f"[adsb_serial] {msg}", file=sys.stderr)


def parse_sbs(line):
    """
    Parse a single SBS-1 CSV line.
    Returns (icao, updates_dict) or (None, None) if not useful.
    """
    fields = line.strip().split(",")

    if len(fields) < 17 or fields[0] != "MSG":
        return None, None

    icao = fields[SBS_ICAO].strip().upper()
    if not icao:
        return None, None

    msg_type = fields[SBS_MSG_TYPE].strip()
    updates = {}

    if msg_type == ES_AIRBORNE_POS:
        # Position message
        if fields[SBS_ALT].strip():
            updates["alt"] = fields[SBS_ALT].strip()
        if fields[SBS_LAT].strip() and fields[SBS_LON].strip():
            updates["lat"] = fields[SBS_LAT].strip()
            updates["lon"] = fields[SBS_LON].strip()
        if fields[SBS_TIME].strip():
            updates["tim"] = fields[SBS_TIME].strip()

    elif msg_type == ES_AIRBORNE_VEL:
        # Velocity message
        if fields[SBS_SPEED].strip():
            updates["spd"] = fields[SBS_SPEED].strip()
        if fields[SBS_HEADING].strip():
            updates["hdg"] = fields[SBS_HEADING].strip()
        if fields[SBS_VERT_RATE].strip():
            updates["vrt"] = fields[SBS_VERT_RATE].strip()
        if fields[SBS_TIME].strip():
            updates["tim"] = fields[SBS_TIME].strip()

    elif msg_type == SURFACE_POS_ALT or msg_type == AIR_ALT:
        # Altitude update
        if fields[SBS_ALT].strip():
            updates["alt"] = fields[SBS_ALT].strip()
        if fields[SBS_TIME].strip():
            updates["tim"] = fields[SBS_TIME].strip()

    if not updates:
        return None, None

    return icao, updates


def format_packet(icao, state):
    """
    Format a packet from aircraft state.
    Returns None if required fields are missing.
    """
    for field in REQUIRED:
        if field not in state:
            return None

    alt = state["alt"]
    lat = state["lat"]
    lon = state["lon"]
    spd = state["spd"]
    tim = state["tim"]

    # Truncate lat/lon to 5 decimal places to save bytes
    try:
        lat = f"{float(lat):.5f}"
        lon = f"{float(lon):.5f}"
    except ValueError:
        return None

    return f"{icao},{alt},{lat},{lon},{spd},{tim}\n"


def open_serial():
    """Open serial port, retrying on failure."""
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
            log(f"Opened serial port {SERIAL_PORT} at {BAUD_RATE} baud")
            return ser
        except serial.SerialException as e:
            log(f"Failed to open serial port: {e} — retrying in {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)


def open_dump1090():
    """Connect to dump1090, retrying on failure."""
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect((DUMP1090_HOST, DUMP1090_PORT))
            sock.settimeout(10)
            log(f"Connected to dump1090 at {DUMP1090_HOST}:{DUMP1090_PORT}")
            return sock
        except (socket.error, ConnectionRefusedError) as e:
            log(f"Failed to connect to dump1090: {e} — retrying in {RECONNECT_DELAY}s")
            time.sleep(RECONNECT_DELAY)


def main():
    # aircraft_state[icao] = { "alt": ..., "lat": ..., ... }
    aircraft_state = defaultdict(dict)
    # aircraft_seen[icao] = last seen timestamp
    aircraft_seen = {}

    ser = open_serial()
    sock = open_dump1090()
    buf = ""

    log("Running — forwarding ADS-B data to serial")

    while True:
        try:
            # Read data from dump1090
            try:
                data = sock.recv(4096).decode("ascii", errors="ignore")
            except socket.timeout:
                data = ""
            except (socket.error, OSError) as e:
                log(f"dump1090 connection lost: {e} — reconnecting")
                sock.close()
                sock = open_dump1090()
                buf = ""
                continue

            if not data:
                # Connection closed by dump1090
                log("dump1090 closed connection — reconnecting")
                sock.close()
                sock = open_dump1090()
                buf = ""
                continue

            buf += data

            # Process complete lines
            while "\n" in buf:
                line, buf = buf.split("\n", 1)
                line = line.strip()
                if not line:
                    # continue

                icao, updates = parse_sbs(line)
                if icao is None:
                    # continue

                # Update aircraft state
                #aircraft_state[icao].update(updates)
                #aircraft_seen[icao] = time.time()

                # Try to emit a packet
                #packet = format_packet(icao, aircraft_state[icao])
                #if packet is None:
                    #continue
                packet = "7C39F6,3675,-27.44316,153.15009,151,120,19:18:02.833\n"

                try:
                    ser.write(packet.encode("ascii"))
                except serial.SerialException as e:
                    log(f"Serial write error: {e} — reconnecting serial")
                    ser.close()
                    ser = open_serial()

            # Flush stale aircraft
            now = time.time()
            stale = [icao for icao, t in aircraft_seen.items()
                     if now - t > STALE_TIMEOUT]
            for icao in stale:
                del aircraft_state[icao]
                del aircraft_seen[icao]
                log(f"Flushed stale aircraft {icao}")

        except KeyboardInterrupt:
            log("Interrupted — shutting down")
            sock.close()
            ser.close()
            sys.exit(0)


if __name__ == "__main__":
    main()
