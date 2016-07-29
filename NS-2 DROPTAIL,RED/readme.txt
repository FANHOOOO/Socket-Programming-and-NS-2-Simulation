This is a guide to use the program written by 

Venkatesh Prasad    UIN:124003455

Yifan Jiang    UIN:924001904




The files included are
 - ns2.tcl


To Run the NS2 simulation 
ns ns2.tcl <queue_mechanism> <scenario_no>



Queue_mechanism should be DROPTAIL or RED

scenario_no should be 1 or 2


for example:
ns ns2.tcl DROPTAIL 1




Architecture:

ns2.tcl:


Take the input command line to get corresponding Queue_mechanism and scenario number. Then construct different network structure with different TCP or UDP. Define a record function, set time interval to 1 second and then start recording average throughput for every 1 second from time 30. After each 1 second, reset counter to 0 and record a new interval average throughput. Display current throughput on terminal. When go through 150 seconds, calculate the total average throughput from time 30 to time 180. Then program stop.




