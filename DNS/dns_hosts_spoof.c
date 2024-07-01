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

#include <endian.h>
#include <ares.h>

#include "dns.h"
#include "dns_struct_def.h"

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

  // buf[recv_len] = 0x0;

  printf("[+] RECEIVED:\n\n");
  // printf("%s", (char *)buf);

  printf("DIRECT CAST:\n");
  printf("id: %u\n", ((s_dns_hs *)buf)->id);
  printf("qr: %u\n", ((s_dns_hs *)buf)->flags.qr);
  printf("opcode: %u\n", ((s_dns_hs *)buf)->flags.opcode);
  printf("AA: %u\n", ((s_dns_hs *)buf)->flags.aa);
  printf("qcount: %i\n", ntohs(((s_dns_hs *)buf)->qdcount));

  printf("qtype: %s\n", tr_dns_type_str((ntohs(((s_dns_qs *)buf)->type))));
  printf("qclass: %s\n", tr_dns_class_str(ntohs(((s_dns_qs *)buf)->class)));
  exit(1);

  int status;

  status = ares_library_init(ARES_LIB_INIT_ALL);

  if (status != ARES_SUCCESS)
  {
   printf("ares_library_init: %s\n", ares_strerror(status));
   return 1;
  }

  // struct ares_query *query = (struct ares_query *)buf;
  // printf("%s\n", query->qtype);

  ares_status_t ares_res;
  ares_dns_record_t *dnsrec;

  ares_res = ares_dns_parse((const unsigned char *)buf, buf_length, ARES_DNS_PARSE_AN_BASE_RAW, &dnsrec);

  if (ares_res != ARES_SUCCESS)
  {

   perror("ares_dns_parse() error: \n");
  }

  // struct ares_question *ap;

  // printf("%lu\n", sizeof(struct ares_dns_record));
  // printf("%lu\n", (struct ares_answer *)dnsrec->type);
  //  printf("after res_dns_parse():\n");
  //  printf("id: %i\n", ((s_dns_hs *)dnsrec)->id);
  //  printf("qr: %i\n", ((s_dns_hs *)dnsrec)->flags.qr);
  //  printf("opcode: %i\n", ((s_dns_hs *)dnsrec)->flags.opcode);
  //  printf("AA: %i\n", ((s_dns_hs *)dnsrec)->flags.aa);
  //  printf("qcount: %i\n", ntohs(((s_dns_hs *)dnsrec)->qdcount));
  //  printf("%i\n", ((s_dns_hs *)dnsrec)->flags.opcode);

  ares_dns_record_destroy(dnsrec);
  ares_library_cleanup();
 }

 return 0;
}