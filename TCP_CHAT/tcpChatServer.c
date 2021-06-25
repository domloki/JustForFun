#include "tcpChatServer.h"

int sockFd;

void exitHandler()
{
    printf ( "Server exiting\n" );
    close (sockFd);
    exit(0);
}

void serverSend ( clientSockFdP clientConn, userDefHdrP usrMsg )
{
    int servMsgSendFlag = 0;
    int msgSize = sizeof ( userDefHdrT );
    char noClientMsg[] = "No other client to chat with";
    for ( int i=0; i<MAX_CLIENTS; i++ )
    {
        int sendLen = 0;
        if ( clientConn->clientSock[i] != -1 && 
             clientConn->clientSock[i] != htonl (usrMsg->srcClientId ) )
        {
            /** sending normal broadcast messages */
            sendLen = send ( clientConn->clientSock[i], 
                             ( const void * )usrMsg, 
                              msgSize, MSG_DONTWAIT );
            if ( sendLen < 0 )
            {
                continue;
            }
        }
        else if ( clientConn->actvConn == 1 && servMsgSendFlag == 0 )
        {
            int sendLen = 0;
            free ( usrMsg );
            usrMsg = ( userDefHdrP ) calloc ( 1, sizeof (userDefHdrT) );
            if ( NULL == usrMsg )
            {
                printf ("Memory allocation failed\n");
                exit(0);
            }
            memcpy ( usrMsg->msg, noClientMsg, 30 );
            usrMsg->srcClientId = htonl ( clientConn->clientSock[i] );
            usrMsg->msgId = htonl ( NO_CLIENT_TO_CHAT );
            /** sending unicast message in case client is the only left in chat room */
            sendLen = send ( clientConn->clientSock[i],
                            (void *)usrMsg,
                            sizeof (userDefHdrT), 0 );
            if ( sendLen < 0 )
            {
                printf ("Send failed\n");
            }
            servMsgSendFlag = 1;
        }
    }
}


void *serverRecv ( void * clientConn1 )
{
    clientSockFdP clientConn = ( clientSockFdP ) clientConn1;
    userDefHdrP usrMsg = NULL;
    int recvLen = 0;
    int i=0;
    int usrMsgLen = sizeof ( userDefHdrT );
    while (1)
    {
        i=0;
        for (; i<MAX_CLIENTS; i++)
        {
            if ( clientConn->clientSock[i] != -1 )
            {
                usrMsg = ( userDefHdrP ) calloc ( 1, sizeof (userDefHdrT) );
                if ( NULL == usrMsg )
                {
                    printf ("Memory allocation failed\n");
                    exit(0);
                }

                recvLen = recv ( clientConn->clientSock[i], 
                                 (void *)usrMsg, usrMsgLen , 
                                  MSG_DONTWAIT );
                if ( recvLen < 0 )
                {
                    if ( errno == EAGAIN )
                    {
                        free ( usrMsg );
                        usrMsg = NULL;
                        continue;
                    }
                    printf ("Recv failed %d\n", errno);
                    free ( usrMsg );
                    usrMsg = NULL;
                    exit(0);
                }
                if ( ntohl (usrMsg->msgId) == CLIENT_BYE_TO_SERVER )
                {
                    if ( clientConn->actvConn != 1 )
                    {
                        free ( usrMsg );
                        usrMsg = ( userDefHdrP ) calloc ( 1, sizeof ( userDefHdrT ) );
                        if ( NULL == usrMsg )
                        {
                            printf ("Memory allocation failed\n");
                            close (clientConn->serverSockFd);
                            exit(0);
                        }
                        usrMsg->msgId = htonl ( CLIENT_BYE_TO_ALL );
                        usrMsg->srcClientId = htonl ( clientConn->clientSock[i] );
                        serverSend ( clientConn, usrMsg );
                    }
                    printf ("Client %d left chat server\n", clientConn->clientSock[i]);
                    close ( clientConn->clientSock[i] );
                    clientConn->clientSock[i] = -1;
                    clientConn->actvConn--;
                    printf ("Number of connected client are %d\n", 
                                                    clientConn->actvConn);
                }
                else if ( ntohl (usrMsg->msgId) == CLIENT_CHAT_TO_SERVER )
                {
                    serverSend ( clientConn, usrMsg );
                }
                else
                {
                    printf ("Invalid message from client\n");
                    free ( usrMsg );
                    break;
                }
                free ( usrMsg );
            }
        }
    }
}


/**
 * @brief Function to accept client connection
 * 
 * @param clientConn1 : Poiter to client connection
 */
void *acceptConn ( void* clientConn1 )
{
    clientSockFdP clientConn = ( clientSockFdP )clientConn1;
    int i;
    int sendLen;
    userDefHdrP firstMsg;
    for ( i=0; i < MAX_CLIENTS; i++ )
    {
        clientConn->clientSock[i] = -1;
    }
    clientConn->actvConn = i = 0;
    do
    {
        clientConn->clientSock[i] = 
            accept ( clientConn->serverSockFd,
            ( struct sockaddr * )&clientConn->clientAddr[i],
            &clientConn->clientAddrLen[i] );
        if ( clientConn->clientSock[i] < 0 )
        {
            printf ("Accept failed\n");
            return NULL;
        }
        printf ("Client %d added to list of active clients\n", clientConn->clientSock[i]);
        firstMsg = ( userDefHdrP ) calloc ( 1, sizeof (userDefHdrT) );
        if ( NULL == firstMsg )
        {
            printf ("Memory Allocation failed\n");
            return NULL;
        }

        firstMsg->msgId = htonl (SERVER_INFO_TO_CLIENT);
        firstMsg->srcClientId = htonl(clientConn->clientSock[i]);

        /** sending unique client ID to client */
        sendLen = send ( clientConn->clientSock[i], (const void *)firstMsg, 
                                        sizeof ( userDefHdrT ), MSG_DONTWAIT );
        if ( sendLen < 0 )
        {
            printf ("Failed to send message to client\n");
            return NULL;
        }
        free ( firstMsg );
        firstMsg = NULL;
        clientConn->actvConn++;
        if ( clientConn->actvConn > 1 )
        {
            firstMsg = ( userDefHdrP ) calloc ( 1, sizeof ( userDefHdrT ) );
            if ( NULL == firstMsg )
            {
                printf ("Memory allocation failed\n");
            }
            firstMsg->msgId = htonl (SERVER_NEW_CLIENT_JOINED);
            firstMsg->srcClientId = htonl ( clientConn->clientSock[i] );
            serverSend ( clientConn, firstMsg );
        }
        free ( firstMsg );
        firstMsg = NULL;
        i++;
        printf ( "Number of connected clients are %d\n", 
                                    clientConn->actvConn );
    } while ( i <= MAX_CLIENTS );
    if ( MAX_CLIENTS == i )
    {
        printf("max connection limit reached at server\n");
        return NULL;
    }
}


/**
 * @brief Main/Driver function 
 */
int main ( )
{
    int serverSocket;
    struct sockaddr_in serverAddr;
    clientSockFdP clientConn = NULL;
    pthread_t acceptThread, sendThread, recvThread;

    /** Creating socket for server */ 
    if ( ( serverSocket = socket ( AF_INET, SOCK_STREAM, IPPROTO_TCP ) ) < 0 )
    {
        printf ("server socket creation failed\n");
        return -1;
    }
    printf ("Server socket establishment successfull\n");

    /** Setting sever address */
    memset ( &serverAddr, 0, sizeof ( struct sockaddr_in ) );
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = SERVER_TCP_PORT;
    serverAddr.sin_addr.s_addr = inet_addr ( "127.0.0.1" );
    
    /** Binding the socket */
    if ( ( bind ( serverSocket, ( const struct sockaddr *)&serverAddr, 
                                    sizeof ( struct sockaddr_in ) ) ) < 0 )
    {
        printf ("Bind failed %d\n", errno);
        close (serverSocket);
        return -1;
    }
    printf ("Bind successfull for server\n");

    /** Open for listing */
    if ( ( listen ( serverSocket, MAX_CLIENTS ) ) < 0 )
    {
        printf ("Listen failed\n");
        close ( serverSocket );
        return -1;
    }
    printf ("Server in listing mode ready to accept clients\n");

    clientConn = ( clientSockFdP ) calloc ( 1, sizeof (clientSockFdT) );
    if ( NULL == clientConn )
    {
        printf ("Memory allocation failed\n");
        close ( serverSocket );
        return -1;
    }
    clientConn->serverSockFd = serverSocket;

    sockFd = serverSocket;
    signal (SIGINT, exitHandler);
    if ( ( pthread_create ( &acceptThread, NULL, acceptConn, clientConn ) ) != 0 )
    {
        printf ("Thread creation failed\n");
        close ( serverSocket );
        return -1;
    }

    if ( ( pthread_create ( &sendThread, NULL, serverRecv, clientConn ) ) != 0 )
    {
        printf ("Thread creation failed\n");
        close ( serverSocket );
        return -1;
    }
    pthread_join (acceptThread, NULL);

    return 0;
}

/** EOF */