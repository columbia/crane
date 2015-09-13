1. There may be some compiler dependencies you may need to resolve if you see something like this below:

/home/heming/rcs/smt+mc/xtern/apps/npb/ua-l: /home/heming/rcs/smt+mc//xtern//apps/openmp/install/lib64/libstdc++.so.6:
version `GLIBCXX_3.4.15' not found (required by /home/heming/rcs/smt+mc//xtern//dync_hook/interpose.so)


Method to solve this (on 64 bits. for 32 bits, it is similar):
> cd $XTERN_ROOT/apps/openmp/install/lib64
> mv libstdc++.so libstdc++.so.bak
> mv libstdc++.so.6 libstdc++.so.6.bak
> ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6 libstdc++.so
> ln -s /usr/lib/x86_64-linux-gnu/libstdc++.so.6.0.XX libstdc++.so.6

Then you should be able to run the NPB UA program.

2. Patch(es) for dBug
To run npb-lu with dBug, you need to patch (lu.patch) and recompile npt suite.
The lu.patch is only for dBug, so by default (for xtern) it is disabled. 
To enable this patch, you need to uncommment the following content in 'mk' script, and './mk again. 
"
#patch -p1 < ../patch/lu.patch
"

