%{

#include <stdio.h>
#include <unistd.h>
#include "cmd.h"
#include "parser.tab.h"

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
ESC                     {return E0;}
SHIFT-CTRL              {return SHIFT_CTRL;}
SHIFT-ALT               {return SHIFT_ALT;}
CTRL-ALT                {return CTRL_ALT;}
SHIFT-CTRL-ALT          {return SHIFT_CTRL_ALT;}
SHIFT-ALTGR             {return SHIFT_ALTGR;}
CTRL-ALTGR              {return CTRL_ALTGR;}
SHIFT-CTRL-ALTGR        {return SHIFT_CTRL_ALTGR;}
SHIFT                   {return SHIFT;}
CTRL                    {return CTRL;}
ALT                     {return ALT;}
ALTGR                   {return ALTGR;}
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

