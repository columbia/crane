#!/usr/bin/python2.7

# Current version has been tested on mongoose, mediatomb and clamav. 
# Also working for apache httpd. I manually ran the test instead of automatically deploying and launching the test using master.py. 

import os, sys, subprocess
import time

def find_pid_by_name(server_name):
    pid_list = []
    try:
        pid_list = map(int, subprocess.check_output(["pidof", server_name]).split())
    except subprocess.CalledProcessError:
        pass
    return pid_list

def checkpoint_server(server_name, server_pid, cnt):
    # notice that if this node is head, timebubble.sock should be closed 
    with open("checkpoint-output.txt", "a") as fout:
        # p_close_sock = subprocess.Popen("sudo gdb -p " + str(server_pid) + " --batch -ex \'call close(4)\'", shell=True, stdout=subprocess.PIPE)
        # output, err = p_close_sock.communicate()
        # fout.write(output)
        p_checkpoint = subprocess.Popen("./checkpoint-restore.sh checkpoint " + server_name + " ./checkpoint", shell=True, stdout=subprocess.PIPE)
        output, err = p_checkpoint.communicate()
        rc = p_checkpoint.returncode
        fout.write(output)
        fout.write("return code: " + str(rc) + "\n")
        fout.write("cnt: " + str(cnt) + "\n")
    return rc
    
if __name__ == "__main__":
    if len(sys.argv) != 2:
        exit(1)
    
    server_name = sys.argv[1]
    os.system("rm checkpoint-output.txt")
    with open("checkpoint-output.txt", "a") as fout:
        fout.write("server: " + server_name + "\n")

    cnt = 0
    while True:
        print "debug msg"
        # if server quits
        pid = find_pid_by_name(server_name)
        if len(pid) == 0:
            print "no process running. "
            exit(0)
        
        ret = checkpoint_server(server_name, pid[0], cnt)
        if ret == 0:
            time.sleep(60)
            cnt = 0
        elif ret == 1 or ret == 3:
            # dump failed
            if cnt == 5:
                cnt = 0
                time.sleep(30) 
            else:
                cnt += 1
                time.sleep(3)
        else:
            time.sleep(10)
            cnt = 0

