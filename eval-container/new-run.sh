#!/bin/bash

# a new starter file that runs the test several times
# ./run.sh configs/mongoose.sh no_build joint_sched 10
mkdir -p perf_log

if [ ! $1 ]; then
  echo "invalid usage. "
fi

# run the server config script
source $1 $3;

build_project="true"
if [ $2"X" != "X" ]; then
  if [ $2 == "no_build" ]; then
    build_project="false";
	elif [ $2 == "build" ]; then
    build_project="true";
	fi
fi

if [ $build_project == "true" ]; then
  # Update worker-build.py to the server
  scp worker-build.py bug03.cs.columbia.edu:~/
  scp worker-build.py bug01.cs.columbia.edu:~/
  scp worker-build.py bug02.cs.columbia.edu:~/
fi

# Update worker-run.py to the server
scp worker-run.py bug03.cs.columbia.edu:~/
scp worker-run.py bug01.cs.columbia.edu:~/
scp worker-run.py bug02.cs.columbia.edu:~/

# Update criu-cr.py to the bug02 (a.k.a. Node 2)
scp criu-cr.py bug02.cs.columbia.edu:~/

echo "running master.py for $4 rounds"
for i in `seq 1 $4`; do
  echo "round $i"
  ./master.py -a ${app} -x ${xtern} -p ${proxy} -l ${leader_elect} \
    -k ${checkpoint} -t ${checkpoint_period} \
    -c ${msmr_root_client} -s ${msmr_root_server} \
    --sp ${sch_paxos} --sd ${sch_dmt} \
    --scmd "${server_cmd}" --ccmd "${client_cmd}" -b ${build_project} ${analysis_tools} \
    --enable-lxc ${enable_lxc} --dmt-log-output ${dmt_log_output} | tee perf_log/$app-$3-`date +"%s"`.log
done

