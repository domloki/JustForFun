#include "dnsClient.h"

/**
 * @brief Function to count number of dots in dns request
 * 
 * @param   dn : DNS query
 * @param size : Size of DNS query
 * @returns on success number of dots in query 0 otherwise
 */
int countDots ( char *dn, int size )
{
    int count = 0;
    for ( int i=0; i<size;i++ )
    {
        if ( dn[i] == '.' ) count++;
    }
    return count;
}


/** 
 * @brief Function to hardcode DNS header
 * 
 * @param dnsHdr : Pointe to DNS header
 */
void setDnsHdr ( char *dnsHdr )
{
    dnsHdr[0] = 10;
    dnsHdr[1] = 11;
    dnsHdr[2] = 01;
    dnsHdr[5] = 01;
}


/** 
 * @brief To count the character in domain name exclusing dots
 * 
 * @param domainName : Domain Name
 * @param     lenCnf : Pointer to length of domain name
 * @param      stnSz : statement size
 */
void charCount ( char *domainName, int *lenCnt, int stnSz )
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
void stringCpy ( char *dest, char *src, int size, int offset )
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
void dnsCpy ( char *dest, char *src, int size, int offset1, int offset2 )
{
    for ( int i=0; i< size; i++ )
    {
        dest[i+offset1] = src[i+offset2];
    }
}

int setDnsClassAndValue ( char *dn1, int currIdx )
{
    dn1[currIdx++] = 00;
    dn1[currIdx++] = 00;
    dn1[currIdx++] = 01;
    dn1[currIdx++] = 00;
    dn1[currIdx++] = 01;
    return currIdx;
}

int main ()
{
    char *dnsResponse;
    int clientSocket, sendLen, recvLen;
    socklen_t clientAddrLen;
    struct sockaddr_in clientAddr, serverAddr1;
    char dnsHdr[12], usrReq[100];
    int nameSize, dc, lenSz;
    char dn1[200] = {0};
    int currIdx =12, dnLen = 0;
    uint16_t noAns, ipLen;

    memset (dnsHdr, 0, 12);
    /** creating static DNS header */
    setDnsHdr(dnsHdr);

    /** Accepting DNS name from user */
    printf ("Enter domain Name: ");
    scanf ("%s", usrReq);

    nameSize = strlen (usrReq);
    char domainName[nameSize+1];
    strncpy (domainName, usrReq, nameSize+1);

    /** Counting number of . for forming DNS query packet */
    dc = countDots ( domainName, nameSize );
    lenSz = dc+1;
    int lenCnt[lenSz];

    for ( int i=0; i<dc+1; i++ )
    {
        lenCnt[i] = 0;
    }

    charCount (domainName, lenCnt, nameSize);

    /** Copying DNS header */
    stringCpy ( dn1, dnsHdr, 12, 0 );
    
    /** Copting Domain name query */
    for ( int i=0; i< lenSz; i++ )
    {   
        dn1[currIdx++] = lenCnt[i];
        dnsCpy ( dn1, domainName, lenCnt[i], currIdx, dnLen );
        currIdx += lenCnt[i];
        dnLen += lenCnt[i]+1;
    }

    /** Setting DNS class and query */
    currIdx = setDnsClassAndValue ( dn1, currIdx );

    /** Creating UDP socket */
    if ( ( clientSocket = socket ( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 )
    {
        printf ("Socket creation failed\n");
        return -1;
    }

    /** Setting up first server address */
    serverAddr1.sin_family = AF_INET;
    serverAddr1.sin_port = htons (DNS_PORT);
    serverAddr1.sin_addr.s_addr = inet_addr (DNS_SERVER_IP);

    printf ("\nDNS Server: %s\n", DNS_SERVER_IP );
    printf ("DNS query: %s\n",usrReq);
    printf ("\nSending query to server...\n");

    sendLen = sendto ( clientSocket, ( const void * )dn1, currIdx, 0, 
                       (const struct sockaddr *)&serverAddr1, 
                        sizeof(serverAddr1));
    if ( sendLen < 0 )
    {
        printf ("Sending failed %d\n", errno);
        close ( clientSocket );
        return -1;
    }
    printf ("Query sent successfully...\n");

    printf ("Waiting for response from server...\n\n");
    dnsResponse = ( char * ) calloc ( MAX_BUF, sizeof (char) );
    if ( NULL == dnsResponse )
    {
        printf ("Memory allocation failed\n");
        close (clientSocket);
        return -1;
    }
    recvLen = recvfrom ( clientSocket, (void *)dnsResponse, MAX_BUF, 0, 
                         ( struct sockaddr *)&clientAddr, &clientAddrLen );
    if ( recvLen < 0 )
    {
        printf ("Receive failed\n");
        close ( clientSocket );
        return -1;
    }

    /** Counting number of answer responded by server */
    noAns= dnsResponse[6] << 8 | dnsResponse[7];

    int i = 12+nameSize+4+12; /** seting index to ignore DNS header */
    for (; i< recvLen;i++ )
    {
        ipLen = dnsResponse[i]<<8 | dnsResponse[i+1];
        /** Check if response if IP Address or not by evulating length field */
        i+=2; /** skipping length field */
        if ( ipLen == 4 )
        {
            printf ("Name: %s\n",usrReq); 
            printf ("IP address: %d.%d.%d.%d\n\n", dnsResponse[i]&0x000000FF, 
                                     dnsResponse[i+1]&0x000000FF, 
                                     dnsResponse[i+2]&0x000000FF, 
                                     dnsResponse[i+3]&0x000000FF);
        }
        i+=ipLen-1; /** move index to next length filed */
        i+= 10; /** Skipping index to next ip if any*/
        noAns--;
        if ( noAns == 0 ) break;
    }

    free (dnsResponse);
    close (clientSocket);

    return 0;
}
