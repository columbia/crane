#! /usr/local/bin/python

from server_testproxy import serverTest 

a = serverTest("127.0.0.1:14000")
a.startServer()
c = a.send('PST_KernelSST.mat')

outputByteArray = bytearray(c)

with open("./output/PST_KernelSST.mat","w") as output:
    output.write(outputByteArray)

a.killServer()

    

