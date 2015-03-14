# Setups for pgsql
# Notice :
# You need to manually change the server side conf file in data folder.
# Set the port to 7000
app="postgres"                                        # app name appears in process list
xtern=0                                               # 1 use xtern, 0 otherwise
proxy=1                                               # 1 use proxy, 0 otherwise
sch_paxos=0                                           # 1 xtern will schedule with paxos, 0 otherwise
sch_dmt=0                                             # 1 libevent_paxos will schedule with DMT, 0 otherwise
leader_elect=0                                        # 1 enable leader election demo, 0 otherwise
checkpoint=0                                          # 1 use checkpoint on relicas, 0 otherwise
checkpoint_period=10                                  # period of CRIU checkpoint, e.g. 10 seconds
msmr_root_client="/home/ruigu/Workspace/m-smr"        # root dir for m-smr
msmr_root_server="/home/ruigu/SSD/m-smr"
input_url="127.0.0.1"                                 # url for client to query

client_cmd="cd ${msmr_root_client}/apps/pgsql/7000/install && \
            bin/pgbench -i -U root dbtest -h 128.59.17.172 -p 9000 -j 10 -c 20 && \
            bin/pgbench -U root dbtest -h 128.59.17.172 -p 9000 -j 10 -c 20 -t 100 "
                                                      # command to start the clients
server_cmd="'cd ${msmr_root_server}/apps/pgsql/7000/install && bin/pg_ctl start -D ./data && \
             sleep 2 && bin/createdb -O root dbtest '"
                                                      # command to start the real server
