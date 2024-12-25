#ifndef __DNS_CLIENT_H__
#define __DNS_CLIENT_H__

#include <stdint.h>
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
#define QCLASS 1
#define END_CHAR 2
#define FAILURE -1
#define SUCCESS 0

uint16_t idx = 0; /** Global identifer */

/**
 * @brief Enum for query type based on IPv4 or v6
 */
typedef enum qnameType
{
    IPV_4 =1, /** To select IPv4 */
    IPV_6 =28 /** To select IPv6 */
} qnameTypeE;


/** 
 * @brief Structure of DNS header  
 */
typedef struct dnsHeader
{
    uint16_t id;        /** Identifer */
    uint16_t flag;     /** Flags */
    uint16_t qdCount;   /** QD count */
    uint16_t anCount;   /** An count*/
    uint16_t nsCount;   /** NS count */
    uint16_t arCount;   /** AR count */
} dnsHeaderT,
 *dnsHeaderP;

/** 
 * @brief Structure of DNS quey  
 */
typedef struct dnsQuestion
{
    char* qname;        /** DNS query */
    uint16_t qtype;     /** v4 or v6 address */
    uint16_t qclass;    /** class internet or other */
} dnsQuestionT,
 *dnsQuestionP;

/** 
 * @brief Master DNS struct
 */
typedef struct dnsQuery
{
    dnsHeaderT hdr;        /** DNS header */
    dnsQuestionT ques;     /** Actual Query to be parsed */
} dnsQueryT,
 *dnsQueryP;

static int countDots ( char *dn, int size );
static void charCount ( char *domainName, int *lenCnt, int stnSz );
static void stringCpy ( char *dest, char *src, int size, int offset );
static void dnsCpy ( char *dest, char *src, int size, int offset1, int offset2 );
static inline void fillHeader( dnsHeaderP hdr );
static void copyQuery( char* arg, int* querySize, char* qname, int len );
static void fillQuery(dnsQueryP query, char* arg, int len);
static inline void fillType ( dnsQuestionP ques, qnameTypeE type );
static void fillDns(dnsQueryP query, char* arg, int len, qnameTypeE type );
static int createSoc ();
static inline void fillOutGoingSocInfo(struct sockaddr_in* serverAddr1);
static void inline memcpyWrapper( void* dest, void* src, int len );
static void serializeData(char* buff, dnsQueryP query, int len);
static int sendQuery(dnsQueryP query, int len);

#endif /** DNS_CLIENT_H__ */