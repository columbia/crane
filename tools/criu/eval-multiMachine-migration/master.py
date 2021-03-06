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
cur_env = os.environ.copy()

def kill_previous_process(args):
    print "Killing previous related processes"
    cmd = 'killall -9 worker-run.py server.out %s &> /dev/null' % (args.app)
    rcmd = 'parallel-ssh -v -p 3 -i -t 10 -h hostfile {command}'.format(
            command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    #print output
    # killall criu-cr.py via sudo
    cmd = 'sudo killall -9 criu-cr.py &> /dev/null' 
    rcmd = 'parallel-ssh -v -p 1 -i -t 10 -h worker2 {command}'.format(
            command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()

def run_servers(args):
    cmd = "~/worker-run.py -a %s -x %d -c %s -m s -i 0 --scmd %s" % (args.app,
         args.xtern, args.msmr_root_server, args.scmd)
    #cmd = "~/worker-run.py -a %s -x %d -c %s -m s -i 0 --scmd %s" % (args.app,
     #       args.xtern, args.msmr_root_server, "'/home/bluecloudmatrix/m-smr/apps/mongoose/mg-server -p 7000'")
    print "replaying server master node command: "

    rcmd = "parallel-ssh -v -p 1 -i -t 10 -h head \"%s\"" % (cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    
    for node_id in xrange(1, 3):
        #wcmd = "~/worker-run.py -a %s -x %d -c %s -m r -i %d --scmd %s" % (
          #      args.app, args.xtern, args.msmr_root_server, node_id, "'/home/bluecloudmatrix/m-smr/apps/mongoose/mg-server -p 700'"+str(node_id))
        wcmd = "~/worker-run.py -a %s -x %d -c %s -m r -i %d --scmd %s" % (
                args.app, args.xtern, args.msmr_root_server, node_id, args.scmd)
        rcmd_workers = "parallel-ssh -v -p 1 -i -t 10 -h worker%d \"%s\"" % (
                node_id, wcmd)
        print "Master: replaying master node command: "
        print rcmd_workers
        # Start the secondary nodes one by one
        p = subprocess.Popen(rcmd_workers, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
    

def restart_head(args):
    #cmd = '"~/head-restart.py"'
    cmd = 'killall -9 server.out'
    rcmd_head = 'parallel-ssh -v -p 1 -i -t 10 -h head {command}'.format(
        command=cmd)
    p = subprocess.Popen(rcmd_head, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_clients(args):
    cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    cmd = '$MSMR_ROOT/apps/apache/install/bin/ab -n 10 -c 1 http://128.59.17.172:9000/'
    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    
def run_clients2(args):
    cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    cmd = '$MSMR_ROOT/apps/apache/install/bin/ab -n 10 -c 1 http://128.59.17.173:9001/'
    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_criu(args):
    shcmd = "tmux send -t checkpoint:1 'sudo ~/criu-cr.py &> ~/criu-cr.log' ENTER"
    psshcmd = "parallel-ssh -v -p 1 -i -t 5 -h worker2 \"%s\""%(shcmd)
    print "Master: replaying master node command: "
    print psshcmd
    p = subprocess.Popen(psshcmd, shell=True, executable='/bin/bash', stdout=subprocess.PIPE)
    
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

    # Read param file

    # Create directory for storing logs 
    #log_name =  datetime.datetime.now().strftime("%Y%m%d-%H-%M-%S")
    #log_dir = "%s%s-%s" % (param["LOGDIR"], options.identifier, log_name)

    #Utils.mkdir(log_dir)
    
    #console_log = "%s/console.log" % log_dir

    # Killall the previous experiment
    kill_previous_process(args) 

    # Clean up the directory

    run_servers(args) 
    time.sleep(10)

    # CRIU
    run_criu(args)

    time.sleep(10)
    run_clients(args)
    time.sleep(20)

    #restart_head(args)
    #time.sleep(20)

    #run_clients2(args)
    kill_previous_process(args) 


###############################################################################
# Main - Parse command line options and invoke main()   
if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Explauncher(master)')

    parser.add_argument('-a', type=str, dest="app", action="store",
            help="Name of the application. e.g. microbenchmark.")
    parser.add_argument('-x', type=int, dest="xtern", action="store",
            help="Whether xtern is enabled.")
    parser.add_argument('-c', type=str, dest="msmr_root_client", action="store",
            help="The directory of m-smr.")
    parser.add_argument('-s', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")
    parser.add_argument('--scmd', type=str, dest="scmd", action="store",
            help="The command to execute the real server.")

    args = parser.parse_args()
    print "Replaying parameters:"
    print "App : " + args.app
    print "xtern : " + str(args.xtern)
    print "msmr_root_client : " + args.msmr_root_client
    print "msmr_root_server : " + args.msmr_root_server

    main_start_time = time.time()

    MSMR_ROOT = args.msmr_root_client
    main(args)

    main_end_time = time.time()

    logger.info("Total time : %f sec", main_end_time - main_start_time)
