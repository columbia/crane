#!/usr/bin/python2.7

import re
import os
from copy import copy

# walk in the perf log base dir and parse the logs
def check_per_logs(base):
    # init the return dict
    result = dict()
    result['mg-server']  = {'orig': [], 'xtern_only': [], 'proxy_only': [], 'separate_sched': [], 'joint_sched': []}
    result['httpd'] = copy(result['mg-server'])
    result['clamd'] = copy(result['mg-server'])
    result['mediatomb'] = copy(result['mg-server'])

    for root, dirs, files in os.walk(base): 
        for fname in files:
            if fname.endswith(".log"):   
                with open(os.path.join(root, fname), 'r') as f:
                    content = f.read() 
                    substr = None
                    if file.startswith('mediatomb') or file.startswith('httpd') or file.startswith('mg-server'):
                        substr = re.search(r'Time per request:[ \t]+(\d+\.\d+) \[ms\] \(mean, across all concurrent requests\)', content)
                    elif file.startswith('clamd'):
                        substr = re.search(r'Time: (\d+\.\d+) sec', content)

                    if substr is not None:
                        num = float(a.group(1)) 
                        tmp = None
                        if file.startswith('httpd'):
                            tmp = result['httpd']
                        elif file.startswith('clamd'):
                            tmp = result['clamd']
                        elif file.startswith('mediatomb'):
                            tmp = result['mediatomb']
                        elif file.startswith('mg-server'):
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

    return result

def do_stat(result):
    from numpy import median
    for k1 in result.keys():
        for k2 in result[k1].keys(): 
            if len(result[k1][k2]) > 0: 
                print 'server: %s; \t\t policy: %s; \t\t med: %f ' %(k1, k2, median(result[k1][k2]))

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="This script does some stat on the performance result files. ")
    parser.add_argument('-b', '--base', default='./perf_log', type=str, help='The base directory of the performance results. ')
    args = parser.parse_args()
    do_stat(check_perf_logs(args.base))
