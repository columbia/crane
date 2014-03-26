Concoord Archive Repository Description by Cheng Liu
=============================================================


##1.Generation
- This package contains some useful version of concoord and the source code may have been modified by Cheng Liu

##2.Installation 
- To keep every python packages local without root privilige, we can install the python packet by 
  --user option, then you can build the requirement with many options:

###Build From Source 

0. Install the requirement of concoord(dnspython,msgpack-python)
1. Python setup.py install --user (From the source )
2. add "/home/[username]/.local/bin" to your PATH
- [Github] (https://github.com/denizalti/concoord)

###Build From EasyInstall or Pip

0. pip install concoord --user(The latest Stable Version of Concoord)
1. easy\_install --user concoord

###Uninstall 

0. If you install the concoord in the user level, you can just pip uninstall concoord, no matter what method you use to install it.

###Additional Explanation

0. Concoord is represented by a python package, everytime you install it, it will creates a binary file in your ~/.local/bin, multiple concoord versions can exist, as long as you carefully address this different concoord binary file which is the entry point of the concoord, or maybe you can refer to
- [virtualenv](http://www.virtualenv.org/en/latest/)

1. All the source codes of Concoord are in the concoord-version/concoord directory, and the author presents many useful test scripts in **concoord-version/concoord/test directory**, but without root privilege, no all of them can be executed.(For some scripts need operations about iptables), read the test code will give you a more clear way to check concoord.

2. The Object built by Cheng Liu is in the upper level directory chengliu/myobject, to make it run properly, you may need to modify the concoord source code, I make this patch for you in chengliu/, you can choose the latest one, you can modify it randomly, but remember to record history version and the existing file name format is encouraged.

3. There is also simple test script provided by Cheng Liu to show you how to run concoord experiment, you can check it from chengliu/myobject/mytest

