# Setups for Proftpd
# Notice :
# You need to manually change the port number m-smr/apps/proftpd/install/etc/proftpd.conf

app="proftpd"                                         # app name appears in process list
xtern=1                                               # 1 use xtern, 0 otherwise.
proxy=1                                               # 1 use proxy, 0 otherwise
sch_paxos=1                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=1                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=10                                  # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client="/home/ruigu/Workspace/m-smr"        # root dir for m-smr
msmr_root_server="/home/ruigu/SSD/m-smr"
input_url="127.0.0.1"                                 # url for client to query

if [ $proxy -eq 1 ]
then
    client_cmd="cd ${msmr_root_client}/apps/proftpd/benchmark && ./bin/dkftpbench -hlocalhost -P9000 -n20 -c20 -t10 -k1 -uftpuser -pftpuser -v1 -fx10k.dat"
else
    client_cmd="cd ${msmr_root_client}/apps/proftpd/benchmark && ./bin/dkftpbench -hlocalhost -P7000 -n20 -c20 -t10 -k1 -uftpuser -pftpuser -v1 -fx10k.dat"
fi
                                                      # command to start the clients
server_cmd="'cd ${msmr_root_server}/apps/proftpd/install && sudo ./sbin/proftpd -c ${msmr_root_server}/apps/proftpd/install/etc/proftpd.conf -d 10'"
                                                      # command to start the real server
