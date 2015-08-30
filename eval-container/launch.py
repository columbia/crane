#!/usr/bin/python2.7

import subprocess
import os

cur_env = os.environ.copy()
cur_env['LD_PRELOAD'] = '/home/tianyu/workspace/m-smr/libevent_paxos/client-ld-preload/libclilib.so'
cmd = './py_http.py'

p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
output, err = p.communicate()
print output
