#!/usr/bin/python2.7

import argparse
from multiprocessing import Process
import time
import logging
import subprocess
import argparse
import os
from os.path import expanduser

logger = logging.getLogger("Benchmark.Worker")

#Remote directory enviroment variable
MSMR_ROOT = ''
XTERN_ROOT = ''
GIT_VERSION = ''
CONTAINER = "u1"
HOME = expanduser("~") # Assume the MSMR_ROOT path is the same in host OS as in lxc container.
USER = os.environ["USER"]

def exec_cmd_with_env(strcmd):
    cur_env = os.environ.copy()
    cur_env['MSMR_ROOT'] = MSMR_ROOT
    cur_env['XTERN_ROOT'] = XTERN_ROOT    
    # Read current machine's env variable: this require current machine's project path setting is the same as those server machines.
    if os.environ.has_key('LD_LIBRARY_PATH'):
        cur_env['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH'] + ":" + MSMR_ROOT + "/libevent_paxos/.local/lib"
    else:
        cur_env['LD_LIBRARY_PATH'] = MSMR_ROOT + "/libevent_paxos/.local/lib"
    proc = subprocess.Popen(strcmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
    output, err = proc.communicate()
    print output
    time.sleep(1)

def main(args):
    """
    Main module of worker-runbuild.py
    """
    print("Rebuilding the whole project at git version: %s " % (GIT_VERSION))
    cmd = "git config --global user.email \"%s@cs.columbia.edu\"" % (USER)
    os.system(cmd)
    cmd = "git config --global user.name \"%s\"" % (USER);
    os.system(cmd)

    # Get the latest git version.
    dirstring = "%s" % (XTERN_ROOT)
    os.chdir(dirstring)
    os.system("git stash")

    dirstring = "%s/libevent_paxos" % (MSMR_ROOT)
    os.chdir(dirstring)
    os.system("git stash")

    dirstring = "%s" % (MSMR_ROOT)
    os.chdir(dirstring)
    os.system("git stash")
    os.system("git pull")
    os.system("git submodule update")

    # Build.
    dirstring = "%s/obj" % (XTERN_ROOT)
    os.chdir(dirstring)
    exec_cmd_with_env("make clean; make; make install &> /dev/null")

    dirstring = "%s/libevent_paxos" % (MSMR_ROOT)
    os.chdir(dirstring)
    exec_cmd_with_env("make clean; make &> /dev/null")

    if args.enable_lxc == "yes":
        # First, restart the container.
        cur_env = os.environ.copy()
        os.chdir(HOME)
        print "Restarting the lxc container %s" %(CONTAINER)
        cmd = "sudo lxc-stop -n %s" % (CONTAINER)
        p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
        cmd = "sudo lxc-start -n %s" % (CONTAINER)
        p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
        time.sleep(1)
    
        # Copy the worker-build.py into lxc via /dev/shm (already setup the map).
        cmd = "cp worker-build.py /dev/shm"
        p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output
        cmd = "sudo lxc-attach -n %s -- mv /dev/shm/worker-build.py %s" % (CONTAINER, HOME) 
        p = subprocess.Popen(cmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output

        # Run worker-build.py, but this time without lxc-enabled (because we are not nested lxc).
        cmd = "~/worker-build.py -s %s" % (MSMR_ROOT)
        psshcmd = "sudo lxc-attach -n %s -- " % (CONTAINER)
        psshcmd = psshcmd + cmd;
        p = subprocess.Popen(psshcmd, env=cur_env, shell=True, stdout=subprocess.PIPE)
        output, err = p.communicate()
        print output

###############################################################################
# Main - Parse command line options and invoke main()
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Explauncher(worker)')

    parser.add_argument('-v', type=str, dest="git_version", action="store", default="",
            help="Git version of the project.")
    parser.add_argument('-s', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")
    parser.add_argument('--enable-lxc', type=str, dest="enable_lxc",
            action="store", default="no", help="The tool to run the server in a lxc container.")

    args = parser.parse_args()
    # Checking missing arguments
    
    main_start_time = time.time()

    GIT_VERSION = args.git_version
    MSMR_ROOT = args.msmr_root_server
    XTERN_ROOT = MSMR_ROOT + '/xtern'
    main(args)

    main_end_time = time.time()

