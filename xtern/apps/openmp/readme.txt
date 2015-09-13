0. Install some other stuff.
$ sudo apt-get install libgmp-dev libmpfr-dev libmpc-dev
$ sudo apt-get install gcc-4.5 g++-4.5 g++-4.5-multilib
And link your gcc-4.5 and g++-4.5 to be gcc and g++.

1. Link directory or file.
On 64 bits machine:
Nothing needs to be done.

On 32 bits machine:
$ sudo ln -s /usr/include/i386-linux-gnu/gnu/stubs-32.h /usr/include/gnu/stubs-32.h

2. Build.
$ cd $XTERN_ROOT/apps/openmp/
$ ./mk
Then you will see these files in local directory:
libgomp.a
libstdc++.a
ex-for
rand-para

3. Build two simple programs with openmp (the "ex-for" and "rand-para" executables are alreaady built, I 
am just telling you how they were built).
$ cd $XTERN_ROOT/apps/openmp/
$ export XTERN_ANNOT_LIB="-I$XTERN_ROOT/include -L$XTERN_ROOT/dync_hook -Wl,--rpath,$XTERN_ROOT/dync_hook -lxtern-annot"
$ g++ -g -static-libgcc -static-libstdc++ -D_GLIBCXX_PARALLEL -L. -fopenmp rand-shuf.cpp -o rand-para -lgomp $XTERN_ANNOT_LIB
$ g++ -g -static-libgcc -static-libstdc++ -D_GLIBCXX_PARALLEL -L. -fopenmp examples/ex-for.cpp -o ex-for -lgomp $XTERN_ANNOT_LIB

4. Run with xtern.
$ LD_PRELOAD=$XTERN_ROOT/dync_hook/interpose.so ./ex-for

5. Run with dbug.
$ $SMT_MC_ROOT/mc-tools/dbug/install/bin/explorer.rb --prefix $SMT_MC_ROOT/mc-tools/dbug/install ./ex-for

