use sbtest;
db.dropDatabase();
use sbtest;
db.addUser('myuser', 'mypass');
db.auth('myuser', 'mypass');
exit;
