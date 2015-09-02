#!/bin/bash

if [ ! -f worker-run-orig.py ]; then
    echo "git checkout & create worker-run.py original copy"
    git checkout -- worker-run.py
    cp worker-run.py worker-run-orig.py
fi

rounds=3

for server_name in mongoose clamav mediatomb apache; do
    for min in 10 100 10000 100000; do
        ./change_sched_with_paxos_maxmin.sh $min
        sed -n "46,50p" worker-run.py
        ./new-run.sh configs/"$server_name".sh no_build joint_sched $rounds
    done
done
