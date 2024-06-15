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

void *get_in_addr(struct sockaddr *sa)
{
 return sa->sa_family == AF_INET
            ? (void *)&(((struct sockaddr_in *)sa)->sin_addr)
            : (void *)&(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main()
{
 struct addrinfo hints;
 struct addrinfo *res; // contains the results from the getaddrinfo() function call

 memset(&hints, 0, sizeof hints); // initialize the hints struct to be empty
 hints.ai_family = AF_UNSPEC;     // doesn't matter if its IPv4 or IPv6
 hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
 hints.ai_flags = AI_PASSIVE;     // fills in IP for me

 char PORT[10] = "8080";

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

 freeaddrinfo(res);

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
   perror("acccept() error: \n");
   exit(1);
  }

  printf("new connection\n");

  if (!fork()) // child process
  {

   close(sockfd); // child process doesn't need the listener

   int req_len = 6000;
   char *req = (char *)malloc(req_len);
   memset(req, 0, req_len);

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
    exit(0);
   }

   printf("RECEIVED:\n");
   printf("%s", (char *)buf);

   strcat(req, (char *)buf);

   //
   // CREATE NEW SOCKET - connect to the destination server
   //

   struct addrinfo hints, *res;

   // struct in_addr *in = get_in_addr((struct sockaddr *)(&recv_addr));
   // char dest_addr[20];

   // sprintf(dest_addr ,"%s", inet_ntoa(*in));
   // printf("%s\n", dest_addr);

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

   // char ipstr[INET_ADDRSTRLEN];
   // struct sockaddr_storage addr;

   // socklen_t len = sizeof addr;
   // getpeername(new_sockfd, (struct sockaddr*)&addr, &len);

   // int port;
   // struct sockaddr_in *s = (struct sockaddr_in *)&addr;

   // port = ntohs(s->sin_port);
   // inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

   // printf("Peer IP address: %s\n", ipstr);
   // printf("Peer port      : %d\n", port);

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

    // unsigned int host_addr_len = strlen(host_addr);
    // host_addr = (char *)realloc(host_addr, host_addr_len);
   }

   else
   {
    printf("regex didn't match\n");
   }

   regfree(&reg);

   // host_addr[strcspn(host_addr, "\n")] = 0x0; // remove newline (\n) char

   if (!(strstr(host_addr, "httpbin.org") != NULL || strstr(host_addr, "httpforever.com") != NULL))
   {
    printf("abort\n");
    exit(1);
   }

   printf("Proxying traffic to host: %s\n", host_addr);
   getaddrinfo(host_addr, "80", &hints, &res);

   int dest_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
   if (connect(dest_sockfd, res->ai_addr, res->ai_addrlen) == -1)
   {
    perror("connect() error: \n");
    exit(1);
   }

   //
   // SEND TO destination server
   //

   int dest_req_len = strlen(req);

   if ((send(dest_sockfd, req, dest_req_len, 0)) == -1)
   {
    perror("send() error: \n");
    exit(1);
   }

   int dest_buf_len = 65536;
   int dest_recv_len_total;

   while (1)
   {
    char *dest_buf = malloc(dest_buf_len);

    int dest_recv_len = recv(dest_sockfd, dest_buf, dest_buf_len, 0);
    if (dest_recv_len == -1)
    {
     perror("recv() error: \n");
     exit(1);
    }
    else if (dest_recv_len == 0)
    {
     printf("closing connection\n");
     break;
    }

    //
    // PARSE Content-Length value (will only be defined in the first response data)
    //

    regex_t reg;
    size_t nmatch = 2;
    regmatch_t pmatch[2];

    int regcomp_res = regcomp(&reg, "Content-Length:[[:space:]](([[:digit:]])*)", REG_EXTENDED);

    if (regcomp_res)
    {
     int reg_err_len = 100;
     char reg_err_buf[reg_err_len];

     regerror(regcomp_res, &reg, reg_err_buf, reg_err_len);
     printf("regcomp error: %s\n", reg_err_buf);
    }

    char *content_len = (char *)malloc(6);

    if (!regexec(&reg, (char *)dest_buf, nmatch, pmatch, 0))
    {

     sprintf(content_len, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, &((char *)(dest_buf))[pmatch[1].rm_so]);
    }

    else
    {
     printf("regex didn't match\n");
     continue;
    }

    regfree(&reg);

    printf("\n****Received bytes from dest server: %d\n", dest_recv_len);
    printf("content length: %s\n", content_len);
    // printf("%s\n", dest_buf);

    dest_recv_len_total += dest_recv_len;

    //
    // SEND back on listening socket
    //

    int bytes_sent;

    if ((bytes_sent = send(new_sockfd, dest_buf, dest_recv_len_total, 0)) == -1)
    {
     perror("send() error: \n");
     exit(1);
    }

    printf("Bytes sent back to client: %d\n", bytes_sent);
   }

   printf("Total bytes sent: %d\n", dest_recv_len_total);
  }

  // close(new_sockfd);
  // exit(0);

  // printf("Request len: %lu\n", strlen(req));

  close(new_sockfd);
 }

 return 0;
}