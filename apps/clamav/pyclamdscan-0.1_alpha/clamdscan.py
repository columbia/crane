#!/usr/bin/env python

"""
This is a command line replacement for clamdscan that will allow the use
of a remote clamd server
"""

import sys , os , getopt , math
from PyClamd import OPyClamd , Timer

class ClamdScan(object):
    def __init__(self):
        self.clam = OPyClamd()
        self.quiet = False
        self.summary = True
        self.infectedOnly = False
        self.remove = False
        self.sockPath = ''
        self.host = '127.0.0.1'
        self.port = 3310
        self.fileList = []
        self.exitVal = 0
        self.setOptions()
        # Set what we are going to use for the connection to clamd
        if self.sockPath:
            self.clam.UseSocket = 'UNIX'
            self.clam.Socket = self.sockPath
        else:
            self.clam.UseSocket = 'NET'
            self.clam.Host = self.host
            self.clam.Port = self.port
    
    def _usage(self , exitCode=0):
        print """Usage: clamdscan.py [OPTIONS] file [file , file , ...]
    -h,--help                         Show this help
    -V,--version                      Show clamd version and exit
    --quiet                           Only output error messages
    --no-summary                      Disable the summary at the end of scanning
    --remove                          Remove the infected files. Be careful!
    -t HOST,--host=HOST               The clamd host to connect to
    -p PORT,--port=PORT               The port to connect to on the clamd host
    -u SOCKET,--unix-socket=SOCKET    Path to the unix socket to use.
                                      NOTE: This overrides any setting for host
                                            and port.
        """
        sys.exit(exitCode)
        
    def _printVersion(self , exitCode=0):
        print self.clam.clamdVersion()
        sys.exit(exitCode)
        
    def _getFTime(self , timerTime):
        """
        Returns a formatted time string for the float passed in
        
        timerTime (float): a floating point representing an amount of secs
        
        return: string
        """
        retStr = '%0.3f sec' % timerTime
        mins = int(math.floor(timerTime / 60.0))
        secs = int(round(timerTime - (mins * 60.0)))
        retStr += ' (%d m %d s)' % (mins , secs)
        return retStr
    
    def _printResults(self , fileName , results , scanTime):
        summaryHead = '\n----------- SCAN SUMMARY -----------'
        if fileName == '-':
            fileName = 'stream'
        if results:
            self.exitVal = 1
            if self.quiet: return
            key = results.keys()[0]
            print '%s: %s FOUND' % (fileName , results[key])
            if self.summary:
                print '%s\nInfected files: 1\nTime: %s' % (summaryHead ,
                                                        self._getFTime(scanTime))
        else:
            if self.quiet: return
            print '%s: OK' % fileName
            if self.summary:
                print '%s\nInfected files: 0\nTime: %s' % (summaryHead ,
                                                        self._getFTime(scanTime))
        return
        
    def setOptions(self):
        shortOpts = 'hVt:p:u:'
        longOpts = ['help' , 'version' , 'quiet' , 'no-summary'
                    'remove' , 'host=' , 'port=' , 'unix-socket=']
        try:
            (optList , fileList) = getopt.getopt(sys.argv[1:] , shortOpts , 
                                                 longOpts)
        except getopt.GetoptError:
            self._usage(1)
            
        # Set the file list
        self.fileList = fileList
        
        for opt in optList:
            if opt[0] == '-h' or opt[0] == '--help':
                self._usage()
            elif opt[0] == '-V' or opt[0] == '--version':
                self._printVersion()
            elif opt[0] == '--quiet':
                self.quiet = True
            elif opt[0] == '--no-summary':
                self.summary = False
            elif opt[0] == '--remove':
                self.remove = True
            elif opt[0] == '-t' or opt[0] == '--host':
                self.host = opt[1]
            elif opt[0] == '-p' or opt[0] == '--port':
                self.port = int(opt[1])
            elif opt[0] == '-u' or opt[0] == '--unix-socket':
                self.sockPath = opt[1]
    
    def scanFiles(self):
        for f in self.fileList:
            results = None
            timer = Timer()
            if f == '-':
                # read the file from stdin
                buf = ''
                while True:
                    block = sys.stdin.read(4096)
                    if not block: break
                    buf += block
                try:
                    timer.start()
                    results = self.clam.scanStream(buf)
                    timer.stop()
                except:
                    sys.stderr.write('%s: %s\n' % (sys.exc_type , 
                                                   sys.exc_value))
                    sys.exit(2)
            else:
                try:
                    timer.start()
                    results = self.clam.scanFile(f)
                    timer.stop()
                except:
                    sys.stderr.write('%s: %s\n' % (sys.exc_type , 
                                                   sys.exc_value))
                    sys.exit(2)
                if results and self.remove:
                    try:
                        os.unlink(results.keys()[0])
                    except:
                        sys.stderr.write('%s: %s\n' %(sys.exc_type , 
                                                      sys.exc_value))
            self._printResults(f , results , timer.TotalTime)
    
    

if __name__ == '__main__':
    cds = ClamdScan()
    cds.scanFiles()