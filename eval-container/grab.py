#!/usr/bin/python2.7
# Time per request:       73.322 [ms] (mean, across all concurrent requests)

import re, os

# init
result = dict()
result['mg-server']  = {'orig': [], 'xtern_only': [], 'proxy_only': [], 'separate_sched': [], 'joint_sched': []}
result['httpd'] = {'orig': [], 'xtern_only': [], 'proxy_only': [], 'separate_sched': [], 'joint_sched': []}
result['clamd'] = {'orig': [], 'xtern_only': [], 'proxy_only': [], 'separate_sched': [], 'joint_sched': []}
result['mediatomb'] = {'orig': [], 'xtern_only': [], 'proxy_only': [], 'separate_sched': [], 'joint_sched': []}

for file in os.listdir("./perf_log"):
	if file.endswith(".log"):	
		# print(file)
		with open('perf_log/' + file, 'r') as f:
			content = f.read()
			# print 'length: ', len(content)
			a = None

			if file.startswith('apache') or file.startswith('httpd') or file.startswith('mg-server'):
				a = re.search(r'Time per request:[ \t]+(\d+\.\d+) \[ms\] \(mean, across all concurrent requests\)', content)
			elif file.startswith('clamd'):
				a = re.search(r'Time: (\d+\.\d+) sec', content)

			if a is not None:
				num = float(a.group(1))
				# print 'result: ', num
				tmp = None
				if file.startswith('httpd'):
					tmp = result['httpd']
				if file.startswith('clamd'):
					tmp = result['clamd']
				if file.startswith('mediatomb'):
					tmp = result['mediatomb']
				if file.startswith('mg-server'):
 					tmp = result['mg-server']

				if 'orig' in file:
					tmp['orig'].append(num)
				if 'xtern_only' in file:
					tmp['xtern_only'].append(num)
				if 'proxy_only' in file:
					tmp['proxy_only'].append(num)
				if 'separate_sched' in file:
					tmp['separate_sched'].append(num)
				if 'joint_sched' in file:
					tmp['joint_sched'].append(num)

# print result
from numpy import median
for k1 in result.keys():
	for k2 in result[k1].keys():
		# print result[k1][k2]
		if len(result[k1][k2]) > 0:
			# print result[k1][k2]
			# print 'server: ', k1, '\t\tpolicy: ', k2, '\t\tmedian: ', median(result[k1][k2])
			print 'server: %s; \t\t policy: %s; \t\t med: %f ' %(k1, k2, median(result[k1][k2]))
