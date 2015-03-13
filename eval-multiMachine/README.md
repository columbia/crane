# explauncher
Experiment launcher for general distributed applications

A brief introduction for each component of the experiment launcher.
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

How can I use the script:

1. The current scenario is in order to use these scripts, you need to have 4
machines. 1 serves as client, 3 serve as servers. My scripts will need to be
started on this client machines and all the configurations are only need to 
change on this client machine.

2. The only thing you need to change is in each configs/${server_name}.sh file, 
change the path of MSMR_ROOT according to the path on your client machine and 
server machines.
For example, right now, I've created symbolic links to make sure all my MSMR_ROOT
folder on different servers will have the same path. 

Notice:
1. In run.sh, change the parameters listed in the beginning. It's highly
suggested that the paths across all servers are consistent. For example,
create a symbolic link in the home folder to MSMR_ROOT will do.

2. There are many default assumptions. For example, I've assumed that all the
applications will run on bug03, bug01 and bug02. Bug03 will be the default
leader. If you want to change the rolls of remote server, first change it in hostfile.
Then change the head, worker1, worker2 file.

3. The current scripts enable the demo of leader election. To disable it, simply
comment the corresponding code in master.py which right after the comment "Starts
leader election demo". Don't comment kill_previous_process.

4. The script will be executed in the home folder of your remote machines. Some
configuration files will be copied there. Modify you local configuration files
in your home folder instead of the original conf file in the source code repo.

5. My scripts couldn't properly remove the tmp file created by xtern. So if you
find that client can't receive responses, try cleaning the tmp files first.
Right now, they're 3 of them. 1 under your home folder. 2 in /dev/shm ahd /tmp.
