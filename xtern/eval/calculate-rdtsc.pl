#!/usr/bin/perl

#
# Copyright (c) 2013,  Regents of the Columbia University 
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other 
# materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

# Usage: ./calculate-rdtsc.pl <directory which stores a recorded schedule from one execution>
# E.g., ./draw-time-chart.pl ./rdtsc_out

$numThreads = 0;
%totalEvents;

$CPUFREQ = `lscpu | grep "MHz" | awk \'{print \$3}\'`;
chomp($CPUFREQ);
$curDir = `pwd`;
unless(-d @ARGV[0]){
    print "Directory @ARGV[0] for rdtsc does not exist, this is fine, bye...";
}
$skipTid1 = 1; # skip idle thread of parrot.
if (scalar(@ARGV) >= 2 && @ARGV[1] eq "--keep-tid1") {
	$skipTid1 = 0;
}

$debug = 1;
sub dbg {
	if ($debug) {
		print $_[0];
	}
}

sub processSortedFile {
	my $sortedFile = $_[0];
	my @fields;
	my $tid;
	my $op;
	my $opSuffix;
	my $clock;
	my $delta;
	my $eip;

	# global states.
	my $numTid = 0;
	my %tidMap;								# map from pthread self id to the showing up tid order.
	my %startClocks; 								# key is tid+op+eip
	my %endClocks; 								# key is tid+op+eip
	my $globalSyncTime = 0;
	my %tidClocks;								# key is tid
	my %tidStartClocks;							# key is tid
	my %tidLastClocks;							# key is tid, last can be start or end.
	my $allThreadsClocks = 0;
	my %syncClocks;								# key is op+eip
	my %numSyncs;								# key is op+eip
	my $numPthreadSync = 0;
	my $startClock = 0;
	my $endClock = 0;
	my $allThreadsSyncWaitTime = 0;


	# Open the file and process it.
	open(LOG, $sortedFile);
	foreach $line (<LOG>) {
		@fields = split(/ /, $line);
		$tid = $fields[0];
		$op = $fields[1];
		$opSuffix = $fields[2];
		$clock = $fields[3];
		$eip = $fields[4];
		chomp($eip);
		#print "PRINT $tid $op $opSuffix $clock\n";

		# update start and end clock.
		if ($startClock == 0) {
			$startClock = $clock;
		}
		$endClock = $clock;
		$tidLastClocks{$tid} = $clock;

		if ($opSuffix eq "START") {
			$startClocks{$tid.$op.$eip} = $clock;
			if ($tidMap{$tid} eq "") {
				$tidMap{$tid} = $numTid;
				$numTid++;
				$tidStartClocks{$tid} = $clock;
			}
		} else {
			if (!($skipTid1 == 1 && $tidMap{$tid} == 1)) { # check whether to skip tid 1, the idle thread of parrot.
				$endClocks{$tid.$op.$eip} = $clock;
				# And update stats here.
				$delta = $endClocks{$tid.$op.$eip} - $startClocks{$tid.$op.$eip};
				if ($delta > 1e8) {
					dbg "Self $tid (tid $tidMap{$tid}) $op eip $eip delta: (end: $endClocks{$tid.$op.$eip}, start: $startClocks{$tid.$op.$eip}) $delta (".$delta/$CPUFREQ*0.000001." s).\n";
				}

				if (!($op =~ m/----/)) { # Ignore parrot internal sync events for global sync wait time and per thread sync wait time.
					# Update global stat.
					$globalSyncTime += $delta;

					# Update per thread stat.
					if ($tidClocks{$tid} eq "") {
						#dbg "New thread $tid.\n";
						$tidClocks{$tid} = $delta;
					} else {
						#dbg "Old thread $tid.\n";
						$tidClocks{$tid} += $delta;
					}
				}

				# Update per sync op stat.
				if ($syncClocks{$op.".".$eip} eq "") {
					$syncClocks{$op.".".$eip} = $delta;
					$numSyncs{$op.".".$eip} = 1;
				} else {
					$syncClocks{$op.".".$eip} += $delta;
					$numSyncs{$op.".".$eip} += 1;
				}
			}

		}

		
	}
	close(LOG);

	# Print stat.
	print "\n\n\n";
	print "CPU frequency $CPUFREQ MHz.\n";
	print "Global execution clock ".($endClock - $startClock)." (".($endClock - $startClock)/$CPUFREQ*0.000001." s).\n";

	# Per tid.
	print "\nSorted by sync wait time, ascending:\n";
	print "Global sync wait clock $globalSyncTime of all threads (".$globalSyncTime/$CPUFREQ*0.000001." s).\n";
	for $key ( sort {$tidClocks{$a} <=> $tidClocks{$b}} keys %tidClocks) {
		if (!($skipTid1 == 1 && $tidMap{$key} == 1)) {
			print "Thread $tidMap{$key} (pthread self $key) sync wait time $tidClocks{$key} (".$tidClocks{$key}/$CPUFREQ*0.000001." s).\n";
		}
	}
	print "\n";
	for $key ( sort {$tidStartClocks{$a} <=> $tidStartClocks{$b}} keys %tidStartClocks) {
		if (!($skipTid1 == 1 && $tidMap{$key} == 1)) {
			my $threadExedTime = $tidLastClocks{$key} - $tidStartClocks{$key};
			$allThreadsClocks += $threadExedTime;
			print "Thread $tidMap{$key} (pthread self $key) execution time $threadExedTime (".$threadExedTime/$CPUFREQ*0.000001." s).\n";
		}
	}
	print "All threads $allThreadsClocks execution time $allThreadsClocks (".$allThreadsClocks/$CPUFREQ*0.000001." s).\n";
	print "\n";

	# Per sync.
	print "\nSorted by sync wait time, ascending:\n";
	for $key ( sort {$syncClocks{$a} <=> $syncClocks{$b}} keys %syncClocks) {
		print "Sync $key sync wait time (# $numSyncs{$key}) $syncClocks{$key} (".$syncClocks{$key}/$CPUFREQ*0.000001." s, or ".100*$syncClocks{$key}/$allThreadsClocks."%).\n";
		if (!($key =~ m/----/)) {	# Ignore the events with "----", only calculate the pthread sync events.
			$allThreadsSyncWaitTime += $syncClocks{$key};
			$numPthreadSync += $numSyncs{$key};
		}
	}
	print "All threads $allThreadsClocks Libc sync (# $numPthreadSync) wait time $allThreadsSyncWaitTime (".$allThreadsSyncWaitTime/$CPUFREQ*0.000001." s, or ".100*$allThreadsSyncWaitTime/$allThreadsClocks."%).\n";
	print "All threads $allThreadsClocks execution time $allThreadsClocks (".$allThreadsClocks/$CPUFREQ*0.000001." s).\n";
	print "\n";

	@fields = split(/\//, @ARGV[0]);
        print "$fields[scalar(@fields)-2] FORMAT:	".($endClock - $startClock)/$CPUFREQ*0.000001."	".$allThreadsClocks/$CPUFREQ*0.000001."	".$syncClocks{"----lineupStart.(nil)"}/$CPUFREQ*0.000001.
        	"	".$syncClocks{"--------GET_TURN.(nil)"}/$CPUFREQ*0.000001."	".$syncClocks{"--------RRScheduler::wait.(nil)"}/$CPUFREQ*0.000001.
        	"	".$allThreadsSyncWaitTime/$CPUFREQ*0.000001."\n";

	print "\n\n\n";
}

sub parseLog {
	my $dirPath = $_[0];

	opendir (DIR, ".");
	while (my $file = readdir(DIR)) {
		next if ($file =~ m/^\./);
		next if ($file =~ m/\.log/);
		next if (!($file =~ m/^pself/));
		print "Processing $dirPath/$file...\n";

		#system("sort -t \" \" -k4 $file > sorted-$file");
		processSortedFile("$file");
	}
}

chdir($ARGV[0]);
parseLog($ARGV[0]);

# clean up.
#system("tar zcvf $ARGV[0].tar.gz $ARGV[0]");
#system("rm -rf $ARGV[0]");

chdir($curDir);

