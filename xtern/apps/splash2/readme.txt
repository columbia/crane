1. Patch(es) for dBug
To run splash2-fmm with dBug, you need to patch (fmm-dbug.patch) and recompile splash2.
The fmm-dbug.patch is only for dBug, so by default (for xtern) it is disabled.
To enable this patch, you need to uncommment the following content in 'mk' script, and './mk again.
"
#patch -p0 < patch/fmm-dbug.patch
"
