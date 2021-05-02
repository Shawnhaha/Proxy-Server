/*
proxy server
Author: Shangda Li
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define MAX 1024
//function for deleting the oldest url and writing the newest url
//removing the oldest webpage file
void updateList(char url[MAX], char vistime[20]) {
  FILE *fp;
  FILE *temp;
  char str[256];
  char filename[20];
  int i = 1;

  fp = fopen("list.txt", "r");
  temp = fopen("temp_list.txt", "w");

  while (!feof(fp)){
    bzero(str, sizeof(str));
    fgets(str, sizeof(str), fp);
    if (i == 1) {
      char *ptr = strtok(str, " ");
      ptr = strtok(NULL, "\n");
      sprintf(filename, "%s", ptr);  //get YYYYMMDDhhmmss for filename
      remove(filename);
      printf("Filename: %s has been removed!\n", filename);
    }
    if (i != 1) {   //skip the first line
      fprintf(temp, "%s", str);
    }
    i++;
  }

  fprintf(temp,"%s %s\n", url, vistime);  //print the newest url with visiting time to list file

  fclose(fp);
  fclose(temp);
  remove("list.txt");
  rename("temp_list.txt", "list.txt");
}

int main(int argc, char *argv[]) {
    int sockfd1, sockfd2, connfd;
    struct sockaddr_in servaddr;
    struct sockaddr_in socketAddr_in;
    struct hostent *host;
    char buff[MAX];
    char url[MAX];
    int PORT;

    if(argc == 2) {
     PORT = atoi(argv[1]);
    }
    else {
     printf("Please run proxy server with a valid port number\n");
     exit(1);
    }

    // creat socket1 for contact with client
    sockfd1 = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    bind(sockfd1, (struct sockaddr*)&servaddr, sizeof(servaddr));

    //listen to incoming connections
    listen(sockfd1, 8);

    // Accept connection
    connfd = accept(sockfd1, (struct sockaddr*)NULL, NULL);

    read(connfd, url, sizeof(url));     // read url from client

    FILE *list;
    int list_status = access("list.txt", F_OK);  //check if list file exists
    char str[256];
    int urlNum = 0;

    if (list_status==0) {
      list = fopen("list.txt","r");  //if list file exists, read it
      while (!feof(list)){
        bzero(str, sizeof(str));
        fgets(str, sizeof(str), list);
        if(memcmp(url, str, strlen(url)) == 0) {  //if we already have received url in the url in the list
          char *ptr = strtok(str, " ");
          char filename[20];
          ptr = strtok(NULL, "\n");
          sprintf(filename, "%s", ptr);  //get YYYYMMDDhhmmss for filename

          //read webpage file and send data to client
          FILE *web;
          web = fopen(filename, "r");
          while(!feof(web)){
            bzero(buff, sizeof(buff));
            fgets(buff, sizeof(buff), web);
            write(connfd, buff, sizeof(buff));
            //printf("%s", buff);
          }
          bzero(buff, sizeof(buff));
          fclose(web);           //close web file
          exit(0);
        }
        urlNum++;
      }
    }
    else{
      list = fopen("list.txt","w");  //if no list file, create one
    }
    fclose(list);

    char tempurl[MAX];
    strcpy(tempurl,url);  //keep original url
    char *hostname = strtok(tempurl, "/");   //use strtok to get host name
    char *path = strtok(NULL, "\0");  //get path
    if (path == NULL) {
      path = "";
    }

    //create HTTP request string
    char request[MAX];
    sprintf(request,
        "GET /%s HTTP/1.0\r\n"
        "Host: %s\r\n"
        "\r\n", path, hostname);

    //get IP address
    host = gethostbyname(hostname);

    //create a new socket for connecting to web server
    sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
    //socket check
    if (sockfd2 < 0) {
        perror("socket error");
        exit(1);
    }

    socketAddr_in.sin_family = AF_INET;
    socketAddr_in.sin_port = htons(80);
    socketAddr_in.sin_addr = *(struct in_addr*)host->h_addr;

    //connect
    if (connect(sockfd2, (struct sockaddr*) &socketAddr_in, sizeof(socketAddr_in))!=0) {
        perror("connect error");
        close(sockfd2);
        exit(0);
    }

    //get time when a web was visited
    time_t rawtime = time(NULL);
    struct tm *ptm = localtime(&rawtime);
    char visTime[20];
    sprintf(visTime, "%04d%02d%02d%02d%02d%02d", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour,ptm->tm_min, ptm->tm_sec);

    // write webpage file
    FILE *web;
    web = fopen(visTime, "w");
    int isOK = 0;  //http status
    int bno = 1;  //buffer number

    // send HTTP request
    write(sockfd2, request, sizeof(request));
    //receive data from web server, send it to client, and store it to file
    while (read(sockfd2, buff, sizeof(buff))>0){
      write(connfd, buff, sizeof(buff));
      if (bno == 1) {  //the first buffer
        if(strstr(buff, "200 OK")) {
          isOK = 1;
          printf("200 MATCHES, print to file!\n");
        }
        else {
          isOK = 0;
          fclose(web);
          remove(visTime);
          printf("200 NOT MATCHES, not print to file!\n");
        }
      }
      if (isOK == 1) {
        fprintf(web, "%s\n", buff);           //print to webpage file
      }
      bzero(buff,sizeof(buff));
      bno++;
    }
    fclose(web);

    //edit list file
    if (isOK == 1) {
      if (urlNum <= 5){
        list = fopen("list.txt","a");   //if list file exists and not reach 5 lines, append
        //opening file error check
        if(list == NULL) {
              printf("File error!");
              exit(1);
        }
      }
      else{
        updateList(url, visTime);
      }

      fprintf(list,"%s %s\n", url, visTime);  //print url with visiting time to list file
      fclose(list);
    }

    //close the sockets
    close(connfd);
    close(sockfd2);
    return 0;
}
