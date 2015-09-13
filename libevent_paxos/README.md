Reusable Consensus Component For RSM
======================================
By Milannic(milannic.cheng.liu@gmail.com)

Usage
--------------------------------------
this program replies on **libevent-2.0.21** for network communication, **libconfig-1.4.9**  for node configuration and **libdb(Berkeley DB)-5.1.29** for data persistently store.**Uthash** is used for C-implementation Hashtable. 

Please Use ./mk in the root dir to download and install the dependencies(those libraries will be installed in ./.local, and sources files will be kept in ./dep-lib)

Compilation of the program needs gcc-4.8 which is indicated in the Makefile(some C99 features are used),actually gcc-4.7 is also available, a symbol link may be used to do so.

Source Code
-----------------------------------------
##1.src
source files
###1.1 config-comp
implementation of configuration file parsing module.
###1.2 consensus
implementation of normal case request consensus and deliver
###1.3 db
implementation of database operation module.
###1.4 replica-sys
the replicate node which integrates other modules to be one.
###1.5 util
some helper functions.
###1.6 include
header files of the program.


Usage
-----------------------------------------
Generally Speaking, we expose system\_initialize(user\_cb),system\_run(nod),system\_exit(node) three functions to you, you just need to pass your own callback function to the system, and run it. 

When there is request from client and is agreed among the majority of the replica group, your user call back function will be invoked.


Test
----------------------------------------
Now we have four test(make test after make):
##1.Sample Test
blank test for illustration .
##2.Node Start Test
start a primary node, make sure all the initializations finished.
##3.Ping Test
start a primary node and one secondary node,then kill the primary node, the secondary should notice that it haven't heard from the leader for a long time, then start a leader election.
##4.Normal Case Test
start a replica group, and also many clients sending requests to the primary concurrently. The output in all the replica group should be completely the same.


To-Do
-----------------------------------------
##Leader Election Component
##Client Side Library
##Proxy Integrated with DMT


Misc
-----------------------------------------
Potential Bug is possible in Current Version,Any Suggestion is welcomed.
