#!/usr/bin/python2.7

import argparse
from multiprocessing import Process
import time
import logging
import subprocess
import argparse
import os

logger = logging.getLogger("Benchmark.Worker")

#Remote directory enviroment variable
MSMR_ROOT = ''
cur_env = os.environ.copy()

def execute_proxy(args):

    # Preparing the environment
    cur_env['LD_LIBRARY_PATH'] = MSMR_ROOT + '/libevent_paxos/.local/lib'
    cur_env['CONFIG_FILE'] = MSMR_ROOT + '/libevent_paxos/target/nodes.local.cfg'
    cur_env['SERVER_PROGRAM'] = MSMR_ROOT + '/libevent_paxos/target/server.out'

    cmd = 'rm -rf ./.db ./log && mkdir ./log && \
           $SERVER_PROGRAM -n %d -r -m %s -c $CONFIG_FILE -l ./log 1> ./log/node_%d_stdout 2>./log/node_%d_stderr &' % (
           args.node_id, args.mode, args.node_id, args.node_id)

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def execute_servers(args):

    #cmd = '$MSMR_ROOT/apps/apache/install/bin/apachectl -f $MSMR_ROOT/apps/apache/install/conf/httpd.conf -k start'
    cmd = args.scmd
    print "replay real server command:"
    print cmd

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def restart_proxy(args):
    # Preparing the environment
    cur_env['LD_LIBRARY_PATH'] = MSMR_ROOT + '/libevent_paxos/.local/lib'
    cur_env['CONFIG_FILE'] = MSMR_ROOT + '/libevent_paxos/target/nodes.local.cfg'
    cur_env['SERVER_PROGRAM'] = MSMR_ROOT + '/libevent_paxos/target/server.out'


    # We don't restart the server for now
    cmd = 'killall -9 server.out'

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def main(args):
    """
    Main module of worker-run.py
    """

    execute_servers(args)
    # Wait a while fot the real server to set up
    time.sleep(5)
    # Wait a while fot the proxy server to set up
    execute_proxy(args)


###############################################################################
# Main - Parse command line options and invoke main()
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Explauncher(worker)')

    parser.add_argument('-a', type=str, dest="app", action="store",
            help="Name of the application. e.g. microbenchmark.")
    parser.add_argument('-x', type=int, dest="xtern", action="store",
            help="Whether xtern is enabled.")
    parser.add_argument('-c', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")
    parser.add_argument('-m', type=str, dest="mode", action="store",
            help="The mode of the node, main or secondary.")
    parser.add_argument('-i', type=int, dest="node_id", action="store",
            help="The id of the node.")
    parser.add_argument('--scmd', type=str, dest="scmd", action="store",
            help="The command to execute the real server.")

    args = parser.parse_args()
    # Checking missing arguments
    
    main_start_time = time.time()

    MSMR_ROOT = args.msmr_root_server
    main(args)

    main_end_time = time.time()

