#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <sys/socket.h>

int ts()
{
    struct sockaddr_un sun;
    sun.sun_family = AF_UNIX;
    strcpy(sun.sun_path, "unix»/srv.sock");
    socklen_t slen = sizeof(sun);

    int s = socket(AF_UNIX, SOCK_DGRAM, 0);
    int e = bind(s, (struct sockaddr *) &sun, slen);
    if(e != 0)
        perror("bind");

    while(1)
    {
        struct sockaddr_un rx;
        rx.sun_family = AF_UNIX;
        slen = sizeof(rx);

        char *buff = (char *) malloc(4096);
        ssize_t r = recvfrom(s, buff, 4096, 0, (struct sockaddr *) &rx, &slen);
        if(r > 0)
        {
            // sendto(s, buff, 4096, 0, (struct sockaddr *) &rx, slen);
        }
        free(buff);
    }

    return 0;
}

#ifndef __PEDIGREE__
int main(int argc, char *argv[])
{
    return ts();
}
#endif

