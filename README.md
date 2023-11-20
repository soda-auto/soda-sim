# SodaSim  

SodaSim is an OpenSource vehicle simulator built to test and verify vehicles, robots and their various internal systems throughout the entire development lifecycle.
SodaSim is a plugin for UnrealEngine that turns UnrealEngine into a vehicle simulator tool.
![SodaSim](Docs/img/intro.jpg)

# Base Features
  
## Vehicle Digital Twin
This is a "component based"  methodology where a vehicle is perceived as an assembly of small, autonomous units termed "components". 
These components can signify actual devices such as lidar or transmission systems, or embody algorithms, such as Advanced Driver Assistance (ADS).

Leveraging this approach, you can swiftly construct a vehicle model by merging pre-defined, fully operational components, 
and by crafting bespoke components exclusively for the segments of the vehicle you are focusing on.

Furthermore, this component-centric method facilitates a seamless transition between Software-in-the-Loop (SIL) and Hardware-in-the-Loop (HIL), 
provided that the Sim is amalgamated with an appropriate HIL device. 
This transition can be accomplished by merely substituting a virtual component with a physical counterpart, 
negating the need to modify other sectors of the vehicle model.

## Scenario Design and Editing
The Soda Editor Mode allow to add and edit specail types of actors (Scenario Actors and Soda Vehicles) on a level. 
This could be path trajectories, pedestrians, cars or even the weather. There is one special type of actor [Scenario Action](https://www.notion.so/Scenario-Action-TODO-576a948a2d1645c7bcc6ee672d752596). 
It is a simple visual programming language that allows you to create simple scenarios.

## Other
There are also a huge number of auxiliary capabilities, for example, recording a dataset, CI/CD and Automated Testing, integrated DB and other.
Not all features are available in the free version. 
Some featers avalible only in privat zone by subscribe, more details about this see [here]().  
Full list of features see [here](https://www.notion.so/Introduction-2176a979629f454c82091fe0f14de3f8).  

# Docs
All documentation is [here](https://www.notion.so/SODA-Sim-7cfab234b91b4b37b01969d79af4b41e)  
Qick start [here](https://www.notion.so/Quick-Start-54c987fd269f4770b43fce4a53dd5f90)  
C++ API [here]()

# Roadmap

* Integration with [Project Chrono](https://projectchrono.org/) for accurate vehicle physics simulation.
* Analog/Digital Input/Output hardware interface. 
We have almost finished developing our own hardware, which will allow virtual Analog/Digital Input/Output to be mapped to real hardware for HIL purpose.
* Realtime Python Scripts. Support [Unreal Editor Python](https://docs.unrealengine.com/5.2/en-US/scripting-the-unreal-editor-using-python/) to work in game mode.
* Simulink Model Importing. Ability to develop vehicle components using Simulinks and import them into the simulator.
* LIN interfce support.
* City Traffic Generation based on the Summo.
* Support of the OpenScenario.
* Distributed simulation. Horisontal parallelization of simulation on multiple computers to simulate more vehicle sensors.
* Integration with ROS/ROS2.

# Contact
Please feel free to provide feedback or ask questions by creating a Github issue. For inquiries about collaboration, please email us at ivan@soda.com.

# Copyright and License
Copyright © 2023 SODA.AUTO UK LTD. ALL RIGHTS RESERVED.  
This software contains code licensed as described in LICENSE.  
