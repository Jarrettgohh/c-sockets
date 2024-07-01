#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <regex.h>
#include <string.h>
#include <errno.h>

#define MAX_HOSTNAME_LEN 253
#define PORT "8080"

int replace_str_regex(char *find_regex, char *replace, char *buf, int buf_strlen)
{

 regex_t reg;
 size_t nmatch = 2;
 regmatch_t pmatch[2];

 int regcomp_res = regcomp(&reg, find_regex, REG_EXTENDED);

 if (regcomp_res)
 {
  int reg_err_len = 100;
  char reg_err_buf[reg_err_len];

  regerror(regcomp_res, &reg, reg_err_buf, reg_err_len);
  printf("regcomp error: %s\n", reg_err_buf);
 }

 char *regex_str = (char *)malloc(100);

 if (!regexec(&reg, (char *)buf, nmatch, pmatch, 0))
 {

  int start = pmatch[0].rm_so;
  int end = pmatch[0].rm_eo;

  char *replaced_buf = malloc(buf_strlen);
  strcpy(replaced_buf, buf);
  memset(&replaced_buf[start], (unsigned char)' ', end - start);

  printf("printing buf:\n");
  printf("%s\n", replaced_buf);

  // printf("%.*s", pmatch[0].rm_eo - pmatch[0].rm_so, &((char *)(buf))[pmatch[0].rm_so]);
 }
}

int main()
{
 struct addrinfo hints;
 struct addrinfo *res; // contains the results from the getaddrinfo() function call

 memset(&hints, 0, sizeof hints); // initialize the hints struct to be empty
 hints.ai_family = AF_UNSPEC;     // doesn't matter if its IPv4 or IPv6
 hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
 hints.ai_flags = AI_PASSIVE;     // fills in IP for me

 int status;

 if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
 {
  perror("getaddrinfo() error:\n");
  exit(1);
 }

 //
 //**
 // BIND, LISTEN, ACCEPT
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

 int MAX_CONN = 10;

 if (listen(sockfd, MAX_CONN) == -1)
 {
  perror("listen() error: \n");
  exit(1);
 }

 int new_sockfd;

 while (1)
 {

  struct sockaddr_storage recv_addr;
  socklen_t addr_size = sizeof recv_addr;

  if ((new_sockfd = accept(sockfd, (struct sockaddr *)&recv_addr, &addr_size)) == -1)
  {
   perror("accept() error: \n");
   exit(1);
  }

  printf("\n\n--------------------------------\n\n**** New connection\n\n--------------------------------\n\n");

  // struct in_addr *in = get_in_addr((struct sockaddr *)(&recv_addr));
  // printf("%s\n", inet_ntoa(*in));

  if (!fork()) // child process
  {

   close(sockfd); // child process doesn't need the listener

   //
   // RECV
   //

   int buf_len = 1000;
   void *buf = malloc(buf_len);
   memset(buf, 0, buf_len);

   int recv_len = recv(new_sockfd, buf, buf_len, 0);
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

   //
   // PARSE the host address from the HTTP request headers
   //

   regex_t reg;
   size_t nmatch = 2;
   regmatch_t pmatch[2];

   int regcomp_res = regcomp(&reg, "Host:[[:space:]](([[:alnum:]]||\\.)*)", REG_EXTENDED);

   if (regcomp_res)
   {
    int reg_err_len = 100;
    char reg_err_buf[reg_err_len];

    regerror(regcomp_res, &reg, reg_err_buf, reg_err_len);
    printf("regcomp error: %s\n", reg_err_buf);
   }

   char *host_addr = (char *)malloc(MAX_HOSTNAME_LEN);

   if (!regexec(&reg, (char *)buf, nmatch, pmatch, 0))
   {

    sprintf(host_addr, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, &((char *)(buf))[pmatch[1].rm_so]);
   }

   else
   {
    printf("regex didn't match\n");
   }

   regfree(&reg);

   //
   // ** HOSTNAME WHITELIST
   //
   // if (!(strstr(host_addr, "httpbin.org") != NULL || strstr(host_addr, "httpforever.com") != NULL || strstr(host_addr, "neverssl.com") != NULL || strstr(host_addr, "gohxiangzheng.com") != NULL || strstr(host_addr, "localhost") != NULL))
   // {
   //  printf("Host not specified in whitelist, aborting...\n");
   //  exit(1);
   // }

   printf("[+] RECEIVED:\n\n");
   printf("%s", (char *)buf);
   printf("%lu\n", sizeof(buf));

   typedef struct
   {
    char *random;
   } ClientHello;

   ClientHello *client_hello = (ClientHello *)buf;
   printf("ClientHello received:\n");
   printf("%s\n", client_hello->random);

   //
   //

   // int sockfd_secondary = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   freeaddrinfo(res);
   close(new_sockfd);
   exit(0);
  }

  close(new_sockfd);
 }

 return 0;
}