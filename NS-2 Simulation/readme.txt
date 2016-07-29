This is a guide to use the program written by 
Venkatesh Prasad    UIN:124003455
Yifan Jiang    UIN:924001904


The files included are
 - ns2.tcl

To Run the NS2 simulation 
ns ns2.tcl <TCP_flavor> <case_no>

TCP_flavor should be VEGAS or SACK
case_no should be 1, 2, or 3

for example:
ns ns2.tcl VEGAS 1


Architecture:

ns2.tcl:

Take the input command line to get corresponding TCP flavor and case number. Then construct different network structure with different TCP and different end-to-end RTTs. Define a record function, set time interval to 10 seconds and then start recording average throughput for every 10 seconds from time 100. After each 10 seconds, reset counter to 0 and record a new interval average throughput. Display current throughput on terminal. When go through 400 seconds, calculate the total average throughput from time 100 to time 400. Then program stop.




