#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "globals.h"
#include "parser.tab.h"

extern int yylex();
extern char **environ;
extern YYSTYPE yylval;

int waserr = 0;

void exec_cmd(cmd_t *);

int yywrap()
{
}

int yyerror()
{
  printf("Syntax error!\n");
  waserr = 1;
  return 1;
}

int main(char argc, char **argv)
{
//  setenv("TERM", "vt100", 1);
  while(1)
  {
    
    fprintf(stderr, "PShell $ ");
      yyparse();
      if (waserr == 1)
      {
        waserr = 0;
        continue;
      }
      if (yylval.cmd->nargs > 0)
      {
          if (!strcmp(yylval.cmd->argv[0], "exit"))
              exit(0);
          exec_cmd(yylval.cmd);
      }
  }
}

void resolve_variable(char *src, char *dest)
{
  char *var = (char*)getenv(src);
  if (var)
    strcpy(dest, (const char*)var);
  else
    *dest = 0;
}

void exec_cmd(cmd_t *cmd)
{
  char *argv2[32];
  int idx;
  for(idx = 0; idx < cmd->nargs; idx++)
  {
    argv2[idx] = &cmd->argv[idx][0];
  }
  argv2[cmd->nargs] = (char*)0; // Null-terminate argv.

  int ret = fork();
  if (ret == 0)
  {
      if (strlen(cmd->in))
      {
	int fd = open(cmd->in, O_RDONLY, 0);
          if (fd == -1)
          {
              printf("Error: could not open %s for reading.\n", cmd->in);
              exit(1);
          }
//          dup2(fd, 0); // Change stdin.
      }
      if (strlen(cmd->err))
      {
	int fd = open(cmd->err, O_WRONLY|O_CREAT, 0);
          if (fd == -1)
          {
              printf("Error: could not open %s for writing.\n", cmd->err);
              exit(1);
          }
//          dup2(fd, 2); // Change stderr.
      }
      if (strlen(cmd->out))
      {
	int fd = open(cmd->out, O_WRONLY|O_CREAT, 0);
          if (fd == -1)
          {
              printf("Error: could not open %s for writing.\n", cmd->out);
              exit(1);
          }
//          dup2(fd, 1); // Change stdout.
      }

      execve(argv2[0], argv2, environ);
      fprintf(stderr, "Error executing command: %s.\n", argv2[0]);
      exit(1);
  }

  if (cmd->background)
  {
      printf("[%d] %s &\n", ret, argv2[0]);
  }
  else
  {
      waitpid(ret, NULL);
  }

}
