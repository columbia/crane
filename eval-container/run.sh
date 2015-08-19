#!/bin/bash

# This is the starter file of the whole experiment.

if [ ! $1 ];
then
        echo "Usage: $0 <application cfg file> <optional flags>"
        echo "Example (build project and run): $0 configs/mongoose.sh"
	echo "Example (run only): $0 configs/mongoose.sh no_build"
        exit 1;
fi

source $1;
build_project="true";
if [ $2"X" != "X" ];
then
	if [ $2 == "no_build" ];
	then
	        build_project="false";
	fi
fi

if [ $build_project == "true" ];
then
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

./master.py -a ${app} -x ${xtern} -p ${proxy} -l ${leader_elect} \
        -k ${checkpoint} -t ${checkpoint_period} \
        -c ${msmr_root_client} -s ${msmr_root_server} \
        --sp ${sch_paxos} --sd ${sch_dmt} \
        --scmd "${server_cmd}" --ccmd "${client_cmd}" -b ${build_project} ${analysis_tools} \
	--enable-lxc ${enable_lxc} --dmt-log-output ${dmt_log_output}

