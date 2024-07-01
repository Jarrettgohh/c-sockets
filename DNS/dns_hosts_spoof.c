#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdbool.h>
#include <regex.h>
#include <string.h>
#include <errno.h>

#include "dns_def.h"

#define PORT "8090"

int main()
{

 struct addrinfo hints;
 struct addrinfo *res; // contains the results from the getaddrinfo() function call

 memset(&hints, 0, sizeof hints); // initialize the hints struct to be empty
 hints.ai_family = AF_UNSPEC;     // doesn't matter if its IPv4 or IPv6
 hints.ai_socktype = SOCK_DGRAM;  // UDP sockets
 hints.ai_flags = AI_PASSIVE;     // fills in IP for me

 int status;

 if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
 {
  perror("getaddrinfo() error:\n");
  exit(1);
 }

 //
 //**
 // BIND only
 //**
 //

 int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

 int yes = 1;

 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
 {
  perror("setsockopt() error");
  exit(1);
 }

 if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1)
 {
  perror("bind() error: \n");
  exit(1);
 }

 while (1)
 {

  //
  // RECV
  //

  socklen_t fromlen;
  struct sockaddr_storage addr;

  char buf[1000];
  // void *buf = malloc(1000);
  size_t buf_length = sizeof(buf);
  // void *buf = malloc(buf_len);
  // memset(buf, 0, buf_length);

  fromlen = sizeof addr;

  int recv_len = recvfrom(sockfd, buf, buf_length, 0, (struct sockaddr *)&addr, &fromlen);
  if (recv_len == -1)
  {
   perror("recv() error: \n");
   exit(1);
  }
  else if (recv_len == 0)
  {
   printf("closing connection\n");
   break;
  }

  printf("recv()'d %d bytes of data in buf\n", recv_len);
  printf("[+] RECEIVED:\n\n");

  printf("id: %u\n", ((s_dns_hs *)buf)->id);
  printf("qr: %u\n", ((s_dns_hs *)buf)->flags.qr);
  printf("opcode: %u\n", ((s_dns_hs *)buf)->flags.opcode);
  printf("AA: %u\n", ((s_dns_hs *)buf)->flags.aa);

  ssize_t n;       // number of questions
  s_dns_hs *hs;    // headers
  s_dns_qs *qs;    // questions
  uint8_t *offset; // offset of question from start of buffer, essentially the size of the header
  uint8_t qname[DNS_QNAME_MAX_LEN];
  size_t qname_len;

  hs = (s_dns_hs *)buf;
  n = (ssize_t)ntohs(hs->qdcount);

  offset = buf + sizeof(*hs);
  printf("# question section: %zu\n", (size_t)n);
  while (n-- > 0)
  {
   qname_len = 0;
   parse_label(buf, offset, qname, &qname_len);
   if (qname_len == 0)
   {
    qname[0] = 0;
   }
   offset += qname_len;
   qs = (s_dns_qs *)offset;

   printf(" -> %s  %s  %s\n", qname,
          tr_dns_type_str(ntohs(qs->type)),
          tr_dns_class_str(ntohs(qs->class)));

   offset += sizeof(*qs);
  }

  // printf("qcount: %i\n", ntohs(((s_dns_hs *)buf)->qdcount));
  // printf("qtype: %s\n", tr_dns_type_str((ntohs(((s_dns_qs *)buf)->type))));
  // printf("qclass: %s\n", tr_dns_class_str(ntohs(((s_dns_qs *)buf)->class)));
 }

 return 0;
}