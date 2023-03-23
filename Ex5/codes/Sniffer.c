#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <sys/socket.h>
#include <pcap.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <pcap/pcap.h>


int i=0;

/* Ethernet header */
struct ethheader
{
  u_char ether_dhost[ETHER_ADDR_LEN]; /* destination host address */
  u_char ether_shost[ETHER_ADDR_LEN]; /* source host address */
  u_short ether_type;                 /* IP? ARP? RARP? etc */
};

// ip header

struct ipheader {
unsigned char      iph_ihl:4, //IP header length
                        iph_ver:4; //IP version
unsigned char      iph_tos; //Type of service
unsigned short int iph_len; //IP Packet length (data + header)
unsigned short int iph_ident; //Identification
unsigned short int iph_flag:3, //Fragmentation flags
                        iph_offset:13; //Flags offset
unsigned char      iph_ttl; //Time to Live
unsigned char      iph_protocol; //Protocol type
unsigned short int iph_chksum; //IP datagram checksum
struct  in_addr    iph_sourceip; //Source IP address 
struct  in_addr    iph_destip;   //Destination IP address 
};


/* app header*/
struct app_h
{
    uint32_t timestamp;
    uint16_t total_length;
    union
    {
        uint16_t reserved:3,cache_flag:1,steps_flag:1,type_flag:1,status_code:10;
        uint16_t flags;
    };
    
    uint16_t cache_control;
    uint16_t padding;
};
void got_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *packet)
{
  FILE *file;
  file = fopen("209326776_322998287.txt", "a+");
    /**
    https://stackoverflow.com/questions/70449344/if-i-have-populated-all-fields-of-iphdr-then-how-to-calculate-the-tcphdr-doff-a
    https://www.opensourceforu.com/2015/03/a-guide-to-using-raw-sockets/
    https://stackoverflow.com/questions/11383497/libpcap-payload-offset-66-but-sizeofheaders-doff-62
    */
    struct ethheader *etr = (struct ethheader *)(packet);
    struct ipheader *ip = (struct ipheader *)(packet + sizeof(struct ethheader));
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct ethheader) + ip->iph_ihl*4);
    struct app_h *app = (struct app_h *)(packet + sizeof(struct ethheader) + (ip->iph_ihl*4 + tcp->doff*4));
    unsigned char *data = (char*)(struct app_h *)(app + sizeof(struct app_h));


    char* soruceaddr = inet_ntoa(ip->iph_sourceip);
    char* destaddr = inet_ntoa(ip->iph_destip);
    uint16_t sorucPort = ntohs(tcp->source);
    uint16_t destPort = ntohs(tcp->dest);
    uint32_t timestamp = ntohl(app->timestamp);
    uint16_t total = ntohs(app->total_length);
    app -> flags = ntohs(app->flags); 
    uint16_t cache = ((app->flags>>12) & 1);
    uint16_t steps = ((app->flags>>11) & 1);
    uint16_t type  = ((app->flags>>10) & 1);
    uint16_t status= app->status_code;
    uint16_t cache_c = ntohs(app->cache_control);


    fprintf(file,"---------------got packet %d---------\n",i);
    i++;
    fprintf( file,"source_ip: %s\n",soruceaddr);
    fprintf( file,"dest_ip: %s\n", destaddr);
    fprintf(file,"source_port: %hu\n",sorucPort);
    fprintf( file,"dest_port: %hu\n", destPort);
    fprintf( file,"timestamp: %u\n",timestamp);
    fprintf( file,"total_length: %hu\n", total);
    fprintf(file, "cache_flag: %hu\n",cache);
    fprintf(file,"steps_flag: %hu\n", steps);
    fprintf(file,"type_flag: %hu\n", type);
    fprintf(file, "status_code: %hu\n",status);
    fprintf(file,"cache_control: %hu\n",cache_c);

    for (int i = 0; i < header->len; i++)//data
      {
          //fprintf(fp, "%02x ", packet[i]);
          fprintf( file,"%02x ", packet[i]);
          if((i%16)==0){
          fprintf(file,"\n");
        }
      }
    fprintf(file, "}\n");
    
  fclose(file);

  }





int main()
{
  // file = fopen("209326776_322998287.txt", "a+");
//   fprintf(file, "This is a test string.\n");

  pcap_t *handle;
  char errbuf[PCAP_ERRBUF_SIZE];
  struct bpf_program fp;
  //filter to 9999
  //port portnamenum


  char filter_exp[] = "tcp port 9999";
  bpf_u_int32 net;

  /* Open the session in promiscuous mode
  Step 1: Open live pcap session on NIC with name lo
  The first argument is the device that we specified in the previous section.
  snaplen is an integer which defines the maximum number of bytes to be captured by pcap.
  promisc, when set to true, brings the interface into promiscuous mode (however, even if it is set to false,
  it is possible under specific cases for the interface to be in promiscuous mode, anyway).
  to_ms is the read time out in milliseconds (a value of 0 means no time out;
  on at least some platforms, this means that you may wait until a sufficient number of packets arrive before seeing any packets,
  so you should use a non-zero timeout).
  Lastly, ebuf is a string we can store any error messages within (as we did above with errbuf). The function returns our session handler.*/
  handle = pcap_open_live("lo", BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL)
  {
    fprintf(stderr, "Couldn't open device %s: %s\n", "lo", errbuf);
    return (2);
  }
  
  /* Compile and apply the filter
    Step 2: Compile filter_exp into BPF psuedo-code 
    Before applying our filter, we must "compile" it. The filter expression is kept in a regular string (char array).
    The first argument is our session handle (pcap_t *handle in our previous example).
    Following that is a reference to the place we will store the compiled version of our filter.
    Then comes the expression itself, in regular string format.
    Next is an integer that decides if the expression should be "optimized" or not 
    (0 is false, 1 is true—standard stuff).
    Finally, we must specify the network mask of the network the filter applies to.
    The function returns -1 on failure; all other values imply success.
    */
  int comp =pcap_compile(handle, &fp, filter_exp, 0, net) ;
  if (comp == -1)
  {
    fprintf(stderr, "Couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return (2);
  }
  
/*
The first argument is our session handler,
 the second is a reference to the compiled version 
 of the expression (presumably the same variable as the second argument to pcap_compile()).
*/

  if (pcap_setfilter(handle, &fp) == -1)
  {
    fprintf(stderr, "Couldn't install filter %s: %s\n", filter_exp, pcap_geterr(handle));
    return (2);
  }
   
  // Step 3: Capture packets
 /*
  The  functions that one can use to define their callback are pcap_loop() and pcap_dispatch(),
  these are very similar in their usage of callbacks. 
  Both of them call a callback function every time a packet is sniffed that meets our filter requirements
  (if any filter exists, of course. If not, then all packets that are sniffed are sent to the callback.)
 */
  pcap_loop(handle, -1, got_packet, NULL);
  
  pcap_close(handle); // Close the handle
  // fclose(file);  

  return 0;
}

