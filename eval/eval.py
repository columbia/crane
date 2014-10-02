import os

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
	return

def processBench(config, bench):
	return

def workers(semaphore, lock, configs, bench):
	return

if __name__ == "__main__":
	print "just a test"
