#!/usr/bin/python2.7

import os, sys, subprocess
import time

def find_pid_by_name(server_name):
    pid_list = []
    try:
        pid_list = map(int, subprocess.check_output(["pidof", server_name]).split())
    except subprocess.CalledProcessError:
        pass
    return pid_list

def checkpoint_server(server_name, server_pid):
    # notice that if this node is head, timebubble.sock should be closed 
    with open("checkpoint-output.txt", "a") as fout:
        # p_close_sock = subprocess.Popen("sudo gdb -p " + str(server_pid) + " --batch -ex \'call close(4)\'", shell=True, stdout=subprocess.PIPE)
        # output, err = p_close_sock.communicate()
        # fout.write(output)
        p_checkpoint = subprocess.Popen("./checkpoint-restore.sh checkpoint " + server_name + " ./checkpoint", shell=True, stdout=subprocess.PIPE)
        output, err = p_checkpoint.communicate()
        fout.write(output)
    
if __name__ == "__main__":
    if len(sys.argv) != 2:
        exit(1)
    
    server_name = sys.argv[1]
    os.system("rm checkpoint-output.txt")
    with open("checkpoint-output.txt", "a") as fout:
        fout.write("server: " + server_name)

    while True:
        # if server quits
        pid = find_pid_by_name(server_name)
        if len(pid) == 0:
            print "no process running. "
            exit(0)
        
        checkpoint_server(server_name, pid[0])
        time.sleep(60)

