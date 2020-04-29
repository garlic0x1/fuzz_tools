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

char* remove_whitespace(char* name)
{
	int i = 0;
    while(name[i] != '\0')
    {
        i++;
    }
    name[i-1] = '\0';
    return name;
}

int get_req(char *host, char *page) {

  enum CONSTEXPR { MAXLINE = 100};
  char request[MAXLINE + 1], response[MAXLINE + 1];
  char *ptr;

  in_addr_t in_addr;
  struct protoent *protoent;
  struct hostent *hostent;
  struct sockaddr_in sockaddr_in;

  int sockfd;
  size_t n;

  unsigned short port = 443;

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
         "Host: %s\n"
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

  // init openssl
  SSL_load_error_strings();
  SSL_library_init();
  SSL_CTX *ssl_ctx = SSL_CTX_new (SSLv23_client_method ());

  // create ssl connection and attatch to socket
  SSL *conn = SSL_new(ssl_ctx);
  SSL_set_fd(conn,sockfd);

  //perform handshake
  int err = SSL_connect(conn);
  if (err != 1) {
    perror("failed handshake\n");
    exit(EXIT_FAILURE);
  }

  SSL_write(conn, request, 256);
  SSL_read(conn, response, MAXLINE);
  printf("SSL_read():\n%s\n",response);

}


int main(int argc, char **argv) {

  //set host and wordlist from user options
  char *hostlist = argv[1];
  char *path = argv[2];

  // set up for file read
  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  // open wordlist at the file pointer
  fp = fopen(hostlist, "r");
  if (fp == NULL) {
    fprintf(stderr, "No file %s\n", hostlist);
  }

  // for every line
  while ((read = getline(&line, &len, fp)) != -1) {
    //do stuff with line
    remove_whitespace(line);
    get_req(line, path);
  }

  // clean up
  free(line);
  fclose(fp);
  return 0;
}
