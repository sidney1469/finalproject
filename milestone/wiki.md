# Project Title
Wheat Weywot | Tracking Aircraft for a Sustainable future for Brisbane

# Team Member List and Roles
## Sidney Neil (Wheat Wolf)
Sidney will work on controlling the Pan and Tilt servo and developing the web dashboard in which will display aircraft utilising the data aquired from the nodes.
## Fiachra Richards
## Xander Akison
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
The web dashboard should be able to plot aircraft and user (Base Node) position, with no loss of data as recieved from the base. It should be able to plot at least 30 planes at once, and store 60 seconds of plane data for each plane.
### 2. Dashboard to Actuator Communication
User should be able to select and toggle to a plane on the dashboard and have the servo reflect the planes position within a 3 second window.
### 3. Actuator Accuracy
The servo be able to update position to point at the chosen plane no less then once per second, to within 15 degrees of precision.

### 4. Linked List
The Base node should store data for  30 planes as received from the receiver node in a linked list.

### 5. System runtime & Memory Management
Memory will be dynamically managed across the 3 nodes, such that the system can run for at least 10 minutes with no issues.

### 6: Maintain Good Code Style and Coding Practices
Code will reflect the Google C++ Style Guide and code will be sufficiently commented.

## System Overview

### Block Diagram

## System/Sensor Integration

## Wireless Network Communications

## Algorithms
# Equipment
