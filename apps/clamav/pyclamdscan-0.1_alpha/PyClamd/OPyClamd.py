#!/usr/bin/env python

"""
This is an object oriented approach to pyclamd.py written by 
Alexandre Norman - norman@xael.org
"""

import socket , types , os , os.path

__version__ = '$Id: OPyClamd.py,v 1.2 2007/05/10 16:13:56 jay Exp $'
__all__ = ['BufferTooLong' , 'ScanError' , 'FileNotFound' , 
           'InvalidConnectionType', 'OPyClamd' , 'initUnixSocket' ,
           'initNetworkSocket']

def initUnixSocket(self , path=None):
    """
    Sets the appropriate properties to use a unix socket
    
    path (string): path to unix socket
    
    return: (obj) OPyClamd or None
    
    Raises:
        - ScanError: in case of socket communication problem
    """
    if not os.path.exists(path):
        raise FileNotFound , 'Invalid socket path: %s' % path
    
    clam = OPyClamd(useSocket='UNIX' , socket=path)
    
    if clam.ping():
        return clam
    return None
    
def initNetworkSocket(self , host=None , port=None):
    """
    Sets the appropriate properties to use a network socket
    
    host (string): clamd server address
    port (int): clamd server port
 
    return: (obj) OPyClamd or None
    
    Raises:
        - ScanError: in case of socket communication problem
    """
    clam = OPyClamd(useSocket='NET' , host=host , port=port)
        
    if clam.ping():
        return clam
    return None

class BufferTooLong(Exception):
    pass

class ScanError(Exception):
    pass

class FileNotFound(Exception):
    pass

class InvalidConnectionType(Exception):
    pass

class OPyClamd(object):
    """
    OPyClamd.py
    
    Author: Alexandre Norman - norman@xael.org
    Author: Jay Deiman - administrator@splitstreams.com
    Note: Mr. Norman wrote the original pyclamd.py and this has been rewritten
          to take a more object oriented approach to his work
    License: GPL
    
    Usage:
        # Initalize the connection for tcp usage
        clam = OPyClamd(useSocket='NET' , host='127.0.0.1' , port=3310)
        # Or initialize the connection for unix socket usage
        clam = OPyClamd(useSocket='UNIX' , socket='/var/run/clamd')
        
        # Get clamd version
        print clam.clamdVersion()
        
        # Scan a string
        print clam.scanStream(string)
        
        # Scan a file on the machine that is running clamd
        print clam.scanFile('/path/to/file')
    """
    def __init__(self , **dict):
        """
        Initializes all the values of local properties
        
        useSocket (string): can be 'UNIX' or 'NET'
            - default: None
        socket (string): path to unix socket
            - default: '/var/run/clamd'
        host (string): either a dns resolvable name or ip address
            - default: '127.0.0.1'
        port (int): the port number that clamd is running on
            - default: 3310
        
        return: nothing
        """
        try:
            self.useSocket = dict['useSocket']
        except KeyError:
            self.useSocket = None
            
        try:
            self.socket = dict['socket']
        except KeyError:
            self.socket = '/var/run/clamd'
            
        try:
            self.host = dict['host']
        except KeyError:
            self.host = '127.0.0.1'
            
        try:
            self.port = int(dict['port'])
        except KeyError:
            self.port = 3310
            
        return
    
    ##########################
    # Public Methods
    ##########################   
    def ping(self):
        """
        Send a PING to the clamd server, which should reply with PONG
        
        return: True if the server replies to PING
        
        Raises:
            - ScanError: in case of socket communication problem
        """
        s = self._initSocket()
        try:
            s.send('PING')
            result = s.recv(5).strip()
            s.close()
        except:
            raise ScanError , 'Could not ping clamd server: %s:%d' % \
                                (self.host , self.port)
        
        if result == 'PONG':
            return True
        else:
            raise ScanError , 'Could not ping clamd server: %s:%d' % \
                                (self.host , self.port)
        return
    
    def clamdVersion(self):
        """
        Get clamd version
        
        return: (string) clamd version
        
        Raises:
            - ScanError: in case of socket communication problem
        """
        s = self._initSocket()
        s.send('VERSION')
        result = s.recv(20000).strip()
        s.close()
        return result
    
    def clamdReload(self):
        """
        Force clamd to reload signature database
        
        return: (string) "RELOADING"
        
        Raises:
            - ScanError: in case of socket communication problems
        """
        s = self._initSocket()
        s.send('RELOAD')
        result = s.recv(20000).strip()
        s.close()
        return result
    
    def clamdShutdown(self):
        """
        Force clamd to shutdown and exit
        
        return: nothing
        
        Raises:
            - ScanError: in case of socket communication problems
        """
        s = self._initSocket()
        s.send('SHUTDOWN')
        result = s.recv(20000)
        s.close()
        return result
    
    def scanFile(self , file):
        """
        Scan a file given by filename and stop on virus
        
        file MUST BE AN ABSOLUTE PATH!
        
        return either:
            - dict: {filename: 'virusname'}
            - None if no virus found
        
        Raises:
            - ScanError: in case of socket communication problems
            - FileNotFound: in case of invalid file, 'file'
        """
        if not os.path.isfile(file):
            raise FileNotFound , 'Could not find file: %s' % file
        if not (self.useSocket == 'UNIX' or self.host == '127.0.0.1' or
                                         self.host == 'localhost'):
            raise InvalidConnectionType , 'You must be using a local socket ' \
                                          'to scan local files'
        s = self._initSocket()
        s.send('SCAN %s' % file)
        result = ''
        dr = {}
        while True:
            block = s.recv(4096)
            if not block: break
            result += block
        if result:
            (fileName , virusName) = map((lambda s: s.strip()) , 
                                         result.split(':'))
            if virusName[-5:] == 'ERROR':
                raise ScanError , virusName
            elif virusName[-5:] == 'FOUND':
                dr[fileName] = virusName[:-6]
        s.close()
        if dr:
            return dr
        return None
    
    def contScanFile(self , file):
        """
        Scans a local file or directory given by string 'file'
        
        file MUST BE AN ABSOLUTE PATH!
        
        return either:
            - dict: {filename1: 'virusname' , filename2: 'virusname'}
            - None if no virus found
        
        Raises:
            - ScanError: in case of socket communication problems
            - FileNotFound: in case of invalid file, 'file'
        """
        if not os.path.exists(file):
            raise FileNotFound , 'Could not find path: %s' % file
        s = self._initSocket()
        s.send('CONTSCAN %s' % file)
        result = ''
        dr = {}
        while True:
            block = s.recv(4096)
            if not block: break
            result += block
        if result:
            results = result.split('\n')
            for res in results:
                (fileName , virusName) = map((lambda s: s.strip()) , 
                                             res.split(':'))
                if virusName[-5:] == 'ERROR':
                    raise ScanError , virusName
                elif virusName[-5:] == 'FOUND':
                    dr[fileName] = virusName[:-6]
        s.close()
        if dr:
            return dr
        return None
    
    def scanStream(self , buffer):
        """
        Scans a string buffer
        
        returns either:
            - dict: {filename: 'virusname'}
            - None if no virus found
        
        Raises:
            - BufferTooLong: if the buffer size exceeds clamd limits
            - ScanError: in case of socket communication problems
        """
        s = self._initSocket()
        s.send('STREAM')
        dataPort = int(s.recv(50).strip().split(' ')[1])
        ds = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
        ds.connect((self.host , dataPort))
        
        sent = ds.send(buffer)
        ds.close()
        if sent < len(buffer):
            raise BufferTooLong , str(len(buffer))
        
        result = ''
        dr = {}
        while True:
            block = s.recv(4096)
            if not block: break
            result += block
        if result:
            (fileName , virusName) = map((lambda s: s.strip()) , 
                                         result.split(':'))
            if virusName[-5:] == 'ERROR':
                raise ScanError , virusName
            elif virusName[-5:] == 'FOUND':
                dr[fileName] = virusName[:-6]
        s.close()
        if dr:
            return dr
        return None
    
    ##########################
    # Private Methods
    ##########################
    def _initSocket(self):
        """
        Private method to initialize a socket and return it
        
        Raises: 
            'ScanError' on connection error
        """
        if self.useSocket == 'UNIX':
            s = socket.socket(socket.AF_UNIX , socket.SOCK_STREAM)
            try:
                s.connect(self.socket)
            except socket.error:
                raise ScanError , 'Could not reach clamd using unix socket ' \
                                  '(%s)' % self.socket
        elif self.useSocket == 'NET':
            s = socket.socket(socket.AF_INET , socket.SOCK_STREAM)
            try:
                s.connect((self.host , self.port))
            except socket.error:
                raise ScanError , 'Could not reach clamd using network ' \
                                  '(%s:%d)' % (self.host , self.port)
        else:
            raise ScanError , 'Could not reach clamd: connection not ' \
                              'initialized'
        return s
    
    ##########################
    # Mutators
    ##########################
    def _setUseSocket(self , type):
        if type == 'NET' or type == 'UNIX':
            self.useSocket = type
        else:
            raise InvalidConnectionType , 'UseSocket must be "NET" or "UNIX"' \
                                          ': %s' % type
        return
    
    def _getUseSocket(self):
        return self.useSocket
    
    def _setSocket(self , path):
        if os.path.exists(path):
            self.socket = path
        return
    
    def _getSocket(self):
        return self.socket
    
    def _setHost(self , host):
        self.host = host or self.host
        return
    
    def _getHost(self):
        return self.host
    
    def _setPort(self , port):
        if port:
            port = int(port)
            if port:
                self.port = port
        return
    
    def _getPort(self):
        return self.port
    
    ##########################
    # Properties
    ##########################
    UseSocket = property(_getUseSocket , _setUseSocket)
    Socket = property(_getSocket , _setSocket)
    Host = property(_getHost , _setHost)
    Port = property(_getPort , _setPort)