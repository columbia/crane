# explauncher
Experiment launcher for general distributed applications

1. run.sh
This file is aiming at generating all the configuration parameters and
feed them to the following scripts.

2. master.py
This file starts all remote scripts on the server. The remote scripts
on the server will start server application on each server machine.

3. worker-run.py
This is the file that will be located on each remote server machine.
It starts the server applications. 
