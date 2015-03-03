#!/usr/bin/python2.7

from optparse import OptionParser
from multiprocessing import Process
import time
import logging
import subprocess
import os

logger = logging.getLogger("Benchmark.Worker")

#Remote directory enviroment variable
MSMR_ROOT = '/home/ruigu/SSD/m-smr'
cur_env = os.environ.copy()
cur_env['MSMR_ROOT'] = '/home/ruigu/SSD/m-smr'

def execute_proxy():

    # Preparing the environment
    cur_env['LD_LIBRARY_PATH'] = MSMR_ROOT + '/libevent_paxos/.local/lib'
    cur_env['CONFIG_FILE'] = MSMR_ROOT + '/libevent_paxos/target/nodes.local.cfg'
    cur_env['SERVER_PROGRAM'] = MSMR_ROOT + '/libevent_paxos/target/server.out'


    cmd = '$SERVER_PROGRAM -n 0 -r -m s -c $CONFIG_FILE'

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def execute_servers():

    cmd = '$MSMR_ROOT/apps/apache/install/bin/apachectl \
        -f $MSMR_ROOT/apps/apache/install/conf/httpd.conf \
        -k start'

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def execute_clients():

    # Clients send requests to proxy server
    cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    cmd = '$MSMR_ROOT/apps/apache/install/bin/ab \
        -n 10 -c 10 http://128.59.17.171:9000/'

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def main(options):
    """
    Main module of worker-run.py
    """
    execute_servers()
    # Wait a while fot the real server to set up
    time.sleep(5)
    # Wait a while fot the proxy server to set up
    execute_proxy()
    #time.sleep(5)
    #execute_clients()


###############################################################################
# Main - Parse command line options and invoke main()
if __name__ == "__main__":
    parser = OptionParser(description="Microbenchmark experiment tool (worker).")

    (options, args) = parser.parse_args()

    # Checking missing arguments
    
    main_start_time = time.time()

    main(options)

    main_end_time = time.time()

