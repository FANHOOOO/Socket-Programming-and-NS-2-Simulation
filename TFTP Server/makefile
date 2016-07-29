all: server.o  
		gcc server.o -o server.out -lpthread

server.o: server.c tftp.h
		gcc -c server.c -lpthread
clean: 
		rm -rf *o server
