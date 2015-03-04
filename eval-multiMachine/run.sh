#!/bin/bash

# This is the starter file of the whole experiment. This file automatically generate the configuration file with a suffix of .cfg.

app="httpd"                                           # app name appears in process list
xtern=0                                               # 1 use xtern, 0 otherwise.
msmr_root="/home/ruigu/Workspace/m-smr"               # root dir for m-smr
input_url="127.0.0.1"                                 # url for client to query
client_cmd="${msmr_root}/apps/apache/install/bin/ab -n 10 -c 10 http://128.59.17.171:9000/"
                                                      # command to start the clients

./master.py -a ${app} -x ${xtern} -d ${msmr_root}
#./master.py -a ${app} -f ${flavor} -p ${conf_file} -q ${conf_client_file} \
  #-m -i ${application}-${flavor}-${id}-n${t_server_machines}-m${t_client_machines} \
  #-s${t_servers}-c${t_clients}-b${t_buildings}-p${t_primes}

sleep 2
