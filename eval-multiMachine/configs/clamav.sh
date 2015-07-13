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
msmr_root_client=`echo $MSMR_ROOT`                    # root dir for m-smr. this command assumes that curent machine's m-smr path is the same as the servers'.
msmr_root_server=`echo $MSMR_ROOT`
input_url="127.0.0.1"                                 # url for client to query
analysis_tools=""                                     # for executing analysis tools (e.g., analysis_tools="--worker1=helgrind")

if [ $proxy -eq 1 ]
then
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/clamav/ && \
	LD_PRELOAD=${msmr_root_server}/libevent_paxos/client-ld-preload/libclilib.so \
	${msmr_root_client}/apps/clamav/install/bin/clamdscan --config-file=./client-eth0-9000.conf \
	-m ${msmr_root_client}/apps/clamav/install/* '"
else
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/clamav/ && \
	${msmr_root_client}/apps/clamav/install/bin/clamdscan --config-file=./client-lo-7000.conf \
        -m ${msmr_root_client}/apps/clamav/install/* '"
fi

# We didn't use start-server script because we have to LD_PRELOAD for the whole scripts.
# Thus, server.conf will be corrupted by the xtern stderr(Init share memory stuff.)
server_cmd="'${msmr_root_server}/apps/clamav/install/sbin/clamd \
	--config-file=${msmr_root_server}/apps/clamav/client-lo-7000.conf &> out.txt & '"
