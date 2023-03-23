/*
    TCP/IP-server
*/

#include <stdio.h>

// Linux and other UNIXes
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


#define SERVER_PORT 9999  // The port that the server listens
#define BUFFER_SIZE 1024
#define SIZE 1262570  

int main() {
    char buffer1[SIZE/2]={'\0'};
    char buffer2[SIZE/2]={'\0'};
    char buffCC[16]={0};
    static int size = 0;
    struct timeval start_t, end_t;
    double timeAlgoC , timeAlgoR;
    double cubic[1000]={0};
    double reno[1000]={0};

    // signal(SIGPIPE, SIG_IGN);  // on linux to prevent crash on closing socket

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
    int totalRecive=2;
    memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1) {
            printf("listen failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            return -1;
        }
    while (1) {
        // Receive a message from client 
        int bytesReceived  =0;
        int totHalf1 =0;
        strcpy(buffCC, "cubic"); 
        int len = strlen(buffCC);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buffCC, len) != 0) {
        perror("setsockopt"); 
        return -1;
        }
        len = sizeof(buffCC); 
        if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buffCC, &len) != 0) {
            perror("getsockopt"); 
            return -1; 
        } 
        gettimeofday(&start_t,NULL);//get the time
	    while(bytesReceived <SIZE/2 ) {
             bytesReceived += recv(clientSocket, buffer1, 1024,0);
	        //totHalf1 += bytesReceived;
	    }
        gettimeofday(&end_t,NULL);
        
        if (bytesReceived == -1) {
            printf("recv failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            close(clientSocket);
            return -1;
        } else {
            printf("received %d bytes from clinet: \n", bytesReceived);
        }
    
        timeAlgoC = 0;
        timeAlgoC = (end_t.tv_sec -start_t.tv_sec)*1000;
        timeAlgoC+= (end_t.tv_usec-start_t.tv_usec)/1000.0;
        printf("time of recv cubic %f \n", timeAlgoC);
        
        cubic[size]=timeAlgoC;
        timeAlgoC = 0;
        //authentication
        long xor_ans=11101000100111; /// =1101001111000xor10000001011111=6776xor8287
        int bytesSent;
        bytesSent= send(clientSocket,  &xor_ans, sizeof(xor_ans), 0); //send the authentication to the client
        //printf("%d == bytesSent\n",bytesSent);
        int messageLen = sizeof(long);
        if (bytesSent == -1) { //error
            printf("send() failed with error code : %d", errno);
            close(listeningSocket);
            close(clientSocket);
            return -1;
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //end authentication
        strcpy(buffCC, "reno"); 
        len = strlen(buffCC);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buffCC, len) != 0) {
        perror("setsockopt"); 
        return -1;
        }
        len = sizeof(buffCC); 
        if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buffCC, &len) != 0) {
            perror("getsockopt"); 
            return -1; 
        } 
        //second recv
        gettimeofday(&start_t,NULL);
        int totHalf2 =0;
	    while(totHalf2 <SIZE/2 ) {
            bytesReceived = recv(clientSocket, buffer2, 1024,0);
	        totHalf2 += bytesReceived;
	    }
        gettimeofday(&end_t,NULL);
        if (bytesReceived == -1) {
            printf("recv failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            close(clientSocket);
            return -1;
        } else {
        printf("received %d bytes from clinet: \n", totHalf2);
        }
        timeAlgoR = 0;
        timeAlgoR = (end_t.tv_sec -start_t.tv_sec)*1000;
        timeAlgoR+= (end_t.tv_usec-start_t.tv_usec)/1000.0;
        printf("time of recv reno %f \n", timeAlgoR);
        reno[size]=timeAlgoR;
        timeAlgoR = 0;
        
        //authentication
        //xor_ans=11101000100111; /// =1101001111000xor10000001011111=6776xor8287
        bytesSent= send(clientSocket,  &xor_ans, sizeof(xor_ans), 0);
        messageLen = sizeof(long);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
            close(listeningSocket);
            close(clientSocket);
            return -1;
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //end authentication
        //get recv if to continue or exit 
        char buffer[2];
        memset(buffer, 0, 1);
        bytesReceived = recv(clientSocket, buffer, 1024, 0);
        
        if (bytesReceived == -1) {
            printf("recv failed with error code : %d", errno);
            // close the sockets
            close(listeningSocket);
            close(clientSocket);
            return -1;
        }
        printf("received %d bytes from clinet:\n ", bytesReceived);
        //printf(" %s", buffer);
        if(buffer[0]=='H'){
            printf("bye************************************bye\n");
            close(listeningSocket);
            break;
        }
        //send to sender ack
        bytesSent= send(clientSocket,  &xor_ans, sizeof(xor_ans), 0);
        messageLen = sizeof(long);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
            close(listeningSocket);
            close(clientSocket);
            return -1;
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //end authentication
        printf("\n");
        size++;
        
    }
    
    //Calculate the average time
    double avergeC=0;
    double avergeR=0;
    for(int i = 0; i<=size;i++){
        printf("time of algo cubic %d = %f\n" ,i+1 ,cubic[i]);
        avergeC+=cubic[i];
        printf("time of algo reno  %d = %f\n" ,i+1 ,reno[i]);
        avergeR+=reno[i];
    }
    avergeC=avergeC/(size+1);
    avergeR=avergeR/(size+1);
    printf("\n");
    printf("*********************  averge  ***************\n");
    printf("averge time for cubic is %f, and for reno %f for %d run\n",avergeC,avergeR,size+1);
    

    
    
    close(listeningSocket);

    return 0;
}
