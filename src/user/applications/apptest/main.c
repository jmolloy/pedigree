#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>

#include <signal.h>
#include <syslog.h>
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
    printf("Exploding!\n");
    *((int*) 0) = 1234;
    return 0;

    int i;

    // test multi-threaded FPU code
    if ( fork() == 0 )
    {
        for(i = 0; i < 10; i++)
        {
            float f = i * 12.151;
            printf("fork: %f\n", f);
        }
        
        if ( fork() == 0 )
        {
            for(i = 0; i < 10; i++)
            {
                float f = i * 11.15135;
                printf("fork-fork: %f\n", f);
            }

            return EXIT_SUCCESS;
        }
        else
        {
            for(i = 0; i < 10; i++)
            {
                float f = i * 1.1002;
                printf("fork-parent: %f\n", f);
            }
            
            return EXIT_SUCCESS;
        }

        return EXIT_SUCCESS;
    }
    else
    {
        for(i = 0; i < 10; i++)
        {
            float f = i * 15.151;
            printf("parent: %f\n", f);
        }
        
        return EXIT_SUCCESS;
    }

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

    //while (1)
    //{
        //printf("Press any key to fork and wait.\n");

        //char c = getchar();
        int n;
        if ( (n=fork()) == 0 )
        {
            srand(getpid()*time(NULL));
            unsigned int x = 0;
            while(1)
            {
                unsigned int r = rand()%0xffff;
                double m = sqrt((double)(r*r));
                if(m != r)
                {
                    printf("Child Err, r = %u, m = %f, x = %u.\n", r, m, x);
                    return EXIT_SUCCESS;
                }
                if(x>=0x100000)
                {
                    printf("Child Success, r = %u, m = %f, x = %u.\n", r, m, x);
                    return EXIT_SUCCESS;
                }
                x++;
            }
        }
        else
        {
            srand(getpid()*time(NULL));
            unsigned int x2 = 0;
            while(1)
            {
                unsigned int r2 = rand()%0xffff;
                float m2 = sqrtf((float)(r2*r2));
                if(m2 != r2)
                {
                    printf("Parent Err, r = %u, m = %f, x = %u.\n", r2, m2, x2);
                    wait();
                    return EXIT_SUCCESS;
                }
                if(x2>=0x100000)
                {
                    printf("Parent Success, r = %u, m = %f, x = %u.\n", r2, m2, x2);
                    wait();
                    return EXIT_SUCCESS;
                }
                x2++;
            }
        }
    //}

    return EXIT_SUCCESS;
}
