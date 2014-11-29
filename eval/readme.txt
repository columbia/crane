This is evaluation framework of m-smr system!
Run:
	./eval.py *.cfg

Results (last updated on 11/27/2014):
	[Server=Mongoose, Server count=3, Client=Ab, Client count=100]
	With proxy:
		System:
			Consensus Time(5434):
				mean: 11784.3829405 us
				std: 12637.0019955
			Response Time(5434):  
				mean: 12137.4390869 us
				std: 12670.9796392
			Throughput: 1125.57004518 Req/s
		Real Server:
			Time per request: 6024 us
			Requests per second: 1660.03
	Without proxy:
		Time per request: 1865 us
		Requests per second: 5361.93 Req/s

	[Server=Apache, Server count=3, Client=Ab, Client count=100]
	With proxy:
		System:
			Consensus Time(5577): 
				mean: 10093.2689001 us
				std: 15218.2857854
			Response Time(5577):
				mean: 10359.6252461 us
				std: 15281.7748126
			Throughput: 1963.44173126 Req/s
		Real Server:
			Time per request: 7110 us
			Requests per second: 1406.47
	Without proxy:
		Time per request: 17985 us
		Requests per second: 556.02 Req/s

	[Server=Lighttpd, Server count=3, Client=Ab, Client count=100]
	With proxy:
		System:
			Concensus Time:
				mean: 8588.36478158 us
				std: 12737.463843
			Response Time: 
				mean: 8869.2045171 us
				std: 12776.5457869
			Throughput: 776.35067849 Req/s
		Real Server:
			Time per request: 37253 us
			Requests per second: 268.43
	Without proxy:
		Time per request: 26864 us
		Requests per second: 372.25 Req/s

	[Server=Pgsql, Server count=3, Client=pgbench, Client count=2]
	With proxy:
		System:
			Consensus Time(28176): 
				mean: 886.624785799 us
				std: 6457.45645969
			Response Time(28176):
				mean: 991.669019673 us
				std: 6524.97343144
			Throughput: 262.586008948 Req/s
		Real Server:
			tps: 49.670571
	Without proxy:
		tps: 107.301376

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
