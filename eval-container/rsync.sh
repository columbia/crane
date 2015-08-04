# This file syncs the source file of libevent_paxos between your local machine
# and the remote server. It also compiles the synced code on the remote server.

rsync -r ~/SSD/m-smr/libevent_paxos/src/ bug01.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src
rsync -r ~/SSD/m-smr/libevent_paxos/src/ bug02.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src
rsync -r ~/SSD/m-smr/libevent_paxos/src/ bug03.cs.columbia.edu:/home/ruigu/SSD/m-smr/libevent_paxos/src

parallel-ssh -v -p 1 -t 10 -h hostfile "cd /home/ruigu/SSD/m-smr/libevent_paxos && make clean && make"

echo "Sync Done"
