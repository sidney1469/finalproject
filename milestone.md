# Sensors

SAM-M8Q - GPS locator for base node https://www.sparkfun.com/sparkfun-gps-breakout-chip-antenna-sam-m8q-qwiic.html
RTL2832U - Transponder https://www.realtek.com/Product/Index?id=615

# Actuators

Pan & Tilt Servos https://www.sparkfun.com/pan-tilt-bracket-kit-single-attachment.html

# Node Connections

    1. RTL2832U Connected to PC1 (OR PC1 is raspberry pi, remove base node)
    2. PC1 Connected to BASE NODE (OR PC1 is raspberry pi, remove base node)
    3. SAM-M8Q connected to BASE NODE (or raspberry pi) using UART pins
    4. PC NODE Connected to PC2
    5. PC2 Web Dashboard
    6. ACTUATOR NODE Connected to power (could be a battery bank or PC3)
    7. ACTUATOR NODE Connected to Pan & Tilt Servos

# Project Description

# Wireless Network / IoT Protocols

BLE Message Types

## Aircraft Telemetry Packet

1. ICAO
2. Lat, Lon, Alt
3. Heading
4. Timestamp

## Node 3 GPS Packet

1. Node latitude
2. Node longitude
3. Fix quality

## Actuator Command Packet

1. Selected ICAO
2. Target bearing
3. Optional elevation angle

# DIKW Pyramid

## Data

Raw ADS‑B frames

Raw GPS NMEA sentences

## Information

Aircraft positions

Node positions

## Knowledge

Real‑time relative geometry between aircraft and node

Aircraft movement patterns

## Wisdom

System autonomously points to aircraft of interest

Enables situational awareness and spatial understanding
