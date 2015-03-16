#!/usr/bin/python2.7

# make sure install psutil first: 
#==========================================
# wget https://pypi.python.org/packages/source/p/psutil/psutil-2.1.3.tar.gz 
# tar zxvf psutil-2.1.3.tar.gz
# cd psutil-2.1.3/
# python setup.py install
#==========================================
# the script should be run via 'sudo criu-cr.py'
# make sure you have installed CRIU first
# but if you run the script on bug02, you can use CRIU of user bluecloudmatrix(i.e. Qiushan)
# The paths below are qiushan's, you can use them.
# the script on Node 2 will be called by parallel-ssh
# using it to do checkpoint/restore on libevent_paxos and real server(such as mongoose)
# Like a daemon, do C/R periodically

from subprocess import Popen, PIPE
import time
import os
import sys
import string 
import psutil
import re
from time import clock
import datetime
import argparse

CRIU_PATH = ''
CRIU_PROGRAM = ''
CRIU_IMAGE_PATH = ''
CRIU_SERVER_IMGAE_PATH = ''
cur_env = os.environ.copy()

# check the script process chain
def check_parent():
	print "[ Checking the process chain of the criu-cr.py script]"
	spid = get_pid('criu-cr.py')
	print "< criu-py pid is: %s >" % (spid)
	ppid = psutil.Process(spid).ppid()
	pname = psutil.Process(ppid).name()
	print "< its parent pid is: %s, name is: %s >" % (ppid, pname)
	gpid = psutil.Process(ppid).ppid()
	gname = psutil.Process(gpid).name()
	print "< its grandfather pid is: %s, name is: %s >" % (gpid, gname)
	print "[ Note: if its upstream processes has sshd, then CRIU will do checkpoint in a ssh environment which is a terrible thing, \
		and it is not qualified for the migration between the different physical machines. ]"
	print "[ Note: this version of the script criu-cr.py only supports CRIU to do checkpoint/restore on one machine and make sure not reboot]"
	print "[ Important!! \
		I have found that it has sshd, so it will fail when do migration between different physical machines, in fact, it fails indeed. \
		So I have done such: \
		when start to run the script, fistly let it sleep some time, in the time span\
		paralle-ssh will quit, so the parent pid of the script won't has sshd, in fact, the process chain will be changed to such:\
		init->bash->sudo->criu-cr.py. \
		But the cgroup.img is still not empty which will make our migration failed\
		So I need to do further work \
		so far, I want to use tumx attach"


# get the process pid according to its process name
# with the aid of psutil
# so you need to install psutil first
def get_pid(name):
	process_list = psutil.get_process_list()
	regex = "pid=(\d+),\sname=\'" + name + "\'"
	#print regex
	pid = 0
	for line in process_list:
		process_info = str(line)
		ini_regex = re.compile(regex)
		result = ini_regex.search(process_info)
		if result != None:
			pid = string.atoi(result.group(1))
			#print result.group()
			return pid
			break

# CRIU dumps the process and leave it stopped status after dump, then we use killall -SIGCONT to wake it up
def criu_dump(pid, identity, after_dump_status):
	if psutil.Process(pid).status() == psutil.STATUS_SLEEPING:
		print "< the process status is SLEEPING >"

	# set environment variables
	#cur_env['LD_LIBRARY_PATH'] = CRIU_PATH + '/../x86_64-linux-gnu/lib'
	cur_env['CRIU_PROGRAM'] = CRIU_PROGRAM
	if identity == 'client':
		cur_env['CRIU_IMAGE_PATH'] = CRIU_IMAGE_PATH
	elif identity == 'server':
		cur_env['CRIU_IMAGE_PATH'] = CRIU_SERVER_IMGAE_PATH
		
	# dump cmd
	if after_dump_status == '-s': # DUMP and leave it stopped after dump
		cmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/bluecloudmatrix/criu/deps/criu-x86_64/../x86_64-linux-gnu/lib && \
			$CRIU_PROGRAM dump -s -D $CRIU_IMAGE_PATH -t %s \
			--shell-job --tcp-established' % (pid)
	elif after_dump_status == '-d': # after dump, kill the process
		cmd = 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/bluecloudmatrix/criu/deps/criu-x86_64/../x86_64-linux-gnu/lib && \
			$CRIU_PROGRAM dump -D $CRIU_IMAGE_PATH -t %s \
			--shell-job --tcp-established' % (pid)

	print "[ replay real command ]"
	print cmd

	start_time = datetime.datetime.now()
	feedback = Popen(cmd, env=cur_env, shell=True, stdout=PIPE, stderr=PIPE)
	std_out, std_err = feedback.communicate()
	print "[ CRIU dumping stdout is: ]"
	print std_out 
	print "[ CRIU dumping stderr is: ]"
	print std_err
	print "[ Note! If the above info didn't show Dumping Failed explicitly, just show some errors, please ignore them.]"
	finish_time = datetime.datetime.now()
	print "[ have finished dumping, the usage time of CRIU dump (ms): ]"
	print int((finish_time - start_time).total_seconds()*1000) # on bug02, it shows about 60ms


# Wake Up
# in criu dump, we add -s which makes libevent_paxos process stopped after dumping
# now we use killall instead of kill to wake it up
def wake_check(pid, pname):
	if psutil.Process(pid).status() == psutil.STATUS_STOPPED:
		print "< the process status is STOPPED >"
	print "< now wake up the process >"
	cmd = 'killall -SIGCONT ' + pname
	print "[ replay real command ]"
	print cmd
	p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
	out, err = p.communicate()
	print out 
	print err

	# check
	pid_1 = get_pid(pname)
	if pid_1 == pid:
		if psutil.Process(pid_1).status() == psutil.STATUS_RUNNING:
			print "wake up successfully! RUNNING"
		elif psutil.Process(pid_1).status() == psutil.STATUS_SLEEPING:
			print "wake up successfully! SLEEPING"
		elif psutil.Process(pid_1).status() == psutil.STATUS_STOPPED:
			print "wake up failed! STOPPED"
		else:
			print "other status: %s" % (psutil.Process(pid_1).status())	


# criu checkpoint/restore
def criu_cr(args):
	print "[ enter CRIU ]"

	# get the pid of the checkpointed process
	# args.server is the name of the real server process
	# 'server.out' is the name of libevent_paxos process
	pid_s = get_pid(args.server)
	pid_c = get_pid('server.out')
	if pid_s > 0 and pid_c > 0:
		# dump
		criu_dump(pid_s, 'server', '-s')
		criu_dump(pid_c, 'client', '-s')
		# wake them up by killall -SIGCONT
		wake_check(pid_s, args.server)
		wake_check(pid_c, 'server.out')
	elif pid_s == 0 :
		print "< the real server process does not exist >"
	elif pid_c == 0:
		print "< the server.out process does not exist >"

	print "[ quit CRIU ]"


# Main Func
# 10/03/15
if __name__ == "__main__":
        parser = argparse.ArgumentParser(description='CRIU checkpoint/restore')
        parser.add_argument('-s', type=str, dest="server", action="store",
            help="Name of the real server. e.g. mg-server")
        parser.add_argument('-t', type=int, dest="period", action="store",
            help="Period of CRIU checkpoint")

        args = parser.parse_args()


	# for migration, to check the parent process of the criu-cr.py script
	check_parent()

	# skill: waiting for the sshd of parallel-ssh to quit
	# it should be in a flash, because the remote cmd does not use  p.communicate() to block
	# so we choose an appropriate time span
	time.sleep(10) 

	# the script should be run on bug02(a.k.a, Node 2) 
        # make sure 'sudo criu-cr.py'
        # if you are not the user bluecloudmatrix, you can also use the below paths
	CRIU_PATH = '/home/bluecloudmatrix/criu/deps/criu-x86_64'
	# the path of CRIU program and it should be used in ROOT
	CRIU_PROGRAM = CRIU_PATH + '/criu'
	# the directory used for storing the checkpoint image files of libevent_paxos process and the path is self-defined
	CRIU_IMAGE_PATH = CRIU_PATH + '/checkpoint'
	# the directory used for storing the checkpoint image files of real server process, such as mongoose, and the path is self-defined
	CRIU_SERVER_IMGAE_PATH = CRIU_PATH + '/checkpoint_server'

	# for migration, to check the parent process of the criu-cr.py script
	check_parent()

	print "============ CRIU 1.4 Welcome ============"
        print "< enter listenning loop >"
	while True:
		print "< start to use CRIU >"
		criu_cr(args)
		print "< sleeping %d seconds... >" % (args.period)
		time.sleep(args.period)
	print "============ CRIU 1.4 The End ============"



