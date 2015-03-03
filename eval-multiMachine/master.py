#!/usr/bin/python2.7

from optparse import OptionParser
import time
import logging
import datetime
import signal
import sys
import os
import subprocess

logger = logging.getLogger("Benchmark.Master")

MSMR_ROOT = '/home/ruigu/Workspace/m-smr'
cur_env = os.environ.copy()

def kill_previous_process():
    cmd = 'killall -9 worker-run.py server.out httpd'
    rcmd = 'parallel-ssh -v -p 3 -i -t 10 -h hostfile {command}'.format(
        command=cmd)
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_servers():
    cmd = '"~/worker-run.py"'
    rcmd = 'parallel-ssh -v -p 1 -i -t 10 -h head {command}'.format(
        command=cmd)
    rcmd_workers = 'parallel-ssh -v -p 2 -i -t 10 -h workers {command}'.format(
        command=cmd)
    # Start the head node first
    p = subprocess.Popen(rcmd, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    # Start the secondary nodes after awhile
    p = subprocess.Popen(rcmd_workers, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def restart_head():
    cmd = '"~/head-restart.py"'
    rcmd_head = 'parallel-ssh -v -p 1 -i -t 10 -h head {command}'.format(
        command=cmd)
    p = subprocess.Popen(rcmd_head, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def run_clients():
    cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    cmd = '$MSMR_ROOT/apps/apache/install/bin/ab -n 10 -c 10 http://128.59.17.171:9000/'
    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output
    
def run_clients2():
    cur_env['LD_PRELOAD'] = MSMR_ROOT + '/libevent_paxos/client-ld-preload/libclilib.so'
    cmd = '$MSMR_ROOT/apps/apache/install/bin/ab -n 10 -c 10 http://128.59.17.172:9001/'
    p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = p.communicate()
    print output

def main(options):
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
    kill_previous_process() 

    # Clean up the directory

    run_servers() 
    time.sleep(5)

    run_clients()
    time.sleep(5)

    restart_head()
    time.sleep(20)

    run_clients2()
    #kill_previous_process() 

    logger.info("Done")

    
if __name__ == "__main__":
    parser = OptionParser(description="Microbenchmark experiment tool.")

    parser.add_option("-a", "--application", dest="app", action="store", 
            type="string",
            help="Name of the application. e.g. microbenchmark.")

    (options, args) = parser.parse_args()
    
    # Check missing argument
    if not options.app:
        parser.error("Missing --application")

    main_start_time = time.time()

    main(options)

    main_end_time = time.time()

    logger.info("Total time : %f sec", main_end_time - main_start_time)
