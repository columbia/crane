#!/bin/bash

# rm *.log
rounds=11

./new-run.sh configs/mongoose.sh no_build joint_sched $rounds
./new-run.sh configs/mongoose.sh no_build separate_sched $rounds
./new-run.sh configs/mongoose.sh no_build xtern_only $rounds
./new-run.sh configs/mongoose.sh no_build proxy_only $rounds
./new-run.sh configs/mongoose.sh no_build orig $rounds

./new-run.sh configs/apache.sh no_build joint_sched $rounds
./new-run.sh configs/apache.sh no_build separate_sched $rounds
./new-run.sh configs/apache.sh no_build xtern_only $rounds
./new-run.sh configs/apache.sh no_build proxy_only $rounds
./new-run.sh configs/apache.sh no_build orig $rounds

./new-run.sh configs/clamav.sh no_build joint_sched $rounds
./new-run.sh configs/clamav.sh no_build separate_sched $rounds
./new-run.sh configs/clamav.sh no_build xtern_only $rounds
./new-run.sh configs/clamav.sh no_build proxy_only $rounds
./new-run.sh configs/clamav.sh no_build orig $rounds

./new-run.sh configs/mediatomb.sh no_build joint_sched $rounds
./new-run.sh configs/mediatomb.sh no_build separate_sched $rounds
./new-run.sh configs/mediatomb.sh no_build xtern_only $rounds
./new-run.sh configs/mediatomb.sh no_build proxy_only $rounds
./new-run.sh configs/mediatomb.sh no_build orig $rounds
# ./grab.py
