#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <inttypes.h> // used to declare uint16 for data
#include "tftp.h"

#define MYPORT "4095"    // the port users will be connecting to
#define MAXBUFLEN 279
#define DATALEN 512
#define MAX_CLIENTS 100 //No of connections that the program can handle

void handleClients(void *para)
{
    //First packet to go from this thread with the new port number
    param *recv;
    param recv_param;
    recv = (param *)para;

    memcpy(&recv_param,recv,sizeof(recv_param));
    int port = atoi(MYPORT)+recv_param.clientCount;
    //Create new socket to sending data
    int newSocketFD,rv;

    struct addrinfo hints,*newServinfo,*p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_DGRAM;
    hints.ai_flags=AI_PASSIVE;

    char portNo[5];
    sprintf(portNo, "%d", port);

    if ((rv = getaddrinfo(NULL, portNo, &hints, &newServinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return;
    }

    // loop through all the results and bind to the first we can
    for(p = newServinfo; p != NULL; p = p->ai_next) {
        if ((newSocketFD = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("Error connecting to new socket\n");
            continue;
        }
        if (bind(newSocketFD, p->ai_addr, p->ai_addrlen) == -1) {
            close(newSocketFD);
            perror("Binding Error for new socket\n");
            continue;
        }
        break;
    }//End of for loop

    freeaddrinfo(newServinfo);

    struct data dataPacket;
    char mem[DATALEN];
    char read_data[5];
    int i=0,BlkNo=0,numbytes; // Used for handling the count
    struct timeval timeStart,timeEnd,timeACK;
    int timeUsed=0;

    clearerr(recv_param.f);

    while(!feof(recv_param.f)&&!ferror(recv_param.f))
    {
        BlkNo++;
        memset(&dataPacket,0,sizeof(dataPacket));
        memset(&mem,0,sizeof(mem));
        for(i=0;i<512;i++){
            if(!feof(recv_param.f)){
                mem[i]=fgetc(recv_param.f);
                //putchar(mem[i]); //Was used to check proper read
            }
            else{
                i--;
                break;
            }
        }

        memcpy(&dataPacket.data,(const void *)mem,sizeof(mem));
        dataPacket.blkNumber=htons(BlkNo);
        dataPacket.opCode=htons(3);

        gettimeofday(&timeStart,0);

        if(sendto(newSocketFD ,(char *)&dataPacket,i+4,0,
            (struct sockaddr *)&recv_param.clntAdd, recv_param.clientAddLen)<0){
            perror("Error Sending Data\n");
            exit(1);
                    }
        printf("%d Transmitted\n & Pckt size was %d",BlkNo,i+4);

        fd_set allset;
        FD_ZERO(&allset);
        FD_SET(newSocketFD,&allset);
        while(timeUsed<=5){
            //Revive ACK if recived then go ahead, else retransmit
            //If time out then exit(1) no longer handle the file
            timeACK.tv_sec=0;
            timeACK.tv_usec=10000;
            if(select(newSocketFD+1,&allset,NULL,NULL,&timeACK)>0){
                if(FD_ISSET(newSocketFD,&allset)){
                    if ((numbytes = recvfrom(newSocketFD, read_data, sizeof(read_data) , 0,
                            (struct sockaddr *)&recv_param.clntAdd, &recv_param.clientAddLen)) == -1) {
                            perror("Error in ACK\n");
                            exit(1);
                        }
                    else{
                        uint16_t BlckNoRecvd;
                        memcpy(&BlckNoRecvd,read_data+2,sizeof(BlckNoRecvd));
                        if(read_data[1]==4&&BlkNo==ntohs(BlckNoRecvd))
                        {
                            timeUsed=0;//So that it doesnt cause problem Next time loop is called
                            //Also incated sucessful transmission of data;
                            printf("%d ACK Recived\n",BlkNo);
                            break;
                        }
                        else{//If the Block no. did not match then we retransmit

                        }
                    }

                }

            }
         //Re transmit

        if(sendto(newSocketFD ,(char *)&dataPacket,i+4,0,
            (struct sockaddr *)&recv_param.clntAdd, recv_param.clientAddLen)<0){
            perror("Error Resending Data\n");
            exit(1);
                    }
        printf("%d Re transmitted\n",BlkNo);

        gettimeofday(&timeEnd,0);

        timeUsed=timeEnd.tv_sec-timeStart.tv_sec;

        }//End of while(timeUsed)

    if(timeUsed!=0){// This happens when Valid ACK is not recived
        printf("Connection timed out\n");
        break;
    }

    }
    printf("File transmission Ended\n");
    close(newSocketFD);
    return;

}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void)
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;

    int rv;
    int numbytes;
    struct sockaddr_in client_addr;
    char buf[MAXBUFLEN];
    unsigned int addr_len;


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // set to AF_INET to force IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Local host

    if ((rv = getaddrinfo(NULL, MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("Error connecting to socket");
            continue;
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("Binding Error");
            continue;
        }
        break;
    }// End of for loop

    if (p == NULL) {
        fprintf(stderr, "Failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    printf("Connection Setup: Waiting for clients\n");

    //Declarations related to threads
    pthread_t clnt_id[MAX_CLIENTS];
    int countClients;
    countClients=0;
    FILE *fhandler;

    struct error error_pckg;// Handles sending the error message to client
    addr_len = sizeof client_addr;
    while(1){
        memset(&client_addr,0,sizeof(client_addr)); //Clears client address every time before receiving
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
                (struct sockaddr *)&client_addr, &addr_len)) == -1) {
                perror("Error on client request\n");
                exit(1);
            }

        if(buf[1]==1){ // We received an RRQ
            //printf("valid request recived\n");
            printf("Requested for file :%s\n",buf+2);
            fhandler = fopen(buf+2,"rb"); //We open file with binary read mode to avoid \r\n converstions
                    if(fhandler==NULL)
                    { //This means that the file is not existing
                        printf("File name %s does not exist\n",buf+2);
                        //Create the error package to be sent
                        error_pckg.errCode=htons(1);
                        error_pckg.opCode=htons(5);
                        strcpy(error_pckg.errMsg,"File not found!");
                        if(sendto(sockfd,(char *)&error_pckg,sizeof(error_pckg),0,(struct sockaddr *)&client_addr, addr_len)<0){
                            perror("Could not send error message\n");
                            exit(1);
                        }

                    }
                    else{//In this case the file exists hence pass the argument to the thread to handle the communication from here on
                        countClients++;
                        param pass_data;
                        pass_data.clientCount=countClients;
                        pass_data.f=fhandler;
                        pass_data.clientAddLen = addr_len;
                        pass_data.clntAdd =client_addr;
                        int err_p=pthread_create(&clnt_id[countClients-1],NULL,&handleClients,&pass_data);
                        if(err_p<0){
                            perror("Error creating thread\n");
                            exit(1);
                        }

                        pthread_detach(clnt_id[countClients-1]);

                    }

            }

        else{ //Send an Error Message stating unknown request

            error_pckg.errCode=htons(4);
            error_pckg.opCode=htons(5);
            strcpy(error_pckg.errMsg,"Req. type unknown!");
            if(sendto(sockfd,(char *)&error_pckg,sizeof(error_pckg),0,(struct sockaddr *)&client_addr, addr_len)<0){
                perror("Could not send error message\n");
                exit(1);
            }
        }

    }//End of while loop
    close(sockfd);
    return 0;
}
