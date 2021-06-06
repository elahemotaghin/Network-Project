#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
  
// Define the Packet Constants
// traceroute packet size
#define TRACEROUTE_PKT_S 64
   
// Automatic port number
#define PORT_NO 0 
  
// Automatic port number
#define TRACEROUTE_SLEEP_RATE 1000000  
  
// Define the Ping Loop
int tracerouteloop=1;

//Define the Packet size 
int traceroute_packet_size = 64;

//Define timeout delay for receiving packets in seconds
int recv_timeout = 1;
  
// traceroute packet structure
struct traceroute_pkt{
    struct icmphdr hdr;
    char msg[TRACEROUTE_PKT_S-sizeof(struct icmphdr)];
};
  
// Calculating the Check Sum
unsigned short checksum(void *b, int len){   
    unsigned short *buf = b;
    unsigned int sum=0;
    unsigned short result;
  
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

int atoint(char s[]){
    int i, n = 0;
    for (i = 0; s[i] >= '0' && s[i] <= '9' ; i++){
        n = 10 * n + (s[i] - '0');
    }
    return n;
    
}  

// Interrupt handler
void intHandler(int dummy){
    tracerouteloop=0;
}
  
// Performs a DNS lookup 
char *dns_lookup(char *addr_host, struct sockaddr_in *addr_con){
    printf("\nResolving DNS..\n");
    struct hostent *host_entity;
    char *ip=(char*)malloc(NI_MAXHOST*sizeof(char));
    int i;
  
    if ((host_entity = gethostbyname(addr_host)) == NULL)
    {
        // No ip found for hostname
        return NULL;
    }
      
    //filling up address structure
    strcpy(ip, inet_ntoa(*(struct in_addr *) host_entity->h_addr));
  
    (*addr_con).sin_family = host_entity->h_addrtype;
    (*addr_con).sin_port = htons (PORT_NO);
    (*addr_con).sin_addr.s_addr  = *(long*)host_entity->h_addr;
  
    return ip;
      
}
  
// Resolves the reverse lookup of the hostname
char* reverse_dns_lookup(char *ip_addr){
    struct sockaddr_in temp_addr;    
    socklen_t len;
    char buf[NI_MAXHOST], *ret_buf;
  
    temp_addr.sin_family = AF_INET;
    temp_addr.sin_addr.s_addr = inet_addr(ip_addr);
    len = sizeof(struct sockaddr_in);
  
    if (getnameinfo((struct sockaddr *) &temp_addr, len, buf, 
                    sizeof(buf), NULL, 0, NI_NAMEREQD)) 
    {
        printf("Could not resolve reverse lookup of hostname\n");
        return NULL;
    }
    ret_buf = (char*)malloc((strlen(buf) +1)*sizeof(char) );
    strcpy(ret_buf, buf);
    return ret_buf;
}
  
// make a traceroute request
void send_traceroute_req(int traceroute_sockfd, struct sockaddr_in *traceroute_addr, char *traceroute_dom, char *traceroute_ip, char *rev_host, int ttl_val, int max_try){
    int msg_count=0, i, addr_len, flag=1, msg_received_count=0, try_count = 0;
      
    struct traceroute_pkt pckt;
    struct sockaddr_in r_addr;
    struct timespec time_start, time_end, tfs, tfe;
    long double rtt_msec=0, total_msec=0;
    struct timeval tv_out;
    tv_out.tv_sec = recv_timeout;
    tv_out.tv_usec = 0;
  
    clock_gettime(CLOCK_MONOTONIC, &tfs);

    if (setsockopt(traceroute_sockfd, SOL_IP, IP_TTL, &ttl_val, sizeof(ttl_val)) != 0){
        printf("\nSetting socket options to TTL failed!\n");
        return;
    }
  
    else{
        printf("\nSocket set to TTL..\n");
    }
  
    // setting timeout of recv setting
    setsockopt(traceroute_sockfd, SOL_SOCKET, SO_RCVTIMEO,
                   (const char*)&tv_out, sizeof tv_out);
  
      // flag is whether packet was sent or not
      flag=1;
     
      //filling packet
      bzero(&pckt, sizeof(pckt));
          
      pckt.hdr.type = ICMP_ECHO;
      pckt.hdr.un.echo.id = getpid();
        
      for ( i = 0; i < sizeof(pckt.msg)-1; i++ )
          pckt.msg[i] = i+'0';
        
      pckt.msg[i] = 0;
      pckt.hdr.un.echo.sequence = msg_count++;
      pckt.hdr.checksum = checksum(&pckt, sizeof(pckt));
  
      usleep(TRACEROUTE_SLEEP_RATE);

      for(int i = 0 ; i < max_try ; i++){
        //send packet
        flag = 1;
        try_count++;
        clock_gettime(CLOCK_MONOTONIC, &time_start);
        if ( sendto(traceroute_sockfd, &pckt, sizeof(pckt), 0, 
            (struct sockaddr*) traceroute_addr, 
              sizeof(*traceroute_addr)) <= 0)
        {
            printf("\nPacket Sending Failed!\n");
            flag=0;
        }
        else{
          break;
        }
      }

      //receive packet
      addr_len=sizeof(r_addr);
  
      if ( recvfrom(traceroute_sockfd, &pckt, sizeof(pckt), 0, 
             (struct sockaddr*)&r_addr, &addr_len) <= 0
              && msg_count>1) 
      {
          printf("\nPacket receive failed!\n");
      }
  
      else{

          clock_gettime(CLOCK_MONOTONIC, &time_end);
              
          double timeElapsed = ((double)(time_end.tv_nsec - time_start.tv_nsec))/1000000.0;
          rtt_msec = (time_end.tv_sec-time_start.tv_sec) * 1000.0 + timeElapsed;
              
          // if packet was not sent, don't receive
          if(flag)
          {
                rev_host = reverse_dns_lookup(traceroute_ip);      
                printf("%d bytes from %s (h: %s) (%s) msg_seq=%d ttl=%d rtt = %Lf ms and number of try is %d.\n", traceroute_packet_size, traceroute_dom, rev_host,
                traceroute_ip, msg_count,ttl_val, rtt_msec, try_count);
                printf("saddr = %d, %s: %d\n", r_addr.sin_family, inet_ntoa(r_addr.sin_addr), r_addr.sin_port);
                  //printf("%d bytes from %s (h: %s) (%s) msg_seq=%d ttl=%d rtt = %Lf ms number of trys is %d.\n", 
                    //    traceroute_packet_size, traceroute_dom, rev_host, traceroute_ip, msg_count, ttl_val, rtt_msec, try_count);

                msg_received_count++;

          }
      }    
    clock_gettime(CLOCK_MONOTONIC, &tfe);
    double timeElapsed = ((double)(tfe.tv_nsec - tfs.tv_nsec))/1000000.0;
      
    total_msec = (tfe.tv_sec-tfs.tv_sec)*1000.0+ timeElapsed;
                     
    printf("\n===%s traceroute statistics===\n", traceroute_ip);
    printf("\n%d packets sent, %d packets received, %f percent packet loss. Total time: %Lf ms.\n\n", 
           msg_count, msg_received_count,
           ((msg_count - msg_received_count)/msg_count) * 100.0,
          total_msec); 
}
  
// Driver Code
int main(int argc, char *argv[]){
    int start_ttl = 1, ttl_val = 64, max_try_number = 1;
    puts("Enter recieve timeout");
    scanf("%d", &recv_timeout);

    puts("Enter tracerote packet size");
    scanf("%d", &traceroute_packet_size);

    puts("Enter TTL start point");
    scanf("%d", &start_ttl);

    puts("Enter max TTL value");
    scanf("%d", &ttl_val);

    puts("Enter max try number value");
    scanf("%d", &max_try_number);

    if(argc != 2){
        printf("\nFormat %s <address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int sockfd;
    char *ip_addr, *reverse_hostname;
    struct sockaddr_in addr_con;
    int addrlen = sizeof(addr_con);
    char net_buf[NI_MAXHOST];
  
    ip_addr = dns_lookup(argv[1], &addr_con);
    if(ip_addr == NULL){
        printf("\nDNS lookup failed! Could not resolve hostname!\n");
        exit(EXIT_FAILURE);
    }
  
    reverse_hostname = reverse_dns_lookup(ip_addr);
    printf("\nTrying to connect to '%s' IP: %s\n", argv[1], ip_addr);
    printf("\nReverse Lookup domain: %s", reverse_hostname);

    //socket()
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(sockfd < 0){
        perror("\nSocket file descriptor not received!!\n");
        exit(EXIT_FAILURE);
    }
    else
        printf("\nSocket file descriptor %d received\n", sockfd);
  
    signal(SIGINT, intHandler);//catching interrupt
    for(int i = start_ttl ; i <= ttl_val ; i++){
      //send pings continuously
      printf("\n*** TLL is %d ***", i);
      send_traceroute_req(sockfd, &addr_con, reverse_hostname, ip_addr, argv[1], i, max_try_number);
    }
    return 0;
}
