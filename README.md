m-smr
=====

Multi-threaded State Machine Replication

Install depdendent libraries/tools. Use ubuntu 14.04 hardy, with amd64.
> sudo apt-get install build-essential
> sudo apt-get install libboost-dev git subversion nano libconfig-dev libevent-dev \
	libsqlite3-dev libdb-dev libboost-system-dev autoconf m4 dejagnu flex bison axel zlib1g-dev \
	libbz2-dev libxml-libxml-perl python-pip python-setuptools python-dev libxslt1-dev libxml2-dev \
	wget curl php5-cgi psmisc mencoder
> sudo pip install numpy
> sudo pip install OutputCheck          (this utility is only required by the testing framework)


0. Commands to checkout a brand-new project:
> git clone https://github.com/columbia/crane
> cd $MSMR_ROOT
> git pull


1. Set env vars in ~/.bashrc.
export MSMR_ROOT=<absolute path of crane>
export XTERN_ROOT=<absolute path of crane/xtern>
export LD_LIBRARY_PATH=$MSMR_ROOT/libevent_paxos/.local/lib:$LD_LIBRARY_PATH


2. Compile xtern.
> cd $XTERN_ROOT
> mkdir obj
> cd obj
> ./../configure --prefix=$XTERN_ROOT/install
> make clean; make; make install

Note: for xtern, for everytime you pull the latest project from github,
please run:
> cd $XTERN_ROOT/obj
> make clean; make; make install     <PLEASE RUN MAKE CLEAN EVERYTIME>

3. Compile libevent_paxos.
> cd $MSMR_ROOT/libevent_paxos
> ./mk
> make clean; make  <PLEASE RUN MAKE CLEAN EVERYTIME>
TBD. reconfigure IP addresses for Crane.


4. Install analysis tools.
(1) Install valgrind.
> sudo apt-get install valgrind

(2) Install dynamorio.
> sudo apt-get install cmake doxygen transfig imagemagick ghostscript subversion
> sudo su root
> cd /usr/share/
> git clone https://github.com/DynamoRIO/dynamorio.git
> cd dynamorio && mkdir build && cd build
> cmake .. && make -j && make drcov
> cd tools && ln -s $PWD/../drcov.drrun64 .
> mkdir lib64 && cd lib64 && mkdir release && cd release
> ln -s $PWD/../../../clients/lib64/release/libdrcov.so .
> exit (exit from sudoer)

Test dynamorio with the "drcov" code coverage tool. If these commands succeed, run "ls -l *.log" in current directory.
> /usr/share/dynamorio/build/bin64/drrun -t drcov -- ls -l
> /usr/share/dynamorio/build/bin64/drrun -t drcov -- /bin/bash $XTERN_ROOT/scripts/wrap-xtern.sh ls -l


5. Install lxc 1.1 (in the host OS)
> sudo add-apt-repository ppa:ubuntu-lxc/daily
> sudo apt-get update
> sudo apt-get install lxc
> lxc-create --version
  1.1.0
Create a new container named "u1" (if this container does not exist in /var/lib/lxc/u1)
> sudo lxc-create -t ubuntu -n u1 -- -r trusty -a amd64
This command will take a few minutes to download all trusty (14.04) initial packages.

Start the server for the first time, so that we have "/var/lib/lxc/u1/config and /var/lib/lxc/u1/fstab".
> sudo lxc-start -n u1
> sudo lxc-stop -n u1

Config u1 fstab to share memory between the proxy (in host OS) and the server process (in container).
>sudo echo "/dev/shm dev/shm none bind,create=dir" > /var/lib/lxc/u1/fstab

Append these lines to /var/lib/lxc/u1/config
# for crane project.
lxc.network.ipv4 = 10.0.3.111/16
lxc.console = none
lxc.tty = 0
lxc.cgroup.devices.deny = c 5:1 rwm
lxc.rootfs = /var/lib/lxc/u1/rootfs
lxc.mount = /var/lib/lxc/u1/fstab
lxc.mount.auto = proc:rw sys:rw cgroup-full:rw
lxc.aa_profile = unconfined

And restart the container.
> sudo lxc-start -n u1
Check the IP.
> ssh ubuntu@10.0.3.111 
   Enter password: "ubuntu"
This "ubuntu" user is already a sudoer. 
Then, you are free to install crane, gcc, criu, and other packages in this container.

Add your own username into the lxc without asking password anymore.
For example, in bug03 machine:
Generate a private/public key pair, put it into your ~/.ssh/.
Rename the private key to be ~/.ssh/lxc_priv_key
Append the public key to the u1 container's ~/.ssh/auth..._keys
Then
> ssh your_user_name@10.0.3.111 -i ~/.ssh/lxc_priv_key
Make sure you can login without entering password.
When you run sudo in the u1 container, avoid asking sudo password, append this line to /etc/sudoers
> ruigu ALL = NOPASSWD : ALL


6. Install CRIU (just one way installation work, in the u1 container).
> cd $MSMR_ROOT/tools/criu/ 
> wget http://download.openvz.org/criu/criu-1.6.tar.bz2
> tar jxvf criu-1.6.tar.bz2
> sudo apt-get install libprotobuf-dev libprotoc-dev protobuf-c-compiler \
	libprotobuf-c0 libprotobuf-c0-dev asciidoc bsdmainutils protobuf-compiler
> cd criu-1.6
> make -j
> sudo make install (the PREFIX directory for criu by default is /usr/local/)
> which criu
  /usr/local/sbin/criu


7. Install server applications depending on which one you would like to run.
Currently we have tested Crane with these servers: mongoose, apache, clamav, mediatomb, and mysql.
> cd $MSMR_ROOT/apps/apache
> ./mk

8. Config the IPs of three replicas and one client machine.
The default IPs are already set for the RCS@Columbia team, so if you are in this team, you don't need to run this command.
You will need three replica machines to host servers, and one client machine.
For instance, in our Crane project, our client machine is 128.59.21.11, three replicas: (128.59.17.174 (head), 128.59.17.172, 128.59.17.173).
If your machine IPs are not this setting, please run these same commands on all the replica (server) machines:
> cd $MSMR_ROOT/eval-container
> ./config-node-IPs.sh <primary IP> <backup1 IP> <backup2 IP>"

For instance, we ran this same command (including the order of IPs) on 128.59.17.174, 128.59.17.172, 128.59.17.173.
> ./config-node-IPs.sh 128.59.17.174 128.59.17.172 128.59.17.173


9. Run apache with Crane. Run the below commands on your "client" machine (not any server machine).

But, please first make sure the firewalls of our client and replica machines are correctly set so that your client machine can indeed
sent requests to the server machines. To verify this, you can just manually start an apache server on each replica (server) machine, 
and manually launch a "ab" client benchmark on your client machine, and see whether ab gets responses correctly.

For instance, these commands are ran on our client machine: 128.59.21.11.
> cd $MSMR_ROOT/eval-container
> ./new-run.sh configs/apache.sh no_build joint_sched 1
Run the apache un-replicated nondeterministic execution.
> ./new-run.sh configs/apache.sh no_build orig 1
Run the apache with proxy (paxos) only.
> ./new-run.sh configs/apache.sh no_build proxy_only 1
Run the apache with DMT (Parrot) only.
> ./new-run.sh configs/apache.sh no_build xtern_only 1

Below are one sample output from the ab server, if you ran any one of the above "new-run.sh" commands.
===============================
This is ApacheBench, Version 2.3 <$Revision: 655654 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 128.59.17.174 (be patient)


Server Software:        Apache/2.2.11
Server Hostname:        128.59.17.174
Server Port:            9000

Document Path:          /test.php
Document Length:        27 bytes

Concurrency Level:      8
Time taken for tests:   148.135 seconds
Complete requests:      1000
Failed requests:        0
Write errors:           0
Total transferred:      212000 bytes
HTML transferred:       27000 bytes
Requests per second:    6.75 [#/sec] (mean)
Time per request:       1185.077 [ms] (mean)
Time per request:       148.135 [ms] (mean, across all concurrent requests)   ***** This number is the one that Crane's evaluation grabs.
Transfer rate:          1.40 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       1
Processing:  1151 1185  39.2   1177    1543
Waiting:     1150 1184  39.2   1176    1542
Total:       1151 1185  39.2   1177    1543

Percentage of the requests served within a certain time (ms)
  50%   1177
  66%   1182
  75%   1186
  80%   1190
  90%   1211
  95%   1230
  98%   1262
  99%   1329
 100%   1543 (longest request)

