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
GIT_VERSION = ''


def main(args):
    """
    Main module of worker-runbuild.py
    """
    print("Rebuilding the whole project at git version: %s " % (GIT_VERSION))

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
    os.system("make clean; make; make install")

    dirstring = "%s/libevent_paxos" % (MSMR_ROOT)
    os.chdir(dirstring)
    os.system("make clean; make")


###############################################################################
# Main - Parse command line options and invoke main()
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Explauncher(worker)')

    parser.add_argument('-v', type=str, dest="git_version", action="store",
            help="Git version of the project.")
    parser.add_argument('-s', type=str, dest="msmr_root_server", action="store",
            help="The directory of m-smr.")

    args = parser.parse_args()
    # Checking missing arguments
    
    main_start_time = time.time()

    GIT_VERSION = args.git_version
    MSMR_ROOT = args.msmr_root_server
    XTERN_ROOT = MSMR_ROOT + '/xtern'
    main(args)

    main_end_time = time.time()

