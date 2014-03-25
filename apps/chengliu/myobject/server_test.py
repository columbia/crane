"""
@author: Milannic Cheng Liu
@note: demo pbzip object that use parrot based pbzip2 
@copyright: See LICENSE
"""
import subprocess
import os
import sysv_ipc
import signal
#import urllib2
import requests
import struct

class serverTest():
    def __init__(self):
        self.parameter = "-t 2"
        self.bin_path = "/home/milannic/myxtern_compilation/apps/mongoose/mg-server"
        self.running = 0
    def test(self):
        return 5543
    def startServer(self):
        if not self.running:
            self.logical_clock_write = sysv_ipc.SharedMemory(87654321,sysv_ipc.IPC_CREAT,0600,sysv_ipc.PAGE_SIZE)
            os.environ["LD_PRELOAD"] = "/home/milannic/myxtern_compilation/dync_hook/interpose.so"
            args = (self.bin_path)
            popen = subprocess.Popen(args, stdout=subprocess.PIPE)
            self.cpid=popen.pid 
            self.popen = popen
            self.running = 1
        else:
            pass
    def killServer(self):
        #remove the shared memory
        if self.running == 1:
            self.logical_clock_write.remove()
            os.kill(int(self.cpid), signal.SIGTERM)
            self.popen.wait();
            # Check if the process that we killed is alive.
            try: 
               os.kill(int(self.cpid), 0)
               return "the server has been killed"
               raise Exception("""wasn't able to kill the process 
                                  HINT:use signal.SIGKILL or signal.SIGABORT""")
            except OSError as ex:
                print ex

    def getLogicalClock(self):
        #from the shared memory get the logical clock of the program
        dec_clock = 0
        bin_clock = ''
        byte_clock = self.logical_clock_write.read(8);
        for byte in byte_clock:
            dec = struct.unpack("B",byte)[0]
            bin_t = bin(dec)[2:]
            while len(bin_t) < 8:
                bin_t = '0' + bin_t
            bin_clock = bin_t + bin_clock
        dec_clock = int(bin_clock,2)
        return dec_clock

    def rSend(self,url,logical_clock):
        #from the shared memory get the logical clock of the program
#        print url
#        print logical_clock
#        return 123
        if not self.running:
            self.logical_clock_write = sysv_ipc.SharedMemory(87654321,sysv_ipc.IPC_CREAT,0600,sysv_ipc.PAGE_SIZE)
            os.environ["LD_PRELOAD"] = "/home/milannic/myxtern_compilation/dync_hook/interpose.so"
            args = (self.bin_path)
            popen = subprocess.Popen(args, stdout=subprocess.PIPE)
            self.cpid=popen.pid 
            self.popen = popen
            self.running = 1
        if self.running == 1:
            self.logical_clock_write.write(struct.pack('<q',logical_clock),0)
            url = "http://127.0.0.1:8080/"+url
            r = requests.get(url)
            return r.content
        else:
            print "no sever is running"
            return None


    def send(self,url):
        if self.running == 1:
            url = "http://127.0.0.1:8080/"+url
            r = requests.get(url)
            return r.content
        else:
            print "no sever is running"
            return None
