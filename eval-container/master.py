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
USER = os.environ["USER"]
MSMR_ROOT = os.environ["MSMR_ROOT"]

def kill_previous_process(args):
    
    print "Killing residual processes"
    # Heming: added %s-amd64-, this is for killing valgrind sub processes such as helgrind-amd64-.
    cmd = 'sudo killall -9 mencoder worker-run.py server.out %s %s-amd64- %s-amd64- %s-amd64-' % (
            args.app, args.head, args.worker1, args.worker2)
    rcmd = 'parallel-ssh -l {username} -v -p 3 -i -t 15 -h hostfile {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    print "Removing temporaries"
    cmd = 'sudo rm -rf /dev/shm/*'
    rcmd = 'parallel-ssh -l {username} -v -p 3 -i -t 15 -h hostfile {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    # killall criu-cr.py via sudo
    # bug02 worker2
    cmd = 'sudo killall -9 criu-cr.py &> /dev/null' 
    rcmd = 'parallel-ssh -l {username} -v -p 1 -i -t 15 -h worker2 {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def build_project(args):
    cmd = "~/worker-build.py -s %s --enable-lxc %s" % (args.msmr_root_server, args.enable_lxc)
    print "Building project on node: "

    rcmd = "parallel-ssh -l %s -v -p 1 -i -t 25 -h head \"%s\"" % (USER, cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    for node_id in xrange(1, 3):
        wcmd = "~/worker-build.py -s %s --enable-lxc %s" % (args.msmr_root_server, args.enable_lxc)
        rcmd_workers = "parallel-ssh -l %s -v -p 1 -i -t 25 -h worker%d \"%s\"" % (
                USER, node_id, wcmd)
        print "Building project: "
        print rcmd_workers
        # Start the secondary nodes one by one
        p = subprocess.Popen(rcmd_workers, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output

def run_servers(args, start_proxy_only, start_server_only):
    print "Starting proxy only or starting server only: " + start_proxy_only + " " + start_server_only
    cmd = "~/worker-run.py -a %s -x %d -p %d -k %d -c %s -m s -i 0 --sp %d --sd %d --scmd %s --tool %s --enable-lxc %s --dmt-log-output %d --start_proxy_only %s --start_server_only %s" % (
            args.app, args.xtern, args.proxy, args.checkpoint,
            args.msmr_root_server, args.sp, args.sd, args.scmd, args.head, args.enable_lxc, args.dmt_log_output,
            start_proxy_only, start_server_only)
    print "replaying server master node command: "

    rcmd = "parallel-ssh -l %s -v -p 1 -i -t 25 -h head \"%s\"" % (USER, cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    # We only test one node for now
    # Don't forget to change nodes.local.cfg group_size on bug03!!!!!
    #return

    if args.proxy == 0:
        return

    for node_id in xrange(1, 3):
        worker_tool = "none"
        if node_id == 1 and args.worker1 != "none":
          worker_tool = args.worker1
        if node_id == 2 and args.worker2 != "none":
          worker_tool = args.worker2
        wcmd = "~/worker-run.py -a %s -x %d -p %d -k %d -c %s -m r -i %d --sp %d --sd %d --scmd %s --tool %s --enable-lxc %s --dmt-log-output %d --start_proxy_only %s --start_server_only %s" % (
                args.app, args.xtern, args.proxy, args.checkpoint,
                args.msmr_root_server, node_id, args.sp, args.sd, args.scmd, worker_tool, args.enable_lxc, args.dmt_log_output,
                start_proxy_only, start_server_only)
        rcmd_workers = "parallel-ssh -l %s -v -p 1 -i -t 25 -h worker%d \"%s\"" % (
                USER, node_id, wcmd)
        print "Master: replaying master node command: "
        print rcmd_workers
        # Start the secondary nodes one by one
        p = subprocess.Popen(rcmd_workers, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output

def restart_head(args):
    #cmd = '"~/head-restart.py"'
    cmd = 'sudo killall -9 server.out ' + args.app
    rcmd_head = 'parallel-ssh -l {username} -v -p 1 -i -t 15 -h head {command}'.format(
        username=USER, command=cmd)
    p = subprocess.Popen(rcmd_head, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    time.sleep(2)
    # We don't do checkpoint restore for now!
    return 

    cmd = "~/worker-run.py -a %s -x %d -p %d -k %d -c %s -m s -i 0 --start_proxy_only yes --enable-lxc yes --sp %d --sd %d --scmd %s  --enable-lxc %s" % (
            args.app, args.xtern, args.proxy, args.checkpoint,
            args.msmr_root_server, args.sp, args.sd, args.scmd, args.enable_lxc)
    print "replaying server master node command: "
    rcmd = "parallel-ssh -l %s -v -p 1 -i -t 25 -h head \"%s\"" % (USER, cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

    time.sleep(10)

    # Restore the checkpoint
    cmd = "cd %s/eval-container && ./checkpoint-restore.sh restore %s checkpoint-*.tar.gz >| restore_output &" % (
            MSMR_ROOT, args.app)
    print "replaying server master node command: "
    rcmd = "parallel-ssh -l %s -v -p 1 -i -t 600 -h head \"%s\"" % (USER, cmd)
    print rcmd
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output 
    

def run_clients(args, newIP=False):
    cur_env = os.environ.copy()
    # When client and server are required to be on the same side(mediatomb), 
    # you may wan to comment the following LD_PRELOAD line to prevent some errors.
    if args.proxy == 1 and args.app != "clamd" and args.app != "mediatomb":
        print "Preload client library"
        cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    # Just a temporary hack on the new server ip 
    if(newIP):
      args.ccmd = '/home/ruigu/Workspace/m-smr/apps/apache/install/bin/ab -n 128 -c 8 http://128.59.17.172:9000/test.php'
    print "client cmd reply : " + args.ccmd

    p = subprocess.Popen(args.ccmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    
# note: must use sudo
# we run criu on node 2(bug02), so parallel-ssh should -h worker2
#def run_criu(args):
    #shcmd = "sudo ~/criu-cr.py -s %s -t %d &> ~/criu-cr.log" % (args.app, args.checkpoint_period)
    #psshcmd = "parallel-ssh -v -p 1 -i -t 5 -h worker2 \"%s\""%(shcmd)
    #print "Master: replaying master node command: "
    #print psshcmd
    #p = subprocess.Popen(psshcmd, shell=True, stdout=subprocess.PIPE)
    #'''
    ## below is for debugging
    #output, err = p.communicate()
    #print output
    #if "FAILURE" in output:
        #print "killall directly"
        #kill_previous_process(args) 
        #sys.exit(0)
    #'''

def run_criu(args):
    # 1. Checkpoint the server process
    cmd = "sudo ~/su-checkpoint.sh %s" % (args.app)
    rcmd = 'parallel-ssh -l {username} -v -p 3 -i -t 15 -h head {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    # 2. Kill both proxy and server on bug03
    cmd = 'sudo killall -9 server.out %s' % (args.app)
    rcmd = 'parallel-ssh -l {username} -v -p 3 -i -t 15 -h head {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    # 3. Restore server process
    cmd = "sudo ~/su-restore.sh ~/%s-*" % (args.app)
    rcmd = 'parallel-ssh -l {username} -v -p 3 -i -t 15 -h head {command}'.format(
            username=USER, command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    # 4. Restore proxy


def main(args):
    """
    Main module of master.py
    """

    # Build the project to make sure all replicas are consistent.
    if args.build_project == "true":
        build_project(args)

    # Killall the previous experiment
    kill_previous_process(args) 

    run_servers(args, "yes", "no") # Start all proxies first, because we need proxy to connect with each other when the first server starts.
    run_servers(args, "no", "yes") # Then start the servers.
    print "Deployment Done! Wait 10s for the servers to become stable!!!"
    time.sleep(10)
    
    # Sending requests before the expriments
    print "Client starts : !!! Before checkpoint & leader election!!!"
    run_clients(args)
    client_sleep = 10
    if args.sd == 1 and args.sp == 1 and args.dmt_log_output == 1:
        client_sleep = 120
    exit_print = "Client workload done. Please grab performance result. Wait %d seconds before exit. " % (client_sleep)
    print exit_print
    time.sleep(client_sleep)

    if args.checkpoint == 1:
        # Start Qiushan's script
        # run CRIU only on bug02(Node 2)
        #run_criu(args)
        # Wait for at least 20s before running client
        # Checkpoint a process will be finished in a flash, about 20~60ms
        #time.sleep(20)

        # Start Heming's script
        print "Start Heming's checkpoint"
        run_criu(args)


    # Sending requests after the expriments
    #print "Let's wait 5s for the checkpointed process to become stable"
    #time.sleep(5)
    #print "Client starts : !!! After checkpoint !!!"
    #run_clients(args)
    #time.sleep(5)

    # Starts the leader election demo
    if args.leader_ele == 1 and args.proxy == 1:
        restart_head(args)
        #time.sleep(30)
        #print "Client starts : !!! After leader election!!!"
        #run_clients(args, True)

    print "Wait for 20s to kill all the processes"
    time.sleep(20)
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
    parser.add_argument('-b', type=str, dest="build_project", action="store", default="false",
            help="The command to rebuild the whole project.")
    parser.add_argument('--head', type=str, dest="head", action="store", default="none",
            help="The analysis tool to run on the head machine.")
    parser.add_argument('--worker1', type=str, dest="worker1", action="store", default="none",
            help="The analysis tool to run on the worker1 machine.")
    parser.add_argument('--worker2', type=str, dest="worker2", action="store", default="none",
            help="The analysis tool to run on the worker2 machine.")
    parser.add_argument('--enable-lxc', type=str, dest="enable_lxc",
            action="store", default="no", help="The tool to run the server in a lxc container.")
    parser.add_argument('--dmt-log-output', type=int, dest="dmt_log_output",
            action="store", default=0, help="Run the DMT and log outputs (send(), write()) of servers.")

    args = parser.parse_args()
    print "Replaying parameters:"
    print "app : " + args.app
    print "proxy : " + str(args.proxy)
    print "xtern : " + str(args.xtern)
    print "joint : " + str(args.sp) + " " + str(args.sd)
    print "leader election : " + str(args.leader_ele)
    print "checkpoint : " + str(args.checkpoint)
    print "checkpoint_period : " + str(args.checkpoint_period)
    print "MSMR_ROOT : " + args.msmr_root_client
    print "build project : " + args.build_project

    main_start_time = time.time()

    MSMR_ROOT = args.msmr_root_client
    main(args)

    main_end_time = time.time()

    logger.info("Total time : %f sec", main_end_time - main_start_time)
