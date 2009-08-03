#ifndef _SYS_UTIME_H
#define _SYS_UTIME_H

#ifdef __cplusplus
extern "C" {
#endif

struct utimbuf
{
  time_t actime;
  time_t modtime;
};

int utime(const char *path, const struct utimbuf *times);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_UTIME_H */
