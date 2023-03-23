#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h> // gettimeofday()
#include <sys/types.h>
#include <unistd.h>
// command: make clean && make all && ./parta

#define SERVER_PORT 3001
#define SERVER_IP_ADDRESS "127.0.0.1" 

// IPv4 header len without options
#define IP4_HDRLEN 20

// ICMP header len for echo req
#define ICMP_HDRLEN 8

// Checksum algo
unsigned short calculate_checksum(unsigned short *paddress, int len);

int main(int argc, char ** argv)
{
    if(argc!=2){
        printf("usage: %s <addres>\n" ,argv[1]);
        exit(1);
    }
    char * destination_ip = argv[1];
    char *arg[2];
    arg[0] = "./watchdog";
    arg[1] = NULL;
    int pid = fork();
    if(pid == 0)
    {
        arg[1]=argv[1];
        execvp(arg[0], arg);
    }

    sleep(2);

    int sock1=0;
    /*finish to read file and close*/
    //socket() creates an endpoint for communication and returns a file descriptor that refers to that endpoint. 
    //On success, a file descriptor for the new socket is returned.else -1.
    sock1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock1 == -1) {// if -1 returne -error
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
    int connectResult = connect(sock1, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1) {
        printf("connect() failed with error code : %d", errno);
        // cleanup the socket;
        close(sock1);
        return -1;
    }

    printf("connected to server\n");

    int icmpSeq = 0;
    struct icmp icmphdr; // ICMP-header
    char data[IP_MAXPACKET] = "This is the ping.\n";

    int datalen = strlen(data) + 1;

 
    struct sockaddr_in dest_in;
    memset(&dest_in, 0, sizeof(struct sockaddr_in));
    dest_in.sin_family = AF_INET;

    // The port is irrelant for Networking and therefore was zeroed.
    dest_in.sin_addr.s_addr = inet_addr(destination_ip);
    //convert IPv4/6 addresses from text to binary form
    int check= inet_pton(AF_INET, destination_ip, &(dest_in.sin_addr.s_addr));
    if(check<=0){ //erorr
        printf("Invalid IP Address\n");
        return 0;
    }

    // Create raw socket for IP-RAW (make IP-header by yourself)
    int sock = -1;
    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        fprintf(stderr, "socket() failed with error: %d", errno);
        fprintf(stderr, "To create a raw socket, the process needs to be run by Admin/root user.\n\n");
        return -1;
    }
        printf("ping %s: %d data bytes\n", argv[1],datalen);
while(1){
       //===================
    // ICMP header
    //===================

    // Message Type (8 bits): ICMP_ECHO_REQUEST
    icmphdr.icmp_type = ICMP_ECHO;

    // Message Code (8 bits): echo request
    icmphdr.icmp_code = 0;

    // Identifier (16 bits): some number to trace the response.
    // It will be copied to the response packet and used to map response to the request sent earlier.
    // Thus, it serves as a Transaction-ID when we need to make "ping"
    icmphdr.icmp_id = 18;

    // Sequence Number (16 bits): starts at 0
    icmphdr.icmp_seq = 0;

    // ICMP header checksum (16 bits): set to 0 not to include into checksum calculation
    icmphdr.icmp_cksum = 0;

    // Combine the packet
    char packet[IP_MAXPACKET];

    // Next, ICMP header
    memcpy((packet), &icmphdr, ICMP_HDRLEN);

    // After ICMP header, add the ICMP data.
    memcpy(packet + ICMP_HDRLEN, data, datalen);

    // Calculate the ICMP header checksum
    icmphdr.icmp_cksum = calculate_checksum((unsigned short *)(packet), ICMP_HDRLEN + datalen);
    memcpy((packet), &icmphdr, ICMP_HDRLEN);

    char okMsg [] = {0};
    int didSend = send(sock1,okMsg,sizeof(okMsg) + 1, 0);
    if(didSend < 0)
    {
        printf("Error sending");
    }
    
    struct timeval start, end;
    gettimeofday(&start, 0);

    // Send the packet using sendto() for sending datagrams.
    int bytes_sent = sendto(sock, packet, ICMP_HDRLEN + datalen, 0, (struct sockaddr *)&dest_in, sizeof(dest_in));
    if (bytes_sent == -1)
    {
        fprintf(stderr, "sendto() failed with error: %d", errno);
        return -1;
    }


    //printf("Successfuly sent one packet : ICMP HEADER : %d bytes, data length : %d , icmp header : %d \n", bytes_sent, datalen, ICMP_HDRLEN);
    // Get the ping response
    bzero(packet, IP_MAXPACKET);
    socklen_t len = sizeof(dest_in);
    size_t bytes_received = -1;
    int ttl;
    while ((bytes_received = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&dest_in, &len)))
    {
        if (bytes_received > 0)
        {
            // Check the IP header
            struct iphdr *iphdr = (struct iphdr *)packet;
            struct icmphdr *icmphdr = (struct icmphdr *)(packet + (iphdr->ihl * 4));
            //printf("%ld bytes from %s\n", bytes_received, inet_ntoa(dest_in.sin_addr));
            ttl= iphdr->ttl;

            //printf("Successfuly received one packet with %ld bytes : data length : %d , icmp header : %d , ip header : %d \n", bytes_received, datalen, ICMP_HDRLEN, IP4_HDRLEN);

            break;
        }
    }

    gettimeofday(&end, 0);

    char reply[IP_MAXPACKET];
    memcpy(reply, packet + ICMP_HDRLEN + IP4_HDRLEN, datalen);
    // printf("ICMP reply: %s \n", reply);

    float milliseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
    unsigned long microseconds = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec);
    //printf("\n%ld bytes from %s icmp_seq= %d. RTT: %f milliseconds (%ld microseconds)\n", bytes_received, inet_ntoa(dest_in.sin_addr),icmpSeq, milliseconds, microseconds);
     printf("%ld bytes from %s icmp_seq= %d.ttl=%d RTT: %f milliseconds\n", bytes_received, inet_ntoa(dest_in.sin_addr),icmpSeq, ttl, milliseconds);
    //sleep(1);
    icmpSeq++;
}
    // Close the raw socket descriptor.
    close(sock);

    return 0;

}


// Compute checksum (RFC 1071).
unsigned short calculate_checksum(unsigned short *paddress, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short *w = paddress;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *((unsigned char *)&answer) = *((unsigned char *)w);
        sum += answer;
    }

    // add back carry outs from top 16 bits to low 16 bits
    sum = (sum >> 16) + (sum & 0xffff); // add hi 16 to low 16
    sum += (sum >> 16);                 // add carry
    answer = ~sum;                      // truncate to 16 bits

    return answer;
}
