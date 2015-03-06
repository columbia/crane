rsync -r /home/ruigu/SSD/m-smr/libevent_paxos/src/ bug00.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src
rsync -r /home/ruigu/SSD/m-smr/libevent_paxos/src/ bug01.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src
rsync -r /home/ruigu/SSD/m-smr/libevent_paxos/src/ bug02.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src
parallel-ssh -v -p 1 -t 10 -h hostfile "cd /home/ruigu/SSD/m-smr/libevent_paxos && make clean && make"
echo "Sync Done"
