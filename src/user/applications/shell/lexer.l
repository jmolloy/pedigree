%{

#include <stdio.h>
#include <unistd.h>
#include "globals.h"
#include "parser.tab.h"

typedef YYSTYPE type_t;

type_t text_var();
type_t num_var();

%}
%option always-interactive
%%

\n                      {return NEWLINE;}
\$                      {return DOLLAR;}
\{                      {return LBRACE;}
\}                      {return RBRACE;}
\&                      {return AMPERSAND;}
"2>"                    {return REDIR_ERR;}
\>                      {return REDIR_OUT;}
\<                      {return REDIR_IN;}
[ \t]                   {}
[.:a-zA-Z0-9\-+\[\]/]+            {yylval = text_var(); return STRING;}
.                       {return ERROR;}


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
