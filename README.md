m-smr
=====

Multi-threaded State Machine Replication

0. Set env vars in ~/.bashrc.
export $MSMR_ROOT=<...>
export $XTERN_ROOT=<...>


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
