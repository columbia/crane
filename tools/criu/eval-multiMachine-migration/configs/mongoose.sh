# Setups for mongoose
app="mg-server"                                       # app name appears in process list
xtern=0                                               # 1 use xtern, 0 otherwise.
msmr_root_client="/home/liuqiushan/BCMatrix/Replication_Columbia/m-smr"        # root dir for m-smr
msmr_root_server="/home/bluecloudmatrix/m-smr"
input_url="127.0.0.1"                                 # url for client to query

client_cmd="${msmr_root_client}/apps/apache/install/bin/ab -n 10 -c 1 http://128.59.17.172:9000/"
                                                      # command to start the clients
server_cmd="' ${msmr_root_server}/apps/mongoose/mg-server -t 2 '"
                                                      # command to start the real server
