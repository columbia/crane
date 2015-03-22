1. Install mencoder and sqlite3
sudo apt-get install mencoder libsqlite3-dev

2. Make
cd $MSMR_ROOT/apps/mediatomb
./mk

3. Start server and client.

4. Note: mediatom has a database in MSMR_ROOT/apps/mediatomb/.mediatomb,
I have prepared for it in the mk script. You can also rerun the generate-database script
to regenerate the database. The database already has one 15MB avi file.
The conf file of this server is MSMR_ROOT/apps/mediatomb/.mediatomb/config.xml.
You don't need to worry about these if you run start-server and run-client,
I have setup everything for you.
