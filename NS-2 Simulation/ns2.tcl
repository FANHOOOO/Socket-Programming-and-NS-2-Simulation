# this is a ns2.tcl
#Create new simulator
set ns [new Simulator]

if {$argc != 2} {
	puts stderr "ERROR! Wrong Input Format!!"
	exit 1
}
set TCP_flavor [lindex $argv 0]
set case_no [lindex $argv 1]


#Define different colors for data flow(for NAM)
$ns color 1 Blue
$ns color 2 Red

#Open the Trace file
set tracefile1 [open out.tr w]
$ns trace-all $tracefile1

#Open the NAM trace file
set namfile [open out.nam w]
$ns namtrace-all $namfile

#Define a finish procedure
proc finish {} {
	global ns tracefile1 namfile
	$ns flush-trace
	close $tracefile1
	close $namfile
	exec nam out.nam &
	exit 0
}


set sum1 0
set sum2 0

#Define a record procedure
proc record {} {
	global sink1 sink2 sum1 sum2
	set ns [Simulator instance]
	set time 10.0
	set bw0 [$sink1 set bytes_]
	set bw1 [$sink2 set bytes_]
	set cur_time [$ns now]
	
	puts "Src1: time: $cur_time throughput: [expr $bw0/$time*8/1000000] , Src2: time: $cur_time throughput: [expr $bw1/$time*8/1000000]"
	set sum1 [expr $bw0/$time*8/1000000]
	set sum2 [expr $bw1/$time*8/1000000]
	$sink1 set bytes_ 0
	$sink2 set bytes_ 0
	$ns at [expr $cur_time+$time] "record"
	if {$cur_time == 400} {
		puts "Average Throughput for Src1-Rcv1 link is [expr $sum1/30] Mbps"	
		puts "Average Throughput in Src2-Rcv2 link is [expr $sum2/30] Mbps"
		set ratio [expr $sum1/$sum2]
		puts "Ratio of Throughputs: [expr $ratio]"	
		}
	
}

#Create 6 nodes
set R1 [$ns node]
set R2 [$ns node]
set src1 [$ns node]
set src2 [$ns node]
set rcv1 [$ns node]
set rcv2 [$ns node]

#Create links between the nodes
$ns duplex-link $R1 $R2 1Mb 5ms DropTail
if {$case_no == 1} {
	$ns duplex-link $src1 $R1 10Mb 5ms DropTail
	$ns duplex-link $R2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $src2 $R1 10Mb 12.5ms DropTail
	$ns duplex-link $R2 $rcv2 10Mb 12.5ms DropTail
}
if {$case_no == 2} {
	$ns duplex-link $src1 $R1 10Mb 5ms DropTail
	$ns duplex-link $R2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $src2 $R1 10Mb 20ms DropTail
	$ns duplex-link $R2 $rcv2 10Mb 20ms DropTail
}
if {$case_no == 3} {
	$ns duplex-link $src1 $R1 10Mb 5ms DropTail
	$ns duplex-link $R2 $rcv1 10Mb 5ms DropTail
	$ns duplex-link $src2 $R1 10Mb 27.5ms DropTail
	$ns duplex-link $R2 $rcv2 10Mb 27.5ms DropTail
}

#Give node position
$ns duplex-link-op $R1 $R2 orient right
$ns duplex-link-op $src1 $R1 orient right-down
$ns duplex-link-op $src2 $R1 orient right-up
$ns duplex-link-op $R2 $rcv1 orient right-up
$ns duplex-link-op $R2 $rcv2 orient right-down

#Setup a TCP connection
if {$TCP_flavor == "VEGAS"} {
	set tcp1 [new Agent/TCP/Vegas]
	$tcp1 set class_ 2
	$ns attach-agent $src1 $tcp1
	set sink1 [new Agent/TCPSink]
	$ns attach-agent $rcv1 $sink1
	$ns connect $tcp1 $sink1
	$tcp1 set fid_ 1

	set tcp2 [new Agent/TCP/Vegas]
	$tcp2 set class_ 2
	$ns attach-agent $src2 $tcp2
	set sink2 [new Agent/TCPSink]
	$ns attach-agent $rcv2 $sink2
	$ns connect $tcp2 $sink2
	$tcp2 set fid_ 2
}

if {$TCP_flavor == "SACK"} {
	set tcp1 [new Agent/TCP/Sack1]
	$tcp1 set class_ 2
	$ns attach-agent $src1 $tcp1
	set sink1 [new Agent/TCPSink/Sack1]
	$ns attach-agent $rcv1 $sink1
	$ns connect $tcp1 $sink1
	$tcp1 set fid_ 1

	set tcp2 [new Agent/TCP/Sack1]
	$tcp2 set class_ 2
	$ns attach-agent $src2 $tcp2
	set sink2 [new Agent/TCPSink/Sack1]
	$ns attach-agent $rcv2 $sink2
	$ns connect $tcp2 $sink2
	$tcp2 set fid_ 2
}

#setup FTP over TCP connection
set ftp1 [new Application/FTP]
$ftp1 attach-agent $tcp1
$ftp1 set type_ FTP

set ftp2 [new Application/FTP]
$ftp2 attach-agent $tcp2
$ftp2 set type_ FTP

#Schedule events for FTP agents
$ns at 100.0 "record"
$ns at 0.0 "$ftp1 start"
$ns at 0.0 "$ftp2 start"
$ns at 401.0 "$ftp1 stop"
$ns at 401.0 "$ftp2 stop"

#Call finish procedure after 400 seconds of simulation time
$ns at 402.0 "finish"

#Run the simulation
$ns run











