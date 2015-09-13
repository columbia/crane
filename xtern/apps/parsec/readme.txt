* These scripts have been tested on Ubuntu 12.04 x86_64/i686


==== Installation ====
1.1 Setup local gcc-4.2
    We have to use gcc-4.2 here because some parsec programs only compile with gcc-4.2.
    (Before doing this, please create a symbolic link first
    if your operating system is Ubuntu 64 bits machine.
    Make sure /usr/lib64 points to /usr/lib/x86_64-linux-gnu)
    ($ sudo ln -s /usr/lib/x86_64-linux-gnu /usr/lib64 )
    $ ./setup-gcc-4.2

1.2 Get required Parsec Benchmark Suite
    $ ./setup-parsec

1.3 There may be some compiler dependencies you may need to resolve if you see something like this below:

freqmine-openmp: smt+mc/xtern/apps/parsec/gcc-4.2/lib64/libstdc++.so.6:
version `GLIBCXX_3.4.15' not found (required by smt+mc/mc-tools/dbug/install-xtern+dbug/lib/libdbug.so)


Method to solve this (on 64 bits. for 32 bits, it is similar):
> cd $XTERN_ROOT/apps/parsec/gcc-4.2/lib64
> mv libstdc++.so libstdc++.so.bak
> mv libstdc++.so.6 libstdc++.so.6.bak
> ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6 libstdc++.so
> ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.XX libstdc++.so.6

