1. Build.
> cd $XTERN_ROOT/apps/aget
> ./mk

2. Run.
> cd $XTERN_ROOT/apps/aget
> ./run

3. How to run the model checking experiments (dBug-only and Parrot+dBug) with aget+mongoose.
Rebuild dBug on the aget branch:
> cd $SMT_MC_ROOT/mc-tools/dbug
> git checkout aget
> git pull
> cd $SMT_MC_ROOT/mc-tools
> ./mk-dbug

Prepare:
> cd $XTERN_ROOT/apps/mongoose
> cd ./mk
> cd $XTERN_ROOT/apps/aget
> Modify the paths in aget-model-chk-wrapper.cc to match your own paths.
> g++ aget-model-chk-wrapper.cc -o wrapper

Run dbug-only model checking:
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer aget-model-chk-wrapper.xml

Run Parrot+dBug model checking:
> cd XTERN_ROOT/eval/model-chk-wrappers
> make
> cd $XTERN_ROOT/apps/aget
> cp aget-model-chk-wrapper.xml smtmc-wrapper.xml
> Add this line to smtmc-wrapper.xml, before the "program command", order matters:
    <interposition command="/home/heming/rcs/smt+mc//xtern/eval/model-chk-wrapper/wrapper"/>
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer smtmc-wrapper.xml
