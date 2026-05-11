# Project Title

Wheat Weywot | Tracking Aircraft for a Sustainable future for Brisbane

# Team Member List and Roles

## Xander — Wheat Wallaby

### Sensors + Data Handling (Sensor Node)

Xander will focus on embedded code for the Sensor Node, as well as python code for the Raspberry Pi sensor controller. This will require establishing a BLE peripheral connection with the Base Node, and forming packets of plane data to send. The Raspberry Pi will be set up as a sensor controller for the RTL2832U, piping incoming data to the node over UART serial. Data will be decoded, filtered, sorted and packaged to .json. A zephyr dirver will be created for the SAM-M8Q GPS module, and gps data will be sent to the Base Node when required. The Node will ahve to handle package BLE timing, alongside sensor polling for both sensors.

## Fiachra — Wheat Weasel

### Core Intelligence + Control (Base Node)

Fiachra will focus on writing embedded code for the Base Node. This includes establishing two BLE NUS central connections to work with peripheral connections at the Actuator Node, and Sensor Node respectively. The Base Node must also parse incoming .json data, and format it as a min-heap. The Base Node will have to run a Kalman Filter on the incoming data, and compute nearest calculations such as relative position. It should also be able to contiously track positional data for a chosen plane (dashboard). Fiachra will also develop a uart serial communication with the dashboard, with ability to send data to, and recieve data/requests from the dashboard. Finally, the chosen plane (dashboard) position should be communicated to the Actuator Node. The Base Node must also maintain a multi-threaded state machine and manage memory throughout.

## Sidney — Wheat Wolf

### Dashboard + Actuation + Integration

Sidney will handle dashboard design, and integration as well as embedded code for the Actuator Node. This will involve creating a Zephyr driver for the 2-axis servo at the Actuator Node, utilising PWM. Sidney will introduce a BLE NUS central connection to receive positional commands from the Base Node. He will also have to develop the web dashboard UI to display current planes and plane data and allow for user selection of visible planes. It should also be able to visualise the continious flight poth of the chosen plane, and send the selected plane ID to the base node.

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

While we know we can’t in this project stop the development of houses and urbanization of industrial areas. We believe every single bit of info helps which is why we have produced the solution which can help Brisbanites create informed decisions on their housing like never before.

### The Solution

The proposed solution aims to repurpose well paved data for a change.

Utilising the RTL2832U transponder to extract precise coordinates of Aircraft we aim to utilise our own dataset and compare with preexisting data to create a accessible and interactive interface built with every Brisbanite in mind.

More Technically speaking to achieve this - we will have 3 Bluetooth nodes, 2 PC's, a Pan and Tilt Servo, RTL2832U transponder and a SAM-M8Q GPS Locator node.

The Pan and Tilt servo will be utilised to create a accurate connection between the radio and Plane which will be selected on the interface.

#### But How Does This Link to Noise Pollution?

Recent Surveys suggest 70% of people living in areas under a flight path report that they have been affected on a daily basis by aircraft flying overhead to some extent.

Showing that having a highly reliable plane tracking and data aquisition tool would be game changing not only for Brisbanites but for the public health of Australia.

## Deliverables & Key Performance Indicators

### 1. Dashboard Data Vision

The web dashboard should be able to plot aircraft and user (Base Node) position, with no loss of data as received from the base. It should be able to plot at least 30 planes at once, and store 60 seconds of plane data for each plane.

### 2. Dashboard to Actuator Communication

User should be able to select and toggle to a plane on the dashboard and have the servo reflect the planes position within a 3 second window.

### 3. Actuator Accuracy

The servo be able to update position to point at the chosen plane no less then once per second, to within 15 degrees of precision.

### 4. Linked List

The Base node should store data for 30 planes as received from the Sensor Node in a min-heap.

### 5. System runtime & Memory Management

Memory will be dynamically managed across the 3 nodes, such that the system can run for at least 10 minutes with no issues.

### 6: Maintain Good Code Style and Coding Practices

Code will reflect the Google C++ Style Guide and code will be sufficiently commented.

## System Overview

### Block Diagram

![](block_diagram.png)

## System/Sensor Integration

## Wireless Network Communications

### Raspberry Pi

The raspberry pi is responsible for receiving the information from the RTL2832U transponder using Dump1090, once processed, the relevant plane information will be sent via serial in a json format to the zephyr based sensor node. The json format will be the following.

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

### Sensor Node

The sensor node is responsible for the following:

- Reading GPS data from the SAM-M8Q (via UART pins)
- Receiving plane data from the raspberry pi (via USB-C) // Also power
- Sending new json packet with both sets of data

The SAM-M8Q continuously sends GPS data over UART in the form

    $GNGGA,UTC,LATITUDE,N,LONGITUDE,E,QUALITY,SATELLITE_COUNT,HDOP,ALTITUDE,M,CHECKSUM

a polling function will read the current value and package the data with the plane data from the raspberry pi.

The packaged packet will be transmitted via BLE to the base node, this will use BLE NUS to communicate with the other zephyr based xiao board. The packet will be formatted in json with the following format.

    { "plane_count": int,
    "planes": [
    {"hex": str, "lat": float,
    "long": float, "head": float,
    "velo": float},
    ...
    ], "position": [
    {"lat": float, "long",
    float, "alt": float }]}

### Base Node

The base node is responsible for the following:

- Receiving telemetry data from sensor node
- Filtering for closest planes and passing data to the PC for display on the web dashboard
- Calculating the required servo positions and communicating to the Actuator Node

The communication to the Actuator Node will be via bluetooth nus communication where it will send json encoded pitch and yaw values.

    {"pitch": float, "yaw": float}

### Actuator Node

The Actuator Node receives the required pitch and yaw values from the base node and moves the servos using PWM signals connected to GPIO pins.

### Message Protocol Diagram

![](message_protocol_diagram.png)

## Algorithms
### Base Node
- Min-heap - For storing 30 closest planes and details
- Kalman Filter - For smoothing out location data
### Web Interface
- Uber's H3 algorithm - Heat mapping on the interface for mapping hot spots for planes
## DIKW Pyramid Abstraction

![](DIKW_diagram.png)

## Project Software/Hardware management

## Zephyr RTOS Advanced Libraries and Kernel Features

# Equipment
