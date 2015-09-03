#!/bin/bash

rm worker-run.py
cp worker-run-orig.py worker-run.py

# test 1/10/1000/10000
usleep_val=$1
sed -i "49i \ \ \ \ os.system(\"sed -i -e 's/sched_with_paxos_usleep = [0-9]\\\+/sched_with_paxos_usleep  = $usleep_val/g' local.options\")" worker-run.py
