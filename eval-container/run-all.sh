#!/bin/bash

rm *.log

./new-run.sh configs/mongoose.sh no_build joint_sched 5
./new-run.sh configs/mongoose.sh no_build separate_sched 5
./new-run.sh configs/mongoose.sh no_build xtern_only 5
./new-run.sh configs/mongoose.sh no_build proxy_only 5
./new-run.sh configs/mongoose.sh no_build orig 5

./new-run.sh configs/clamav.sh no_build joint_sched 5
./new-run.sh configs/clamav.sh no_build separate_sched 5
./new-run.sh configs/clamav.sh no_build xtern_only 5
./new-run.sh configs/clamav.sh no_build proxy_only 5
./new-run.sh configs/clamav.sh no_build orig 5

./new-run.sh configs/apache.sh no_build joint_sched 5
./new-run.sh configs/apache.sh no_build separate_sched 5
./new-run.sh configs/apache.sh no_build xtern_only 5
./new-run.sh configs/apache.sh no_build proxy_only 5
./new-run.sh configs/apache.sh no_build orig 5
