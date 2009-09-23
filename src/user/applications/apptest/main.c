#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <signal.h>
#include <sys/termios.h>
#include <sys/time.h>

static void wait_thread(void)
{
    time_t start_time = time(NULL);
 
    while (time(NULL) == start_time)
    {
        // do nothing except chew CPU slices for up to one second.
    }
}

volatile int whee = 0;
 
static void *thread_func(void *vptr_args)
{
    int i;

    printf("Hello world from thread %d!\n", pthread_self());
 
    for (i = 0; i < 80; i++)
    {
        //printf(" b\n");
        //while(!whee);
        //whee = 1;
        fputs("b", stdout);
        //whee = 0;
        // wait_thread();
    }
 
    return NULL;
}
 
int main(void)
{
/*    int i;
    pthread_t thread;

    int whee;
    int* p = &whee;
*/
//    if (pthread_create(&thread, NULL, thread_func, NULL) != 0)
//    {
//        printf("Fail: %s\n", strerror(errno));
//        return EXIT_FAILURE;
//    }
 
    /*for (i = 0; i < 20; i++)
    {
        while(whee);
        whee = 0;
        fputs("a", stdout);
        whee = 1;
        // wait_thread();
    }*/
 
//    if (pthread_join(thread, NULL) != 0)
//    {
//        return EXIT_FAILURE;
//    }

//    printf("\nAnd now main() has waited for the thread to run.\n");
    struct termios t;
    tcgetattr(0, &t);

    struct termios oldt = t;
    t.c_lflag &= ~(ECHO|ECHOE|ECHOK|ICANON|IEXTEN);
    tcsetattr(0, 0, &t);

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    struct timeval ts;
    ts.tv_sec = 5;
    ts.tv_usec = 0;
    int ret = select(1, &fds, 0, 0, &ts);

    tcsetattr(0, 0, &oldt);

    printf("ret: %d\n", ret);
    return EXIT_SUCCESS;
}
