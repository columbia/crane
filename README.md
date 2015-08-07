m-smr
=====

Multi-threaded State Machine Replication

Install depdendent libraries/tools. Use ubuntu 14.04 hardy, with amd64.
> sudo apt-get install build-essential
> sudo apt-get install libboost-dev git subversion nano libconfig-dev libevent-dev \
	libsqlite3-dev libdb-dev libboost-system-dev autoconf m4 dejagnu flex bison axel zlib1g-dev \
	libbz2-dev libxml-libxml-perl python-pip python-setuptools python-dev libxslt1-dev libxml2-dev \
	wget curl php5-cgi psmisc
> sudo pip install numpy
> sudo pip install OutputCheck          (this utility is only required by the testing framework)

0. Set env vars in ~/.bashrc.
export MSMR_ROOT=<...>
export XTERN_ROOT=<...>
export LD_LIBRARY_PATH=$MSMR_ROOT/libevent_paxos/.local/lib:$LD_LIBRARY_PATH


1. Commands to checkout a brand-new project:
> git clone https://github.com/hemingcui/m-smr
> cd $MSMR_ROOT
> git pull
> git submodule init
> git submodule update

Then,  you will have the latest xtern and libevent_paxos repo (submodules).
You do not need to checkout these submodules one by one, just run the git
commands above.


2. Commands to get the latest project:
> cd $MSMR_ROOT
> git pull
> git submodule update

3. Compile xtern and libevent_paxos.
> cd $XTERN_ROOT
> mkdir obj
> cd obj
> ./../configure --prefix=$XTERN_ROOT/install
> make clean; make; make install

Note: for xtern, for everytime you pull the latest project from github,
please run:
> cd $XTERN_ROOT/obj
> make clean; make; make install     <PLEASE RUN MAKE CLEAN EVERYTIME>


> cd $MSMR_ROOT/libevent_paxos
> ./mk
> make clean; make  <PLEASE RUN MAKE CLEAN EVERYTIME>


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
