#!/bin/sh

test -d autoconf && test -f autoconf/configure.ac && cd autoconf
test -f configure.ac || die "Can't find 'autoconf' dir; please cd into it first"
echo "Regenerating aclocal.m4 with aclocal"
rm -f aclocal.m4
echo "Regenerating configure with autoconf"
echo autoconf --warnings=all -o ../configure configure.ac || die "autoconf failed"
autoconf --warnings=all -o ../configure configure.ac || die "autoconf failed"
cd ..
echo "Regenerating config.h.in with autoheader"
autoheader --warnings=all \
    -I autoconf -I autoconf/m4 \
    autoconf/configure.ac || die "autoheader failed"
exit 0
