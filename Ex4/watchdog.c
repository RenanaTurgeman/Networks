#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <stdio.h>

#define SERVER_PORT 3001  // The port that the server listens


int main(int argc, char* args[])
{
    // Open the listening (server) socket
    int listeningSocket = -1;
    //socket()  creates an endpoint for communication and returns a file descriptor that refers to that endpoint. 
    //On success, a file descriptor for the new socket is returned.else -1.
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1) { // if -1 returne -error
        printf("Could not create listening socket : %d", errno);
        return 1;
    }
    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    //
    int enableReuse = 1;
    // getsockopt() manipulate options for the socket referred to by the file descriptor listeningSocket.
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0) { //on error -1 returned. (on success 0 returned)
        printf("setsockopt() failed with error code : %d", errno);
        return 1;
    }
    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY; // any IP at this port (Address to accept any incoming messages)
    serverAddress.sin_port = htons(SERVER_PORT);  // network order (makes byte order consistent)

    // Bind the socket to the port with any IP at this port
    //after socket is cearted,  bind() assigns the address specified by &serverAddress to the socket referred to by the  file  descriptor
    //  listeningSocket
    int bindResult = bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindResult == -1) { //On error, -1 is returned ( On success -0)
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1) { //On error, -1 is returned ( On success -0)
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    // Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    struct sockaddr_in clientAddress;  //
    socklen_t clientAddressLen = sizeof(clientAddress);
    //int totalRecive=2;
    memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1) {
            printf("listen failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            return -1;
        }

    int timeToWait = 10;
    char message [] = {0,0,0};


    // to measure 10 secenod to know if out ("killed")
    int res=0;
    int total_timer=0;
    
    struct timeval current_time1, end_time;
    gettimeofday(&current_time1, NULL);

    while (total_timer < 10){
        // recev the packet and zero the time 
        int rec= recv(clientSocket, &res, 4, MSG_DONTWAIT);
        if(rec > 0){
            gettimeofday(&current_time1, NULL);
            continue;
        }
        else{
        gettimeofday(&end_time, NULL);
        total_timer= end_time.tv_sec- current_time1.tv_sec;
        }   
    }

    
    close(clientSocket);
    close(listeningSocket);
    printf("server %s cannot be reached.\n",args[1]);
    kill(getppid(), 9);
    
    return 0;
}