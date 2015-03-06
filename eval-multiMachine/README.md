# explauncher
Experiment launcher for general distributed applications

1. run.sh
This file is aiming at generating all the configuration parameters and
feed them to the following scripts.

2. master.py
This file starts all remote scripts on the server. The remote scripts
on the server will start proxies and server applications on each server 
machine.

3. worker-run.py
This is the remote script that will be put on the remote server. 
It starts the proxies and server applications. 

How to customize the scripts:
1. In run.sh, change the parameters listed in the beginning. It's highly
suggested that the paths across all servers are consistent. For example,
create a symbolic link in the home folder to MSMR_ROOT will do.

2. In work-run.py, I've added a placeholder where we need to load the library
XTERN.

3. There are many default settings. For example, I've assumed that all the
applications will run on bug00, bug01 and bug02. Bug00 will be the default
leader. If you want to change the remote server, first change it in hostfile.
Then change the head, worker1, worker2 file.
