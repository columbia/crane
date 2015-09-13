Typical (and recommended) systems configuration for Parrot (a.k.a., xtern)
================
Hardware: we have tried both 4-core and 24-core machines with 64 bits.
OS: Ubuntu 11.10.
gcc/g++: 4.5 (please use this gcc version if possible). One suggestion
is you could install 4.5 and setup a local link for it in your own PATH environment 
variable, so that your "gcc -v" command would return 4.5.X.




Installing Parrot (xtern)
================

0. Install some libraries/tools.
> sudo apt-get install gcc-4.5 g++-4.5 gcc-multilib g++-multilib
> sudo apt-get install dejagnu flex bison axel libboost-dev libtiff4-dev
> sudo apt-get install zlib1g-dev libbz2-dev libxml-libxml-perl python-pip python-setuptools python-dev
> sudo pip install numpy
> sudo pip install OutputCheck          (this utility is only required by the testing framework)
> sudo apt-get install libxslt1-dev libxml2-dev
> sudo easy_install-2.7 lxml
> sudo apt-get install libgomp1 libgmp-dev libmpfr-dev libmpc-dev

And also install some libraries for some applications.
> sudo apt-get install libmp3lame-dev


1. Add $XTERN_ROOT (the absolute path of "xtern") into environment variables
in your ~/.bashrc. Run "echo $XTERN_ROOT" and "echo $LD_LIBRARY_PATH"
to make sure they are correct.
> export XTERN_ROOT=the absolute path of "smt+mc/xtern"
> export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$XTERN_ROOT/dync_hook


2. Performance hints.
> cd $XTERN_ROOT/apps/
> find . -name *annot*.patch
For example, the $XTERN_ROOT/apps/openmp/patch/add-xtern-annot.patch
file is the generic performance hints that benefit all OpenMP programs in 
our benchmarks.


3. Config.
> cd $XTERN_ROOT
> mkdir -p obj
> cd obj
> ./../configure --prefix=$XTERN_ROOT/install


4. Make. Every time after you 'git pull' xtern, you should go to this directory and make it.
Always run "make clean" first, and then "make", and then "make install", as show below.
> cd $XTERN_ROOT/obj
> make clean && make && make install


5. Test. It may take a few minutes. If it all passes, then everything has been installed correctly.
This step may take a few minutes, depending on hardware speed.
> cd $XTERN_ROOT/obj
> make -C test check




Running Parrot (xtern)
================
1. Run an application with xtern.
Normally each app directory should contain a "mk" (build the x86 of the app),
a "run" (run the app with both xtern and original non-deterministic execution),
and a "chk-run" (run the app with the dbug tool only).
> cd $XTERN_ROOT/apps/pbzip2
> ./mk
> ./run
The above "run" script will first run the pbzip2 with xtern,
and then run it non-deterministically, and both their execution time will be shown.
Xtern has a bunch of options at run time (with default values).
When you run an app with xtern, by default it will first find a 
"local.options" file and then run xtern with the options specified within
this local file, if this file does not exist, xtern will be run with (default)
options specified in the "$XTERN_ROOT/default.options" file.


2. How to run the xtern evaluation framework. You can skip this step unless you 
want to get a comprehensive performance overhead of xtern over all its benchmarks.
Let's use the phoenix benchmark suite as an example:
> cd $XTERN_ROOT/apps/phoenix/
> ./mk
> cd $XTERN_ROOT/eval/
> ./eval.py phoenix-standard.cfg
> find ./current -name stats.txt
Then you will see a bunch of these "stats.txt" files containing overhead of
xtern over non-det execution and maybe other stats info.
There are a number of *-standard.cfg files in $XTERN_ROOT/eval/, and they
are the standard inputs which generate the performance overhead of xtern
in our submitted paper. There are other *.cfg in that directory as well, and 
they are for other purposes (different workloads, model checking, etc).
The *.cfg files define all inputs for all the benchmarks, and the format
of a cfg is pretty clean, explicit and easy to use.
If you only want to run one program, you can copy the config for
this program from a cfg file, and paste it to a new cfg file (foo.cfg), and then you 
could run this only one program using "./eval.py foo.cfg".
For example, a config of the "histogram" program in phoenix looks like this:
[phoenix histogram]
REPEATS = 10
INIT_ENV_CMD = sync
INPUTS = histogram_datafiles/large.bmp
EXPORT = MR_NUMPROCS=16 MR_NUMTHREADS=16 MR_L1CACHESIZE=524288
GZIP = histogram.tar.gz

*** NOTE: regarding the number of threads you should use in your evaluation: we used 24-core 
machines in our SOSP '13 Parrot evaluation and we used as many as cores to 
measure the overhead at peak performance of the programs. If your machines 
have fewer cores (e.g., 4-core), you should change the number of threads in 
these cfg files to match "use as many as cores" (i.e., you should use 4 
threads), because when the number of threads in a program is larger than the 
number of cores, the program itself would have exccessive core contention 
at runtime and no longer at its peek performance.


3. Running stl performance results. For all the other *-standard.cfg
files, we only need to directly feed them to the eval.py script. 
For stl, we should run "./eval.py --stl-result stl-standard.cfg",
so that we only measure the "parallel execution time" of these stl
programs and avoid taking the "serial phase" (generating vector inputs)
into account.




Testsuite (xtern/test)
======================

Our testsuite is based on dejagnu.

How to create a directory of tests
  - create a directory somewhere under xtern/test
  - cp dg.exp from another directory to this directory
  - note that dg.exp expects testcases with .c, or .cpp extensions;
    if your testcases use other extensions, please edit dg.exp accordingly

How to write a test
  - create a .c, or .cpp file
  - add the commands  you want to run as comments in the form of (for example)
    // RUN: command
    these RUN: lines will be automatically picked up by RunTests in
    test/lib/default.exp
  - when writing the commands, you can use the variables predefined in
    test/Makefile
  - %s represents the test file, and %t1, %t2, ... represent temporary files.

How to write check output of a test run against expected output?
  - can use the 'diff' command
  - can also use the OutputCheck tool (https://github.com/stp/OutputCheck);
    see below

How to run tests?
  - run entire test suite
    # suppose we're at the root of the build directory
    make check
  - run one testcase only
    # suppose we're at the root of the build directory
    make -C test TESTONE=<path-to-test> check-one

    # for example, to run the test xtern/test/instr/instr-call-test.cpp, we use
    make -C test TESTONE=test/instr/instr-call-test.cpp check-one

  - output (e.g., stdout and stderr) of tests are at
    <buiid-dir>/test/<subdir>/Output

How to use FileCheck?
  - simple usage.  add the following line

     CHECK: text

    to a file containing the expected output, then run

       FileCheck <expected output>

    which checks that the stdin has a line that contains
    'text', e.g.,

       blah  text   blah

    note that "CHECK: text" matchines a line as long as the line
    contains the string text.

    the stdin can be output from other commands.  another trick is to
    include the expected output in the source file itself.  For instance, we can add

    // CHECK: text

    to a .cpp file containing the code of the testcase, then use check the
    output of the testcase against the .cpp file itself, and FileCheck
    will pick up only the CHECK: lines

  - multiple CHECK:

      CHECK: text1
      CHECK: text2
      CHECK: text3

    which checks that the stdin contains three (not necessarily
    consecutive) lines that match the three CHECKs

  - CHECK: can have pattern, and can do matching as well


Unit-testsuite (xtern/unittests)
================================

Our unit-test framework is based on GoogleTest

How to create unit tests
  - check out the examples in xtern/unittests/runtime

How to run unittest
  # go to <build dir>, and run
  make -C unittests
  # then run the test program, for example
  ./unittests/runtime/Debug/recorderTests

