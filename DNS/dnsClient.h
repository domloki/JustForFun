#ifndef __DNS_CLIENT_H__
#define __DNS_CLIENT_H__

#include <stdio.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define DNS_PORT 53
#define MAX_BUF 1000
#define DNS_SERVER_IP "127.0.0.53"

int countDots ( char *dn, int size );
int setDnsClassAndValue ( char *dn1, int currIdx );
void setDnsHdr ( char *dnsHdr );
void charCount ( char *domainName, int *lenCnt, int stnSz );
void stringCpy ( char *dest, char *src, int size, int offset );
void dnsCpy ( char *dest, char *src, int size, int offset1, int offset2 );

#endif /** DNS_CLIENT_H__ */