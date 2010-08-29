
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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
     CTRL = 261,
     SHIFT = 262,
     ALT = 263,
     ALTGR = 264,
     CTRL_SHIFT = 265,
     SHIFT_ALT = 266,
     CTRL_ALT = 267,
     CTRL_SHIFT_ALT = 268,
     SHIFT_ALTGR = 269,
     CTRL_ALTGR = 270,
     CTRL_SHIFT_ALTGR = 271,
     SET_COMBINE = 272,
     COMBINE = 273,
     OPEN_SQ = 274,
     CLOSE_SQ = 275,
     DEFINE = 276,
     QUOTED_CHAR = 277,
     CODE_POINT = 278,
     NUM = 279,
     ERROR = 280,
     END = 281
   };
#endif
/* Tokens.  */
#define NEWLINE 258
#define QUOTE 259
#define STRING 260
#define CTRL 261
#define SHIFT 262
#define ALT 263
#define ALTGR 264
#define CTRL_SHIFT 265
#define SHIFT_ALT 266
#define CTRL_ALT 267
#define CTRL_SHIFT_ALT 268
#define SHIFT_ALTGR 269
#define CTRL_ALTGR 270
#define CTRL_SHIFT_ALTGR 271
#define SET_COMBINE 272
#define COMBINE 273
#define OPEN_SQ 274
#define CLOSE_SQ 275
#define DEFINE 276
#define QUOTED_CHAR 277
#define CODE_POINT 278
#define NUM 279
#define ERROR 280
#define END 281




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 31 "parser.y"

    int n;
    char str[256];
    char c;
    struct cmd *cmd;
    struct cmd_list *cmd_list;



/* Line 1676 of yacc.c  */
#line 114 "parser.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


