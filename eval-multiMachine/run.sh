#!/bin/bash

# This is the starter file of the whole experiment.

source configs/new-mongoose.sh
#source configs/mongoose.sh
#source configs/apache.sh
#source configs/ssdb.sh
#source configs/pgsql.sh
#source configs/mongod.sh
#source configs/proftpd.sh
#source configs/mysqld.sh
#source configs/clamav.sh
#source configs/mediatomb.sh

# Update worker-build.py to the server
scp worker-build.py bug03.cs.columbia.edu:~/
scp worker-build.py bug01.cs.columbia.edu:~/
scp worker-build.py bug02.cs.columbia.edu:~/

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
        --scmd "${server_cmd}" --ccmd "${client_cmd}" -b "true"

