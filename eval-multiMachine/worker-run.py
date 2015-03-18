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
XTERN_ROOT = ''

def execute_proxy(args):

    cur_env = os.environ.copy()
    # Preparing the environment
    cur_env['MSMR_ROOT'] = MSMR_ROOT
    cur_env['XTERN_ROOT'] = XTERN_ROOT
    cur_env['LD_LIBRARY_PATH'] = MSMR_ROOT + '/libevent_paxos/.local/lib'
    if not os.path.isfile('nodes.local.cfg'):
        print "Copy nodes.local.cfg to current folder"
        tcmd = 'cp $MSMR_ROOT/libevent_paxos/target/nodes.local.cfg .'
        proc =subprocess.Popen(tcmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        time.sleep(1)
    os.system("sed -i -e 's/sched_with_dmt = [0-9]\+/sched_with_dmt = " + str(args.sd) + "/g' nodes.local.cfg")

    # Copy the xtern configuration file to the current folder 
    if not os.path.isfile('local.options'):
        print "Copy default.options to current folder"
        tcmd = 'cp $XTERN_ROOT/default.options ~/local.options'
        proc = subprocess.Popen(tcmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        time.sleep(1)
    os.system("sed -i -e 's/sched_with_paxos = [0-9]\+/sched_with_paxos = " + str(args.sp) + "/g' local.options")

        
    cur_env['CONFIG_FILE'] = 'nodes.local.cfg'
    cur_env['SERVER_PROGRAM'] = MSMR_ROOT + '/libevent_paxos/target/server.out'
    
    cmd = 'rm -rf ./.db ./log && mkdir ./log && ulimit -c unlimited && \
           $SERVER_PROGRAM -n %d -r -m %s -c $CONFIG_FILE -l ./log 1> ./log/node_%d_stdout 2>./log/node_%d_stderr &' % (
           args.node_id, args.mode, args.node_id, args.node_id)
    print "Replaying Proxy cmd : " + cmd

    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def execute_servers(args):

    cur_env = os.environ.copy()
    cmd = args.scmd
    print "Replay real server command:"
    print cmd

    if args.xtern == 1:
        print "XTERN is enabled. Preload library."
        # PreLoad xtern library here
        cur_env['LD_PRELOAD'] = XTERN_ROOT + '/dync_hook/interpose.so'
        # Sleep a while to load the lib
        time.sleep(2)

    # Don't add print
    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)

def restart_proxy(args):

    cur_env = os.environ.copy()
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
    time.sleep(8)
    if args.proxy == 1:
        execute_proxy(args)


###############################################################################
# Main - Parse command line options and invoke main()
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Explauncher(worker)')

    parser.add_argument('-a', type=str, dest="app", action="store",
            help="Name of the application. e.g. microbenchmark.")
    parser.add_argument('-x', type=int, dest="xtern", action="store",
            help="Whether xtern is enabled.")
    parser.add_argument('-p', type=int, dest="proxy", action="store",
            help="Whether proxy is enabled.")
    parser.add_argument('-k', type=int, dest="checkpoint", action="store",
            help="Whether checkpointing on replicas is enabled.")
    parser.add_argument('-c', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")
    parser.add_argument('-m', type=str, dest="mode", action="store",
            help="The mode of the node, main or secondary.")
    parser.add_argument('-i', type=int, dest="node_id", action="store",
            help="The id of the node.")
    parser.add_argument('--sp', type=int, dest="sp", action="store",
            help="Schedule with paxos.")
    parser.add_argument('--sd', type=int, dest="sd", action="store",
            help="Schedule with DMT.")
    parser.add_argument('--scmd', type=str, dest="scmd", action="store",
            help="The command to execute the real server.")

    args = parser.parse_args()
    # Checking missing arguments
    
    main_start_time = time.time()

    MSMR_ROOT = args.msmr_root_server
    XTERN_ROOT = MSMR_ROOT + '/xtern'
    main(args)

    main_end_time = time.time()

