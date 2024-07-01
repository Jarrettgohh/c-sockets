
//
// ** EXAMPLE WITH c-ares library
//

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

  //  **************
  // ** DNS HEADER PARSING: ID, FLAGS, qdcount, etc.
  //  **************

  unsigned short flags = ares_dns_record_get_flags(dnsrec);

  ares_dns_record_get_id();
  ares_dns_record_get_opcode();

  //  **************
  // ** DNS QUESTION PARSING
  //  **************

  ares_status_t
  ares_dns_record_query_get(const ares_dns_record_t *dnsrec,
                            size_t idx, const char **name,
                            ares_dns_rec_type_t *qtype,
                            ares_dns_class_t *qclass);

  //  **********************
  // ** DNS ANSWERS CRAFTING
  //  **********************

  // ares_dns_record_query_get();
  // ares_dns_rr_get_name();
  // ares_dns_rr_get_type();

  // ares_dns_section_t sect = ARES_SECTION_ANSWER;

  ares_dns_record_t *dnsrec;

  // create an empty DNS record that will be stored in the `dnsrec` variable, loaded with the id, flags, opcode and rcode
  ares_status_t ares_dns_record_create(ares_dns_record_t * *dnsrec,
                                       unsigned short id,
                                       unsigned short flags,
                                       ares_dns_opcode_t opcode,
                                       ares_dns_rcode_t rcode);

  // set the resource record (rr) in the `dnsrec` variable created from the above function
  ares_status_t ares_dns_record_rr_add(ares_dns_rr_t * *rr_out,
                                       ares_dns_record_t * dnsrec,
                                       ares_dns_section_t sect,
                                       const char *name,
                                       ares_dns_rec_type_t type,
                                       ares_dns_class_t rclass,
                                       unsigned int ttl);

  // pass in the variable referenced as `rr_out` from the above function to set the IPV4 address value in the RDATA (?) field of the resource record (rr)
  ares_status_t ares_dns_rr_set_addr(ares_dns_rr_t * dns_rr,
                                     ares_dns_rr_key_t key,
                                     const struct in_addr *addr);

  ares_dns_record_destroy(dnsrec);
  ares_library_cleanup();
 }

 return 0;
}