#!/bin/bash

# This is the starter file of the whole experiment.

source configs/mongoose.sh
#source configs/apache.sh

# Update worker-run.py to the server
scp worker-run.py bug00.cs.columbia.edu:~/
scp worker-run.py bug01.cs.columbia.edu:~/
scp worker-run.py bug02.cs.columbia.edu:~/

./master.py -a ${app} -x ${xtern} -p ${proxy} -k ${checkpoint} \
        -c ${msmr_root_client} \
        -s ${msmr_root_server} --scmd "${server_cmd}"
#./master.py -a ${app} -f ${flavor} -p ${conf_file} -q ${conf_client_file} \
  #-m -i ${application}-${flavor}-${id}-n${t_server_machines}-m${t_client_machines} \
  #-s${t_servers}-c${t_clients}-b${t_buildings}-p${t_primes}

sleep 2
