1. Patch(es) for dBug
To run bdb_rep with dBug, you need to patch (divergence.patch) and recompile bdb_rep.
The divergence.patch is only for dBug, so by default (for xtern) it is disabled.
To enable this patch, you need to uncommment the following content in 'mk' script, and './mk again.
"
#cd db-$VER
#patch -p1 < ../divergence.patch
#cd ..
"
