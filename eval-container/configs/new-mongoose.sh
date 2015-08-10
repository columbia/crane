# Setups for mongoose
app="mg-server"                                       # app name appears in process list
xtern=1                                               # 1 use xtern, 0 otherwise
proxy=1                                               # 1 use proxy, 0 otherwise
sch_paxos=1                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=1                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=5                                   # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client=`echo $MSMR_ROOT`        # root dir for m-smr
msmr_root_server=`echo $MSMR_ROOT`
input_url="127.0.0.1"                                 # url for client to query
analysis_tools=""                                     # for executing analysis tools (e.g., analysis_tools="--worker1=helgrind")
num_req=1
num_thd=1

if [ $proxy -eq 1 ]
then
    if [ $leader_elect -eq 1 ]
    then
        client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n ${num_req} -c ${num_thd} http://128.59.17.174:9000/test.php"
    else
        client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n ${num_req} -c ${num_thd} http://128.59.17.174:9000/test.php"
    fi
else
    client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n ${num_req} -c ${num_thd} http://128.59.17.174:7000/test.php"
fi
                                                      # command to start the clients
server_cmd="'${msmr_root_server}/apps/mongoose/mg-server -I /usr/bin/php-cgi -p 7000 -t 8 &'"
                                                      # command to start the real server
