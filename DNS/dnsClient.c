#include "dnsClient.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Function to count number of dots in dns request
 * 
 * @param   dn : DNS query
 * @param size : Size of DNS query
 * @returns on success number of dots in query 0 otherwise
 */
static int countDots ( char* dn, int size )
{
    int count = 0;
    for ( int i=0; i<size;i++ )
    {
        if ( dn[i] == '.' ) count++;
    }
    return count;
}


/** 
 * @brief To count the character in domain name exclusing dots
 * 
 * @param domainName : Domain Name
 * @param     lenCnf : Pointer to length of domain name
 * @param      stnSz : statement size
 */
static void charCount ( char *domainName, int *lenCnt, int stnSz )
{
    int len = 0;
    for ( int i=0; i<stnSz; i++ )
    {
        if ( domainName[i] != '.' ) lenCnt[len]++;
        else len++;
    }
}


/**
 * @brief To copy string with added offset
 * 
 * @param   dest : Destination char pointer
 * @param    src : Source character pointer
 * @param   size : Amount of char to be copy
 * @param offset : Offset from which data need to be taken
 */
static void stringCpy ( char *dest, char *src, int size, int offset )
{
    for ( int i=0; i< size; i++ )
    {
        dest[i+offset] = src[i];
    }
}


/**
 * @brief To copy string with added offset
 * 
 * @param    dest : Destination char pointer
 * @param     src : Source character pointer
 * @param offset1 : Offset from which data need to be taken
 * @param offset2 : Offset from which data need to be taken
 */
static void dnsCpy ( char *dest, char *src, int size, int offset1, int offset2 )
{
    for ( int i=0; i< size; i++ )
    {
        dest[i+offset1] = src[i+offset2];
    }
}


/**
 * @brief To fill DNS header
 *
 * @param hdr : Pointer to DNS header structure
 */
static inline void fillHeader( dnsHeaderP hdr )
{
    hdr->id = htons(idx+1); /** Fill Identider */
    hdr->flag = htons(256);  /** TODO: make an actual header filling funtion */
    hdr->qdCount = htons(1);
    hdr->anCount = 0;
    hdr->nsCount = 0;
    hdr->arCount = 0;
}


/**
 * @brief To copy the DNS query given by user
 *
 * @param       arg : Actual argument passed by user
 * @param querySize : Number of characters seperated by dot
 * @param     qname : destinaton buffer where query is to be copied
 * @param       len : Nummber of dots +1
 */
static void copyQuery( char* arg, int* querySize, char* qname, int len )
{
    int idx1 = 0;
    int idx2 = 0;
    for ( int i=0; i< len; i++ )
    {
        qname[idx1++] = querySize[i];
        dnsCpy(&qname[0], arg, querySize[i], idx1, idx2);
        idx1 += querySize[i];
        idx2 += querySize[i] + 1;
    }
}


/**
 * @brief To fill entier DNS query Header + query + query data
 *
 * @param query : Pointer to DNS query 
 * @param   arg : Original Query given by user
 * @param   len : Length of query
 */
static void fillQuery(dnsQueryP query, char* arg, int len)
{
    int noOfDots = 0;
    int* querySize = NULL;

    noOfDots = countDots(arg, len);

    querySize = (int *) calloc (noOfDots+1, sizeof(int));
    if ( NULL == querySize )
    {
        printf("Memory allocation failed\n");
        return;
    }

    charCount(arg,querySize,len);

    /** Len +2 is done to write last two bytes as 0x00 to indicate end of character */
    query->ques.qname = (char *) calloc ( (len+END_CHAR), sizeof(char));
    if ( NULL == query->ques.qname )
    {
        printf("Memory allocation failed\n");
        free(querySize);
        return;
    }

    copyQuery(arg, querySize, query->ques.qname, noOfDots+1);
    query->ques.qclass = htons(QCLASS);
    free(querySize);
}


/** 
 * @brief To fill query type as IPv4 or IPv6
 *
 * @param ques : Pointer of type DNS question
 * @param type : ENUM of type IPv_4 or IPv_6
 */
static inline void fillType ( dnsQuestionP ques, qnameTypeE type )
{
    ques->qtype = htons(type);
}


/**
 * @brief Main function for DNS query all sub function are called here
 * 
 * @param query : Pointer to DNS query
 * @param   arg : Original Query given by user
 * @param   len : Length of query
 */
static void fillDns(dnsQueryP query, char* arg, int len, qnameTypeE choice )
{
    fillHeader(&query->hdr);
    fillQuery(query, arg, len);
    fillType(&query->ques, choice);
}


/**
 * @brief Function to create IPv4 socket FD of type UDP 
 * 
 * @returns valid socket FD on success else FAILURE 
 */
static int createSoc()
{
    int socketFd = 0;
    /** Creating UDP socket */
    if ( ( socketFd = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        printf ("Socket creation failed\n");
        return FAILURE;
    }
    return socketFd;
}


/**
 * @brief Function to fill sender related socket data
 *
 * @param serverAddr1 : To store sender info such as Family, port and IP
 */
static inline void fillOutGoingSocInfo(struct sockaddr_in* serverAddr1)
{
     /** Setting up first server address */
    serverAddr1->sin_family = AF_INET;
    serverAddr1->sin_port = htons (DNS_PORT);
    serverAddr1->sin_addr.s_addr = inet_addr (DNS_SERVER_IP);
}


/**
 * @brief A wrapper for memory copy
 * 
 * @param dest : Address of destination memory
 * @param  src : Address of source memory
 * @param  len : Length of meomory to be copied
 */
static inline void memcpyWrapper( void* dest, void* src, int len )
{
    memcpy(dest, src, len);
}


/**
 * @brief To serialize the data to be send on socket
 * 
 * @param  buff : Master buffer where entire out query will be copied 
 * @param query : Pointer of DNS query master structure
 * @param   len : Length of DNS query
 */
static void serializeData(char* buff, dnsQueryP query, int len)
{
    int offset = 0; // To calculate offset for serilization of data

    // Copy the header 
    memcpyWrapper(buff, &query->hdr, sizeof(dnsHeaderT));
    offset += sizeof(dnsHeaderT);

    // Copy the query
    memcpyWrapper(buff+offset, query->ques.qname, len+END_CHAR);
    offset += len + END_CHAR;
    
    // Copy the query class
    memcpyWrapper(buff+offset,&query->ques.qtype, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    //Copy the qyery type
    memcpyWrapper(buff+offset,&query->ques.qclass, sizeof(uint16_t));
    offset += sizeof(uint16_t);
}


/**
 * @brief Function to send query to DNS server
 * 
 * @param query : Function to send DNS query
 * @param   len : Length of DNS query
 * 
 * @returns socket FD if query is send to DNS server else FAILURE
 */
static int sendQuery(dnsQueryP query, int len)
{
    struct sockaddr_in serverAddr1;
    int socketFd = 0;
    int sendLen = 0;
    int masterQueryLen = sizeof(dnsHeaderT) + len + END_CHAR + 2*sizeof(uint16_t);

    socketFd = createSoc();

    if ( FAILURE == socketFd )
    {
        printf("Socket creation failed\n");
        return  FAILURE;
    }

    fillOutGoingSocInfo(&serverAddr1);

    char *buff = (char *) calloc(masterQueryLen, sizeof(char));
    if ( NULL == buff )
    {
        printf("Memory allocation failed\n");
        close (socketFd);
        return FAILURE;
    }

    serializeData(buff, query, len);

    sendLen = sendto ( socketFd, ( const void * )buff, masterQueryLen, 0, 
                       (const struct sockaddr *)&serverAddr1, 
                        sizeof(serverAddr1));
    if ( sendLen < 0 )
    {
        close (socketFd);
        free(buff);
        return FAILURE;
    }
    printf("Socket FD is %d\n", socketFd);
    return socketFd;
}

void printHdr(dnsHeaderT hdr)
{
    printf("Transaction Id: %d\n",ntohs(hdr.id));
    printf("Flag is: %d\n",ntohs(hdr.flag));
    printf("No of Question: %d\n",ntohs(hdr.qdCount));
    printf("No of Answers: %d\n",ntohs(hdr.anCount));
    printf("No of Authority Answers: %d\n",ntohs(hdr.nsCount));
    printf("No of Additional Answers: %d\n",ntohs(hdr.arCount));
}

void printQuery(char* qname, int len)
{
    printf("Query was: ");
    for ( int i=1; i<len-1; i++ )
    {
        if (qname[i] >= 0 && qname[i] <= 9) printf(".");
        else printf("%c",qname[i]);
    }
    printf("\n");
}

void deSeralize(char* dnsResponse, int len)
{
    int offset =0;
    dnsQueryP dnsQuery = ( dnsQueryP ) calloc( 1, sizeof(dnsQueryT));
    if ( NULL == dnsQuery )
    {
        printf("Memory allocation failed");
        return;
    }

    memcpyWrapper(&dnsQuery->hdr, dnsResponse, sizeof(dnsHeaderT));
    offset += sizeof(dnsHeaderT);

    dnsQuery->ques.qname = (char *) calloc(len, sizeof(char));
    if ( NULL == dnsQuery->ques.qname )
    {
        printf("Memory allocation failed");
        return;
    }
    memcpyWrapper(dnsQuery->ques.qname, dnsResponse+offset, len);
    offset +=len;

    memcpyWrapper(&dnsQuery->ques.qtype, dnsResponse+offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    memcpyWrapper(&dnsQuery->ques.qclass, dnsResponse+offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);

    printHdr(dnsQuery->hdr);
    printQuery(dnsQuery->ques.qname, len);
    
}

int acceptResponse( int socketFd, char* dnsResponse, int len )
{
    struct sockaddr_in clientAddr;
    int recvLen = 0;
    socklen_t clientAddrLen;
    int queryLen = 0;

    recvLen = recvfrom ( socketFd, (void *)dnsResponse, MAX_BUF, 0, 
                         ( struct sockaddr *)&clientAddr, &clientAddrLen );
    if ( recvLen < 0 )
    {
        printf ("Receive failed %d\n", errno);
        close ( socketFd );
        return FAILURE;
    }

    queryLen = len + END_CHAR;

    deSeralize(dnsResponse, queryLen);

    return SUCCESS;
}

/**
 * @brief Main driver function
 * 
 * @param argc : Gives the arument count
 * @param argv : Gives the argument vector
 *
 * @returns SUCCESS in case of successfull able to send query and process response
 *          else FAILURE
 */
int main( int argc, char* argv[] )
{
    dnsQueryP query = NULL;
    struct sockaddr_in serverAddr1;
    int socketFd, sendLen;
    int len = 0;
    qnameTypeE type = 0;
    int parser = 1;
    char* dnsResponse = NULL;

    len = strlen(argv[parser]); // Get len of argument

    if ( argc < 2 )
    {
        printf("Invalid query type -h for more info\n");
        return FAILURE;
    }

    switch(*(argv[parser]+1))
    {
        case 'q': len = strlen(argv[parser+2]);
                  break;
        case 'h': 
        default: printf("Syntax is ./dns -q -t(-4 for IPv4, -6 for IPv6) <url>\n");
                 printf("Example of valid quert is ./dns -q -4 www.google.com\n");
                 return FAILURE;
    };

    parser++;

    switch (*(argv[parser]+1)) 
    {
        case '4' : type = IPV_4;
                   break;
        case '6' : type = IPV_6;
                   break;
        default: printf("Invalid syntax, use -h to get help\n");
                 return FAILURE;
    }

    parser++;

    query = ( dnsQueryP ) calloc( 1, sizeof(dnsQueryT));
    if ( NULL == query )
    {
        printf("Memory allocation failed\n");
        return FAILURE;
    }
    dnsResponse = ( char * ) calloc ( MAX_BUF, sizeof (char) );
    if ( NULL == dnsResponse )
    {
        printf ("Memory allocation failed\n");
        close (socketFd);
        free(query->ques.qname);
        free(query);
        return FAILURE;
    }

    fillDns(query, argv[parser], len, type);
    
    socketFd = sendQuery(query, len);

    if ( FAILURE == socketFd )
    {
        printf ("Sending failed\n");
        free(query->ques.qname);
        free(query);
        return  FAILURE;
    }

    free(query->ques.qname);
    free(query);

    if ( FAILURE == acceptResponse( socketFd, dnsResponse, len ) )

    return  SUCCESS;
}

// int main ()
// {
//     char *dnsResponse;
//     int clientSocket, sendLen, recvLen;
//     socklen_t clientAddrLen;
//     struct sockaddr_in clientAddr, serverAddr1;
//     char dnsHdr[12], usrReq[100];
//     int nameSize, dc, lenSz;
//     char dn1[200] = {0};
//     int currIdx =12, dnLen = 0;
//     uint16_t noAns, ipLen;

//     memset (dnsHdr, 0, 12);
//     /** creating static DNS header */
//     setDnsHdr(dnsHdr);

//     /** Accepting DNS name from user */
//     printf ("Enter domain Name: ");
//     scanf ("%s", usrReq);

//     nameSize = strlen (usrReq);
//     char domainName[nameSize+1];
//     strncpy (domainName, usrReq, nameSize+1);

//     /** Counting number of . for forming DNS query packet */
//     dc = countDots ( domainName, nameSize );
//     lenSz = dc+1;
//     int lenCnt[lenSz];

//     for ( int i=0; i<dc+1; i++ )
//     {
//         lenCnt[i] = 0;
//     }

//     charCount (domainName, lenCnt, nameSize);

//     /** Copying DNS header */
//     stringCpy ( dn1, dnsHdr, 12, 0 );
    
//     /** Copting Domain name query */
//     for ( int i=0; i< lenSz; i++ )
//     {   
//         dn1[currIdx++] = lenCnt[i];
//         dnsCpy ( dn1, domainName, lenCnt[i], currIdx, dnLen );
//         currIdx += lenCnt[i];
//         dnLen += lenCnt[i]+1;
//     }

//     /** Setting DNS class and query */
//     currIdx = setDnsClassAndValue ( dn1, currIdx );

//     /** Creating UDP socket */
//     if ( ( clientSocket = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
//     {
//         printf ("Socket creation failed\n");
//         return -1;
//     }

//     /** Setting up first server address */
//     serverAddr1.sin_family = AF_INET;
//     serverAddr1.sin_port = htons (DNS_PORT);
//     serverAddr1.sin_addr.s_addr = inet_addr (DNS_SERVER_IP);

//     printf ("\nDNS Server: %s\n", DNS_SERVER_IP );
//     printf ("DNS query: %s\n",usrReq);
//     printf ("\nSending query to server...\n");

//     sendLen = sendto ( clientSocket, ( const void * )dn1, currIdx, 0, 
//                        (const struct sockaddr *)&serverAddr1, 
//                         sizeof(serverAddr1));
//     if ( sendLen < 0 )
//     {
//         printf ("Sending failed %d\n", errno);
//         close ( clientSocket );
//         return -1;
//     }
//     printf ("Query sent successfully...\n");

//     printf ("Waiting for response from server...\n\n");
//     dnsResponse = ( char * ) calloc ( MAX_BUF, sizeof (char) );
//     if ( NULL == dnsResponse )
//     {
//         printf ("Memory allocation failed\n");
//         close (clientSocket);
//         return -1;
//     }
//     recvLen = recvfrom ( clientSocket, (void *)dnsResponse, MAX_BUF, 0, 
//                          ( struct sockaddr *)&clientAddr, &clientAddrLen );
//     if ( recvLen < 0 )
//     {
//         printf ("Receive failed\n");
//         close ( clientSocket );
//         return -1;
//     }

//     /** Counting number of answer responded by server */
//     noAns= dnsResponse[6] << 8 | dnsResponse[7];

//     int i = 12+nameSize+4+12; /** seting index to ignore DNS header */
//     for (; i< recvLen;i++ )
//     {
//         ipLen = dnsResponse[i]<<8 | dnsResponse[i+1];
//         /** Check if response if IP Address or not by evulating length field */
//         i+=2; /** skipping length field */
//         if ( ipLen == 4 )
//         {
//             printf ("Name: %s\n",usrReq); 
//             printf ("IP address: %d.%d.%d.%d\n\n", dnsResponse[i]&0x000000FF, 
//                                      dnsResponse[i+1]&0x000000FF, 
//                                      dnsResponse[i+2]&0x000000FF, 
//                                      dnsResponse[i+3]&0x000000FF);
//         }
//         i+=ipLen-1; /** move index to next length filed */
//         i+= 10; /** Skipping index to next ip if any*/
//         noAns--;
//         if ( noAns == 0 ) break;
//     }

//     free (dnsResponse);
//     close (clientSocket);

//     return 0;
// }
