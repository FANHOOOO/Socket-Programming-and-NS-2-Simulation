# this is a ns2.tcl
#Create new simulator
set ns [new Simulator]


if {$argc != 2} {
	puts stderr "ERROR! Wrong Input Format!!"
	exit 1
}
set queue_mechanism [lindex $argv 0]
set senario_no [lindex $argv 1]


#Define different colors for data flow(for NAM)
$ns color 1 Blue
$ns color 2 Red

#Open the Trace file
set f0 [open TCP1.tr w]
set f1 [open TCP2.tr w]
set f2 [open UDP.tr w]
set f4 [open out.tr w]

#Define a finish procedure

proc finish1 {} {
	global ns f0 f1 f4
	$ns flush-trace
	close $f0
	close $f1
	close $f4
	exec xgraph TCP1.tr TCP2.tr -geometry 800x400 -x time -y throughput &
	exit 0
}

proc finish2 {} {
	global ns f0 f1 f2 f4
	$ns flush-trace
	close $f0
	close $f1
	close $f2
	close $f4
	exec xgraph TCP1.tr TCP2.tr UDP.tr -geometry 800x400 -x time -y throughput & 
	exit 0
}


set sum1 0
set sum2 0
set sum3 0

#Define a record procedure
proc record1 {} {
	global sink1 sink2 sum1 sum2 f0 f1 f4
	set ns [Simulator instance]
	set time 1.0
	set bw0 [$sink1 set bytes_]
	set bw1 [$sink2 set bytes_]
	set cur_time [$ns now]
	puts $f0 "$cur_time [expr $bw0/$time*8/1000]"
	puts $f1 "$cur_time [expr $bw1/$time*8/1000]"
	puts $f4 "$cur_time [expr $bw0/$time*8/1000] [expr $bw1/$time*8/1000]"
	puts "H1: time: $cur_time throughput: [expr $bw0/$time*8/1000] , H2: time: $cur_time throughput: [expr $bw1/$time*8/1000]"
	set sum1 [expr $sum1+$bw0/$time*8/1000]
	set sum2 [expr $sum2+$bw1/$time*8/1000]
	$sink1 set bytes_ 0
	$sink2 set bytes_ 0
	$ns at [expr $cur_time+$time] "record1"
	if {$cur_time == 180} {
		puts "Average Throughput for H1-H3 link is [expr $sum1/150] Kbps"	
		puts "Average Throughput for H2-H4 link is [expr $sum2/150] Kbps"
		set ratio [expr $sum1/$sum2]
		puts "Ratio of Throughputs: [expr $ratio]"	
		}
	
}

proc record2 {} {
	global sink1 sink2 sink3 sum1 sum2 sum3 f0 f1 f2 f4
	set ns [Simulator instance]
	set time 1.0
	set bw0 [$sink1 set bytes_]
	set bw1 [$sink2 set bytes_]
	set bw2 [$sink3 set bytes_]
	set cur_time [$ns now]
	
	puts $f0 "$cur_time [expr $bw0/$time*8/1000]"
	puts $f1 "$cur_time [expr $bw1/$time*8/1000]"
	puts $f2 "$cur_time [expr $bw2/$time*8/1000]"
	puts $f4 "$cur_time [expr $bw0/$time*8/1000] [expr $bw1/$time*8/1000] [expr $bw2/$time*8/1000]"
	puts "H1: time: $cur_time throughput: [expr $bw0/$time*8/1000] , H2: time: $cur_time throughput: [expr $bw1/$time*8/1000], H3: time: $cur_time throughput: [expr $bw2/$time*8/1000]"
	set sum1 [expr $sum1+$bw0/$time*8/1000]
	set sum2 [expr $sum2+$bw1/$time*8/1000]
	set sum3 [expr $sum3+$bw2/$time*8/1000]
	$sink1 set bytes_ 0
	$sink2 set bytes_ 0
	$sink3 set bytes_ 0
	$ns at [expr $cur_time+$time] "record2"
	if {$cur_time == 180} {
		puts "Average Throughput for H1-H4 link is [expr $sum1/150] Kbps"	
		puts "Average Throughput for H2-H5 link is [expr $sum2/150] Kbps"
		puts "Average Throughput for H3-H6 link is [expr $sum3/150] Kbps"
		}
	
}

#Create 6 nodes
set H1 [$ns node]
set H2 [$ns node]
set H3 [$ns node]
set R1 [$ns node]
set R2 [$ns node]
set H4 [$ns node]
set H5 [$ns node]
set H6 [$ns node]

#Create links between the nodes

if {$senario_no == 1} {
	

	$ns duplex-link $H1 $R1 10Mb 1ms DropTail
	$ns duplex-link $H2 $R1 10Mb 1ms DropTail
	$ns duplex-link $R2 $H3 10Mb 1ms DropTail
	$ns duplex-link $R2 $H4 10Mb 1ms DropTail

	if {$queue_mechanism == "DROPTAIL"} {
		$ns duplex-link $R1 $R2 1Mb 10ms DropTail	
		#set queue size of link
		$ns queue-limit $R1 $R2 20
		$ns queue-limit $R2 $R1 20
	}

	if {$queue_mechanism == "RED"} {
		$ns duplex-link $R1 $R2 1Mb 10ms RED
		#set queue size of link
		$ns queue-limit $R1 $R2 20
		$ns queue-limit $R2 $R1 20
	}

	#Setup a TCP connection
	set tcp1 [new Agent/TCP/Reno]
	$tcp1 set class_ 2
	$ns attach-agent $H1 $tcp1
	set sink1 [new Agent/TCPSink]
	$ns attach-agent $H3 $sink1
	$ns connect $tcp1 $sink1
	$tcp1 set fid_ 1

	set tcp2 [new Agent/TCP/Reno]
	$tcp2 set class_ 2
	$ns attach-agent $H2 $tcp2
	set sink2 [new Agent/TCPSink]
	$ns attach-agent $H4 $sink2
	$ns connect $tcp2 $sink2
	$tcp2 set fid_ 2

	#setup FTP over TCP connection
	set ftp1 [new Application/FTP]
	$ftp1 attach-agent $tcp1
	$ftp1 set type_ FTP

	set ftp2 [new Application/FTP]
	$ftp2 attach-agent $tcp2
	$ftp2 set type_ FTP


	#Give node position
	$ns duplex-link-op $R1 $R2 orient right
	$ns duplex-link-op $H1 $R1 orient right-down
	$ns duplex-link-op $H2 $R1 orient right-up
	$ns duplex-link-op $R2 $H3 orient right-up
	$ns duplex-link-op $R2 $H4 orient right-down

	#Schedule events for FTP agents
	$ns at 30.0 "record1"
	$ns at 0.0 "$ftp1 start"
	$ns at 0.0 "$ftp2 start"
	$ns at 180.5 "$ftp1 stop"
	$ns at 180.5 "$ftp2 stop"

	#Call finish procedure after 400 seconds of simulation time
	$ns at 180.6 "finish1"
}

if {$senario_no == 2} {
	
	$ns duplex-link $H1 $R1 10Mb 1ms DropTail
	$ns duplex-link $H2 $R1 10Mb 1ms DropTail
	$ns duplex-link $H3 $R1 10Mb 1ms DropTail
	$ns duplex-link $R2 $H4 10Mb 1ms DropTail
	$ns duplex-link $R2 $H5 10Mb 1ms DropTail
	$ns duplex-link $R2 $H6 10Mb 1ms DropTail

	if {$queue_mechanism == "DROPTAIL"} {
		$ns duplex-link $R1 $R2 1Mb 10ms DropTail	
		#set queue size of link
		$ns queue-limit $R1 $R2 20
		$ns queue-limit $R2 $R1 20
	}

	if {$queue_mechanism == "RED"} {
		$ns duplex-link $R1 $R2 1Mb 10ms RED
		#set queue size of link
		$ns queue-limit $R1 $R2 20
		$ns queue-limit $R2 $R1 20
	}

	#Setup a TCP connection
	set tcp1 [new Agent/TCP/Reno]
	$tcp1 set class_ 2
	$ns attach-agent $H1 $tcp1
	set sink1 [new Agent/TCPSink]
	$ns attach-agent $H4 $sink1
	$ns connect $tcp1 $sink1
	$tcp1 set fid_ 1

	set tcp2 [new Agent/TCP/Reno]
	$tcp2 set class_ 2
	$ns attach-agent $H2 $tcp2
	set sink2 [new Agent/TCPSink]
	$ns attach-agent $H5 $sink2
	$ns connect $tcp2 $sink2
	$tcp2 set fid_ 2

	#setup a UDP connection
	set udp [new Agent/UDP]
	$ns attach-agent $H3 $udp

	#Create a LossMonitor agent (a traffic sink) and attach it to node dest
    set sink3 [new Agent/LossMonitor]
    $ns attach-agent $H6 $sink3
	$ns connect $udp $sink3


	#setup FTP over TCP connection
	set ftp1 [new Application/FTP]
	$ftp1 attach-agent $tcp1
	$ftp1 set type_ FTP

	set ftp2 [new Application/FTP]
	$ftp2 attach-agent $tcp2
	$ftp2 set type_ FTP

	#setup CBR over UDP connection
	set cbr [new Application/Traffic/CBR]
	$cbr attach-agent $udp
	$cbr set type_ CBR
	$cbr set packet_size_ 100
	$cbr set interval_ 1
	$cbr set rate_ 1mb
	$cbr set random_ 2
	#$cbr set maxpkts_ 100000

	#Give node position
	$ns duplex-link-op $R1 $R2 orient right
	$ns duplex-link-op $H1 $R1 orient right-down
	$ns duplex-link-op $H2 $R1 orient right
	$ns duplex-link-op $H3 $R1 orient right-up
	$ns duplex-link-op $R2 $H4 orient right-up
	$ns duplex-link-op $R2 $H5 orient right
	$ns duplex-link-op $R2 $H6 orient right-down


	#Schedule events for FTP and CBR agents
	$ns at 30.0 "record2"
	$ns at 0.0 "$ftp2 start"
	$ns at 0.0 "$ftp1 start"
	$ns at 0.0 "$cbr start"
	$ns at 180.5 "$ftp1 stop"
	$ns at 180.5 "$ftp2 stop"
	$ns at 180.5 "$cbr stop"

	#Call finish procedure after 400 seconds of simulation time
	$ns at 180.6 "finish2"
}








#Run the simulation
$ns run











