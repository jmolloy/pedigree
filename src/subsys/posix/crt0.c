extern int write(int,int,int);
extern int open(char*,int,int);
extern int main(int,char**,char**);
extern int _exit(int);
extern int sprintf(char*, char*, ...);
extern void setenv(char*,char*,int);

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
    i  = env;
    while (*i)
    {
      // Save the key.
      char *key = *i;
      char *value = *i;
      // Iterate until we see the end of the string or an '='.
      while (*value && *value != '=') value++;
      // If we found a '=', change it to a NULL terminator (for the key) and increment position.
      if (*value == '=') *value++ = '\0';
      // Set the env var.
      setenv(key, value, 1);
      *i++;
    }
  }

  _exit(main(argc, argv, env));

  // Unreachable.
  for (;;);
}
