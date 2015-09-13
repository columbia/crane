0. a good link to setup openldap.
http://blog.csdn.net/magic881213/article/details/7848577

1. Build.
cd $XTERN_ROOT/apps/ldap
./mk

2. Start server (server is started as debug mode, and listens to 4008 port).
./install/libexec/slapd.x86 -d256 -f $XTERN_ROOT/apps/ldap/install/etc/openldap/slapd.conf -h ldap://:4008

3. Add one entry (just a simple client operation).
./install/bin/ldapadd -H ldap://:4008 -x -D "cn=manager, dc=example,dc=com" -w secret -f test2.ldif

4. Run a concurrent benchmark with the openldap server. The benchmark comes from the test suite of openldap.
./run

5. How to run the model checking experiments (dBug-only and Parrot+dBug).
Rebuild dBug on the ldap branch:
> cd $SMT_MC_ROOT/mc-tools/dbug
> git checkout ldap
> git pull
> cd $SMT_MC_ROOT/mc-tools
> ./mk-dbug

Prepare for environment:
> cd $XTERN_ROOT/apps/eval
> ./eval.py -mc --generate-xml-only ldap-dbug.xml
> cd current/266_ldap_slapd_mtread/
> cp $XTERN_ROOT/apps/ldap/*wrapper.* .
> cp $XTERN_ROOT/apps/ldap/*.strategy .
> Modify the paths in wrapper.cc to match your own paths.
> g++ wrapper.cc -o wrapper

Run dbug-only model checking:
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer dbug-only-wrapper.xml

Run Parrot+dBug model checking:
> $SMT_MC_ROOT/mc-tools/dbug/install/bin/dbug-explorer smtmc-wrapper.xml
