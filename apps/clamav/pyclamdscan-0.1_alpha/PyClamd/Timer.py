#!/usr/bin/env python

import time

class Timer(object):
    def __init__(self):
        self._startTime = 0.0
        self._stopTime = 0.0
        self._totalTime = 0.0
    
    def start(self):
        self._startTime = time.time()
    
    def stop(self):
        self._stopTime = time.time()
        self._totalTime = self._stopTime - self._startTime
        
    def _getStartTime(self):
        return self._startTime
    
    def _getStopTime(self):
        return self._stopTime
    
    def _getTotalTime(self):
        return self._totalTime
    
    StartTime = property(_getStartTime , None)
    StopTime = property(_getStopTime , None)
    TotalTime = property(_getTotalTime , None)
    
if __name__ == '__main__':
    import sys
    numTimes = 10000
    t = Timer()
    t.start()
    for i in range(numTimes):
        sys.stderr.write('hello: %d\n' % i)
    t.stop()
    print "TotalTime: %f" % t.TotalTime      