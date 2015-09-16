# Setups for Apache

source configs/default-options.sh
app="httpd"                                           # app name appears in process list

# evaluation options.
xtern=1                                               # 1 use xtern, 0 otherwise.
proxy=1                                               # 1 use proxy, 0 otherwise
sch_paxos=1                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=1                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
enable_lxc="no"
dmt_log_output=1

leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=10                                  # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client=`echo $MSMR_ROOT`        # root dir for m-smr
msmr_root_server=`echo $MSMR_ROOT`
input_url="127.0.0.1"                                 # url for client to query
analysis_tools=""                                     # for executing analysis tools (e.g., analysis_tools="--worker1=helgrind")

proxy_port=7000
if [ $proxy -eq 1 ]
then
    proxy_port=9000
fi
client_cmd="${msmr_root_client}/eval-container/utility-scripts/parallel-curl.sh ${primary_ip} ${proxy_port}"

server_cmd="'${msmr_root_server}/apps/apache/install/bin/apachectl \
	-f ${msmr_root_server}/apps/apache/install/conf/httpd.conf -k start '"
