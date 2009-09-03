%{
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "cmd.h"

extern int yylex(void);
extern int yyerror(const char*);

extern cmd_t *cmds[MAX_CMDS];
extern int n_cmds;

struct cmd_list
{
    struct cmd *cmd;
    struct cmd_list *next;
};

char *quote_char(char c);
cmd_t *make_cmd(unsigned int scancode, unsigned int upoint, char *val, int escape);
void set_cmds(int escape, unsigned int scancode, struct cmd_list *cl);
struct cmd_list *add_to_cmd_list(struct cmd_list *cl, cmd_t *c);

void add_define(char *str, int n);
int lookup_define(char *str);

%}

%union
{
  int n;
  char str[256];
  char c;
  struct cmd *cmd;
  struct cmd_list *cmd_list;
}

%token<str> NEWLINE QUOTE STRING E0 SHIFT CTRL ALT ALTGR SHIFT_CTRL SHIFT_ALT CTRL_ALT SHIFT_CTRL_ALT SHIFT_ALTGR CTRL_ALTGR SHIFT_CTRL_ALTGR SET_COMBINE COMBINE OPEN_SQ CLOSE_SQ DEFINE
%token<c>   QUOTED_CHAR
%token<n>   CODE_POINT NUM
%token      ERROR END

%start command
%type<cmd> point set_combination combine section
%type<cmd_list> command command_part sections
%type<str> string string_internal

%%

command: command_part NEWLINE command
       | NEWLINE command             {}
       | END                         {YYACCEPT;}
       ;

command_part: DEFINE STRING NUM {$$ = 0; add_define($2, $3);}
            | DEFINE STRING CODE_POINT {$$ = 0; add_define($2, $3);}
            | E0  NUM sections {$$ = $3; set_cmds(1, $2, $$);}
            | NUM sections     {$$ = $2; set_cmds(0, $1, $$);}
            | E0 NUM SHIFT     {$$ = 0; cmd_t *c = make_cmd($2, 0, 0, 1); c->set_modifiers = SHIFT_M;}
            | NUM SHIFT        {$$ = 0; cmd_t *c = make_cmd($1, 0, 0, 0); c->set_modifiers = SHIFT_M;}
            | E0 NUM CTRL      {$$ = 0; cmd_t *c = make_cmd($2, 0, 0, 1); c->set_modifiers = CTRL_M;}
            | NUM CTRL         {$$ = 0; cmd_t *c = make_cmd($1, 0, 0, 0); c->set_modifiers = CTRL_M;}
            | E0 NUM ALT       {$$ = 0; cmd_t *c = make_cmd($2, 0, 0, 1); c->set_modifiers = ALT_M;}
            | NUM ALT          {$$ = 0; cmd_t *c = make_cmd($1, 0, 0, 0); c->set_modifiers = ALT_M;}
            | E0 NUM ALTGR     {$$ = 0; cmd_t *c = make_cmd($2, 0, 0, 1); c->set_modifiers = ALTGR_M;}
            | NUM ALTGR        {$$ = 0; cmd_t *c = make_cmd($1, 0, 0, 0); c->set_modifiers = ALTGR_M;}
            ;

sections: /* Null */           {$$ = 0;}
        | section sections     {$$ = add_to_cmd_list($2, $1);}
        ;

section: combine                    {$$ = $1;}
       | SHIFT combine              {$$ = $2; $$->modifiers = SHIFT_I;}
       | CTRL combine               {$$ = $2; $$->modifiers = CTRL_I;}
       | SHIFT_CTRL combine         {$$ = $2; $$->modifiers = SHIFT_I|CTRL_I;}
       | ALT combine                {$$ = $2; $$->modifiers = ALT_I;}
       | ALTGR combine              {$$ = $2; $$->modifiers = ALTGR_I;}
       | SHIFT_ALT combine          {$$ = $2; $$->modifiers = SHIFT_I|ALT_I;}
       | CTRL_ALT combine           {$$ = $2; $$->modifiers = CTRL_I|ALT_I;}
       | SHIFT_CTRL_ALT combine     {$$ = $2; $$->modifiers = SHIFT_I|CTRL_I|ALT_I;}
       | SHIFT_ALTGR combine        {$$ = $2; $$->modifiers = SHIFT_I|ALTGR_I;}
       | CTRL_ALTGR combine         {$$ = $2; $$->modifiers = CTRL_I|ALTGR_I;}
       | SHIFT_CTRL_ALTGR combine   {$$ = $2; $$->modifiers = SHIFT_I|CTRL_I|ALTGR_I;}
       ;

combine: point {$$ = $1;}
       | set_combination {$$ = $1;}
       | COMBINE OPEN_SQ NUM CLOSE_SQ point    {$$ = $5; $$->combinators = $3;}
       | COMBINE OPEN_SQ STRING CLOSE_SQ point {$$ = $5; $$->combinators = lookup_define($3);}
       ;

point: CODE_POINT {$$ = make_cmd(0, $1, (char*)"", 0);}
     | string     {$$ = make_cmd(0, 0,  $1, 0);}
     | STRING     {$$ = make_cmd(0, lookup_define($1), (char*)"", 0);}
     ;

set_combination: SET_COMBINE OPEN_SQ NUM CLOSE_SQ OPEN_SQ CODE_POINT CLOSE_SQ {$$ = make_cmd(0, $6, (char*)"", 0); $$->set_modifiers = $3;}
               | SET_COMBINE OPEN_SQ STRING CLOSE_SQ OPEN_SQ CODE_POINT CLOSE_SQ {$$ = make_cmd(0, $6, (char*)"", 0); $$->set_modifiers = lookup_define($3);}
               ;

string: QUOTE string_internal QUOTE {strcpy($$, $2);}
      ;

string_internal: /* NULL */                   {strcpy($$, "");}
               | STRING string_internal       {strcpy($$, $1); strcat($$, $2);}
               | QUOTED_CHAR string_internal  {strcpy($$, quote_char($1)); strcat($$, $2);}
               ;

%%

char *quote_char(char c)
{
    switch (c)
    {
      case 'e': return (char*)"\e";
      default: return (char*)"?";
    }
}

struct cmd_list *add_to_cmd_list(struct cmd_list *cl, cmd_t *c)
{
    struct cmd_list *cl2 = (struct cmd_list*)malloc(sizeof(struct cmd_list));
    cl2->cmd = c;
    cl2->next = 0;

    if (cl == 0)
        return cl2;
    else
    {
        struct cmd_list *cl3 = cl;
        while (cl3->next)
            cl3 = cl3->next;
        cl3->next = cl2;
        return cl;
    }
}

cmd_t *make_cmd(unsigned int scancode, unsigned int unicode_point, char *val, int escape)
{
  cmd_t *c = (cmd_t*)malloc(sizeof(cmd_t));
  c->scancode = scancode;
  c->unicode_point = unicode_point;
  if (val)
    c->val = strdup(val);
  else
    c->val = 0;
  c->escape = escape;
  c->modifiers = 0;

  cmds[n_cmds++] = c;
  return c;
}

void set_cmds(int escape, unsigned int scancode, struct cmd_list *cl)
{
    while (cl)
    {
        cl->cmd->escape = escape;
        cl->cmd->scancode = scancode;
        cl = cl->next;
    }
}

struct def
{
    char str[32];
    int n;
} defines[1024];
int ndefines = 0;

void add_define(char *str, int n)
{
    memset(defines[ndefines].str, 0, 32);
    strcpy(defines[ndefines].str, str);
    defines[ndefines++].n = n;
}

int lookup_define(char *str)
{
    int i;
    for (i = 0; i < ndefines; i++)
    {
        if (!strcmp(defines[i].str, str))
            return defines[i].n;
    }
    fprintf(stderr, "Error: define `%s' not found.\n", str);
    exit(1);
}
