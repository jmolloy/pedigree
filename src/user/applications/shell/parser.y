%{
#include <stdio.h>
#include <unistd.h>
#include "globals.h"

extern void resolve_variable(char *src, char *dest);

extern void exec_cmd(cmd_t *);
cmd_t *make_cmd();
cmd_t *add_arg(cmd_t*, char*);
cmd_t *merge_cmd(cmd_t*,cmd_t*);
cmd_t *link_cmds(cmd_t*,cmd_t*);
cmd_t *pipe_cmds(cmd_t*,cmd_t*);
%}

%union
{
  int n;
  char str[256];
  struct cmd *cmd;
}

%token<str> STRING DOLLAR LBRACE RBRACE NEWLINE AMPERSAND ERROR REDIR_OUT REDIR_IN REDIR_ERR
%start command
%type<cmd> command command_part
%type<str> variable

%%

command: command_part NEWLINE                {yylval.cmd=$$;YYACCEPT;} 
       ;

command_part:                            {$$ = make_cmd();}
            | STRING command_part       {$$ = make_cmd();
                                         $$ = add_arg($$, (char*)&$1);
                                         $$ = merge_cmd($$, $2);}
            | variable command_part     {$$ = make_cmd();
                                         $$ = add_arg($$, (char*)&$1);
                                         $$ = merge_cmd($$, $2);}
            | AMPERSAND command_part    {$$ = $2;
                                         $$->background = 1;}
            | REDIR_ERR STRING command_part {$$ = $3;
                                             strcpy($$->err, $2);}
            | REDIR_OUT STRING command_part {$$ = $3;
                                             strcpy($$->out, $2);}
            | REDIR_IN STRING command_part {$$ = $3;
                                            strcpy($$->in, $2);}
            ; 

variable: DOLLAR LBRACE STRING RBRACE   {resolve_variable((char*)&$3, (char*)&$$);}
        | DOLLAR STRING                 {resolve_variable((char*)&$2, (char*)&$$);}
        ;

%%

cmd_t *make_cmd()
{
	cmd_t *toRet = (cmd_t*)malloc(sizeof(cmd_t));
	toRet->nargs = 0;
	toRet->in[0] = toRet->out[0] = toRet->err[0] = '\0';
	toRet->background = 0;
	return toRet;
}

cmd_t *add_arg(cmd_t *cmd, char *arg)
{
	strcpy(cmd->argv[cmd->nargs], arg);
	cmd->nargs++;
	return cmd;
}

cmd_t *merge_cmd(cmd_t *cmd1, cmd_t *cmd2)
{
	int n1=cmd1->nargs,n2=0;
	for(n2 = 0; n2 < cmd2->nargs; n1++,n2++)
	{
		strcpy(cmd1->argv[n1], cmd2->argv[n2]);
	}
	cmd1->nargs += cmd2->nargs;
	if (cmd2->background > 0)
	{
		cmd1->background = 1;
	}
        if (strlen(cmd2->in))
        {
            strcpy(cmd1->in, cmd2->in);
        }
        if (strlen(cmd2->out))
        {
            strcpy(cmd1->out, cmd2->out);
        }
        if (strlen(cmd2->err))
        {
            strcpy(cmd1->err, cmd2->err);
        }
        free(cmd2);
	return cmd1;
}
