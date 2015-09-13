0 Redis is a single-threaded server. So it is not a first priority for xtern.

1 Must make this option to be 3000 in order to make redis server work with xtern.
nanosec_per_turn = 3000


2. How to run the model checking experiments (dBug-only and Parrot+dBug) with redis-server + redis-benchmark.
Rebuild dBug on the redis branch:
> cd $SMT_MC_ROOT/mc-tools/dbug
> git checkout redis
> git pull
> cd $SMT_MC_ROOT/mc-tools
> ./mk-dbug

Prepare:
> cd $XTERN_ROOT/apps/redis
> Modify the paths in redis-model-chk-wrapper.cc to match your own paths.
> g++ redis-model-chk-wrapper.cc -o wrapper

Run dbug-only model checking:
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer redis-model-chk-wrapper.xml

Run Parrot+dBug model checking:
> cd XTERN_ROOT/eval/model-chk-wrappers
> make
> cd $XTERN_ROOT/apps/redis
> cp redis-model-chk-wrapper.xml smtmc-wrapper.xml
> Add this line to smtmc-wrapper.xml, before the "program command", order matters:
    <interposition command="/home/heming/rcs/smt+mc//xtern/eval/model-chk-wrapper/wrapper"/>
> export REDIS_DETACH=1       <--- set this macro, which is required by dbug.
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer smtmc-wrapper.xml
