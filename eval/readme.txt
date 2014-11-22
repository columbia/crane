This is evaluation framework of m-smr system!
Run:
	./eval.py *.cfg

Results (last updated on 11/21/2014):
	[Server=Mongoose, Server count=3, Client=Ab, Client count=100]
	With proxy:
		Consensus Time:
			mean: 21044.2632079 us
			std: 18930.0781884
		Response Time:  
			mean: 21044.5909105 us
			std: 18930.0817163
		Throughput: 1650.34539674 Req/s
	Without proxy:
		Response Time: 1865 us
		Throughput: 5361.93 Req/s

	[Server=Apache, Server count=3, Client=Ab, Client count=100]
	With proxy:
		Consensus Time: 
			mean: 33476.0137477 us
			std: 29464.3784512
		Response Time:
			mean: 33476.3199097 us
			std: 29464.3921525
		Throughput: 1478.994135 Req/s
	Without proxy:
		Response Time: 17985 us
		Throughput: 556.02 Req/s

	[Server=Lighttpd, Server count=3, Client=Ab, Client count=100]
	With proxy:
		Concensus Time:
			mean: 22787.4596385 us
			std: 16067.4879456
		Response Time: 
			mean: 22787.739878 us
			std: 16067.5244515
		Throughput: 1237.37989103 Req/s
	Without proxy:
		Response Time: 26864 us
		Throughput: 372.25 Req/s

	[Server=Pgsql, Server count=3, Client=pgbench, Client count=100]
	With proxy:
		Consensus Time: 
			mean: 25450.4891003 us
			std: 18756.3035088
		Response Time:
			mean: 25451.084145 us
			std: 18756.3308419
		Throughput: 658.945655901 Req/s
	Without proxy:
		Throughput: 107.301376 Transaction/s

	[Server=Ssdb, Server count=3, Client=ssdb-bench, Client count=100]
	With proxy:
		Consensus Time: 
			mean: 46967.2674903 us
			std: 19923.150514
		Response Time: 
			mean: 46967.6448614 us
			std: 19923.0598068
		Throughput: 3193.70895769 Req/s
	Without proxy:
		Response Time: 256000 us
		Throughput: 39098 Query/s

	[Server=Mongodb, Server count=3, Client=ycsb, Client count=1]
	With proxy:
		Consensus Time: 
			mean: 31611.5360169 us
			std: 33265.9289061
		Response Time:
			mean: 31611.8883481 us
			std: 33265.9362168
		Throughput: 556.45594725 Req/s
	Without proxy:
		Average Latency: 7236.46511627907 us
		Throughput: 2801.1204481792715 Req/s

	[Server=proftpd, Server count=1, Client=dkftpbench, Client Count=2]
	With Proxy:
		Concensus Time:
			mean: 820195.412486 us
			std: 376619.771528
		Response Time:
			mean: 820195.608919 us
			std: 376619.813539
		Throughput: 52.4910836419 Req/s

	[Server=memcached, Server Count=3, Client=memslap, Client Count=100]
	With Proxy:
		Concensus Time:
			mean: 3060.86621104 us
			std: 4309.08754029
		Response Time:
			mean: 3061.58168637 us
			std: 4309.0610575
		Throughput: 2046.9926672 Req/s
	Without Proxy:
		Loading time: 8000 us

	[Server=mysql]
	(to be done)
	
	[Server=ldap]
	(to be done)
