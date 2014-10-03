import os
import subprocess
from signal import signal

def getPaxosDefaultOptions():
	default = {}
	return default

def getConfigFullPath(config_file):
	try:
		with open(config_file) as f: pass
	except IOError as e:
		logging.warning("'%s' does not exist" % config_file)
		return None
	return os.path.abspath(config_file)

def readConfigFile(config_file):
	try:
		newConfig = ConfigParser.ConfigParser({"REPEATS":"1",
						       "INPUTS":"",
						       "EXPORT":"",
						       "CLIENT_CONFIG":"",
						       "CLIENT_INPUT":"",
						       "SERVER_CONFIG":"",
						       "EVALUATION":"",
						       "DBUG":"-1",
						       "DBUG_INPUT":"",
						       "DBUG_OUTPUT":"",
						       "DBUG_TIMEOUT":"60"})
		ret = newConfig.read(config_file)
	except ConfigParser.MissingSectionHeaderError as e:
		logging.error(str(e))
	except ConfigParser.ParsingError as e:
		logging.error(str(e))
	except ConfigParser.Error as e:
		logging.critical("strange error? " + str(e))
	else:
		if ret:	
			return newConfig

#make directory
def mkdir_p(path):
	try:
		os.makedirs(path)
	except OSError as exc: 
		if exc.errno == errno.EEXIST and os.path.isdir(path):
			logging.warning("%s already exists" % path)
			pass
		else: raise

def genRunDir(config_file, git_info):
	if args.model_checking:
		dir_name = "M"
	else:
		dir_name = ""
	from os.path import basename
	config_name = os.path.splitext(basename(config_file))[0]
	from time import strftime
	dir_name += config_name + strftime("%Y%b%d_%H%M%S") + '_' + git_info[0] +git_info[1]
	mkdir_p(dir_name)
	logging.debug("creating %s" % dir_name)
	return os.path.abspath(dir_name)

def extract_apps_exec(bench, apps_dir=""):
	bench = bench.partition('"')[0].partition("'")[0]
	apps = bench.split()
	if apps.__len__() < 1:
		raise Exception("cannot parse executible file name")
	elif apps.__len__() == 1:
		return apps[0], os.path.abspath(apps_dir + '/' + apps[0] + '/' +apps[0])
	else:
		return apps[0], os.path.abspath(apps_dir + '/' + apps[0] + '/' +apps[1])

def checkExist(file, flags=os.X_OK):
	if not os.path.exists(file) or not os.path.isfile(file) or not os.access(file, flags):
		return False
	return True

def copy_file(src, dst):
	import shutil
	shutil.copy(src, dst)

def write_stats(time1, time2, time3, time4, repeats):
	try:
		import numpy
	except ImportError:
		logging.error("please install 'numpy' module")
	time1_avg = numpy.average(time1)
	time1_std = numpy.std(time1)
	time2_avg = numpy.average(time2)
	time2_std = numpy.std(time2)
	time3_avg = numpy.averge(time3)
	time3_std = numpy.std(time3)
	time4_avg = numpy.average(time4)
	time4_std = numpy.std(time4)
	import math
	with open("stats.txt", "w") as stats:
		stats.write('Average Concensus Time:{0}ms'.format(time3_avg-time2_avg))
		stats.write('Average Processing Time:{0}ms'.format(time4_avg-time1_avg))

def preSetting(config, bench, apps_name):
	return

def execBench(cmd, repeats, out_dir,
	      client_cmd="", client_terminate_server=False,
	      init_env_cmd=""):
	mkdir_p(out_dir)
	for i in range(int(repeats)):
		sys.stderr.write("        PROGRESS: %5d/%d\r" % (i+1, int(repeats)))
		with open('%s/output.%d' % (out_dir, i), 'w', 102400) as log_file:
			if init_env_cmd:
				os.system(init_env_cmd)
			proc = subprocess.Popen(cmd, stdout=log_file, stderr=subprocess.STDOUT, shell=True, executable=bash_path, bufsize=102400)
			if client_cmd:
				time.sleep(1)
				with open('%s/client.%d' % (out_dir, i), 'w', 102400) as client_log_file:
					client_proc = subprocess.Popen(client_cmd, stdout=client_log_file, stderr=subprocess.STDOUT, shell=True, executable=bash_path, bufsize=102400)
					client_proc.wait()
				if client_terminate_server:
					os.killpg(proc.pid, signal.SIGTERM)
				proc.wait()
				time.sleep(2)
			else:
				try:
					proc.wait()
				except KeyboardInterrupt as k:
					try:
						os.killpg(proc.pid, signal.SIGTERM)
					except:
						pass
					raise k
		try:
			os.renames('out', '%s/out.%d' % (out_dir, i))
		except OSError:
			pass

def processBench(config, bench):
		
	logging.debug("processing: " + bench)
	specified_evaluation = config.get(bench, 'EVALUATION')
	apps_name, exec_file = extract_apps_exec(bench, APPS)
	logging.debug("app = %s" % apps_name)
	logging.debug("executible file = %s" % exec_file)
	if not specified_evaluation and not checkExist(exec_file, os.X_OK):
		logging.warning("%s does not exist, skip [%s]" % (exec_file, bench))
		return
	
	segs = re.sub(r'(\")|(\.)|/|\'', '', bench).split()
	if args.model_checking and not args.check_all:
		dir_name = config.get(bench, 'DBUG') + '_'
	else:
		dir_name = ""
	dir_name += '_'.join(segs)
	mkdir_p(dir_name)
	os.chdir(dir_name)
	
	generate_local_options(config, bench)
	inputs = config.get(bench, 'inputs')
	repeats = config.get(bench, 'repeats')
	
	if specified_evaluation:
		specified = __import__(specified_evaluation, globals(), locals(), [], -1)
		specified.evaluation(int(repeats))
		return
	
	preSetting(config, bench, apps_name)
	
	export = config.get(bench, 'EXPORT')
	if export:
		logging.debug("export %s", export)
	
	init_env_cmd = config.get(bench, 'INIT_ENV_CMD')
	if init_env_cmd:
		logging.info("presetting cmd in each round %s" % init_env_cmd)
		
	# check if this is a server-client app
	client_cmd = config.get(bench, 'C_CMD')
	client_terminate_server = bool(int(config.get(bench, 'C_TERMINATE_SERVER')))
	use_client_stats = bool(int(config.get(bench, 'C_STATS')))
	if client_cmd:
		if client_with_xtern:
			client_cmd = XTERN_PRELOAD + ' ' + client_cmd
		client_cmd = 'time ' + client_cmd
		logging.info("client command : %s" % client_cmd)
		logging.debug("terminate server after client finish job : " + str(client_terminate_server))
		logging.debug("evaluate performance by using stats of client : " + str(use_client_stats))

	# generate command for MSMR [time LD_PRELOAD=... exec args...]
	msmr_command = ' '.join(['time', export, exec_file] + inputs.split())
	logging.info("executing '%s'" % msmr_command)
	if not args.compare_only:
		execBench(msmr_command, repeats, 'msmr', client_cmd, client_terminate_server, init_env_cmd)
	client_cmd = config.get(bench, 'C_CMD')
	if client_cmd:
		client_cmd = 'time ' + client_cmd
		
	# get stats
	msmr_cost = []
	for i in range(int(repeats)):
		if args.compare_only:
			msmr_cost += [1.0]
			continue
		if client_cmd and use_client_stats:
			log_file_name = 'msmr/client.%d' % i
		else:
			log_file_name = 'msmr/output.%d' % i
		for line in (open(log_file_name, 'r').readlines() if args.stl_result else reversed(open(log_file_name, 'r').readlines())):
			if re.search('^real [0-9]+\.[0-9][0-9][0-9]$', line):
				msmr_cost += [float(line.split()[1])]
				break
	
	write_stats(xtern_cost, nondet_cost, int(repeats))
	# copy exec file
	copy_file(os.path.realpath(exec_file), os.path.basename(exec_file))
	
def workers(semaphore, lock, configs, bench):
	from multiprocessing import Process
	with semaphore:
		p = Process(target=processBench, args=(configs, bench))
		with lock:
			logging.debug("STARTING %s" % bench)
			p.start()
		p.join()
		with lock:
			logging.debug("FINISH %s" % bench)

if __name__ == "__main__":
	print "just a test"
