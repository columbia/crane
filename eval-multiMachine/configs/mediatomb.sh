# Setups for MediaTomb
# Notice :
# 1. For each mode, change start-sever, run-client and local.options accordingly.
# 2. Better comment out the LD_PRELOAD for client in master.py

app="mediatomb"                                       # app name appears in process list
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
    if [ $leader_elect -eq 1 ]
    then
        client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/mediatomb/ && ./run-client 127.0.0.1 9000 '"
        #client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 8 -c 8 http://128.59.17.174:9000/content/media/object_id/8/res_id/none/pr_name/vlcmpeg/tr/1"
    else
        client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/mediatomb/ && ./run-client 127.0.0.1 9000 '"
        #client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 8 -c 8 http://128.59.17.174:9000/content/media/object_id/8/res_id/none/pr_name/vlcmpeg/tr/1"
    fi
else
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/mediatomb/ && ./run-client 127.0.0.1 7000 '"
    #client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 8 -c 8 http://128.59.17.174:7000/content/media/object_id/8/res_id/none/pr_name/vlcmpeg/tr/1"
fi
                                                      # command to start the clients
#server_cmd="'${msmr_root_server}/apps/mediatomb/install/bin/mediatomb -i 128.59.17.174 -p 7000 -m ${msmr_root_server}/apps/mediatomb &'"
server_cmd="'cd ${msmr_root_server}/apps/mediatomb && ./start-server 127.0.0.1 7000 '"
#server_cmd="'cd ${msmr_root_server}/apps/mediatomb && ./start-server 128.59.17.174 7000 '"
                                                      # command to start the real server
