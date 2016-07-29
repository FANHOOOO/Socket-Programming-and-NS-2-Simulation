/*This is client code*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/select.h>
#include "SBCP.h"


int main(int argc, char *argv[])
{
    int sockfd, portno, n, j;
    struct sockaddr_in serv_addr;  
    fd_set reset;
    int maxfd;
    int nready;
    struct sockaddr_in servaddr_v4;
    struct sockaddr_in6 servaddr_v6;
    struct addrinfo *servinfo;
    
    //clear SBCP message and add a pointer to it 
    bzero(&sbcp, sizeof(sbcp));
    void *S = &sbcp;
    

    //check the format of login in
    if (argc < 4) {
       fprintf(stderr,"Use the format:%s Username server_ip server_port\n", argv[0]);
       exit(0);
    }  

    //set up serv_addr
    portno = atoi(argv[3]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //get address information
    bzero((char *) &servinfo, sizeof(servinfo));
    getaddrinfo(argv[2],NULL,NULL,&servinfo);
     
    //set up serv_addr fo both IPv4 and IPv6
    if (servinfo->ai_family == AF_INET6){
        //set tup IPv6 address
        memset(&servaddr_v6,0,sizeof(servaddr_v6));
        servaddr_v6.sin6_family = AF_INET6;
        servaddr_v6.sin6_port = htons(portno);
        inet_pton(AF_INET6, argv[2], &servaddr_v6.sin6_addr);
        //create a IPv6 socket
        sockfd = socket(servinfo->ai_family, SOCK_STREAM,0);
        if (sockfd < 0) perror("ERROR opening socket");
        //bind servaddr_v6 and when bind failed, report error
        if (connect(sockfd, (struct sockaddr *)&servaddr_v6, sizeof(servaddr_v6)) < 0){
         perror("ERROR on connecting");
         exit(0);
        } 
     } else {
        //set tup IPv4 address
        memset(&servaddr_v4,0,sizeof(servaddr_v4));
        servaddr_v4.sin_family = AF_INET;
        servaddr_v4.sin_port = portno;
        inet_pton(AF_INET, argv[2], &servaddr_v4.sin_addr);
        //create a IPv4 socket
        sockfd = socket(servinfo->ai_family, SOCK_STREAM,0);
        if (sockfd < 0) perror("ERROR opening socket");
        //bind servaddr_v4 and when bind failed, report error
        if (connect(sockfd, (struct sockaddr *)&servaddr_v4, sizeof(servaddr_v4)) < 0){
         perror("ERROR on connecting");
         exit(0);
        }
    } 

    //Define a JOIN packet and send it to server
    sbcp.Vrsn = 3;
    sbcp.Type = 2;
    sbcp.Length = sizeof(sbcp);
    sbcp.username.Type = 2;
    sbcp.username.Length = sizeof(sbcp.username);
    strcpy(sbcp.username.Username, argv[1]);
    n = send(sockfd, S, sizeof(sbcp),0);
    if (n < 0) perror("ERROR sending to socket");

    maxfd = sockfd;

    //begin infinite loop
    while(1){

        FD_ZERO(&reset);
        FD_SET(sockfd, &reset);
        FD_SET(0, &reset);
        nready = select(maxfd+1, &reset,NULL,NULL,NULL);
        if (nready < 0) perror("Error selecting");
		if(FD_ISSET(sockfd, &reset)){
			//Receive message from server
			bzero(&sbcp, sizeof(sbcp));
			n = recv(sockfd, S, sizeof(sbcp),0);
			if (n < 0) perror("ERROR receiving from socket");
            if (n == 0) {
                puts("\nServer is not available!!");
                close(sockfd);
                return 0;
            }
            if (sbcp.Type == 5){
                //Check duplicate Username
                if(strcmp(sbcp.reason.Reason,"This username is taken!!") == 0){
                    puts(sbcp.reason.Reason);
                    close(sockfd);
                    return 0;
                }
                //Check the space for a new user
                if(strcmp(sbcp.reason.Reason,"There is no space for a new user!") == 0){
                    puts(sbcp.reason.Reason);
                    close(sockfd);
                    return 0;
                }
                if(strcmp(sbcp.reason.Reason,":quit\n") == 0){
                    puts("The server is not available!!");
                    close(sockfd);
                    return 0;
                }
                
            }

            //Check successfully connect to server, Receive ACK and FWD
            if(sbcp.Type == 7){
                puts("Successfully connected to Server!!");
                fprintf(stderr, "There is total %d clients online:\n", sbcp.clientcount.CliNum);
                for(j=0;j<50;j++){
                    if(strlen(sbcp.clientlist.ClientName[j]) != 0){
                    fprintf( stderr, "User: %s ", sbcp.clientlist.ClientName[j]);
                    }
                }
                
            }else if(sbcp.Type == 8){
                fprintf(stderr, "\nUser: %s is online!\n", sbcp.username.Username);
                fprintf(stderr, "%s: ",argv[1]);
            }else if(sbcp.Type == 6){
                fprintf(stderr, "\nUser: %s is offline!\n", sbcp.username.Username);
                fprintf(stderr, "%s: ", argv[1]);
            }else if(sbcp.Type == 9){
                fprintf(stderr, "\n\nUser: %s is idle!!\n\n", sbcp.username.Username);
                fprintf(stderr, "%s: ", argv[1]);
            }else {
                //Receive message from server and Discard any message that is not understood
                if (sbcp.Vrsn != 3){
                puts("Message is not understood");
                bzero(&sbcp, sizeof(sbcp));
                continue;
                }else{
                printf("\n%s: %s",sbcp.username.Username, sbcp.message.Msg);
                fprintf(stderr, "%s: ", argv[1]);
                }
            }
        }       			
		


            //check the message typed
        if(FD_ISSET(0, &reset)){
            bzero(&sbcp, sizeof(sbcp));
            sbcp.Vrsn = 3;
            sbcp.Length = sizeof(sbcp);
            sbcp.message.Type = 4;
            sbcp.message.Length = sizeof(sbcp.message);
            fgets(sbcp.message.Msg,512,stdin);
            //Define the quit operator
            if(strcmp(sbcp.message.Msg,":quit\n") == 0){
                sbcp.Type = 6;
                n = send(sockfd, S, sizeof(sbcp), 0);
                if (n < 0) perror("ERROR forwarding to socket");
                close(sockfd);
                return 0; 
            }else{ 
            //Define a SEND message and send it to server
            sbcp.Type = 4;
            n = send(sockfd, S, sizeof(sbcp), 0);
            if (n < 0) perror("ERROR writing to socket");
            fprintf(stderr, "(Message Sent)\n%s: ", argv[1]);            
            }    
        }
        
	}//End of While(1) 
    close(sockfd);
    return 0;
}
