m-smr
=====

Multi-threaded State Machine Replication

Install depdendent libraries/tools. Use ubuntu 14.04 hardy, with amd64.
> sudo apt-get install build-essential
> sudo apt-get install libboost-dev git subversion nano libconfig-dev libevent-dev \
	libsqlite3-dev libdb-dev libboost-system-dev autoconf m4 dejagnu flex bison axel zlib1g-dev \
	libbz2-dev libxml-libxml-perl python-pip python-setuptools python-dev libxslt1-dev libxml2-dev \
	wget curl
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

5. Install CRIU (just one way installation work).
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


6. Install lxc 1.1
> sudo add-apt-repository ppa:ubuntu-lxc/daily
> sudo apt-get update
> sudo apt-get install lxc
> lxc-create --version
  1.1.0
Create a new container named "u1" (if this container does not exist in /var/lib/lxc/u1)
> sudo lxc-create -t ubuntu -n u1 -- -r trusty -a amd64
> sudo lxc-create -t download -n u1 -- --dist ubuntu --release trusty --arch amd64
Config u1 fstab to share memory between the proxy (in host OS) and the server process (in container).
>sudo echo "/dev/shm dev/shm none bind,create=dir" > /var/lib/lxc/u1/fstab
>sudo echo "lxc.mount = /var/lib/lxc/u1/fstab" > /var/lib/lxc/u1/config
> sudo lxc-start -n u1
> ssh ubuntu@$(sudo lxc-info -i -H -n u1) 
   Enter password: "ubuntu"
This "ubuntu" user is already a sudoer. 
Config static IP for the container. Change this file in the container: /etc/network/interfaces
#auto eth0
#iface eth0 inet dhcp
auto eth0
iface eth0 inet static
        address 10.0.3.111
        netmask 255.255.255.0
        gateway 10.0.3.1
        dns-nameserver 8.8.8.8
And restart the container.
> lxc-stop -n u1
> lxc-start -n u1
Check the IP again.
> sudo lxc-ls --fancy
NAME  STATE    IPV4        IPV6  GROUPS  AUTOSTART  
--------------------------------------------------
u1    RUNNING  10.0.3.111  -     -       NO
> ssh ubuntu@$(sudo lxc-info -i -H -n u1)