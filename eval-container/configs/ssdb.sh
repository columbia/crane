# Setups for ssdb
# Notice :
# You need to change related port number ssdb.conf and ssdb_slave.conf in apps/ssdb/... to 7000 
# I mean the configuration file on the server mahcines.

app="ssdb-server"                                     # app name appears in process list
xtern=0                                               # 1 use xtern, 0 otherwise
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

client_cmd="${msmr_root_client}/apps/ssdb/ssdb-master/tools/ssdb-bench 128.59.17.174 9000 1000 10"
                                                      # command to start the clients
server_cmd="'cd ${msmr_root_server}/apps/ssdb/ssdb-master && rm -rf var && mkdir var && ./ssdb-server -d ssdb.conf '"
                                                      # command to start the real server
