from concoord.blockingclientproxy import ClientProxy
'\n@author: Milannic Cheng Liu\n@note: demo pbzip object that use parrot based pbzip2 \n@copyright: See LICENSE\n'
import subprocess
import os
import sysv_ipc
import signal
import requests
import struct

class serverTest:

    def __init__(self, bootstrap):
        self.proxy = ClientProxy(bootstrap, token='None')

    def __concoordinit__(self):
        return self.proxy.invoke_command('__concoordinit__')

    def test(self):
        return self.proxy.invoke_command('test')

    def startServer(self):
        return self.proxy.invoke_command('startServer')

    def killServer(self):
        return self.proxy.invoke_command('killServer')

    def getLogicalClock(self):
        return self.proxy.invoke_command('getLogicalClock')

    def rSend(self, url, logical_clock):
        return self.proxy.invoke_command('rSend', url, logical_clock)

    def send(self, url):
        return self.proxy.invoke_command('send', url)