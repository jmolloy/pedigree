#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/stat.h>

int open(const char*,int,int);

int main(int argc, char **argv)
{
//  struct termios t;
//  tcgetattr(0, &t);
//  t.c_lflag &= ~ECHONL;
//  tcsetattr(0, 0, &t);

//  fprintf(stderr, "argc: %d, env: %s\n", argc, getenv("TERM"));

//  struct stat buf;
//  int a = fstat(0,&buf);

//  fprintf(stderr, "fstat returned %d, mode: %x\n",a, buf.st_mode);

//  int fd = open("./shell", 0, 0);
//  fprintf(stderr, "stdin: %d, shell: %d\n", isatty(0), isatty(fd));

//   int fd = open("/etc/passwd", 0, 0);
//   unsigned int seed = 0x5a5a5a5a;
//   unsigned char *buf = (unsigned char*)malloc(1026);
//   buf[0] = 0xEE;
//   buf[1025] = 0xEE;
//   while (1)
//   {
//     seed ^= 0x462925a8;
//     unsigned int start = seed&0xFF;

//     lseek(fd, 0, 0);
//     read(fd, &buf[1], 0x18);

//     if (buf[0] != 0xEE)
//     {
//       fprintf(stderr, "Front corruption!: %x\n", buf[0]);
//       exit(1);
//     }
//     if (buf[1025] != 0xEE)
//     {
//       fprintf(stderr, "Rear corruption!\n");
//       exit(1);
//     }
// //    fprintf(stderr, "OK\n");
//   }

  chdir("applications");
  char b[512];
  char *a = getcwd(b,512);
  fprintf(stderr,"cwd: %s\n", b);

  fprintf(stderr,"attempting to page fault!\n");
  char *x = (char*)0;
  x[1] = 'p';
  return 0xdeadbaba;
}
