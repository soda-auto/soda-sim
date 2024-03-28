# SODA.Sim  

[SODA.Sim](https://soda.auto/products/sim/index.html) is designed for seamless software validation of any vehicle, covering the entire process from initial concept through certification to aftermarket updates.

By simulating vehicles at the level of atomic components, such as sensors, vehicle systems, and electronic control units SODA.Sim is not only a perfect tool for AD/ADAS development, but also a virtual proving ground for validation and certification of all vehicle functions.

SODA.Sim is empowered by SODA Validation Library. You can choose available tests or craft your bespoke scenarios in real-time. Be the first to access SODA Validation Library with a built-in AI scenario generator and AI scenario mixer.  

Now we support ***UnrealEngine 5.3***.

![SodaSim](Docs/img/intro.jpg)

Full list of features see [here](https://docs.soda.auto/projects/soda-sim/en/latest/Introduction.html).  

## Docs
All documentation is [here](https://docs.soda.auto/projects/soda-sim)  
Create a new UProject with SODA.Sim is [here](https://docs.soda.auto/projects/soda-sim/en/latest/How_To/Setup_a_New_UProject.html)  
Quick start [here](https://docs.soda.auto/projects/soda-sim/en/latest/How_To/Quick_Start.html)  

## Install

This repository is plugin for UnrealEngine. You can clone this repo to the UnrealEngine's plugins folder or to the project plugins folders. Then you need to follow the next steps: [Setup a New UProject](https://docs.soda.auto/projects/soda-sim/en/latest/How_To/Setup_a_New_UProject.html)

> [!NOTE]
> * Keep in mind this repo contain a submodule [SodaSimProto](https://github.com/soda-auto/SodaSimProto).  Make sure you clone the submodule as well.
> * The repose includes LFS files. Make sure you clone LFS files as well.

See more information about [Working with Plugins in Unreal Engine](https://docs.unrealengine.com/5.0/en-US/working-with-plugins-in-unreal-engine/).

## Supported OS
* Windows
* Linux (not tested)

## Ecosystem
* [soda-sim-ros2](https://github.com/soda-auto/soda-sim-ros2) - support ROS2 capabilities for the SODA.Sim
* [soda-sim-ros2-ws](https://github.com/soda-auto/soda-sim-ros2-ws) - scripts for build ROS2 for [SODA.Sim ROS2](https://github.com/soda-auto/soda-sim-ros2) for Windows and Linux.
* [soda-sim-proto-v1](https://github.com/soda-auto/soda-sim-proto-v1) - implementation of the generic snesors messages serialization for the SodaSim.
* [soda-sim-remote-ctr](https://github.com/soda-auto/soda-sim-remote-ctrl) - HTTP Python client for [Remote Control](https://docs.unrealengine.com/5.3/en-US/remote-control-for-unreal-engine/) implementing [Remote Control API HTTP](https://docs.unrealengine.com/5.3/en-US/remote-control-api-http-reference-for-unreal-engine/)
* [soda-sim-city-sample-vehicles](https://github.com/soda-auto/soda-sim-city-sample-vehicles) - additional GhostVehicles library based on City Sample Vehicles

## Roadmap
* Integration with [Project Chrono](https://projectchrono.org/) for accurate vehicle physics simulation.
* Analog/Digital Input/Output hardware interface. 
We have almost finished developing our own hardware, which will allow virtual Analog/Digital Input/Output to be mapped to real hardware for HIL purpose.
* Realtime Python Scripts. Support [Unreal Editor Python](https://docs.unrealengine.com/5.2/en-US/scripting-the-unreal-editor-using-python/) to work in game mode.
* Simulink Model Importing. Ability to develop vehicle components using Simulinks and import them into the simulator.
* LIN interfce support.
* City Traffic Generation based on the Summo.
* Support of the OpenScenario.
* Distributed simulation. Horisontal parallelization of simulation on multiple computers to simulate more vehicle sensors.

## Contact
Please feel free to provide feedback or ask questions by creating a Github issue. For inquiries about collaboration, please email us at sim@soda.auto.

## Copyright and License
Copyright © 2023 SODA.AUTO UK LTD. ALL RIGHTS RESERVED.  
This software contains code licensed as described in [LICENSE](LICENSE.md).  

### Third Parties Licenses
Please ensure to comply with the respective licenses when using these third-party components in your project.
Lists the licenses for third-party software used in this project:
* **dbcppp** - Licensed under the [MIT License](https://opensource.org/licenses/MIT).
* **libzmq** - Licensed under the [Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/).
* mongodb Licensed under the [Server Side Public License v1.0](https://www.mongodb.com/licensing/server-side-public-license). MongoDB drivers are available under the [Apache License v2.0](https://www.apache.org/licenses/LICENSE-2.0).
* **mongodb-cxx** - Licensed under the [Apache License, version 2.0](https://www.apache.org/licenses/LICENSE-2.0).
* **opendrive_reader** - Licensed under the [MIT License](https://opensource.org/licenses/MIT).
* **pugixml** - Distributed under the [MIT License](https://opensource.org/licenses/MIT).
* **quickhull** - Licensed under the [BSD-2-Clause license](https://opensource.org/licenses/BSD-2-Clause).
* **3D models** - Licensed under the [CC-BY License](https://creativecommons.org/licenses/by/4.0/).
* **Unreal Engine** - Dependencies Associated with the [Unreal Engine EULA](https://www.unrealengine.com/en-US/eula).

