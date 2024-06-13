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

 // char PORT[10] = "8080";

 // int status;

 // if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0)
 //{
 //       perror("getaddrinfo() error:\n");
 //       exit(1);
 // }

 //
 //**
 // BIND, LISTEN, ACCEPT
 //**
 //

 // int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
 int sockfd = socket(PF_INET, SOCK_STREAM, 0);

 // printf("%d\n", res->ai_family);
 // printf("%d\n", res->ai_protocol);
 // printf("%d\n", res->ai_socktype);

 int yes = 1;

 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1)
 {
  perror("setsockopt() error");
  exit(1);
 }

 struct sockaddr_in my_addr;

 my_addr.sin_family = AF_INET;
 my_addr.sin_port = htons(8080); // short, network byte order
 my_addr.sin_addr.s_addr = INADDR_ANY;
 memset(my_addr.sin_zero, '\0', sizeof my_addr.sin_zero);

 if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof my_addr) == -1)
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

 struct sockaddr_storage their_addr;
 socklen_t addr_size = sizeof their_addr;
 int new_sockfd;

 if ((new_sockfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size)) == -1)
 {
  perror("acccept() error: \n");
  exit(1);
 }

 //
 // SEND
 //

 // freeaddrinfo(res);
 // close(sockfd);
 // return 0;

 char *msg = "hello friend";
 int msg_len, bytes_sent;

 msg_len = strlen(msg);

 if ((bytes_sent = send(new_sockfd, msg, msg_len, 0)) == -1)
 {
  perror("send() error: \n");
  exit(1);
 }

 printf("bytes sent: %d\n", bytes_sent);

 // freeaddrinfo(res);
 close(sockfd);

 return 0;
}