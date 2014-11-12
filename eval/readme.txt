This is evaluation framework of m-smr system!
Run:
	./eval.py *.cfg

Results:
	[Server=Mongoose, Server count=3, Client=Aget, Client count=100]
	With proxy:(only connect and send requests)
		Consensus Time: 748381.282559 us
		Response Time: 748381.876493 us
		Throughput: 516.983562155 Req/s
	Without proxy:
		Aget has no performance data.

	[Server=Mongoose, Server count=3, Client=Ab, Client count=100]
	With proxy:(only connect requests)
		Consensus Time: 523000.638876 us
		Response Time: 523000.87089 us
		Throughput: 639.591270992 Req/s
	Without proxy:
		Response Time: 1865 us
		Throughput: 5361.93 Req/s

	[Server=Apache, Server count=3, Client=Ab, Client count=100]
	With proxy:(Only connect requests)
		Consensus Time: 47278.0047236 us
		Response Time: 47278.3269109 us
		Throughput: 749.869527192 Req/s
	Without proxy:
		Response Time: 17985 us
		Throughput: 556.02 Req/s

	[Server=Lighttpd, Server count=3, Client=Ab, Client count=100]
	With proxy:(Only connect requests)
		Concensus Time: 43085.5622163 us
		Response Time: 43085.7619724 us
		Throughput: 831.293779863 Req/s
	Without proxy:
		Response Time: 26864 us
		Throughput: 372.25 Req/s

	[Serve=pgsql, Server count=3, Client=pgbench, Client count=100]
	With proxy:(connect and send)
		Consensus Time: 668500.679511 us
		Response Time: 668502.354863 us
		Throughput: 145.394200774 Req/s
	Without proxy:
		TPS: 107.301376
