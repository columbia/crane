# Setups for Mongod
app="mongod"                                          # app name appears in process list
xtern=0                                               # 1 use xtern, 0 otherwise.
proxy=0                                               # 1 use proxy, 0 otherwise
sch_paxos=0                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=0                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=10                                  # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client="/home/ruigu/Workspace/m-smr"        # root dir for m-smr
msmr_root_server="/home/ruigu/SSD/m-smr"
input_url="127.0.0.1"                                 # url for client to query
analysis_tools=""                                     # for executing analysis tools (e.g., analysis_tools="--worker1=helgrind")

if [ $proxy -eq 1 ]
then
    client_cmd="cd ${msmr_root_client}/apps/mongodb/ycsb-0.1.4 && ./bin/ycsb run mongodb -s -p mongodb.url=${primary_ip}:9000 -p mongodb.writeConcern=normal -p mongodb.database=local -P workloads/workloadb -threads 2"
else
    client_cmd="cd ${msmr_root_client}/apps/mongodb/ycsb-0.1.4 && ./bin/ycsb run mongodb -s -p mongodb.url=${primary_ip}:7000 -p mongodb.writeConcern=normal -p mongodb.database=local -P workloads/workloadb -threads 2"
fi
                                                      # command to start the clients
server_cmd="'${msmr_root_server}/apps/mongodb/install/bin/mongod --port=7000 --dbpath=${msmr_root_server}/apps/mongodb/install/data --pidfilepath=${msmr_root_server}/apps/mongodb/install/mongod.pid --quiet '"
                                                      # command to start the real server
