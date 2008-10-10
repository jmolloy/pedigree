extern int write(int,int,int);
extern int open(char*,int,int);
extern int main(int,char**);
extern int _exit(int);

extern char **environ;

void *__gxx_personality_v0;

void _start(char **argv, char **env)
{
  //_init_signal();

  if (write(2,0,0) == -1)
  {
    open("/dev/tty", 0, 0);
    open("/dev/tty", 0, 0);
    open("/dev/tty", 0, 0);
  }

  // Count how many args we have.
  int argc;
  if (argv == 0)
  {
    char *p = 0;
    argv = &p;
    argc = 0;
    environ = &p;
  }
  else
  {
    char **i = argv;
    argc = 0;
    while (*i++)
      argc++;
    environ = env;
  }

  _exit(main(argc, argv));

  // Unreachable.
  for (;;);
}
