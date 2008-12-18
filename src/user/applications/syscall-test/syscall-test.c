#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define PASS 1
#define FAIL 0

int test_open ()
{
  printf ("open():\n");
  printf ("\tOpen existing file - ");
  int fd = open ("/applications/shell", O_RDONLY);
  if (fd != -1)
  {
    printf("PASS\n");
  }
  else
  {
    printf("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  printf ("\tOpen nonexistant file - ");
  int fd2 = open ("/applications/penis", O_RDONLY);
  if (fd2 == -1 && errno == ENOENT)
  {
    printf("PASS\n");
  }
  else
  {
    printf ("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  printf ("\tCreate file - ");
  int fd3 = open ("/file-doesnt-exist", O_RDWR | O_CREAT);
  if (fd3 != -1)
  {
    printf("PASS\n");
  }
  else
  {
    printf("FAIL - errno %d (%s)\n", errno, strerror(errno));
  }

  return PASS;
}

int main (int argc, char **argv)
{
  printf ("Syscall test starting...\n");

  test_open ();
  return 0;

}
