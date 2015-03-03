#!/bin/bash

# This is the starter file of the whole experiment. This file automatically generate the configuration file with a suffix of .cfg.

app="Apache"

./master.py -a ${app}
#./master.py -a ${app} -f ${flavor} -p ${conf_file} -q ${conf_client_file} \
  #-m -i ${application}-${flavor}-${id}-n${t_server_machines}-m${t_client_machines} \
  #-s${t_servers}-c${t_clients}-b${t_buildings}-p${t_primes}

sleep 2
