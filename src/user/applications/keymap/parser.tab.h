/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEWLINE = 258,
     QUOTE = 259,
     STRING = 260,
     E0 = 261,
     SHIFT = 262,
     CTRL = 263,
     ALT = 264,
     ALTGR = 265,
     SHIFT_CTRL = 266,
     SHIFT_ALT = 267,
     CTRL_ALT = 268,
     SHIFT_CTRL_ALT = 269,
     SHIFT_ALTGR = 270,
     CTRL_ALTGR = 271,
     SHIFT_CTRL_ALTGR = 272,
     QUOTED_CHAR = 273,
     CODE_POINT = 274,
     NUM = 275,
     ERROR = 276,
     END = 277
   };
#endif
/* Tokens.  */
#define NEWLINE 258
#define QUOTE 259
#define STRING 260
#define E0 261
#define SHIFT 262
#define CTRL 263
#define ALT 264
#define ALTGR 265
#define SHIFT_CTRL 266
#define SHIFT_ALT 267
#define CTRL_ALT 268
#define SHIFT_CTRL_ALT 269
#define SHIFT_ALTGR 270
#define CTRL_ALTGR 271
#define SHIFT_CTRL_ALTGR 272
#define QUOTED_CHAR 273
#define CODE_POINT 274
#define NUM 275
#define ERROR 276
#define END 277




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 17 "parser.y"
{
  int n;
  char str[256];
  char c;
  struct cmd *cmd;
}
/* Line 1489 of yacc.c.  */
#line 100 "parser.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

