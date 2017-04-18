# DTNSIM #

This document describes DTNSIM, a simulator devoted to study different aspects of Delay/Disruption Tolerant Network (DTN) such as routing, forwarding, scheduling, planning, and others. The simulator is implemented on the [Omnet++ framework version 5.0](https://omnetpp.org/) and can be conveniently utilized using the Tkenv environment and modified or extended using the Omnet++ Eclipse-based IDE.

The simulator is still under development: this is a beta version. Nonetheless, feel free to use it and test it. Our contact information is below. 

## Installation ##

### Basic Installation ###

* Download [Omnet++](https://omnetpp.org/omnetpp). DTNSIM was tested on version 5.0, but should work in newer versions as well.
* Import the DTNSIM repository from the Omnet++ IDE (File->Import->Projects from Git->Clone URI).
* Build DTNSIM project.

### Topology Outputs (optional) ###

* Download [Boost](http://www.boost.org/users/download) and extract it. DTNSIM was tested on version 1.63.0, but should work in newer versions as well.
* Select DTNSIM project properties and include boost main folder (boost_1_63_0) in C/C++ General/Paths and Symbols .
* Uncomment line "//#define USE_BOOST_LIBRARIES 1" in src/Config.h: 
* Build DTNSIM project.

Note: DTNSIM can be used without boost libraries but output topology graphs will not be generated.

## Utilization ##

* Open dtnsim_demo.ini config file from the project src folder. Familiarize with the simulation parameters which are explained in the same file.
* Run the simulation. If using Omnet++ IDE, the Tkenv environment will be started. 
* Results (scalar and vectorial statistics) will be stored in the results folder.

Note: Nodes will remain static in the simulation visualization. Indeed, the dynamic of the network is captured in the "contact plans" which comprises a list of all time-bound communication opportunities between nodes. In other words, if simulating mobile network, the mobility should be captured in such contact plans and provided to DTNSIM as input.

## Contact Us ##

If you have any comment, suggestion, or contribution you can reach us at madoerypablo@gmail.com and juanfraire@gmail.com.