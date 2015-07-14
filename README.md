m-smr
=====

Multi-threaded State Machine Replication

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

