#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <errno.h>

#define DESTMAC0 0
#define DESTMAC1 21
#define DESTMAC2 93
#define DESTMAC3 30
#define DESTMAC4 20
#define DESTMAC5 15

int main()
{
 int sock_raw;
 sock_raw = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW); // or first arg: AF_PACKET, last argument: IPPROTO_RAW?

 if (sock_raw == -1) // == -1 for AF_INET
 {
  perror("error in socket:\n");
  return -1;
 }

 //
 // IFINDEX
 //

 struct ifreq ifreq_ifindex; // struct ifreq defined in #include <net/if.h>

 memset(&ifreq_ifindex, 0, sizeof(ifreq_ifindex));
 strncpy(ifreq_ifindex.ifr_name, "eth0", IFNAMSIZ - 1); // giving name of Interface

 if ((ioctl(sock_raw, SIOCGIFINDEX, &ifreq_ifindex)) < 0)
 {
  printf("ioctl error.");
 }

 //
 // HWADDR
 //

 struct ifreq ifreq_hwaddr; // struct ifreq defined in #include <net/if.h>

 memset(&ifreq_hwaddr, 0, sizeof(ifreq_hwaddr));
 strncpy(ifreq_hwaddr.ifr_name, "eth0", IFNAMSIZ - 1); // giving name of Interface

 if ((ioctl(sock_raw, SIOCGIFHWADDR, &ifreq_hwaddr)) < 0)
 {
  printf("ioctl error.");
 }

 //
 // IPADDR
 //

 struct ifreq ifreq_ip;
 memset(&ifreq_ip, 0, sizeof(ifreq_ip));
 strncpy(ifreq_ip.ifr_name, "eth0", IFNAMSIZ - 1); // giving name of Interfac

 if (ioctl(sock_raw, SIOCGIFADDR, &ifreq_ip) < 0) // getting IP Address
 {
  printf("ioctl error.");
 }

 // printf("%u\n", (unsigned char)ifreq_hwaddr.ifr_hwaddr.sa_data[5]);

 // int interface_index = ifreq_i.ifr_ifindex;
 // setsockopt(sock_raw, SOL_SOCKET, SO_BINDTODEVICE, &interface_index, sizeof(interface_index));

 // memset(&saddr_ll.sll_addr, 0x00, 6); // Set dummy MAC (modify as needed)
 // saddr_ll.sll_halen = 6; // Hardware address length (Ethernet)
 // saddr_ll.sll_family = AF_PACKET;

 // int buf_size = 100;
 // unsigned char *buf = (unsigned char *)malloc(100);
 // memset(&buf, 0, sizeof(buf));

 // buf = "hello friend";

 // printf("Sending on interface: %d\n", saddr_ll.sll_ifindex);

 unsigned char *sendbuff = (unsigned char *)malloc(64); // increase in case of more data
 memset(sendbuff, 0, 64);

 int total_len = 0;

 //
 // construct Ethernet header
 //
 struct ethhdr *eth = (struct ethhdr *)(sendbuff);

 eth->h_source[0] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[0]);
 eth->h_source[1] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[1]);
 eth->h_source[2] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[2]);
 eth->h_source[3] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[3]);
 eth->h_source[4] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[4]);
 eth->h_source[5] = (unsigned char)(ifreq_hwaddr.ifr_hwaddr.sa_data[5]);

 /* filling destination mac. DESTMAC0 to DESTMAC5 are macro having octets of mac address. */
 eth->h_dest[0] = DESTMAC0;
 eth->h_dest[1] = DESTMAC1;
 eth->h_dest[2] = DESTMAC2;
 eth->h_dest[3] = DESTMAC3;
 eth->h_dest[4] = DESTMAC4;
 eth->h_dest[5] = DESTMAC5;

 eth->h_proto = htons(ETH_P_IP); // means next header will be IP header

 /* end of ethernet header */
 total_len += sizeof(struct ethhdr);

 //
 // construct IP header
 //

 struct iphdr *iph = (struct iphdr *)(sendbuff + sizeof(struct ethhdr));
 iph->ihl = 5;
 iph->version = 4;
 iph->tos = 16;
 iph->id = htons(10201);
 iph->ttl = 64;
 iph->protocol = 17;
 iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
 iph->daddr = inet_addr("172.24.240.1"); // put destination IP address

 total_len += sizeof(struct iphdr);

 //
 // construct UDP header
 //

 struct udphdr *uh = (struct udphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr));

 uh->source = htons(23451);
 uh->dest = htons(23452);
 uh->check = 0;

 total_len += sizeof(struct udphdr);

 //
 sendbuff[total_len++] = 0xAA;
 sendbuff[total_len++] = 0xBB;
 sendbuff[total_len++] = 0xCC;
 sendbuff[total_len++] = 0xDD;
 sendbuff[total_len++] = 0xEE;
 //

 uh->len = htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));
 iph->tot_len = htons(total_len - sizeof(struct ethhdr));

 struct sockaddr_ll saddr_ll; // struct sockaddr_ll defined in  #include <sys/sockets.h>

 saddr_ll.sll_ifindex = ifreq_ifindex.ifr_ifindex; // set iface index for raw socket to send to
 saddr_ll.sll_family = AF_PACKET;
 saddr_ll.sll_halen = ETH_ALEN;

 printf("Interface index: %d\n", saddr_ll.sll_ifindex);

 // SRC: 00:15:5d:6c:74:34
 // DST (Yoga530): 00:15:5d:1e:14:0f

 saddr_ll.sll_addr[0] = DESTMAC0; // 0x0 => 0
 saddr_ll.sll_addr[1] = DESTMAC1; // 0x15 => 21
 saddr_ll.sll_addr[2] = DESTMAC2; // 0x5d => 93
 saddr_ll.sll_addr[3] = DESTMAC3; // 0x1e => 30
 saddr_ll.sll_addr[4] = DESTMAC4; // 0x14 => 20
 saddr_ll.sll_addr[5] = DESTMAC5; // 0x0f => 15

 char *msg = "hello";

 // int send_len = sendto(sock_raw, "hello", 64, 0, (const struct sockaddr*)&saddr_ll, sizeof(saddr_ll));
 int send_len = sendto(sock_raw, sendbuff, 64, 0, (const struct sockaddr *)&saddr_ll, sizeof(saddr_ll));

 printf("%d\n", send_len);

 if (send_len < 0)
 {
  perror("sendto() error");
  close(sock_raw);
  return -1;
 }

 return 0;
}