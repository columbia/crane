#!/bin/bash

if [ ! -f worker-run-orig.py ]; then
    echo "git checkout & create worker-run.py original copy"
    git checkout -- worker-run.py
    cp worker-run.py worker-run-orig.py
fi

rounds=3

for server_name in mongoose clamav mediatomb apache; do
    for usleep in 1 10 1000 10000; do
        ./change_sched_with_paxos_usleep.sh $usleep
        sed -n "46,49p" worker-run.py
        ./new-run.sh configs/"$server_name".sh no_build joint_sched $rounds
    done
done
