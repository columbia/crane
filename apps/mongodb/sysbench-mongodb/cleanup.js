use sbtest;
db.dropDatabase();
use sbtest;
db.dropUser('myuser');
db.addUser('myuser', 'mypass');
db.auth('myuser', 'mypass');
exit;
