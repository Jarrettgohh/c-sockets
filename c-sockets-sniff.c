#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include <errno.h>

#define IFACE "enp0s3"

int main()
{

 int sock_raw;
 sock_raw = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

 if (sock_raw == -1)
 {
  perror("Error in creating sockett:\n");
  return 1;
 }

 //
 // IFINDEX
 //

 struct ifreq ifreq_ifindex;

 memset(&ifreq_ifindex, 0, sizeof(ifreq_ifindex));
 strncpy(ifreq_ifindex.ifr_name, IFACE, IFNAMSIZ - 1);

 if (ioctl(sock_raw, SIOCGIFINDEX, &ifreq_ifindex) < 0)
 {
  printf("ioctl error:\n");
 }

 ifr.ifr_flags |= IFF_PROMISC;
 if (ioctl(sock, SIOCSIFFLAGS, &ifr) != 0)
 {
  perror("ioctl error:\n");
 }

 //
 // sockadd_ll struct
 //

 struct sockaddr_ll saddr_ll;
 saddr_ll.sll_ifindex = ifreq_ifindex.ifr_ifindex;
 saddr_ll.sll_family = AF_PACKET;
 saddr_ll.sll_halen = ETH_ALEN;

 // 08:00:27:1a:46:34
 saddr_ll.sll_addr[0] = 8;  // 0x08 => 8
 saddr_ll.sll_addr[1] = 0;  // 0x15 => 21
 saddr_ll.sll_addr[2] = 39; // 0x27 => 39
 saddr_ll.sll_addr[3] = 26; // 0x1a => 26
 saddr_ll.sll_addr[4] = 70; // 0x46 => 70
 saddr_ll.sll_addr[5] = 52; // 0x34 => 52

 struct sockaddr saddr;
 int saddr_len = sizeof(saddr);

 int http_data_len = 5000;
 unsigned char *http_data = (unsigned char *)malloc(http_data_len);
 memset(http_data, 0, http_data_len);

 for (;;)
 {

  void *buf = malloc(65536);
  memset(buf, 0, 65536);

  int rcvd = recvfrom(sock_raw, buf, 65536, 0, &saddr, (socklen_t *)&saddr_len);

  if (rcvd < 0)
  {
   perror("recvfrom() error:");
   close(sock_raw);
   return 1;
  }

  struct iphdr *ip = (struct iphdr *)(buf + sizeof(struct ethhdr));
  unsigned iphdr_len_bits = ip->ihl * 4; // value is number of pairs of hex values (multiply 4 for number of bits)

  unsigned char *payload = (unsigned char *)(buf + sizeof(struct ethhdr) + sizeof(struct iphdr) + iphdr_len_bits);
  struct in_addr inaddr;
  inaddr.s_addr = ip->saddr;

  struct tcphdr *tcp = (struct tcphdr *)(buf + sizeof(struct ethhdr) + sizeof(struct iphdr));

  // printf("%u\n", eth->h_source[0]);

  // unsigned char *payload_slice;
  // strncpy(payload_slice, payload, 10);

  // printf("payload: %s", payload_slice);
  // printf("%lu\n", sizeof(buf));

  unsigned int tcp_src_port = tcp->source;
  unsigned int tcp_dest_port = tcp->dest;

  // printf("%u\n", tcp_src_port);
  // printf("%u\n", tcp_dest_port);

  // printf("%u\n", tcp_src_port);
  // printf("%u\n", tcp_dest_port);

  // if(tcp->syn == 1)
  //{
  //       printf("syn 1");
  // }

  if ((tcp_src_port != 8796 && tcp_src_port != 5632) && (tcp_dest_port != 8796 && tcp_dest_port != 5632))
  {

   unsigned char *ip_addr = inet_ntoa(inaddr);

   if (tcp->syn == 1)
   {

    printf("tcpsyn");
    if (strstr(inet_ntoa(inaddr), "192.168.1.115"))
    {
     printf("sending");
    }

    // if(strstr(inet_ntoa(inaddr), "44.221.145.128") && (tcp->ack == 1))
    //{
    //       printf("received");
    // }

    continue;
   }

   if (strstr(ip_addr, "44.221") == NULL)
   {
    continue;
   }

   if (tcp->fin == 1)
   {
    break;
   }

   // printf("%u\n", tcp_src_port);
   // printf("%u\n", tcp_dest_port);

   printf("IP: %s\n", inet_ntoa(inaddr));
   // printf("%s\n", payload);

   // printf("%s\n", payload);
   // strcat(http_data, payload);

   // unsigned char payload_slice[20];
   // strncpy(payload_slice, payload, 1448);

   // printf("payload: %s\n", payload_slice);
   // printf("%ld\n", sizeof(payload));

   // printf("%ld\n", sizeof(buf));
   // printf("%u\n", tcp_src_port);
   // printf("%u\n", tcp_dest_port);

   // for (int i = 0; i < 50; i++)
   //{
   //       char hex_str[10];
   //       //sprintf(hex_str, "%.2x", payload_slice[i]);
   //       //long p_long = strtol(hex_str, NULL, 16);
   //       //printf("%c\n", payload_slice[i]);
   //}

   printf("\n\n");
  }
 }

 // printf("%s\n", http_data);
 // printf("%li\n", strlen(http_data));

 return 0;
}
