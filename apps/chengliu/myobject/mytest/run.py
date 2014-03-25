#! /usr/local/bin/python

from server_testproxy import serverTest 
import time

a = serverTest("127.0.0.1:14000")
a.startServer()
print "wait for 10 seconds"
time.sleep(10)
c = a.send('testSvmlight.mat')

outputByteArray = bytearray(c)

with open("./output/testSvmlight.mat","w") as output:
    output.write(outputByteArray)

print "wait for 10 seconds,we will kill the server"
time.sleep(10)

#a.killServer()

    

