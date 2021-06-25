#include "tcpChatClient.h"

int clientId = -1;

/**
 * @brief Function to send message to chat server
 * 
 * @param sockParam1 : pointer to socket parametes
 */
void *sendMsgg ( void *SockParam1 )
{
    sockPramP SockParam = ( sockPramP ) SockParam1;
    int sendLen;
    userDefHdrP usrMsg;
    char exitstr[] = "bye";
    int exitCount=0;
    int userMsgLen;
    char *recvBuf;
    while (1)
    {
        usrMsg = ( userDefHdrP ) calloc ( 1, sizeof (userDefHdrT));
        if ( NULL == usrMsg )
        {
            printf ("Mempry allocation failed\n");
            close ( SockParam->clientSockId );
            exit(0);
        }
        /** Accepting input from user */
        scanf("%[^\n]%*c", usrMsg->msg);

        for ( int i=0; i<4; i++ )
        {
            /** To check is client want to exit */
            if ( usrMsg->msg[i] == exitstr[i] ) exitCount++;
            else break;
        }
        if ( exitCount == 4 )
        {
            usrMsg->msgId = htonl (CLIENT_BYE_TO_SERVER);
            userMsgLen = sizeof ( userDefHdrT );
        }
        else
        {
            usrMsg->msgId = htonl (CLIENT_CHAT_TO_SERVER);
            userMsgLen = sizeof ( userDefHdrT );
        }

        usrMsg->srcClientId = htonl(clientId);

        /** sending message to chat server */
        sendLen = send ( SockParam->clientSockId, (const void *)usrMsg, 
                                            userMsgLen, MSG_DONTWAIT );
        if ( sendLen < 0 )
        {
            printf ("Send failed\n");
        }

        free ( recvBuf );
        free ( usrMsg );
        recvBuf = NULL;
        usrMsg = NULL;
        if ( exitCount == 4 )
        {
            close ( SockParam->clientSockId );
            exit(0);
        }
        exitCount =0;
    }
}


/**
 * @brief Function to receive message from clients via server
 * 
 * @param sockParam : Parameter to socket
 */
void *recvMsgg ( void *sockParam1 )
{
    sockPramP sockParam = ( sockPramP )sockParam1;
    int recvLen;
    int addrSize = sizeof ( userDefHdrT );
    userDefHdrP  usrMsg;
    while (1)
    {
        usrMsg = ( userDefHdrP ) calloc ( 1, sizeof (userDefHdrT) );
        if ( NULL == usrMsg )
        {
            printf ("Memory allocation failed\n");
            close ( sockParam->clientSockId );
            return NULL;
        }

        /** receive message from server */
        recvLen = recv ( sockParam->clientSockId, (void *)usrMsg, addrSize, 0 );
        if ( recvLen < 0 )
        {
            if ( EINVAL == recvLen )
            {
                free ( usrMsg );
                usrMsg = NULL;
                continue;
            }
            printf ("Error in receive function %d\n", errno);
            close ( sockParam->clientSockId );
            return NULL;
        }

        if ( ntohl ( usrMsg->msgId ) == SERVER_INFO_TO_CLIENT )
        {
            clientId = htonl ( usrMsg->srcClientId );
            printf ("Your Unique Id is %d enter bye to exit chat anytime\n", clientId);
        }
        else if ( ntohl ( usrMsg->msgId ) == CLIENT_CHAT_TO_SERVER )
        {
            printf ("Message from User %d ", htonl (usrMsg->srcClientId) );
            printf ( ": %s\n", (char *)usrMsg->msg );
        }
        else if  ( ntohl ( usrMsg->msgId ) == NO_CLIENT_TO_CHAT )
        {
             printf ("Message from server: %s\n", (char *)usrMsg->msg);
        }
        else if ( ntohl (usrMsg->msgId) == SERVER_NEW_CLIENT_JOINED )
        {
            printf ("Client %d joined the chat server\n", ntohl ( usrMsg->srcClientId ));
        }
        else if ( ntohl (usrMsg->msgId) == CLIENT_BYE_TO_ALL )
        {
            printf ( "Client %d left the chat server\n", ntohl(usrMsg->srcClientId) );
        }
        free ( usrMsg );
        usrMsg = NULL;
    }
}


/**
 * @brief Function to Drive Clinet
 */
int main ()
{
    int clientSock;
    struct sockaddr_in servAddr;
    sockPramP sockParam;
    pthread_t sendThr, recvThr;

    /** Creating socket */
    if ( ( clientSock = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        printf ("Socket creation failed\n");
        return -1;
    }
    printf ("Socket Creation successfull for client\n");

    sockParam = ( sockPramP ) calloc ( 1, sizeof ( sockPramT ) );
    if ( NULL == sockParam )
    {
        printf ("Memory allocation failed\n");
        close ( clientSock );
        return -1;
    }

    sockParam->clientSockId = clientSock;
    sockParam->clientAdd = servAddr;

    memset ( &servAddr, 0, sizeof ( struct sockaddr_in ) );
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = 6000 ;
    servAddr.sin_addr.s_addr = inet_addr ( "127.0.0.1" );

    sockParam->clientSockId = clientSock;
    sockParam->clientAdd = servAddr;

    /** Connecting with client */
    if ( ( connect ( clientSock, ( const struct sockaddr * )&servAddr, 
                                  sizeof ( struct sockaddr_in ) ) ) < 0 )
    {
        printf ("Connect failed %d\n", errno);
        close ( clientSock );
        return -1;
    }
    printf ("Successfull connected to server\n");

    /** creating thread to receive incoming messages from other clients */
    if ( ( pthread_create ( &recvThr, NULL, recvMsgg, sockParam ) ) != 0 )
    {
        printf ("Thread creation failed\n");
        close ( clientSock );
        return -1;
    }

    /** Creating thread to send message */
    if ( ( pthread_create ( &sendThr, NULL, sendMsgg, sockParam ) ) != 0 )
    {
        printf ("Thread creation failed\n");
        close ( clientSock );
        return -1;
    }
    pthread_join (recvThr, NULL);

    return 0;
}

/** EOF */
