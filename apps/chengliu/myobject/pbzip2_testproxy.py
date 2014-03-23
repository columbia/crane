from concoord.blockingclientproxy import ClientProxy
'\n@author: Milannic Cheng Liu\n@note: demo pbzip object that use parrot based pbzip2 \n@copyright: See LICENSE\n'
import subprocess
import os

class Pbzip:

    def __init__(self, bootstrap):
        self.proxy = ClientProxy(bootstrap, token='None')

    def __concoordinit__(self):
        return self.proxy.invoke_command('__concoordinit__')

    def execute_pbzip(self):
        return self.proxy.invoke_command('execute_pbzip')

    def get_output_file(self):
        return self.proxy.invoke_command('get_output_file')