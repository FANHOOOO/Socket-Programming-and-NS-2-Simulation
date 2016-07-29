/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include "SBCP.h"

int main(int argc, char *argv[])
{
     int sockfd, acpsockfd,resockfd, portno, maxcli;
     socklen_t clilen;
     struct sockaddr_in servaddr_v4;
     struct sockaddr_in6 servaddr_v6;
     struct sockaddr_storage cli_addr;  
     struct addrinfo *servinfo;
     int n;
     int i, j, maxfd, maxi;
     int nready;
     fd_set reset, allset;

     //clear SBCP message and add a pointer to it

     bzero(&sbcp, sizeof(sbcp));
     void *S = &sbcp;
      
     //check the format of login in
     if (argc < 4) {
         fprintf(stderr,"Use format:%s server_ip server_port max_clients\n", argv[0]);
         exit(1);
     }

     //get max clients
     maxcli = atoi(argv[3]);

     //define a clients array
     int client[maxcli];
     char client_name[maxcli][16];
     memset(client_name,0,maxcli*16*sizeof(char));
     //get port number
     portno = atoi(argv[2]);

     //get address information
     bzero((char *) &servinfo, sizeof(servinfo));
     getaddrinfo(argv[1],NULL,NULL,&servinfo);
     
     //set up serv_addr fo both IPv4 and IPv6
     if (servinfo->ai_family == AF_INET6){
        //set tup IPv6 address
        memset(&servaddr_v6,0,sizeof(servaddr_v6));
        servaddr_v6.sin6_family = AF_INET6;
        servaddr_v6.sin6_port = htons(portno);
        inet_pton(AF_INET6, argv[1], &servaddr_v6.sin6_addr);
        //create a IPv6 socket
        sockfd = socket(servinfo->ai_family, SOCK_STREAM,0);
        if (sockfd < 0) perror("ERROR opening socket");
        //bind servaddr_v6 and when bind failed, report error
        if (bind(sockfd, (struct sockaddr *)&servaddr_v6, sizeof(servaddr_v6)) < 0){
         perror("ERROR on binding");
         exit(0);
        } 
     } else {
        //set tup IPv4 address
        memset(&servaddr_v4,0,sizeof(servaddr_v4));
        servaddr_v4.sin_family = AF_INET;
        servaddr_v4.sin_port = portno;
        inet_pton(AF_INET, argv[1], &servaddr_v4.sin_addr);
        //create a IPv4 socket
        sockfd = socket(servinfo->ai_family, SOCK_STREAM,0);
        if (sockfd < 0) perror("ERROR opening socket");
        //bind servaddr_v4 and when bind failed, report error
        if (bind(sockfd, (struct sockaddr *)&servaddr_v4, sizeof(servaddr_v4)) < 0){
         perror("ERROR on binding");
         exit(0);
        }
    } 
           
     //listen to the clients and accept clients
     listen(sockfd, maxcli);             
     maxfd = sockfd;
     for(i = 0; i < maxcli; i++ ) client[i] = -1;

     //initiate descriptors set
     FD_ZERO(&allset);
     FD_ZERO(&reset);
     FD_SET(sockfd, &allset);
     FD_SET(0, &allset);

     puts("Welcome to Chat Room!! Wait for users!! ");

     //infinite loop  
     loop: while(1){
        reset = allset;        
        nready = select(maxfd+1, &reset, NULL, NULL, NULL);
        if (nready < 0) {
            perror("ERROR selecting");
        }else{    
        //Define the quit operator
        if(FD_ISSET(0, &reset)){
            bzero(&sbcp, sizeof(sbcp));
            sbcp.Vrsn = 3;
            sbcp.Type = 5;
            sbcp.Length = sizeof(sbcp);
            sbcp.reason.Type = 1;
            sbcp.reason.Length = sizeof(sbcp.reason);
            fgets(sbcp.reason.Reason,32,stdin);
            if(strcmp(sbcp.reason.Reason,":quit\n") == 0){
                for(i=0; i<maxcli; i++){
                //send NAK to clients
                    if(client[i] != -1){
                    n = send(client[i], S, sizeof(sbcp), 0);
                    if (n < 0) perror("ERROR forwarding to socket");
                    }
                }
                close(sockfd);
                return 0; 
            }
        }           
        //New connection from clients
        if(FD_ISSET(sockfd, &reset)){ // New Connection
            clilen = sizeof(cli_addr);
            acpsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (acpsockfd < 0) perror("ERROR on accept");
            //read JOIN packet from clients
            bzero(&sbcp, sizeof(sbcp));
            n = recv(acpsockfd, S, sizeof(sbcp),0);
            if (n < 0) perror("ERROR reading from socket");

            //check duplicate usernames
            for(j=0;j<maxcli;j++){
                if (strcmp(sbcp.username.Username, client_name[j]) == 0){
                    puts("Connection Denied: Username Duplicate!!");
                    //send NAK to users
                    sbcp.Vrsn = 3;
                    sbcp.Type = 5;
                    sbcp.Length = sizeof(sbcp);
                    sbcp.reason.Type = 1;
                    sbcp.reason.Length = sizeof(sbcp.reason);
                    strcpy(sbcp.reason.Reason,"This username is taken!!"); 
                    n = send(acpsockfd, S, sizeof(sbcp), 0);
                    if (n < 0) perror("ERROR forwarding to socket");
                    close(acpsockfd);
                    goto loop;
                }                
            }
            //save this client's descriptor and username
            for(i=0;i<maxcli;i++)
                if(client[i]<0){
                    client[i] = acpsockfd;
                    strcpy(client_name[i],sbcp.username.Username);
                    break;
                } 

            //check if the number of clients reach maximum
            if(i == maxcli){
                //send NAK to users
                sbcp.Vrsn = 3;
                sbcp.Type = 5;
                sbcp.Length = sizeof(sbcp);
                sbcp.reason.Type = 1;
                sbcp.reason.Length = sizeof(sbcp.reason);
                strcpy(sbcp.reason.Reason,"There is no space for a new user!"); 
                n = send(acpsockfd, S, sizeof(sbcp), 0);
                if (n < 0) perror("ERROR forwarding to socket");
                close(acpsockfd);
                goto loop;
            }

            //find the number of clients     
            maxi = 0;
            for(j=0;j<maxcli;j++){
                if(client[j] != -1) maxi++;
            } 

            //show this user is online
            printf("User: %s is online\n", sbcp.username.Username);

            //check maxfd
            FD_SET(acpsockfd, &allset);
            if(acpsockfd > maxfd) maxfd = acpsockfd;



            //send an ACK to client to show successful connection
            bzero(&sbcp, sizeof(sbcp));
            sbcp.Vrsn = 3; 
            sbcp.Type = 7;
            sbcp.Length = sizeof(sbcp);
            sbcp.clientcount.Type = 3;
            sbcp.clientcount.Length = sizeof(sbcp.clientcount);
            sbcp.clientcount.CliNum = maxi;
            sbcp.clientlist.Type = 2;
            sbcp.clientlist.Length = sizeof(sbcp.clientlist);
            for(j=0;j< maxcli;j++){
                if(client[j]!= -1){
                    strcpy(sbcp.clientlist.ClientName[j], client_name[j]);
                }
            }
            n = send(acpsockfd, S, sizeof(sbcp), 0);
            if (n < 0) perror("ERROR forwarding to socket");
                
            

            //send an ONLINE to clients to show successful connection
            bzero(&sbcp, sizeof(sbcp));
            sbcp.Vrsn = 3; 
            sbcp.Type = 8;
            sbcp.Length = sizeof(sbcp);
            sbcp.username.Type = 3;
            sbcp.username.Length = sizeof(sbcp.username);
            strcpy(sbcp.username.Username, client_name[i]);
            for(j=0; j<maxcli; j++){
                if(client[j]!= -1){
                n = send(client[j], S, sizeof(sbcp), 0);
                if (n < 0) perror("ERROR forwarding to socket");
                }
            }       
        }

        //for every clients
        for(i =0; i<maxcli;i++){
            resockfd = client[i];
            if(resockfd <0) continue;

            if(FD_ISSET(resockfd,&reset)){
                 //read packets from clients
                 bzero(&sbcp, sizeof(sbcp));
                 n = recv(resockfd, S, sizeof(sbcp),0);
                 if (n < 0) perror("ERROR reading from socket");
                 //check if client quit
                 if(n == 0){
                        close(resockfd);
                        client[i] = -1;
                        printf("User: %s is offline\n", client_name[i]);
                        for(j=0; j<maxcli; j++){
                        if(client[j] != resockfd && client[j] != -1){
                            bzero(&sbcp, sizeof(sbcp));
                            sbcp.Vrsn = 3; 
                            sbcp.Type = 6;
                            sbcp.Length = sizeof(sbcp);
                            sbcp.username.Type = 2;
                            sbcp.username.Length = sizeof(sbcp.username);
                            strcpy(sbcp.username.Username, client_name[i]);
                            for(j=0; j<maxcli; j++){
                                if(client[j] != resockfd && client[j] != -1){
                                n = send(client[j], S, sizeof(sbcp), 0);
                                if (n < 0) perror("ERROR forwarding to socket");
                                }
                            }
                        }
                        }
                        memset(client_name[i],0,16*sizeof(char));
                        FD_CLR(resockfd, &allset);
                    }else {
                 if (sbcp.Vrsn != 3){
                        puts("Message is not understood");
                        bzero(&sbcp, sizeof(sbcp));
                        continue;
                        } 
                 //send message to all the clients except the one send it
                 if (sbcp.Type == 4){
                 printf("Message recieved from %s: %s", client_name[i], sbcp.message.Msg);
                 strcpy (sbcp.username.Username, client_name[i]);
                    for(i=0; i<maxcli; i++){
                        if(client[i] != resockfd && client[i] != -1){
                        n = send(client[i], S, sizeof(sbcp), 0);
                        if (n < 0) perror("ERROR forwarding to socket");
                        }
                    }
                 }else if(sbcp.Type == 6){
                        close(resockfd);
                        client[i] = -1;
                        printf("User: %s is offline\n", client_name[i]);    
                        for(j=0; j<maxcli; j++){
                        if(client[j] != resockfd && client[j] != -1){
                            bzero(&sbcp, sizeof(sbcp));
                            sbcp.Vrsn = 3; 
                            sbcp.Type = 6;
                            sbcp.Length = sizeof(sbcp);
                            sbcp.username.Type = 2;
                            sbcp.username.Length = sizeof(sbcp.username);
                            strcpy(sbcp.username.Username, client_name[i]);
                            for(j=0; j<maxcli; j++){
                                if(client[j] != resockfd && client[j] != -1){
                                n = send(client[j], S, sizeof(sbcp), 0);
                                if (n < 0) perror("ERROR forwarding to socket");
                                }
                            }
                        }
                        }
                        memset(client_name[i],0,16*sizeof(char));
                        FD_CLR(resockfd, &allset);
                 }
                }//End of else

            } //End of if
        }//End else
     }//End of for loop
    }//End of the infinite while loop    
}//End main






