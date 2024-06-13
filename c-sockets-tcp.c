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
#include <linux/tcp.h>
// #include <linux/udp.h>

#include <errno.h>

#define DESTMAC0 0
#define DESTMAC1 21
#define DESTMAC2 93
#define DESTMAC3 30
#define DESTMAC4 20
#define DESTMAC5 15

// char *str_to_hex(char *text)
// {

//         int text_len = strlen(text);
//         char *hex_output = malloc(text_len * 2 + 1); // 2 hex = 1 ASCII char + 1 for null char

//         if (hex_output == NULL)
//         {
//                 perror("Failed to allocated memory.");
//                 return NULL;
//         }

//         int hex_index = 0;

//         for (int i = 0; i < strlen(text); i++)
//         {
//                 char t = text[i];

//                 int char_written = sprintf(&hex_output[hex_index], "%x", t);

//                 if (char_written < 0)
//                 {
//                         perror("Failed to convert to hex.");
//                         return NULL;
//                 }

//                 hex_index += 2;
//         }

//         return hex_output;
// }

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

        /* Pass variable of type struct ifreq to ioctl() function, which will load contents into ifreq_ifindex - which contains information about the IFINDEX (SIOCGIFINDEX)  */
        if (ioctl(sock_raw, SIOCGIFINDEX, &ifreq_ifindex) < 0)
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

        int sendbuff_len = 200;
        unsigned char *sendbuff = (unsigned char *)malloc(sendbuff_len); // increase in case of more data
        memset(sendbuff, 0, sendbuff_len);

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
        iph->protocol = 6; // TCP: 6, UDP: 17
        iph->saddr = inet_addr(inet_ntoa((((struct sockaddr_in *)&(ifreq_ip.ifr_addr))->sin_addr)));
        iph->daddr = inet_addr("172.24.240.1"); // put destination IP address

        total_len += sizeof(struct iphdr);

        //
        // construct TCP header
        //

        struct tcphdr *tcp_h = (struct tcphdr *)(sendbuff + sizeof(struct iphdr) + sizeof(struct ethhdr));

        tcp_h->source = htons(8000);
        tcp_h->dest = htons(8001);
        tcp_h->check = 0;
        tcp_h->seq = 0;
        tcp_h->ack_seq = 0;
        tcp_h->window = htons(1024);
        // tcp_h->urg_ptr = htons(0);
        // tcp_h->syn = 1;
        tcp_h->doff = 5;

        total_len += sizeof(struct tcphdr);

        printf("%d\n", total_len);

        //
        // ** Different methods of loading msg to the TCP payload data in the buffer
        //

        int option = 1;

        // **
        // method 1:

        if (option == 0)
        {

                char msg[50] = "hello friend, how are you doing? opt 0";
                int msg_len = strlen(msg);

                strncpy(&sendbuff[total_len++], msg, msg_len);
                // need to update, as this value is subsequently used to address the IP header `tot_len` field
                total_len += msg_len;
        }

        //**
        // method 2 (seems unnecessary, but for learning) - converts to hex-string, then to decimal:

        if (option == 1)
        {

                char msg[50] = "hello friend, how are you doing? opt 1";
                int msg_len = strlen(msg);

                int hex_i = 0;

                for (int i = 0; i < msg_len; i++)
                {

                        char msg_hex_str[3];
                        sprintf(msg_hex_str, "%x", msg[i]);
                        hex_i += 2;

                        long long_msg = strtol(msg_hex_str, NULL, 16);
                        sendbuff[total_len++] = long_msg;
                }
        }

        //**
        // method 3 (seems unnecessary, but for learning) - converts to decimal-string, then to decimal:

        if (option == 2)
        {

                char msg[50] = "hello friend, how are you doing? opt 2";
                int msg_len = strlen(msg);

                for (int i = 0; i < msg_len; i++)
                {

                        char dec_string[4];
                        sprintf(dec_string, "%d", msg[i]);

                        long long_msg = strtol(dec_string, NULL, 10);
                        sendbuff[total_len++] = long_msg;
                }
        }

        // tcp_h->len = htons((total_len - sizeof(struct iphdr) - sizeof(struct ethhdr)));
        iph->tot_len = htons(total_len - sizeof(struct ethhdr));

        struct sockaddr_ll saddr_ll; // struct sockaddr_ll defined in  #include <sys/sockets.h>

        saddr_ll.sll_ifindex = ifreq_ifindex.ifr_ifindex; // set iface index for raw socket to send to
        saddr_ll.sll_family = AF_PACKET;
        saddr_ll.sll_halen = ETH_ALEN;

        // printf("Interface index: %d\n", saddr_ll.sll_ifindex);

        // SRC: 00:15:5d:6c:74:34
        // DST (Yoga530): 00:15:5d:1e:14:0f

        saddr_ll.sll_addr[0] = DESTMAC0; // 0x0 => 0
        saddr_ll.sll_addr[1] = DESTMAC1; // 0x15 => 21
        saddr_ll.sll_addr[2] = DESTMAC2; // 0x5d => 93
        saddr_ll.sll_addr[3] = DESTMAC3; // 0x1e => 30
        saddr_ll.sll_addr[4] = DESTMAC4; // 0x14 => 20
        saddr_ll.sll_addr[5] = DESTMAC5; // 0x0f => 15

        int send_len = sendto(sock_raw, sendbuff, sendbuff_len, 0, (const struct sockaddr *)&saddr_ll, sizeof(saddr_ll));

        printf("%d\n", send_len);

        if (send_len < 0)
        {
                perror("sendto() error");
                close(sock_raw);
                return -1;
        }

        return 0;
}
