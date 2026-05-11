# Project Title

Tracking Aircraft for a Sustainable Future for Brisbane

# Team Member List and Roles

Sidney Neil, Fiachra Richards, Xander Akison

# Project Overview/Scenario

## Project Description

### The Problem

Brisbane is among the fastest growing populations in the world with a current population of approximately 2.5 million people and counting.
2032 Olympic Games
Additionally with the upcoming 2032 Olympic games the city's population is going to grow exponentially.

### Why is this important?

More people means more housing, more planes overhead and more noise. Accompanying this, the more people consequently means the more urban areas are stretched into industrial areas or outer city areas. Creating a domino effect of the following:

- Longer Travel
- Busier Roads
- Houses in high noise areas

Exposing people to health risks of more gas emittions from both cars and factories, Reduced Sleep and more.

#### We Believe Brisbane _deserves better_.

While we know we can’t in this project stop the development of houses and urbanization of industrial areas. We believe every single bit of info helps which is why we have produced the solution which can help Brisbanians create informed decisions on their housing like never before.

## Deliverables & Key Performance Indicators

## System Overview

### Block Diagram

![](block_diagram.png)

## System/Sensor Integration

## Wireless Network Communications

### Raspberry Pi

The raspberry pi is responsible for receiving the information from the RTL2832U transponder using Dump1090, once processed, the relevant plane information will be sent via serial in a json format to the zephyr based receiver node. The json format will be the following.

    {
    "plane_count": int,
    "planes": [
        {
        "hex": str,
        "lat": float,
        "long": float,
        "head": float,
        "velo": float
        },
        ...
    ]}

### Receiver Node (xiao seed board)

The receiver node is responsible for the following:

- Reading GPS data from the SAM-M8Q (via UART pins)
- Receiving plane data from the raspberry pi (via USB-C) // Also power
- Sending new json packet with both sets of data

The SAM-M8Q continuously sends GPS data over UART in the form

    $GNGGA,UTC,LATITUDE,N,LONGITUDE,E,QUALITY,SATELLITE_COUNT,HDOP,ALTITUDE,M,CHECKSUM

a polling function will read the current value and package the data with the plane data from the raspberry pi.

The packaged packet will be transmitted via BLE to the base node, this will use bluetooth nus to communicate with the other zephyr based xiao board. The packet will be formatted in json with the following format.

    { "plane_count": int,
    "planes": [
    {"hex": str, "lat": float,
    "long": float, "head": float,
    "velo": float},
    ...
    ], "position": [
    {"lat": float, "long",
    float, "alt": float }]}

![](message_protocol_diagram.png)

## Algorithms

## DIKW Pyramid Abstraction

## Project Software/Hardware management

## Zephyr RTOS Advanced Libraries and Kernel Features

# Equipment
