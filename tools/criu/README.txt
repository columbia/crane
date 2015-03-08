=======================================
[Introduction]

CRIU:help us to do checkpoint/restore
=======================================
[Kernel Configuration]

Linux kernel v3.11 or newer is required, with some specific options set.

Note you might have to enable

CONFIG_EXPERT=y
General setup -> Configure standard kernel features (expert users)
option, which depends on

CONFIG_EMBEDDED=y
General setup -> Embedded system
(welcome to Kconfig reverse chains hell).

The following options must be enabled for CRIU to work:

CONFIG_CHECKPOINT_RESTORE=y
General setup -> Checkpoint/restore support
CONFIG_NAMESPACES=y
General setup -> Namespaces support
CONFIG_UTS_NS=y
General setup -> Namespaces support -> UTS namespace
CONFIG_IPC_NS=y
General setup -> Namespaces support -> IPC namespace
CONFIG_PID_NS=y
General setup -> Namespaces support -> PID namespaces
CONFIG_NET_NS=y
General setup -> Namespaces support -> Network namespace
CONFIG_FHANDLE=y
General setup -> open by fhandle syscalls
CONFIG_EVENTFD=y
General setup -> Enable eventfd() system call
CONFIG_EPOLL=y
General setup -> Enable eventpoll support
CONFIG_INOTIFY_USER=y
File systems -> Inotify support for userspace
CONFIG_IA32_EMULATION=y (x86 only)
Executable file formats -> Emulations -> IA32 Emulation
CONFIG_UNIX_DIAG=y
Networking support -> Networking options -> Unix domain sockets -> UNIX: socket monitoring interface
CONFIG_INET_DIAG=y
Networking support -> Networking options -> TCP/IP networking -> INET: socket monitoring interface
CONFIG_INET_UDP_DIAG=y
Networking support -> Networking options -> TCP/IP networking -> INET: socket monitoring interface -> UDP: socket monitoring interface
CONFIG_PACKET_DIAG=y
Networking support -> Networking options -> Packet socket -> Packet: sockets monitoring interface
CONFIG_NETLINK_DIAG=y
Networking support -> Networking options -> Netlink socket -> Netlink: sockets monitoring interface

For some usage scenarios there is an ability to track memory changes and produce incremental dumps. Need to enable
CONFIG_MEM_SOFT_DIRTY=y (optional)
Processor type and features -> Track memory changes

At the moment it's known that CRIU will NOT work if packet generator module is loaded. Thus make sure that either module is unloaded or not compiled at all.
# CONFIG_NET_PKTGEN is not set
Networking support -> Networking options -> Network testing -> Packet generator

=======================================
[Installation] reference from http://www.criu.org/Installation


(Download Source Code)
git clone git://git.criu.org/criu.git
cd criu


(Dependencies)
mkdir -p deps/`uname -m`-linux-gnu
cd deps

sudo apt-get install autoconf curl g++ libtool
git clone https://github.com/google/protobuf.git protobuf
cd protobuf
./autogen.sh
./configure --prefix=`pwd`/../`uname -m`-linux-gnu
make
make install
cd ../..

cd deps
git clone https://github.com/protobuf-c/protobuf-c.git protobuf-c
cd protobuf-c
./autogen.sh
mkdir ../pbc-`uname -m`
cd ../pbc-`uname -m`
../protobuf-c/configure --prefix=`pwd`/../`uname -m`-linux-gnu \
  PKG_CONFIG_PATH=`pwd`/../`uname -m`-linux-gnu/lib/pkgconfig
make
make install
cd ../..

sudo apt-get install AsciiDoc
sudo apt-get install xmlto
sudo apt-get install libprotobuf-dev
sudo apt-get install libprotobuf-c0-dev
sudo apt-get install protobuf-compiler
sudo apt-get install protobuf-c-compiler


(Building CRIU From Source)
cd deps
rsync -a --exclude=.git --exclude=deps .. criu-`uname -m`
cd criu-`uname -m`
make \
  USERCFLAGS="-I`pwd`/../`uname -m`-linux-gnu/include -L`pwd`/../`uname -m`-linux-gnu/lib" \
  PATH="`pwd`/../`uname -m`-linux-gnu/bin:$PATH"
sudo LD_LIBRARY_PATH=`pwd`/../`uname -m`-linux-gnu/lib ./criu check
cd ../..


(Checking That It Works)
# criu check --ms


=======================================
[Script for checkpoint/restore]

criu_daemon.sh

The script must be run via 'sudo su' 




