%{

#include <stdio.h>
#include <unistd.h>
#include "cmd.h"
#include "parser.h"

typedef YYSTYPE type_t;

type_t text_var();
type_t num_var();
type_t unicode_var();
type_t hex_var();

%}

%%

#.*$                    {}
\n                      {return NEWLINE;}
\\.                     {yylval.c = yytext[1]; return QUOTED_CHAR;}
\"                      {return QUOTE;}
U\+[0-9a-fA-F]+         {yylval = unicode_var(); return CODE_POINT;}
CTRL                    {return CTRL;}
SHIFT                   {return SHIFT;}
CTRL-SHIFT              {return CTRL_SHIFT;}
ALT                     {return ALT;}
ALTGR                   {return ALTGR;}
CTRL-ALT                {return CTRL_ALT;}
SHIFT-ALT               {return SHIFT_ALT;}
CTRL-SHIFT-ALT          {return CTRL_SHIFT_ALT;}
CTRL-ALTGR              {return CTRL_ALTGR;}
SHIFT-ALTGR             {return SHIFT_ALTGR;}
CTRL-SHIFT-ALTGR        {return CTRL_SHIFT_ALTGR;}
COMBINE                 {return COMBINE;}
SET-COMBINE             {return SET_COMBINE;}
DEFINE                  {return DEFINE;}
\[                      {return OPEN_SQ;}
\]                      {return CLOSE_SQ;}
0x[0-9A-Fa-f]+          {yylval = hex_var(); return NUM;}
[0-9]+                  {yylval = num_var(); return NUM;}
[a-zA-Z0-9_-]+          {yylval = text_var(); return STRING;}
[ \t]                   {}
<<eof>>                 {return END;}

%%

type_t text_var()
{
    type_t toRet;
    strncpy(toRet.str, yytext, yyleng);
    toRet.str[yyleng] = '\0';
    return toRet;
}

type_t num_var()
{
    type_t toRet;
    toRet.n = atoi(yytext);
    return toRet;
}

type_t unicode_var()
{
    type_t toRet;
    toRet.n = strtoul(yytext+2, 0, 16);
    return toRet;
}

type_t hex_var()
{
    type_t toRet;
    toRet.n = strtoul(yytext, 0, 16);
    return toRet;
}

