#!/usr/bin/env python

#
# Copyright (c) 2013,  Regents of the Columbia University 
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
# materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

import sys
import os
import shlex
import shutil
import glob

uniq_count = 0

def log_dir_name(num):
  return '_'.join(["xtern", "log", num])

def run_program(num, cmd):
  print "running test ", num
  os.system('./' + cmd)
#  os.system(' '.join("mv", "out", log_dir_name(num)));

def analyze_log(num):
  print "running analyzer on test ", num
  if not os.path.exists("out"):
    print 'cannot find log directory, by default it''s out'
    return False
#  m_hash_file = 'analyzing_' + str(num) + '.txt'
#  os.system('$XTERN_ROOT/install/bin/analyzer out -c > ' + m_hash_file)
#  os.system('cat ' + m_hash_file + ' >> analy.log')
  os.system(' '.join(['mv', 'out', 'out_' + str(num)]))

  my_log = 'out_' + str(num)
  found = False
  for i in range(0, num):
    out_file_name = 'ana_cmp_log_' + str(i) + '_' + str(num) + '.log'
    old_log = 'out_' + str(i)
    os.system('$XTERN_ROOT/install/bin/analyzer ' + my_log + ' -cmp ' + old_log + ' > ' + out_file_name + ' 2>&1 ')
    result = open(out_file_name, "r")
    text = result.read()
    result.close()
    if text.find('bipre-matches') >= 0:
      found = True
      break

  if not found:
    global uniq_count
    uniq_count = uniq_count + 1
    print 'uniq_count changed to ', uniq_count
  print 'uniq_count = ', uniq_count

  backup_dir = 'backup_log_' + str(num)
  os.system('mkdir ' + backup_dir)
  os.system('mv *.log ' + backup_dir + '/')

def gen_summary(nruns):
  print 'uniq_count = ', uniq_count
  print 'summary not implemented yet'

def initialize():
  os.system('rm -rf out*')
  os.system('rm -rf backup_log_*')
  os.system('rm -rf analy.log')
  os.system('rm -rf *.log')

def eval_logs(args):
  nruns = 50
  if (len(args) > 0):
    nruns = int(args[0])
  
  uniq = 3
  for run_number in range(1, nruns):
    log_dir = 'backup_log_' + str(run_number)
    os.system('rm tmp2')
    for prev in range(0, run_number):
      log_name = 'ana_cmp_log_{0}_{1}.log'.format(str(prev), str(run_number))
      if not os.path.exists('{0}/{1}'.format(log_dir, log_name)):
        continue
      os.system('grep matched {0}/{1} > tmp'.format(log_dir, log_name))
      os.system('cat tmp | cut -f 2- -d \\< | cut -f 1-1 -d \' \' | sort | uniq >> tmp2')
    count = dict()
    result = open('tmp2', 'r')
    for item in result.read().split():
      if item not in count:
        count[item] = 0
      count[item] = count[item] + 1
    result.close()
    for k in count.keys():
      if (count[k] == run_number):
        uniq = uniq + 1
    print run_number, ' ', uniq

if __name__ == '__main__':
  eval_logs(sys.argv[1:])
