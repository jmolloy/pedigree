// crashtest: tests exception handling in the host POSIX subsystem

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

// Don't give a warning for division by zero
#pragma GCC diagnostic ignored "-Wdiv-by-zero"

void sighandler(int sig)
{
    printf("Happily ignoring signal %d\n", sig);
}

int main(int argc, char *argv[])
{
    printf("crashtest: a return value of zero means success\n");
    
    signal(SIGFPE, sighandler);
    signal(SIGILL, sighandler);
    
    // SIGFPE
    printf("Testing SIGFPE...\n");
    int a = 1 / 0;
    float b = 1.0f / 0.0f;
    double c = 1.0 / 0.0;
    printf("Works.\n");
    
    // SIGILL
    printf("Testing SIGILL...\n");
    char badops[] = {0xab, 0xcd, 0xef, 0x12};
    void (*f)() = (void (*)()) badops;
    f();
    printf("Works.\n");
    
    printf("All signals handled.\n");
    
    return 0;
}

