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

# Project Roles

## Base Node

1. Create BLE central for connection with Receiver node
2. Create BLE central for connection with Actuator node
3. Parse incoming BLE aircraft data stream
4. Maintain in-memory aircraft state database
5. Implement Kalman filter for aircraft position smoothing
6. Compute relative geometry (bearing, distance, optional elevation)
7. Allow aircraft selection input from PC dashboard
8. Track selected aircraft state continuously
9. Calculate servo angle / direction to target aircraft
10. Send control commands to actuator node via BLE
11. Manage system state (current target, tracking status)

## Receiver Node

1. ADS-B decode using RTL-SDR (RTL2832U pipeline)
2. Extract aircraft fields (ICAO, lat, lon, alt, speed, heading)
3. Apply lightweight filtering (remove invalid / duplicate messages)
4. Limit aircraft dataset (e.g. top N by signal strength or proximity)
5. SAM-M8Q GPS driver integration (position + time sync)
6. Format aircraft data into BLE-ready packets (JSON or binary struct)
7. Create BLE peripheral (NUS or custom GATT service)
8. Broadcast aircraft updates at fixed interval (e.g. 1 Hz)
9. Handle BLE connection stability and packet timing

## Actuator Node

1. Create BLE peripheral (control interface)
2. Define BLE service for receiving actuator commands
3. Parse incoming control packets (angle / direction)
4. Implement servo PWM driver
5. Smooth servo movement (rate limiting / interpolation)
6. Add fail-safe behaviour on BLE disconnect
7. Clamp servo limits to safe mechanical range
8. Optional: status feedback (current angle / health)

## Dashboard
1. Build web dashboard UI (map-based aircraft display)
2. Visualise real-time aircraft positions
3. Display selected target aircraft tracking path
4. Connect to Base Node (BLE or WebSocket interface)
5. Allow user selection of target aircraft
6. Send selected aircraft ID to Base Node
7. Store historical aircraft data (optional logging system)
8. Display system debug info (Kalman output, signal quality)
9. Show connection status of all nodes
10. Optional: playback mode for recorded flights

## Xander — Radio + Data Handling
### Phase 1:
1. ADS-B decode using RTL-SDR (RTL2832U pipeline)
2. Extract aircraft fields (ICAO, lat, lon, alt, speed, heading)
### Phase 2:
3. Apply lightweight filtering (remove invalid / duplicate messages)
4. Limit aircraft dataset (e.g. top N by signal strength or proximity)
### Phase 3:
5. SAM-M8Q GPS driver integration (position + time sync)
6. Format aircraft data into BLE-ready packets (JSON or binary struct)
### Phase 4:
7. Create BLE peripheral (NUS or custom GATT service)
8. Broadcast aircraft updates at fixed interval (e.g. 1 Hz)
9. Handle BLE connection stability and packet timing

## Fiachra — Core Intelligence + Control (Base Node)
### Phase 1:
1. Create BLE central for connection with Receiver node
2. Parse incoming BLE aircraft data stream
3. Maintain in-memory aircraft state database
### Phase 2:
4. Implement Kalman filter for aircraft position smoothing
5. Compute relative geometry (bearing, distance, optional elevation)
6. Track selected aircraft state continuously
### Phase 3:
7. Allow aircraft selection input from PC dashboard
8. Calculate servo angle / direction to target aircraft
9. Send control commands to actuator node via BLE
10. Manage system state (current target, tracking status)
### Phase 4:
11. Create BLE central for connection with Actuator node

## Sidney — Dashboard + Actuation + Integration
### Phase 1:
1. Create BLE peripheral (control interface)
2. Define BLE service for receiving actuator commands
3. Parse incoming control packets (angle / direction)
### Phase 2:
4. Implement servo PWM driver
5. Clamp servo limits to safe mechanical range
6. Smooth servo movement (rate limiting / interpolation)
7. Add fail-safe behaviour on BLE disconnect
8. Optional: status feedback (current angle / health)
### Phase 3:
9. Build web dashboard UI (map-based aircraft display)
10. Visualise real-time aircraft positions
11. Show connection status of all nodes
### Phase 4:
12. Allow user selection of target aircraft
13. Send selected aircraft ID to Base Node
14. Display selected target aircraft tracking path
### Phase 5:
15. Display system debug info (Kalman output, signal quality)
16. Store historical aircraft data (optional logging system)
17. Playback mode for recorded flights
