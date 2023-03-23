#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h> 
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/udp.h>


#define P 8000 

int main(int argc, char ** argv){

    printf("in the main\n");

     if(argc!=2){
        printf("usage: %s <addres>\n" ,argv[1]);
        exit(1);
    }

    // Takes the name of a host on the command line
    char * host_ip = argv[1];
    printf("host_ip %s\n", host_ip);
    
    //creates a datagram socket to that host (using port number P+1)
    int host_sock = socket(AF_INET, SOCK_DGRAM,0); 
    if(host_sock == -1){
        printf("Could not create socket : %d", errno);
        close(host_sock);
	    return -1;  
    }
    printf("cerate host sock\n");

    struct sockaddr_in serverAddress;
	memset(&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(P+1);

     int rval = inet_pton(AF_INET, (const char*)host_ip, &serverAddress.sin_addr);
	if (rval == 0)
	{
		printf("inet_pton() failed %d" , errno);
        close(host_sock);
        return -1;
	}

    // creates another datagram socket where it can receive datagrams from any host on port number P
    int p_sock = socket(AF_INET, SOCK_DGRAM,0); 
    if(p_sock == -1){
        printf("Could not create socket : %d", errno);
        close(host_sock);
        close(p_sock);
	    return -1;  
    }
    printf("cerate sock p\n");

    if (bind(host_sock, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("Error binding socket");
        return -1;
    }
    printf("bind host sock\n");

     struct sockaddr_in proxy;
	//Change type variable from int to socklen_t: int fromAddressSize = sizeof(fromAddress);
	socklen_t proxysize = sizeof(proxy);
	memset((char *)&proxy, 0, sizeof(proxy));
    proxy.sin_family = AF_INET;
    proxy.sin_port = htons(P);

    inet_pton(AF_INET, "0.0.0.0", &(proxy.sin_addr)); // IPv4 
    if (bind(p_sock, (struct sockaddr *) &proxy, sizeof(proxy)) < 0) {
        perror("Error binding socket");
        return -1;
    }

    struct sockaddr_in getway;
    socklen_t getwayLen = sizeof(getway);
    printf("bind p sock\n");
    
    while (1)
    {
        printf(" enter to whileloop\n");
        char buffer[1024]= {'\0'};

        memset((char *)&getway, 0, sizeof(getway));
        getwayLen = sizeof(getway);


        int len =recvfrom(p_sock, buffer, sizeof(buffer) -1, 0, (struct sockaddr *) &getway, &getwayLen);
        //The function returns the number of bytes received, or -1 if an error occurs.
        if (len == -1)
	    {
		printf("recvfrom() failed with error code  : %d", errno);
		return -1;
	    }
        printf("receve\n");

        //samples a random number
        float rand = ((float)random())/((float)RAND_MAX);
        printf("rand %f\n" , rand);
        if(rand>0.5){
            //send the message
	        if (sendto(host_sock, buffer, len, 0, (struct sockaddr *) &getway, sizeof(getway)) == -1)
	        {
	    	printf("sendto() failed with error code  : %d",errno);
		    	return -1;
	        }   
            
            printf("*******SEND**********\n");
        }


    }
    
    close(host_sock);
    close(p_sock);

    return 0;
}