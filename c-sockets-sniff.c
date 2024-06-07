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

 //
 // HWADDR
 //

 struct ifreq ifreq_hwaddr;

 memset(&ifreq_hwaddr, 0, sizeof(ifreq_hwaddr));
 strncpy(ifreq_hwaddr.ifr_name, IFACE, IFNAMSIZ - 1);

 if ((ioctl(sock_raw, SIOCGIFHWADDR, &ifreq_hwaddr)) < 0)
 {
  printf("ioctl error:\n");
 }

 //
 // IPADDR
 //

 struct ifreq ifreq_ip;
 memset(&ifreq_ip, 0, sizeof(ifreq_ip));
 strncpy(ifreq_ip.ifr_name, IFACE, IFNAMSIZ - 1); // giving name of Interfac

 if (ioctl(sock_raw, SIOCGIFADDR, &ifreq_ip) < 0) // getting IP Address
 {
  printf("ioctl error:\n");
 }

 // struct sockaddr_ll saddr_ll;
 // saddr_ll.sll_ifindex = ifreq_ifindex.ifr_ifindex;

 struct sockaddr saddr;
 int saddr_len = sizeof(saddr);

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

  if ((tcp_src_port != 8796 && tcp_src_port != 5632) && (tcp_dest_port != 8796 && tcp_dest_port != 5632))
  {

   unsigned char *ip_addr = inet_ntoa(inaddr);

   if (strstr(ip_addr, "152.199") == NULL)
   {
    continue;
   }

   printf("IP: %s\n", inet_ntoa(inaddr));
   // printf("%s\n", strstr(ip_addr, "192.168"));

   unsigned char payload_slice[20];
   strncpy(payload_slice, payload, 300);

   printf("payload: %s\n", payload_slice);
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

 return 0;
}