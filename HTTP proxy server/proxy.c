#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <pthread.h>

#define PORT 80

//Linked list to store the webpages in "cache"
struct page {
  char filename[50];
  char *exptime;
  //char hist[100];
  struct page *next;
} *root, *tail;


/*
This function is the target function of child thread created in the main function. It handles the rest session of a single client after JOIN the chat room by taking 
into the socket file descriptor of the specific client as a parameter. 
*/

void *single_user_session (void *new_sockfd){	
 	int client_sockfd = *((int *)new_sockfd);
 	int newserver_sockfd, numbytes_recv,i;
 	char buffer[BUFSIZ+1], buffer_tmp[BUFSIZ+1], buffer_r[BUFSIZ+1], sendrequest[BUFSIZ+1], filename_get[50], filename_get_tmp[50], *fname, *fname_1, *fname_2;
 	
 	struct page *temp2, *temp3, *temp4, *prevtemp, *prevtemp2;

	if ((numbytes_recv = recv(client_sockfd, buffer, sizeof(buffer),0)) <= 0){
		close(new_sockfd);
		perror("Receiving GET Request Failed!\n");
		exit(1);
	}
	memset(sendrequest, 0, sizeof(sendrequest));
	memset(buffer_tmp, 0, sizeof(buffer_tmp));
	strcpy(sendrequest, buffer);
	strcpy(buffer_tmp, buffer);
    //printf("%s",buffer);
    char *get = "GET";
    if (strncmp(buffer, get, 3) != 0) {
    	printf("Not a GET Request.\n");
	}
	else printf("GET Request Received!\n");
	
	//Resolving the Get Request From the Client
	filename_get_tmp[0] = '\0';
	fname = buffer;
	fname = fname + 5;
	fname_1 = strtok(fname, " ");
	//printf("fname_1: %s\n", fname_1);
	strcpy(filename_get_tmp, fname_1);
	//printf("filename_get_tmp: %s\n", filename_get_tmp);
	fname_2 = strtok(filename_get_tmp, "/");
	while (fname_2 != NULL){
		strcpy(filename_get, fname_2);
		fname_2 = strtok(NULL, "/");
	}
	printf("Target filename: %s\n",filename_get);
	char *spl = "\r\n";
    char *url;
    url = strtok(buffer_tmp, spl);
    url = strtok(NULL, spl);
    url = url + 6;
    printf("Target url: %s\n",url);
    
    int incache = 0; //Flag to judge the file in cache or not
    int expired = 0; //Flag to judge the file is expired or not
    struct tm tm1,*tm3;
    time_t timer2,timer3;
    
    for(temp3 = root; temp3->next != 0; temp3 = temp3->next){
    	if(strcmp(filename_get, temp3->filename) == 0){
    		//Find the same file in cache
    		incache = 1;
    		//Compare the expire time to the current time
    		strptime(temp3->exptime, "%d %b %Y %H:%M:%S", &tm1);
    		tm1.tm_isdst = 0;
    		timer2 = mktime(&tm1);
    		time(&timer3);
    		tm3 = gmtime(&timer3);
    		timer3 = mktime(tm3);
    		if (timer3 >= timer2)
    		{
    			expired = 1; //Page expired
    			break;
    		}
    		else{//File in cache and not expired
    			FILE *existfd;
    			existfd = fopen(filename_get, "r");
    			int data = 0;
    			while (!feof(existfd)){
    				unsigned char msg[BUFSIZ] = "";
    				for (i=0;i<=BUFSIZ-1;i++){
    					if ((data = fgetc(existfd)) == EOF) break;
    					else
    						msg[i] = (unsigned char) data;
    				}
    				send(client_sockfd, (const char *)msg, BUFSIZ, 0);
    			}
    			printf("File found locally and sent directly to the client!\n");
    			close(existfd);
    			break;
    		}
    	}
    	prevtemp2 = temp3;
	}//End of searching same file in cache
	
	//Delete the expired file
	if (expired == 1){
		printf("File requested found in cache but expired!\n");
		for (temp4=root;temp4->next != 0;temp4=temp4->next){
			if (strcmp(filename_get, temp4->filename) == 0 && temp4 == root){
				root = root->next;
				break;
			}
			else if (strcmp(filename_get, temp4->filename) == 0 && temp4 != root){
				prevtemp->next = temp4->next;
				break;
			}
			else{
				prevtemp = temp4;
			}
		}
	}
	
	//Get new file from real server
	if (incache == 0 || expired == 1){
		if ((newserver_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			perror("Server Socket Creation Failed!\n");
			exit(1);
		}
		//Set up newserver address constructure
		struct sockaddr_in newservAddr;
		struct hostent *newserver_ip;
		newserver_ip = gethostbyname(url);
		memset(&newservAddr, 0, sizeof(newservAddr));
		newservAddr.sin_family = AF_INET;
		newservAddr.sin_port = htons(PORT);
		memcpy(&newservAddr.sin_addr.s_addr, newserver_ip->h_addr, newserver_ip->h_length);
		if (connect(newserver_sockfd, (struct sockaddr*)&newservAddr, sizeof(struct sockaddr)) < 0){
			close(newserver_sockfd);
			perror("Connect to HTTP Server Failed!\n");
			exit(1);
		}
		printf("Connected to the %s Server!\n", url);
		
		//Forwarding GET request
		if (send(newserver_sockfd, (char *)sendrequest, BUFSIZ, 0) < 0) {
        	perror("Error in forwarding the GET message.\n");
        } 
        else{
            printf("Fowarding GET successfully.\n");
            //printf("%s", sendrequest);
        }
		
		FILE *fd;
		fd = fopen(filename_get, "w");
		int byterecv, k;
		int startflag = 0;
		char *content, *expmark;
        char *spll = "\n";
        char *p1, *p2;
        char buffer_r1[BUFSIZ+1];
        p2 = (char *)malloc(50*sizeof(char));
		memset(buffer_r, 0, sizeof(buffer_r));
		memset(buffer_r1, 0, sizeof(buffer_r1));
		while ((byterecv = recv(newserver_sockfd, buffer_r, BUFSIZ, 0)) > 0){
			char *bufferc;
			bufferc = malloc(strlen(buffer_r));
			memcpy(bufferc, buffer_r, strlen(buffer_r));
			if (startflag == 0){
				if((p1 = strstr(bufferc, "Expires:"))!=NULL){
					p1 = p1 + 9;
					memcpy(p2, p1, 50);  
                    expmark = strtok(p2, spll);
                    expmark = expmark + 5;
				} 
				else{
					printf("No such file in the server!\n");
					incache = -1;
					expired = -1;
					break;
				}
				content = strstr(buffer_r, "\r\n\r\n");
				if (content != NULL){
					startflag = 1;
					content += 4;
				}
				for (k=1;k<1000;k++){
					if (content == buffer_r+k) break;
				}
				fwrite(content, byterecv-k, 1, fd);
				send(client_sockfd, content, byterecv-k, 0);
			}
			else{
				fwrite(buffer_r, byterecv, 1, fd);
				send(client_sockfd, buffer_r, byterecv, 0);
			}

			memset(buffer_r, 0, sizeof(buffer_r));
		}
		printf("Forwarding Request to HTTP server and Sending Back to the Client done!\n");
		fclose(fd);
		
		//Add new file in the link list
		if (incache == 0){
			strcpy(tail->filename, filename_get);
			tail->exptime = (char *)malloc(50*sizeof(char));
			strcpy(tail->exptime, expmark);
			tail->next = malloc(sizeof(struct page));
			tail = tail->next;
			tail->next = 0;
		}
		
		//Replace old file and add new file at the last position of the link list
		if (incache == 1 && expired == 1){
			temp3->exptime = (char *)malloc(50*sizeof(char));
			strcpy(temp3->exptime, expmark);
			strcpy(tail->filename, temp3->filename);
			tail->exptime = (char *)malloc(50*sizeof(char));
			strcpy(tail->exptime, temp3->exptime);
			tail->next = malloc(sizeof(struct page));
			tail = tail->next;
			tail->next = 0;
		}
	}//End of get new file from HTTP server
	
	//Manage the existing file from LRU to MRU
	if (incache == 1 && expired == 0){
		if (temp3 == root){
			root = root->next;
		}
		else prevtemp2->next = temp3->next;
		strcpy(tail->filename, temp3->filename);
		tail->exptime = (char *)malloc(50*sizeof(char));
		strcpy(tail->exptime, temp3->exptime);
		tail->next = malloc(sizeof(struct page));
		tail = tail->next;
		tail->next = 0;
	}
	printf("Following File Cached Locally:(From LRU to MRU)\n");
	for (temp2=root;temp2->next != 0;temp2=temp2->next){
        printf("%s\n", temp2->filename);
        printf("%s\n", temp2->exptime);              
    }
	close(client_sockfd);
	close(newserver_sockfd);
}//End of child thread function


int main(int argc, char* argv[]){

	int server_sockfd, new_sockfd;
	root = malloc(sizeof(struct page));
	root->next = 0;
	tail = root;

	if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Server Socket Creation Failed!\n");
		exit(1);
	}

	//Set up server address constructure
 	struct sockaddr_in servAddr, clntAddr;
 	struct hostent *server_ip;
 	unsigned int cliAddrLen;
    server_ip = gethostbyname(argv[1]);
    int port = atoi(argv[2]);
 	memset(&servAddr, 0, sizeof(servAddr));
  	servAddr.sin_family=AF_INET;
  	servAddr.sin_port =htons(port);
  	memcpy(&servAddr.sin_addr.s_addr, server_ip->h_addr, server_ip->h_length);
  	int yes =1;
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR,&yes,sizeof(int));
    //Bind
	if ((bind(server_sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr))) < 0){
 		close(server_sockfd);  
    	perror("Bind Failed!\n");
    	exit(1);
   	}
   	
 	//Listen
	if (listen(server_sockfd, 10) < 0){
		close(server_sockfd);
		perror("Listen Failed!\n");
	    exit(1);
   	}
	
    int numbytes_recv, i;
    int maxuser = 100;
    //Child thread descriptor
    pthread_t user_tid[maxuser];
    int currentuser = 0;
    
    //Standing Server
	while(1){
    	//Accept
   		if ((new_sockfd = accept(server_sockfd, (struct sockaddr *)&clntAddr, &cliAddrLen)) < 0){
   			close(server_sockfd);
			perror("Accept Connection Failed!\n");
    		exit(1);
    	}
    	printf("\n*********************************************\n");
    	printf("New Connection Received!\n");
    	currentuser++; //Current number of clients handling by the proxy server
	   	
	   	//Start a child thread to deal with the user session
		int err = pthread_create(&user_tid[currentuser-1], NULL, single_user_session, (void *)&new_sockfd);
		if (err!=0){
			close(new_sockfd);
			perror("User session thread creation failed!\n");
			exit(1);
		}
		pthread_detach(user_tid[currentuser-1]);

		
	}
    
  
	close(server_sockfd);
	return 0;
}
