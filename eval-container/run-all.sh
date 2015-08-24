#!/bin/bash

rm *.log
rounds = 3

./new-run.sh configs/mongoose.sh no_build joint_sched $rounds
./new-run.sh configs/mongoose.sh no_build separate_sched $rounds
# ./new-run.sh configs/mongoose.sh no_build xtern_only 5
./new-run.sh configs/mongoose.sh no_build proxy_only $rounds
# ./new-run.sh configs/mongoose.sh no_build orig 5

./grab.py
