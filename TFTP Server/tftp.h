/*
 * tftp.h
 *
 *  Created on: Oct 9, 2015
 *      Author: venkatesh
 */

#ifndef TFTP_H_
#define TFTP_H_

#define DATALEN 512

#endif /* TFTP_H_ */


typedef struct{
	int clientCount;
	FILE *f;
	struct sockaddr_in clntAdd;
	unsigned int clientAddLen;
}param;

struct data{
	uint16_t opCode;
	uint16_t blkNumber;
	char data[DATALEN];
};

struct error{
	uint16_t opCode;
	uint16_t errCode;
	char errMsg[15];
};
