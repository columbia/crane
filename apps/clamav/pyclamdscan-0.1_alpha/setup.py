import sys
assert sys.version >= '2' , 'Install Python 2.0 or greater'

from distutils.core import setup

# $Id: setup.py,v 1.2 2007/05/10 16:17:05 jay Exp $

def runSetup ():
    setup(name = 'pyclamdscan',
          version = '0.1_alpha',
          author = 'Jay Deiman' ,
          author_email = 'jdeiman@qwest.net' ,
          url = 'http://qwest.com' ,
          license = 'GPLv2' ,
          platforms = [ 'unix' ] ,
          description = 'Partial clamdscan replacement that allows using' \
              ' a remote clamd' ,
          packages = ['PyClamd'] ,
          scripts = ['clamdscan.py']
    )
    
if __name__ == '__main__':
    runSetup()