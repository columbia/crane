#!/usr/bin/python2.7

import httplib
import time
import errno
from socket import error as socket_error

for i in range(20):
    try:
        print 'connecting for %d time, %s' %(i, time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime()))
        conn = httplib.HTTPConnection('bug03.cs.columbia.edu', 9000)
        conn.request('GET','/test.php')
        response = conn.getresponse()
        print 'req succeeded. '
        print response.status, response.reason
        # print response.read()

    except socket_error as serr:
        print 'handling exception. '
        pass
    except httplib.CannotSendRequest:
        print 'cannot send req. '
        pass
    finally:
        print 'closing connection. waiting for 1s...'
        conn.close()
        time.sleep(1)
 
