#!/bin/bash

# This is the starter file of the whole experiment.

# Setups for Apache
#app="httpd"                                           # app name appears in process list
#xtern=0                                               # 1 use xtern, 0 otherwise.
#msmr_root_client="/home/ruigu/Workspace/m-smr"        # root dir for m-smr
#msmr_root_server="/home/ruigu/SSD/m-smr"
#input_url="127.0.0.1"                                 # url for client to query

#client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 10 -c 10 http://128.59.17.171:9000/"
                                                      ## command to start the clients
#server_cmd="'${msmr_root_server}/apps/apache/install/bin/apachectl -f ${msmr_root_server}/apps/apache/install/conf/httpd.conf -k start '"
                                                      # command to start the real server
# Setups for mongoose
app="mg-server"                                       # app name appears in process list
xtern=1                                               # 1 use xtern, 0 otherwise.
msmr_root_client="/home/ruigu/Workspace/m-smr"        # root dir for m-smr
msmr_root_server="/home/ruigu/SSD/m-smr"
input_url="127.0.0.1"                                 # url for client to query

client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 10 -c 10 http://128.59.17.171:9000/"
                                                      # command to start the clients
server_cmd="'${msmr_root_server}/apps/mongoose/mg-server -t 2 '"
                                                      # command to start the real server

# Update worker-run.py to the server
scp worker-run.py bug00.cs.columbia.edu:~/
scp worker-run.py bug01.cs.columbia.edu:~/
scp worker-run.py bug02.cs.columbia.edu:~/

./master.py -a ${app} -x ${xtern} -c ${msmr_root_client} -s ${msmr_root_server} --scmd "${server_cmd}"
#./master.py -a ${app} -f ${flavor} -p ${conf_file} -q ${conf_client_file} \
  #-m -i ${application}-${flavor}-${id}-n${t_server_machines}-m${t_client_machines} \
  #-s${t_servers}-c${t_clients}-b${t_buildings}-p${t_primes}

sleep 2
