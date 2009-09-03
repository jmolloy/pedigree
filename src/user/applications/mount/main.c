#include <stdio.h>
#include <stdlib.h>

extern int pedigree_get_mount(char* mount_buf, char* info_buf, size_t n);

int main(void)
{
    size_t i = 0;
    char *mount_buf = (char*)malloc(64), *info_buf = (char*)malloc(128);
    while(!pedigree_get_mount(mount_buf, info_buf, i))
    {
        printf("%s: %s\n", mount_buf, info_buf);
        i++;
    }
    return 0;
}
