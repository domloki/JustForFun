#ifndef __TCP_CHAT_SERVER_H__
#define __TCP_CHAT_SERVER_H__

#include <stdio.h>
#include <unistd.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define MAX_CLIENTS 1024
#define SERVER_TCP_PORT 6000

/** predefined macros for communication handling */
#define SERVER_INFO_TO_CLIENT 0x00000001
#define CLIENT_CHAT_TO_SERVER 0x00000002
#define CLIENT_BYE_TO_SERVER 0x00000003
#define NO_CLIENT_TO_CHAT 0x00000004
#define SERVER_NEW_CLIENT_JOINED 0x00000005
#define CLIENT_BYE_TO_ALL 0x00000006


/**
 * @brief User defined structure to send and receive message
 */
typedef struct userDefHdr
{
    int msgId;          /**< Message ID */
    int srcClientId;    /**< Source client ID */
    char msg[1400];     /**< message to send */
} userDefHdrT,
 *userDefHdrP;


/**
 * @brief Structure to store socket related info
 */
typedef struct clientSockFd
{
    int serverSockFd;               /**< server socket ID */
    int clientSock[MAX_CLIENTS];    /**< Client socket ID */
    int actvConn;                   /**< Number of active connection */
    int clientAddrLen[MAX_CLIENTS]; /**< Client address length */
    struct sockaddr_in clientAddr[MAX_CLIENTS]; /**< To store client socket FD */
} clientSockFdT,
 *clientSockFdP;


#endif /**  __TCP_CHAT_SERVER_H__ */

/** EOF */