#include <stdio.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <openssl/ssl.h>

int get_req(char *host, char *page) {

enum CONSTEXPR { MAXLINE = 1024};
char request[MAXLINE + 1], response[MAXLINE + 1];
char *ptr;

in_addr_t in_addr;
struct protoent *protoent;
struct hostent *hostent;
struct sockaddr_in sockaddr_in;

int sockfd;
size_t n;

unsigned short port = 80;

// build socket
protoent = getprotobyname("tcp");
  if (protoent == NULL) {
  perror("getprotobyname");
  exit(EXIT_FAILURE);
}

sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
if (sockfd == -1) {
  perror("socket");
  exit(EXIT_FAILURE);
}

// form request
snprintf(request, MAXLINE,
         "GET /%s HTTP/1.0\r\n"
         "Host: %s\r\n"
         "Connection: close\r\n\r\n"
         , page, host);

printf("GET /%s\n"
       "Host: %s\n\n"
       , page, host);


hostent = gethostbyname(host);
if (hostent == NULL) {
  fprintf(stderr, "error: get host %s\n", host);
  exit(EXIT_FAILURE);
}

//create the address as a binary stream          vector of adresses for host
in_addr = inet_addr(inet_ntoa(*(struct in_addr*)*(hostent->h_addr_list)));
if (in_addr == (in_addr_t)-1) {
  fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
  exit(EXIT_FAILURE);
}

// point the socket to the address created before
sockaddr_in.sin_addr.s_addr = in_addr;
// define the type of connection?
sockaddr_in.sin_family = AF_INET;
// set the port
sockaddr_in.sin_port = htons(port);

//connect
if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
  perror("connect");
  exit(EXIT_FAILURE);
}

// write to socket
if (write(sockfd, request, strlen(request)) >= 0) {
  // read response
  while ((n = read(sockfd, response, MAXLINE)) > 0) {
    response[n] = '\0';

    if (fputs(response, stdout) == EOF) {
      printf("fputs error\n");
    }

    // remove trailing chars
    // ptr = strstr(response, "\r\n\r\n");
    // printf("%s", ptr);
  }
}
}

char *c;

int main(int argc, char **argv) {
  //set host and wordlist from user options
  char *host = argv[1];
  char *wordlist = argv[2];

  // set up for file read
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  // open wordlist at the file pointer
  fp = fopen(wordlist, "r");
  if (fp == NULL) {
    fprintf(stderr, "No file %s\n", wordlist);
  }

  // for every line
  while ((read = getline(&line, &len, fp)) != -1) {
    //do stuff with line
    get_req(host, line);
  }

  // clean up
  free(line);
  fclose(fp);
  return 0;
}
