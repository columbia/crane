1. Do checkpoint On bug02,:
> sudo su eval

You must tmux to this bash login and start the mg-server, so that your cgroup
id is "c1".
> tmux a -t checkpoint

> cd /home/eval/m-smr/apps/mongoose

Check wether the current directory's local.options has "sched_with_paxos = 1"
Do not start with many threads otherwise restore on diff machine may fail (mismatch restored thread ids).
> LD_PRELOAD=$XTERN_ROOT/dync_hook/interpose.so \
$MSMR_ROOT/apps/mongoose/mg-server -I /usr/bin/php-cgi -p 7000 -t 2 &

Open another terminal on bug02:
> sudo su
> cd /home/eval/m-smr/tools
> ./su-checkpoint.sh mg-server
> tar checkpoint.tar.gz mg-server-XXXX-checkpoint ~/paxos_queue_file_lock /dev/shm/PAXOS_OP_QUEUE-eval /dev/shm/NODE_ROLE-eval
> scp this checkpoint.tar.gz to bug03.cs.

2. On bug03
> sudo su 
> cp checkpoint.tar.gz to /home/eval/m-smr/tools
> tar zxvf checkpoint.tar.gz, copy the three shared memory files to the same path on bug02.
> ./su-restore.sh $PWD/mg-server-XXXX-checkpoint
> Then you will see the same mg-server process running on bug03!

Run joint scheduling workloads to verify this workload:
> cd /home/eval/m-smr/apps/mongoose/

Start the proxy, but make sure your local.optins and nodes.cfg are the same on both 02 and 03 in the mongoose directory.
> $MSMR_ROOT/libevent_paxos/target/server.out -n 0 -r -m s -c nodes.cfg -l ./log &> proxy.txt &

Run client:

eval@bug03:~/m-smr/apps/mongoose$ LD_PRELOAD=$MSMR_ROOT/libevent_paxos/client-ld-preload/libclilib.so $MSMR_ROOT/apps/apache/install/bin/ab -n 128 -c 8 http://127.0.0.1:9000/readme.txt
This is ApacheBench, Version 2.3 <$Revision: 655654 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)...now I am calling the fake write function
now I am calling the fake write function
......
now I am finished the fake write function
..done


Server Software:        
Server Hostname:        127.0.0.1
Server Port:            9000

Document Path:          /readme.txt
Document Length:        333 bytes

Concurrency Level:      8
Time taken for tests:   0.058 seconds
Complete requests:      128
Failed requests:        0
Write errors:           0
Total transferred:      69760 bytes
HTML transferred:       42624 bytes
Requests per second:    2218.64 [#/sec] (mean)
Time per request:       3.606 [ms] (mean)
Time per request:       0.451 [ms] (mean, across all concurrent requests)
Transfer rate:          1180.82 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    0   0.1      0       0
Processing:     2    3   0.3      3       4
Waiting:        1    3   0.3      3       4
Total:          2    3   0.3      4       5
ERROR: The median and mean for the total time are more than twice the standard
       deviation apart. These results are NOT reliable.

Percentage of the requests served within a certain time (ms)
  50%      4
  66%      4
  75%      4
  80%      4
  90%      4
  95%      4
  98%      4
  99%      4
 100%      5 (longest request)

