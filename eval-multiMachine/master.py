#!/usr/bin/python2.7

import argparse
import time
import logging
import datetime
import signal
import sys
import os
import subprocess

logger = logging.getLogger("Benchmark.Master")

MSMR_ROOT = ''

def kill_previous_process(args):
    print "Removing temporaries"
    cmd = 'rm -rf /dev/shm/*-$USER; \
           rm -rf /tmp/paxos_queue_file_lock; \
           rm -rf $HOME/paxos_queue_file_lock'
    rcmd = 'parallel-ssh -v -p 3 -i -t 15 -h hostfile {command}'.format(
            command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    print "Killing residual processes"
    cmd = 'sudo killall -9 worker-run.py server.out %s' % (args.app)
    rcmd = 'parallel-ssh -v -p 3 -i -t 15 -h hostfile {command}'.format(
            command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    # killall criu-cr.py via sudo
    # bug02 worker2
    cmd = 'sudo killall -9 criu-cr.py &> /dev/null' 
    rcmd = 'parallel-ssh -v -p 1 -i -t 15 -h worker2 {command}'.format(
            command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_servers(args):
    cmd = "~/worker-run.py -a %s -x %d -p %d -k %d -c %s -m s -i 0 --sp %d --sd %d --scmd %s" % (
            args.app, args.xtern, args.proxy, args.checkpoint,
            args.msmr_root_server, args.sp, args.sd, args.scmd)
    print "replaying server master node command: "

    rcmd = "parallel-ssh -v -p 1 -i -t 15 -h head \"%s\"" % (cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    # We only test one node for now
    # Don't forget to change nodes.local.cfg on bug03!!!!!
    #return

    if args.proxy == 0:
        return

    for node_id in xrange(1, 3):
        wcmd = "~/worker-run.py -a %s -x %d -p %d -k %d -c %s -m r -i %d --sp %d --sd %d --scmd %s" % (
                args.app, args.xtern, args.proxy, args.checkpoint,
                args.msmr_root_server, node_id, args.sp, args.sd, args.scmd)
        rcmd_workers = "parallel-ssh -v -p 1 -i -t 15 -h worker%d \"%s\"" % (
                node_id, wcmd)
        print "Master: replaying master node command: "
        print rcmd_workers
        # Start the secondary nodes one by one
        p = subprocess.Popen(rcmd_workers, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output

def restart_head(args):
    #cmd = '"~/head-restart.py"'
    cmd = 'sudo killall -9 server.out'
    rcmd_head = 'parallel-ssh -v -p 1 -i -t 15 -h head {command}'.format(
        command=cmd)
    p = subprocess.Popen(rcmd_head, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_clients(args):
    cur_env = os.environ.copy()
    # Agly hack on the applications that requires client and server on the same side
    if args.proxy == 1 and args.app != "clamd":
        print "Preload client library"
        cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    print "client cmd reply : " + args.ccmd

    p = subprocess.Popen(args.ccmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    
# note: must use sudo
# we run criu on node 2(bug02), so parallel-ssh should -h worker2
def run_criu(args):
    shcmd = "sudo ~/criu-cr.py -s %s -t %d &> ~/criu-cr.log" % (args.app, args.checkpoint_period)
    psshcmd = "parallel-ssh -v -p 1 -i -t 5 -h worker2 \"%s\""%(shcmd)
    print "Master: replaying master node command: "
    print psshcmd
    p = subprocess.Popen(psshcmd, shell=True, stdout=subprocess.PIPE)
    '''
    # below is for debugging
    output, err = p.communicate()
    print output
    if "FAILURE" in output:
        print "killall directly"
        kill_previous_process(args) 
        sys.exit(0)
    '''

def main(args):
    """
    Main module of master.py
    """

    # Killall the previous experiment
    kill_previous_process(args) 

    run_servers(args)
    print "Deployment Done! Wait awhile for the servers to become stable!!!"
    time.sleep(10)
    
    # Sending requests before the expriments
    print "Client starts : !!! Before checkpoint !!!"
    run_clients(args)
    time.sleep(5)

    if args.checkpoint == 1:
        # run CRIU on bug02(Node 2)
        run_criu(args)
        # make sure wait for at least 20s before running client
        # to let CRIU run in the appropriate environment
        # rest assured. real server and libevent_paxos still run normally without influence
        # even if when the CRIU does dump work, beacuse CRIU does dump and then let the
        # checkpointed process run again will be finished in a flash, about 20~60ms
        time.sleep(20)

    # Sending requests after the expriments
    #print "Client starts : !!! After checkpoint !!!"
    #run_clients(args)
    #time.sleep(5)

    # Starts the leader election demo
    if args.leader_ele == 1 and args.proxy == 1:
        restart_head(args)
        time.sleep(20)

        run_clients(args)

    kill_previous_process(args) 


###############################################################################
# Main - Parse command line options and invoke main()   
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Explauncher(master)')

    parser.add_argument('-a', type=str, dest="app", action="store",
            help="Name of the application. e.g. microbenchmark.")
    parser.add_argument('-x', type=int, dest="xtern", action="store",
            help="Whether xtern is enabled.")
    parser.add_argument('-p', type=int, dest="proxy", action="store",
            help="Whether proxy is enabled.")
    parser.add_argument('-l', type=int, dest="leader_ele", action="store",
            help="Whether demo leader election.")
    parser.add_argument('-k', type=int, dest="checkpoint", action="store",
            help="Whether checkpointing on replicas is enabled.")
    parser.add_argument('-t', type=int, dest="checkpoint_period", action="store",
            help="Period of CRIU checkpoint")
    parser.add_argument('-c', type=str, dest="msmr_root_client", action="store",
            help="The directory of m-smr.")
    parser.add_argument('-s', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")
    parser.add_argument('--sp', type=int, dest="sp", action="store",
            help="Schedule with paxos.")
    parser.add_argument('--sd', type=int, dest="sd", action="store",
            help="Schedule with DMT.")
    parser.add_argument('--scmd', type=str, dest="scmd", action="store",
            help="The command to execute the real server.")
    parser.add_argument('--ccmd', type=str, dest="ccmd", action="store",
            help="The command to execute the client.")

    args = parser.parse_args()
    print "Replaying parameters:"
    print "app : " + args.app
    print "xtern : " + str(args.xtern)
    print "leader election : " + str(args.leader_ele)
    print "checkpoint : " + str(args.checkpoint)
    print "checkpoint_period : " + str(args.checkpoint_period)
    print "MSMR_ROOT : " + args.msmr_root_client
    print "proxy : " + str(args.proxy)

    main_start_time = time.time()

    MSMR_ROOT = args.msmr_root_client
    main(args)

    main_end_time = time.time()

    logger.info("Total time : %f sec", main_end_time - main_start_time)
