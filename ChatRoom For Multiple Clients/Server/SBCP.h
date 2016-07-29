/* SBCP.h */
#ifndef SBCP
#define SBCP

#include <stdio.h>


struct USERNAME
{
	unsigned int Type : 16;//Attribute Type	
	unsigned int Length : 16;//Attribute Length
	char Username[16]; //Username
};

struct MESSAGE
{
	unsigned int Type : 16;//Attribute Type	
	unsigned int Length : 16;//Attribute Length
	char Msg[512]; //Username
};

struct REASON
{
	unsigned int Type : 16;//Attribute Type	
	unsigned int Length : 16;//Attribute Length
	char Reason[32]; //Username
};

struct CLIENTNUMBER
{
	unsigned int Type : 16;//Attribute Type	
	unsigned int Length : 16;//Attribute Length
	unsigned int CliNum : 16;//Client Count
};
struct ClIENTLIST{
	unsigned int Type : 16;//Attribute Type	
	unsigned int Length : 16;//Attribute Length
	char ClientName[50][16];//a list of username attribute
};
struct SBCP {
	unsigned int Vrsn : 9;//Version
	unsigned int Type : 7;//Message Type
	unsigned int Length : 16;//Length of struct SBCP
	struct USERNAME username;//username attribute
	struct MESSAGE message;//message attribute
	struct REASON reason;//reason attribute
	struct CLIENTNUMBER clientcount;//client count attribute
	struct ClIENTLIST clientlist;//a list of usernames attribute
}sbcp;



#endif
