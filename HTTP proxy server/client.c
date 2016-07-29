#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <inttypes.h>
#include <pthread.h>

#define USERAGENT "HTMLGET 1.0"
//./client.c <proxy_server_ip> <proxy_server_port> <URL to retrieve>
int main(int argc, char* argv[]){
	int client_sockfd, numbytes_recv;
	
	//Set up server address constructure
	struct sockaddr_in servAddr;
	struct hostent *server_ip;
	server_ip = gethostbyname(argv[1]);
	int port = atoi(argv[2]);
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(port);
	memcpy(&servAddr.sin_addr.s_addr, server_ip->h_addr, server_ip->h_length);
	
	if ((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Client Socket Creation Failed!\n");
		exit(1);
	}
	
	//Connect to server
	if (connect(client_sockfd, (struct sockaddr*)&servAddr, sizeof(struct sockaddr)) < 0){
		close(client_sockfd);
		perror("Connect to Server Failed!\n");
		exit(1);
	}
	else{
		printf("Connected to Proxy Server!\n");
	}
	
	char buf[BUFSIZ+1], buf_tmp[BUFSIZ+1], buf_file[BUFSIZ+1];
	strcpy(buf, argv[3]);
	strcpy(buf_tmp, argv[3]);
	strcpy(buf_file, argv[3]);
	char *url, *document, *filename, *p, *pp;
	
	//Get the url from the command line request
	p = strtok(buf_file, "/");
	while (p != NULL){
		filename = p;
		p = strtok(NULL, "/");	
	}
	//Check if there is "http://" in front of the url
	if (((pp = strstr(buf_tmp, "http://")) != NULL) || ((pp = strstr(buf_tmp, "HTTP://")) != NULL)){
		url = strtok(buf_tmp, "/");
		url = strtok(NULL, "/");
		document = buf + strlen(url) + 8;
	}
	else{
		url = strtok(buf_tmp, "/");
		document = buf + strlen(url) + 1;
	}
	
	printf("%s\n%s\n",url, document);
	

	
	
	//Construct Get Request
	char *query;
  	char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
 
 	query = (char *) malloc(strlen(url)+strlen(document)+strlen(USERAGENT)+strlen(tpl)-5);
  	sprintf(query, tpl, document, url, USERAGENT);
  	//printf("%s\n", query);
  	
  	//Send request to the proxy server
  	if (send(client_sockfd, (char *)query, strlen(query), 0) < 0){
    	perror("Error in forwarding the GET message.\n");
    } 
    else{
        printf("Send GET request successfully.\n");
    }
  	
  	//Downloading file
  	FILE *fd;
  	fd = fopen(filename, "w");
  	char buffer_r[BUFSIZ+1];
  	int count = 0;
  	memset(buffer_r, 0, sizeof(buffer_r));
  	while ((numbytes_recv = recv(client_sockfd, buffer_r, BUFSIZ, 0)) > 0){
  		count++;
  		fwrite(buffer_r, numbytes_recv, 1, fd);
  		memset(buffer_r, 0, sizeof(buffer_r));
  	}
  	if (count == 0) printf("No such file!\n");
	fclose(fd);
	
	close(client_sockfd);
	return 0;
	
}
