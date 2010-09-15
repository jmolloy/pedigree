#ifndef SYS_DIRENT_H
#define SYS_DIRENT_H

#include <sys/types.h>

#define MAXNAMLEN 255

struct dirent
{
  char d_name[MAXNAMLEN];
  ino_t d_ino;
  int d_type;
};

typedef struct ___DIR
{
  int fd;
  struct dirent ent;
} DIR;


#define __dirfd(dir) ((dir)->fd)

#ifdef __cplusplus
extern "C" 
{
#endif

DIR *opendir(const char*);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);
int closedir(DIR *);

#ifdef __cplusplus
}
#endif

#endif
