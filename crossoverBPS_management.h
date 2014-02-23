
#ifndef _crossoverBPS_management_JF
#define _crossoverBPS_management_JF


void moduleCheck(unsigned int *voltage, unsigned char *temp);
void sendValues(unsigned int *voltage, unsigned char *temp, unsigned char boardNum, unsigned char activenumber);
unsigned char checkMessages(unsigned char boardNum, unsigned char activenumber, unsigned int *voltage, unsigned char *temp);
void printValues(unsigned int *voltage, unsigned char *temp, unsigned char boardNum, unsigned char activenumber);


#endif