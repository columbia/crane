# Setups for clamav
# Notice :
# !!!! Change nodes.local.config Node-01 ip address to 127.0.0.1
# !!!! Run start-run and run-client on remote machines localy once to generate the necessary configuration file
# !!!! Don't forget to change the ip address back for the other expriments!!!! 
# !!!! cp local.options to the apps folder and change the joint scheduling parameter accordingly

app="clamd"                                           # app name appears in process list
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
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/clamav/ && LD_PRELOAD=${msmr_root_server}/libevent_paxos/client-ld-preload/libclilib.so ./run-client 9000 '"
else
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/clamav/ && ./run-client 7000 '"
fi
                                                      # command to start the clients
# We didn't use start-server script because we have to LD_PRELOAD for the whole scripts.
# Thus, server.conf will be corrupted by the xtern stderr(Init share memory stuff.)

#server_cmd="'cd ${msmr_root_server}/apps/clamav && ./start-server 7000 '"
server_cmd="'cd ${msmr_root_server}/apps/clamav && ./install/sbin/clamd --config-file=./server.conf &> out.txt & '"
                                                      # command to start the real server
