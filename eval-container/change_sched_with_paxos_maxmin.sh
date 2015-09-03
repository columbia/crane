#!/bin/bash

rm worker-run.py
cp worker-run-orig.py worker-run.py

sched_with_paxos_min=$1
sched_with_paxos_max=$(($sched_with_paxos_min * 10))
echo "(sched_with_paxos_min $sched_with_paxos_min , sched_with_paxos_max $sched_with_paxos_max )"
sed -i "49i \ \ \ \ os.system(\"sed -i -e 's/sched_with_paxos_max = [0-9]\\\+/sched_with_paxos_max = $sched_with_paxos_max/g' local.options\")" worker-run.py
sed -i "49i \ \ \ \ os.system(\"sed -i -e 's/sched_with_paxos_min = [0-9]\\\+/sched_with_paxos_min = $sched_with_paxos_min/g' local.options\")" worker-run.py
