%{
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "cmd.h"

extern cmd_t *cmds[MAX_CMDS];
extern int n_cmds;

char *quote_char(char c);
cmd_t *make_cmd(unsigned int scancode, unsigned int upoint, char *val, int escape);
 void set_cmds(int escape, unsigned int scancode, cmd_t *n, cmd_t *s, cmd_t *c, cmd_t *sc, cmd_t *a, cmd_t *sa, cmd_t *ca, cmd_t *sca, cmd_t *agr, cmd_t *sagr, cmd_t *cagr, cmd_t *scagr);

%}

%union
{
  int n;
  char str[256];
  char c;
  struct cmd *cmd;
}

%token<str> NEWLINE QUOTE STRING E0 SHIFT CTRL ALT ALTGR SHIFT_CTRL SHIFT_ALT CTRL_ALT SHIFT_CTRL_ALT SHIFT_ALTGR CTRL_ALTGR SHIFT_CTRL_ALTGR
%token<c>   QUOTED_CHAR
%token<n>   CODE_POINT NUM
%token      ERROR END

%start command
%type<cmd> command command_part point shift ctrl alt altgr shift_ctrl shift_alt ctrl_alt shift_ctrl_alt shift_altgr ctrl_altgr shift_ctrl_altgr
%type<str> string string_internal

%%

command: command_part NEWLINE command
       | NEWLINE command             {}
       | END                         {YYACCEPT;}
       ;

command_part: E0  NUM point shift ctrl shift_ctrl alt shift_alt ctrl_alt shift_ctrl_alt altgr shift_altgr ctrl_altgr shift_ctrl_altgr {$$ = $3; set_cmds(1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14);}
| NUM point shift ctrl shift_ctrl alt shift_alt ctrl_alt shift_ctrl_alt altgr shift_altgr ctrl_altgr shift_ctrl_altgr     {$$ = $2; set_cmds(0, $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13);}
            | E0 NUM SHIFT     {$$ = make_cmd($2, 0, 0, 1); $$->set_modifiers = SHIFT_F;}
            | NUM SHIFT        {$$ = make_cmd($1, 0, 0, 0); $$->set_modifiers = SHIFT_F;}
            | E0 NUM CTRL      {$$ = make_cmd($2, 0, 0, 1); $$->set_modifiers = CTRL_F;}
            | NUM CTRL         {$$ = make_cmd($1, 0, 0, 0); $$->set_modifiers = CTRL_F;}
            | E0 NUM ALT       {$$ = make_cmd($2, 0, 0, 1); $$->set_modifiers = ALT_F;}
            | NUM ALT          {$$ = make_cmd($1, 0, 0, 0); $$->set_modifiers = ALT_F;}
            | E0 NUM ALTGR     {$$ = make_cmd($2, 0, 0, 1); $$->set_modifiers = ALTGR_F;}
            | NUM ALTGR        {$$ = make_cmd($1, 0, 0, 0); $$->set_modifiers = ALTGR_F;}
            ;

point: CODE_POINT {$$ = make_cmd(0, $1, "", 0);}
     | string     {$$ = make_cmd(0, 0,  $1, 0);}
     ;

shift: /* Empty */ {$$ = 0;}
     | SHIFT point {$$ = $2; $$->modifiers = SHIFT_F;}
     ;

ctrl: /* Empty */ {$$ = 0;}
    | CTRL point {$$ = $2; $$->modifiers = CTRL_F;}
    ;

alt: /* Empty */ {$$ = 0;}
   | ALT point {$$ = $2; $$->modifiers = ALT_F;}
   ;

altgr: /* Empty */ {$$ = 0;}
     | ALTGR point {$$ = $2; $$->modifiers = ALTGR_F;}
     ;

shift_ctrl: /* Empty */ {$$ = 0;}
          | SHIFT_CTRL point {$$ = $2; $$->modifiers = SHIFT_F|CTRL_F;}
          ;

shift_alt: /* Empty */ {$$ = 0;}
          | SHIFT_ALT point {$$ = $2; $$->modifiers = SHIFT_F|ALT_F;}
          ;

ctrl_alt: /* Empty */ {$$ = 0;}
          | CTRL_ALT point {$$ = $2; $$->modifiers = CTRL_F|ALT_F;}
          ;

shift_ctrl_alt: /* Empty */ {$$ = 0;}
              | SHIFT_CTRL_ALT point {$$ = $2; $$->modifiers = CTRL_F|ALT_F;}
              ;

shift_altgr: /* Empty */ {$$ = 0;}
          | SHIFT_ALTGR point {$$ = $2; $$->modifiers = SHIFT_F|ALTGR_F;}
          ;

ctrl_altgr: /* Empty */ {$$ = 0;}
          | CTRL_ALTGR point {$$ = $2; $$->modifiers = CTRL_F|ALTGR_F;}
          ;

shift_ctrl_altgr: /* Empty */ {$$ = 0;}
              | SHIFT_CTRL_ALTGR point {$$ = $2; $$->modifiers = CTRL_F|ALTGR_F;}
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
      case 'e': return "\e";
      default: return "?";
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

void set_cmds(int escape, unsigned int scancode, cmd_t *n, cmd_t *s, cmd_t *c, cmd_t *sc, cmd_t *a, cmd_t *sa, cmd_t *ca, cmd_t *sca, cmd_t *agr, cmd_t *sagr, cmd_t *cagr, cmd_t *scagr)
{
    if (n)
    {
        n->escape = escape;
        n->scancode = scancode;
    }
    if (s)
    {
        s->escape = escape;
        s->scancode = scancode;
    }
    if (c)
    {
        c->escape = escape;
        c->scancode = scancode;
    }
    if (sc)
    {
        sc->escape = escape;
        sc->scancode = scancode;
    }
    if (a)
    {
        a->escape = escape;
        a->scancode = scancode;
    }
    if (sa)
    {
        sa->escape = escape;
        sa->scancode = scancode;
    }
    if (ca)
    {
        ca->escape = escape;
        ca->scancode = scancode;
    }
    if (sca)
    {
        sca->escape = escape;
        sca->scancode = scancode;
    }
    if (agr)
    {
        agr->escape = escape;
        agr->scancode = scancode;
    }
    if (sagr)
    {
        sagr->escape = escape;
        sagr->scancode = scancode;
    }
    if (cagr)
    {
        cagr->escape = escape;
        cagr->scancode = scancode;
    }
    if (scagr)
    {
        scagr->escape = escape;
        scagr->scancode = scancode;
    }
}
