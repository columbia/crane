#!/usr/bin/python2.7

import errno
import httplib
import subprocess
import os
import time
from socket import error as socket_error
import sys

# !!!!!!!!Requirement:
# 1. Enable all the consensus level log option on bug01 and bug02(libeventpaxos/target/nodes.local.cfg)
# 2. Make sure you don't call kill_previous_process() at master.py at last.
# 3. After run.sh being executed, make sure the proxy processes on bug01 and bug02
#    are still running.
#
# Command to run:
# ./launcher.sh

USER = os.environ["USER"]
MSMR_ROOT = os.environ["MSMR_ROOT"]

def main():
    # We'll set the default new leader to be bug01.
    target = 'bug01.cs.columbia.edu'
    cmd = "ps -u %s | grep server.out | wc -l" % (USER)
    rcmd = 'parallel-ssh -l {username} -v -p 1 -i -t 15 -h worker1 {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    if int(output) == 0:
      print "Proxy on bug01 already exit. Abort!"
      #sys.exit()

    print "Waiting for the new leader to be ready!" 

    while(True):
        # Check if the leader is bug01
        cmd = "grep \"Leader.Tries.To.Ping.Other.Nodes\" /home/ruigu/log/node-*-consensus-sys.log | wc -l"
        #rcmd = "parallel-ssh -v -p 1 -i -t 15 -h worker1 \"%s\"" % (cmd)
        rcmd = 'parallel-ssh -l {username} -v -p 1 -i -t 15 -h worker1 {command}'.format(
            username=USER, command=cmd)
        p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
        if int(output) > 1:
          target = 'bug01.cs.columbia.edu'
          print "bug01 is leader!!!!!!"
          break

        # Check if the leader is bug02
        rcmd = 'parallel-ssh -v -p 1 -i -t 15 -h worker2 {command}'.format(
                username=USER, command=cmd)
        p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
        if int(output) > 1:
          target = 'bug02.cs.columbia.edu'
          print "bug02 is leader!!!!!!"
          break


###############################################################################
# Main - Parse command line options and invoke main()   
if __name__ == "__main__":
    main()
