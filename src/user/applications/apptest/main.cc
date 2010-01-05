#ifdef X86

#include <types.h>
#include <test.h>

int main(int argc, char **argv)
{
    Test whee;
    whee.writeLog("Hello world", 12);
    return 0;
}

#else

// Architectures which don't support the native interface will pretend :)

#include <stdio.h>

int main(int argc, char **argv)
{
    printf("Boring old printf works... No native API on this architecture yet");
    return 0;
}

#endif
