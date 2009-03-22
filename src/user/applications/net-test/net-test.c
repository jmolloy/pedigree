#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/** Warning: No real error checking */

#define SERVER 0

int main(int argc, char **argv) {
#if SERVER
  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sock == -1)
  {
    printf("Couldn't get the socket.");
    return 0;
  }
  
  struct sockaddr_in local;
  local.sin_family = AF_INET;
  local.sin_port = htons(80);
  local.sin_addr.s_addr = INADDR_ANY;
  
  int res = bind(sock, (struct sockaddr*) &local, sizeof(local));
  if(res == -1)
  {
    printf("Couldn't bind to an address!\n");
    close(sock);
    return 0;
  }
  
  printf("Listening...\n");
  
  res = listen(sock, 0);
  if(res == -1)
  {
    printf("Couldn't listen\n");
    close(sock);
    return 0;
  }
  
  char* tmp = (char*) malloc(1024);
  
  size_t sz;
  ssize_t n;
  
  struct sockaddr_in remote;
  int client;
  while((client = accept(sock, (struct sockaddr*) &remote, &sz)) >= 0)
  {
    //printf("Accepted connection from %s on port %d\n", inet_ntoa(remote.sin_addr), ntohs(remote.sin_port));
    
    // get the request
    fd_set fd;
    FD_ZERO(&fd);
    FD_SET(client, &fd);

    struct timeval tv;
    tv.tv_sec = 30;
    if(select(sizeof(fd) * 8, &fd, 0, 0, &tv) == 0)
    {
      //printf("Timeout while waiting for data to arrive");
      close(client);
      continue;
    }

    while((n = recv(client, tmp, 1024, 0)) > 0)
    {
      tmp[n] = 0;
      printf("Read %u bytes: %s", n, tmp);
    }
    if(n == -1)
      printf("\nReceive failed");
    //printf("\n");
    
    int go_close = 0;
    if(strcmp(tmp, "quit") == 0)
      go_close = 1;
    
    // send the reply
    strcpy(tmp, "Win");
    sz = strlen(tmp);

    res = send(client, tmp, sz, 0);
    if(res == -1)
    {
      //printf("Sending data failed.\n");
    }
    
    printf("               (Closing... ");
    close(client);
    printf(" Done!)\n");
    
    if(go_close)
      break;
  }
  
  free(tmp);
  
  close(sock);
  
#else

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sock == -1)
  {
    printf("Couldn't get the socket.\n");
    return 0;
  }
  
  fflush(stdout);
  
  struct hostent* ent = gethostbyname("whatismyip.org");
  if(ent == 0)
  {
    close(sock);
    return 0;
  }
  
  struct sockaddr_in remote;
  remote.sin_family = AF_INET;
  remote.sin_port = htons(80);
  remote.sin_addr.s_addr = 0x29ea934b; //inet_addr(ent->h_addr_list[0]);
  
  connect(sock, &remote, sizeof(remote));

  size_t sz, n;
  char* tmp = (char*) malloc(1024);
  strcpy(tmp, "GET / HTTP/1.1\r\nHost: whatismyip.org\r\n\r\n");
  sz = strlen(tmp);

  send(sock, tmp, sz, 0);
  
  fd_set fd;
  FD_ZERO(&fd);
  FD_SET(sock, &fd);

  select(sizeof(fd) * 8, &fd, 0, 0, 0);

  while(n = recv(sock, tmp, 1024, 0))
  {
    tmp[n] = 0;
    printf("Read %u bytes: %s", n, tmp);
  }
  printf("\n");
  
  fflush(stdout);
  
  free(tmp);

  shutdown(sock, SHUT_RDWR);
  
  fflush(stdout);
  
  close(sock);
  
  fflush(stdout);
  
#endif

  return 0;
}
