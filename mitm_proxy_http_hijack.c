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

#define PORT "8080"

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
   perror("acccept() error: \n");
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

   char *host_addr = (char *)malloc(253);

   if (!regexec(&reg, (char *)buf, nmatch, pmatch, 0))
   {

    sprintf(host_addr, "%.*s", pmatch[1].rm_eo - pmatch[1].rm_so, &((char *)(buf))[pmatch[1].rm_so]);

    //
    // realloc memory to actual length
    //
    unsigned int host_addr_len = strlen(host_addr);
    host_addr = (char *)realloc(host_addr, host_addr_len + 1);
   }

   else
   {
    printf("regex didn't match\n");
   }

   regfree(&reg);

   //
   // ** HOSTNAME WHITELIST
   //
   //

   bool should_whitelist = false;

   if (should_whitelist)
   {

    if (!(strstr(host_addr, "httpbin.org") != NULL || strstr(host_addr, "httpforever.com") != NULL || strstr(host_addr, "neverssl.com") != NULL || strstr(host_addr, "gohxiangzheng.com") != NULL))
    {
     printf("Host not specified in whitelist, aborting...\n");
     exit(1);
    }
   }

   printf("[+] RECEIVED:\n\n");
   printf("%s", (char *)buf);

   // printf("%li\n", strlen((char *)buf));
   // break;

   //
   //

   // int sockfd_secondary = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

   //
   // SEND back on listening socket
   //

   // <script type=\"text/javascript\">function fn(){window.open(\"https://google.com\");}fn()</script>

   bool shouldRespond = true;

   if (shouldRespond)
   {

    // char *payload = "<html><head><link rel=\"icon\" type\"image/x-icon\" href=\"https://cdn-icons-png.flaticon.com/256/890/890132.png\"></head><body><img src=\"https://openclipart.org/image/800px/333901\" style=\"width:400px;height:400px\"/>hello friend, its your friendly neighbourhood hacker...</body></html>";

    // char *payload = "<html><head><link rel=\"icon\" type\"image/x-icon\" href=\"https://cdn-icons-png.flaticon.com/256/890/890132.png\"></head><body><img src=\"https://jarrett.gohxiangzheng.com/images/me_reading_book_3.jpeg\" style=\"width:400px;height:400px\"/>hello friend, its your friendly neighbourhood hacker...</body></html>";

    char *payload = "<html><head><link rel=\"icon\" type\"image/x-icon\" href=\"https://go.gov.sg/assets/lion-head-symbol.svg\"></head><body><img src=\"https://jarrett.gohxiangzheng.com/images/me_reading_book_3.jpeg\" style=\"width:400px;height:400px\"/>hello friend, its your friendly neighbourhood hacker...</body></html>";

    long unsigned payload_len = strlen(payload);
    char payload_len_str[10];
    sprintf(payload_len_str, "%lu", payload_len);

    char *msg = malloc(payload_len + 50);
    memset(msg, 0, payload_len);

    sprintf(msg, "HTTP/1.1 200 OK\ncontent-length: %s\n\n%s", payload_len_str, payload);
    // strcpy(msg, "HTTP/1.1 404 Not Found");
    // strcpy(msg, "HTTP/1.1 200 OK\ncontent-length: 12\n\nHELLO FRIEND");
    //
    int msg_len, bytes_sent;

    printf("\n[+] SENDING PAYLOAD:\n%s\n\n", msg);

    msg_len = strlen(msg);

    if ((bytes_sent = send(new_sockfd, msg, msg_len, 0)) == -1)
    {
     perror("send() error: \n");
     exit(1);
    }

    printf("\nBytes sent: %d\n\n\n", bytes_sent);
   }

   freeaddrinfo(res);
   close(new_sockfd);
   exit(0);

   // break;
  }

  close(new_sockfd);
 }

 return 0;
}