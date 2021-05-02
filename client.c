/*
client for proxy server
Author: Shangda Li
*/
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX 1024

int main(int argc, char *argv[]) {
    int sockfd, connfd;
    struct sockaddr_in servaddr;
    char buff[MAX];
    char url[MAX];
    int PORT;

    if(argc == 2) {
     PORT = atoi(argv[1]);
    }
    else {
     printf("Please run client with a valid port number\n");
     exit(1);
    }

    // socket create and varification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    /* Convert IPv4 and IPv6 addresses from text to binary form */
    inet_pton(AF_INET,"129.120.151.94",&(servaddr.sin_addr));
    servaddr.sin_port = htons(PORT);

    // connect the client socket to server socket
    if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))!=0) {
      perror("Connection error");
      close(sockfd);
      exit(1);
    }

    printf("url:");
    scanf("%s",url);

    while(memcmp(url,"www.", 4)!=0) {
      printf("Invaild url. Enter an url starting www.\n");
      printf("url:");
      scanf("%s",url);
    }

    //func(sockfd);
    //send url to the proxy server
    write(sockfd, url, sizeof(url));
    bzero(url, sizeof(url));

    //receive data
    while (read(sockfd, buff, sizeof(buff))>0){
      printf("%s", buff);       //print out
      bzero(buff, sizeof(buff));
    }
    printf("\n");

    // close the socket
    close(sockfd);
    return 0;
  }
