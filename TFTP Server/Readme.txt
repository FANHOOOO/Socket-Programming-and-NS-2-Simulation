This is a guide to use the program written by 
Venkatesh Prasad    UIN:124003455
Yifan Jiang	UIN:924001904


To Compile the File Run make on Server.

The files included are
SERVER
 - server.c
 - tftp.h
 - makefile

To Run the Server
./server.out

For the Server PORT 4095 is used for connections


Architecture:
Our code include the tftp headfile, it defines three types of struct, one is parameters to carry clients’ address, current clients’ number and the pointer to the file requested, another is struct data to send to the clients which contains opCode, block number and data sent, there is also an error struct to send corresponding error information to clients.

In server.c, we define “handleClients”  function, which handles sending data packets to clients, receiving ACK packet from clients and once receiving error packet, retransmit the data packet. Also set a timeout to control transmition, if timeout is greater than 5 seconds, transmition abort. It create a new thread for each client’s request of file transmition, as a result can deal with concurrent data transfer.

