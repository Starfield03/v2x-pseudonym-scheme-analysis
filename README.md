This project investigates how well different pseudonym rotation strategies protect vehicle privacy in V2X networks. It uses a modified Veins framework for simulation and a Python script that acts as a tracking adversary.

The veins code provided has modified 5 files from the initial example that comes with the Veins installation.

"veins-5.3.1/" contains the filepaths to the altered files themselves, not the entire framework.

Within the omnetpp.ini file, at the bottom, the silent periods are configured. 

When running the simulation, the initial pop-up allows you to choose which configuration to use (Time- or Distance-Based Pseudonym Rotation).

At the end of the simulation, an "adversary_results.csv" file will be generated. The "tracking_adversary.py" Python script uses this file for its calculations and graph generation.

The "tracking_adversary.py" file prints out the linkability and completion rate of the input (.csv file). It also plots a graph representing the adversary's tracking success over time.
