"""
@author: Milannic Cheng Liu
@note: demo pbzip object that use parrot based pbzip2 
@copyright: See LICENSE
"""
import subprocess
import os

class Pbzip():
    def __init__(self):
        self.parameter = "-p4 -rkvf"
        self.bin_path = "/home/milannic/myxtern_compilation/apps/mongoose/mg-server"
    def execute_pbzip(self):
	path = "/home/milannic/myxtern_compilation/apps/pbzip2/input.tar"
        os.environ["LD_PRE_LOAD"] = "/home/milannic/myxtern_compilation/dync_hook/interpose.so"
        args = ("/home/milannic/myxtern_compilation/apps/pbzip2/pbzip2",self.parameter,path)
        popen = subprocess.Popen(args, stdout=subprocess.PIPE)
        popen.wait()
        self.file_name = os.path.basename(path)
        #output = popen.stdout.read()
        pass
    def get_output_file(self):
		bytes_string = bytes()
		with open("/home/milannic/myxtern_compilation/apps/pbzip2/"+self.file_name+".bz2","rb") as input_file:
			while True:
				bytes_seq=input_file.read(1024)
				if bytes_seq: 
					bytes_string = bytes_string + bytes_seq
				else:
					break
		return bytes_string
