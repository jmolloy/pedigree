#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define SOCKET_ERROR        -1
#define BUFFER_SIZE         255
#define HOST_NAME_SIZE      255

#define htons(x) ( ((x&0xFF)<<8) | ((x&0xFF00)>>8) )

int  main(int argc, char* argv[])
{
    int hSocket;                 /* handle to socket */
    struct hostent *pHostInfo;   /* holds info about a machine */
    struct sockaddr_in Address;  /* Internet socket address stuct */
    long nHostAddress;
    char pBuffer[BUFFER_SIZE];
    unsigned nReadAmount;
    char strHostName[HOST_NAME_SIZE];
    int nHostPort;

    if(argc < 2)
    {
      printf("\nUsage: apptest host-name\n");
      return 0;
    }
    else
    {
      strcpy(strHostName,argv[1]);
      nHostPort=80;
    }

    printf("\nMaking a socket\n");

    /* make a socket */
    hSocket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    if(hSocket == SOCKET_ERROR)
    {
        printf("\nCould not make a socket\n");
        return 0;
    }

    /* get IP address from name */
    //pHostInfo=gethostbyname(strHostName);
    /* copy address into long */
    //memcpy(&nHostAddress,pHostInfo->h_addr_list[0],pHostInfo->h_length);
    unsigned char ip[] = {172, 30, 1, 198};
    memcpy(&nHostAddress,ip,4);

    /* fill address struct */
    Address.sin_addr.s_addr=nHostAddress;
    Address.sin_port=htons(nHostPort);
    Address.sin_family=AF_INET;

    printf("\nConnecting to %s on port %d\n",strHostName,nHostPort);

    /* connect to host */
    if(connect(hSocket,(struct sockaddr*)&Address,sizeof(Address))
       == SOCKET_ERROR)
    {
        printf("\nCould not connect to host\n");
        return 0;
    }

    printf("Sending HTTP request...\n");

    char *buffer = "GET / HTTP/1.0\r\n\r\n";

    send(hSocket,buffer,strlen(buffer), 0);


    /* read from socket into buffer
    ** number returned by read() and write() is the number of bytes
    ** read or written, with -1 being that an error occured */

    nReadAmount=recv(hSocket,pBuffer,BUFFER_SIZE,0);
    printf("\nReceived \"%s\" from server: %d\n",pBuffer,nReadAmount);

    printf("\nClosing socket\n");
    /* close socket */
    if(close(hSocket) == SOCKET_ERROR)
    {
        printf("\nCould not close socket\n");
        return 0;
    }
}
