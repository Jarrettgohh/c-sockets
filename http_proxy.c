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
   perror("accept() error: \n");
   exit(1);
  }

  printf("\n\n--------------------------------\n\n**** New connection\n\n--------------------------------\n\n");

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
    printf("\n[-] Closing peer connection from client...\n");
    exit(0);
   }

   printf("[+] RECEIVED:\n\n");
   printf("%s", (char *)buf);

   strcat(req, (char *)buf);

   struct addrinfo hints, *res;

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;

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

    //
    // free buffer that is not in use
    //
    free(buf);

    //
    // realloc memory to actual length
    //
    unsigned int host_addr_len = strlen(host_addr);
    host_addr = (char *)realloc(host_addr, host_addr_len + 1);

    // printf("%lu\n", strlen(host_addr));
    // host_addr = (char *)realloc(host_addr, host_addr_len);

    // printf("%s\n", host_addr);
    // exit(1);
   }

   else
   {
    printf("regex didn't match\n");
   }

   regfree(&reg);

   //
   // ** HOSTNAME WHITELIST
   //
   if (!(strstr(host_addr, "httpbin.org") != NULL || strstr(host_addr, "httpforever.com") != NULL || strstr(host_addr, "neverssl.com") != NULL || strstr(host_addr, "www.weather.gov.sg") != NULL))
   {
    printf("Host not specified in whitelist, aborting...\n");
    exit(1);
   }

   printf("\n[+] Proxying traffic to host: %s\n\n", host_addr);
   getaddrinfo(host_addr, "80", &hints, &res);

   //
   // CREATE NEW SOCKET - connect to the destination server
   //

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

   //
   // free buffer
   //
   free(req);

   int dest_buf_len = 65536;
   int dest_recv_len_total = 0;

   //
   // RECEIVE INITIAL response - expected to contain HTTP response headers (with content-length, etc.)
   //

   char *initial_dest_res_buf = malloc(dest_buf_len);

   int dest_recv_len = recv(dest_sockfd, initial_dest_res_buf, dest_buf_len, 0);
   if (dest_recv_len == -1)
   {
    perror("recv() error: \n");
    exit(1);
   }
   else if (dest_recv_len == 0)
   {
    printf("closing connection...\n");
    break;
   }

   //
   // PARSE Content-Length value (will only be defined in the first response data)
   //

   regex_t reg_i;
   size_t nmatch_i = 2;
   regmatch_t pmatch_i[2];

   // regex expression to match content-length HTTP headers value
   int regcomp_res_i = regcomp(&reg_i, "Content-Length:[[:space:]](([[:digit:]])*)", REG_EXTENDED);

   if (regcomp_res_i)
   {
    int reg_err_len = 100;
    char reg_err_buf[reg_err_len];

    regerror(regcomp_res_i, &reg, reg_err_buf, reg_err_len);
    printf("regcomp error: %s\n", reg_err_buf);
   }

   char *content_len_str = (char *)malloc(6);

   if (!regexec(&reg_i, (char *)initial_dest_res_buf, nmatch_i, pmatch_i, 0))
   {
    sprintf(content_len_str, "%.*s", pmatch_i[1].rm_eo - pmatch_i[1].rm_so, &((char *)(initial_dest_res_buf))[pmatch_i[1].rm_so]);
   }

   else
   {
    printf("regex didn't match\n");
    continue;
   }

   regfree(&reg_i);

   dest_recv_len_total += dest_recv_len;

   unsigned long int http_data_len;

   char *data = strstr(initial_dest_res_buf, "\r\n\r\n");
   if (data != NULL)
   {

    http_data_len = strlen(data) - 4; // minus four for the double leading CRLF
   }

   printf("\n>>>> Initial response bytes received from dest server: %d\n", dest_recv_len);
   printf("** Initial content length received: %lu\n", http_data_len);
   printf("** Expected total content length: %s\n", content_len_str);

   //
   // SEND back initial response back on listening socket
   //

   int bytes_sent;

   if ((bytes_sent = send(new_sockfd, initial_dest_res_buf, dest_recv_len, 0)) == -1)
   {
    perror("send() error: \n");
    exit(1);
   }

   printf("\n[+] Intial response bytes sent back to client: %d\n", bytes_sent);

   int content_len = atoi(content_len_str);

   //
   // free buffer
   //
   free(initial_dest_res_buf);
   free(content_len_str);

   //
   // If the entire length as defined in Content-Length HTTP request headers is already sent back in the first response - do not continue receiving on this socket, let it close at the end of the loop
   //
   if (!((unsigned long int)content_len - http_data_len) == 0)
   {

    //
    // Determine additional chunked data length to receive
    //

    unsigned long int remaining_content_len = (unsigned long int)content_len - http_data_len;

    printf("\n--------------------------------\n\nRemaining content length after initial response: %lu\n\n--------------------------------\n", remaining_content_len);

    //
    // RECEIVE additional chunked response
    //

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
      printf("\n\n[-] Closing peer connection from client...\n");
      break;
     }

     printf("\n>>>> Additional bytes received from dest server: %d\n", dest_recv_len);

     dest_recv_len_total += dest_recv_len;

     //
     // SEND back on listening socket
     //

     int bytes_sent;

     if ((bytes_sent = send(new_sockfd, dest_buf, dest_recv_len, 0)) == -1)
     {
      perror("send() error: \n");
      exit(1);
     }

     printf("[+] Bytes sent back to client: %d\n", bytes_sent);

     //
     // free buffer
     //
     free(dest_buf);

     if (dest_recv_len_total >= content_len)
     {
      break;
     }
    }
   }

   printf("\n--------------------------------\n\nTotal bytes proxied from host %s to client (including HTTP response headers): %d\n\n--------------------------------\n", host_addr, dest_recv_len_total);
   printf("\n\n[-] Closing peer connection from client...\n\n");

   //
   // free buffer
   //
   free(host_addr);

   break;
  }

  close(new_sockfd);
 }

 return 0;
}