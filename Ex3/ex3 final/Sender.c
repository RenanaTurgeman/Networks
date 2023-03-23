/*
        TCP/IP client
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h> 

#define SERVER_PORT 9999
#define SERVER_IP_ADDRESS "127.0.0.1"
#define SIZE 1262570  
#define BUFFER_SIZE 1024

int main()  {
    char buffer1[SIZE/2]={'\0'};
    char buffer2[SIZE/2]={'\0'};
    char buffCC[256]={0};
    socklen_t len;
    /*read file to buffer[SIZE]*/
    FILE *fp1; 
	printf("Sending file... %d\n",1);
    //opens the file whose named "yair.txt", the file is opend for reading
	fp1 = fopen("yair.txt", "r");
    if(fp1 == NULL){ //in sucsess fopen return a FILE pointer, if null returned- error
       	perror("Error in reading file.");
        return 2;
    }
    fread(buffer1,sizeof(char),SIZE/2,fp1);
    fread(buffer2,sizeof(char),SIZE/2,fp1);
    fclose(fp1);
    
    int sock=0;
    /*finish to read file and close*/
    //socket() creates an endpoint for communication and returns a file descriptor that refers to that endpoint. 
    //On success, a file descriptor for the new socket is returned.else -1.
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {// if -1 returne -error
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);                                              // (5001 = 0x89 0x13) little endian => (0x13 0x89) network endian (big endian)
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr);  // convert IPv4 and IPv6 addresses from text to binary form
    // e.g. 127.0.0.1 => 0x7f000001 => 01111111.00000000.00000000.00000001 => 2130706433
    if (rval <= 0) {
        printf("inet_pton() failed");
        return -1;
    }

        
    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock);
        return -1;
    }

    printf("connected to server\n");

    // Sends some data to server
    int input=1;
    while(input==1){
        strcpy(buffCC, "cubic"); 
        len = strlen(buffCC);
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffCC, len) != 0) {
            perror("setsockopt"); 
            return -1;
        }
        len = sizeof(buffCC); 
        if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffCC, &len) != 0) {
            perror("getsockopt"); 
            return -1; 
        } 

        printf("***********Algorithm  %s ***********\n" , buffCC);

        //send part one
        int bytesSent = send(sock, buffer1, SIZE/2, 0);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
        } else if (bytesSent == 0) {
           printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < SIZE/2) {
            printf("sent only %d bytes from the required %d.\n", SIZE/2, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //end send 
        //authentication
        long num; 
		// Receive the number from the server
		int bytes_received = recv(sock, &num, sizeof(num), 0);
		if (bytes_received == -1)// if -1 returne -error
		{
			perror("recv");
	    }else{
			printf("num in authentication %ld\n", num);
		}
		long xor_ans=11101000100111; /// =1101001111000xor10000001011111=6776xor8287
		// Check if the 2 numbers are equl
		if (num != xor_ans){
			perror("authentication failed");
		} //else authentication sucsess
        //end authentication
        printf("end authentication");
        
        //second send
        strcpy(buffCC, "reno"); 
        len = strlen(buffCC);
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffCC, len) != 0) {
        perror("setsockopt"); 
        return -1;
        }
        len = sizeof(buffCC); 
        if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buffCC, &len) != 0) {
            perror("getsockopt"); 
            return -1; 
        } 

        printf("***********Algorithm changed to %s ***********\n" , buffCC);

        bytesSent = send(sock, buffer2, SIZE/2, 0);
        if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
        } else if (bytesSent == 0) {
           printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < SIZE/2) {
            printf("sent only %d bytes from the required %d.\n", SIZE/2, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //en to send part 2
        bytes_received = recv(sock, &num, sizeof(num), 0);
		if (bytes_received == -1){
			perror("recv");
		}else{
			printf("num in authentication %ld\n", num);
		}
		xor_ans=11101000100111; /// =1101001111000xor10000001011111=6776xor8287
		// Check if the 2 numbers are equl
		if (num != xor_ans)
		{
			perror("authentication failed");
		}
        //end authentication
        printf("end authentication\n");

        //user decision
        printf("Run again? (1 for yes ,for quit press else number): ");
        scanf(" %d", &input);
        
        //send to sender H or s: s=stay, H=out
            int bytesSen;
            char buffer[1024] = {'\0'};
            char message[] = "H";
            int messageLen = strlen(message) + 1;
            char message1[] = "s";
            int messageLen1 = strlen(message1) + 1;
            //printf("%d %d\n",messageLen1,messageLen);
            if(input==1){
                 bytesSent = send(sock, message1, messageLen1, 0);
            }else{
                 bytesSent = send(sock, message, messageLen, 0);
            }
            if (bytesSent == -1) {
            printf("send() failed with error code : %d", errno);
        } else if (bytesSent == 0) {
            printf("peer has closed the TCP connection prior to send().\n");
        } else if (bytesSent < messageLen) {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        } else {
            printf("message was successfully sent.\n");
        }
        //like to get ack
        bytes_received = recv(sock, &num, sizeof(num), 0);
		if (bytes_received == -1){
			perror("recv");
		}else{
			printf("num in authentication %ld\n", num);
		}

		xor_ans=11101000100111; /// =1101001111000xor10000001011111=6776xor8287
		// Check if the 2 numbers are equl
		if (num != xor_ans){
			perror("authentication failed");
		}
        //end authentication
        printf("end authentication\n");
        printf("\n");
    }
   
    
    
    close(sock);
    return 0;
}
