====================================
# Settings for migration

The environment for eval-multiMachine-migration is:

bluecloudmatrix@bug01 --- Node 0 (a.k.a. Leader Node)
bluecloudmatrix@bug02 --- Node 1
eval@bug03                      --- Node 2

We do checkpoint on Node 2, and do restore on Node 1(i.e. migration from 
Node 2 to Node 1).

When we use CRIU to do dump, we need to run CRIU and the checkpointed 
process in the same tmux session of Node 2, so that we can get rid of the 
influence of bash session and cgroup.img.

====================================
# Updating results

1. 2015/03/11
Precondition: there are no client requests participating (i.e. without ab) 
I have verified that mongoose server in our m-smr system can be 
migrated from Node 2 to Node 1.
And when the mongoose from Node 2 have been migrated to Node 1,
on Node 1 it can still run and handle the requests from ab normally


====================================
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
