# Setups for clamav
# Notice :
# !!!! Change nodes.local.config Node-01 ip address to 127.0.0.1
# !!!! Run start-run and run-client on remote machines localy once to generate the necessary configuration file
# !!!! Don't forget to change the ip address back for the other expriments!!!! 
# !!!! cp local.options to the apps folder and change the joint scheduling parameter accordingly

source configs/default-options.sh
app="mysqld"                                           # app name appears in process list

# evaluation options
xtern=1                                               # 1 use xtern, 0 otherwise.
proxy=1                                               # 1 use proxy, 0 otherwise
sch_paxos=1                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=1                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
enable_lxc="no"

dmt_log_output=0
leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=10                                  # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client=`echo $MSMR_ROOT`                    # root dir for m-smr. this command assumes that curent machine's m-smr path is the same as the servers'.
msmr_root_server=`echo $MSMR_ROOT`
input_url="127.0.0.1"                                 # url for client to query
analysis_tools=""     #"--worker1=helgrind"                                     # for executing analysis tools (e.g., analysis_tools="--worker1=helgrind")

if [ $1"X" != "X" ]; then
  if [ $1"X" == "joint_schedX" ]; then
    use_joint_scheduling_plan;
  elif [ $1"X" == "separate_schedX" ]; then
    use_separate_scheduling_plan;
  elif [ $1"X" == "xtern_onlyX" ]; then
    use_xtern_only_plan;
  elif [ $1"X" == "proxy_onlyX" ]; then
    use_proxy_only_plan;
  elif [ $1"X" == "origX" ]; then
    use_orig_plan;
  fi
  echo "The plan to run is: $1";
else
  echo "No plan specified. The default plan to run is: proxy_only";
  use_proxy_only_plan;
fi
sleep 1

if [ $proxy -eq 1 ]
then
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/mysql; \
	LD_PRELOAD=${msmr_root_server}/libevent_paxos/client-ld-preload/libclilib.so \
	./workload.sh ${primary_ip} 9000 &> workload-out.txt; cat workload-out.txt | grep real; ./cal-avg.sh workload-out.txt '"
else
    client_cmd="parallel-ssh -v -p 1 -i -t 15 -h head 'cd ${msmr_root_server}/apps/mysql; \
        ./workload.sh 127.0.0.1 7000 &> workload-out.txt; cat workload-out.txt | grep real; ./cal-avg.sh workload-out.txt  '"
fi

# We didn't use start-server script because we have to LD_PRELOAD for the whole scripts.
# Thus, server.conf will be corrupted by the xtern stderr(Init share memory stuff.)
server_cmd="'${msmr_root_server}/apps/mysql/mysql-install/libexec/mysqld \
	--defaults-file=${msmr_root_server}/apps/mysql/my.cnf &> server-out.txt & '"
