#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string.h>
#include <errno.h>

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

  // struct in_addr *in = get_in_addr((struct sockaddr *)(&recv_addr));
  // printf("%s\n", inet_ntoa(*in));

  while (1)
  {

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

   printf("%s", (char *)buf);
   printf("%li\n", strlen((char *)buf));
   break;
  }

  //
  //

  // int sockfd_secondary = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

  //
  // SEND back on listening socket
  //

  char *msg = "HTTP/1.1 200 OK\ncontent-length: 100\n\nwasup";
  int msg_len, bytes_sent;

  msg_len = strlen(msg);

  if ((bytes_sent = send(new_sockfd, msg, msg_len, 0)) == -1)
  {
   perror("send() error: \n");
   exit(1);
  }

  printf("bytes sent: %d\n", bytes_sent);

  freeaddrinfo(res);
  close(sockfd);

  break;
 }

 return 0;
}