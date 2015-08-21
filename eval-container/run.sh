#!/bin/bash

# This is the starter file of the whole experiment.

if [ ! $1 ];
then
        echo "Usage: $0 <application cfg file> <optional flags>"
        echo "Example (build project and run): $0 configs/mongoose.sh"
	echo "Example (run only): $0 configs/mongoose.sh no_build"
	echo "Example (run only): $0 configs/mongoose.sh build joint_sched"
	echo ""
	echo "Example (run only): $0 configs/mongoose.sh no_build joint_sched"
	echo "Example (run only): $0 configs/mongoose.sh no_build separate_sched"
	echo "Example (run only): $0 configs/mongoose.sh no_build xtern_only"
	echo "Example (run only): $0 configs/mongoose.sh no_build proxy_only"	
	echo "Example (run only): $0 configs/mongoose.sh no_build orig"
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
	if [ $2 == "build" ];
	then
	        build_project="true";
	fi
fi

if [ $3"X" != "X" ];
then
	if [ $3"X" == "joint_schedX" ];
		use_joint_scheduling_plan;
	fi
	if [ $3"X" == "separate_schedX" ];
		use_separate_scheduling_plan;
	fi
	if [ $3"X" == "xtern_onlyX" ];
		use_xtern_only_plan;
	fi
	if [ $3"X" == "proxy_onlyX" ];
		use_proxy_only_plan;
	fi
	if [ $3"X" == "origX" ];
		use_orig_plan;
	fi
	echo "The plan to run is: $3";
	sleep 1
else
	echo "No plan specified. The default plan to run is: proxy_only";
	use_proxy_only_plan;
	sleep 1
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

