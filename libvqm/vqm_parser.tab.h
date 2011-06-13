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
     TOKEN_MODULE = 258,
     TOKEN_ENDMODULE = 259,
     TOKEN_DEFPARAM = 260,
     TOKEN_ASSIGN = 261,
     TOKEN_INPUT = 262,
     TOKEN_OUTPUT = 263,
     TOKEN_INOUT = 264,
     TOKEN_CONST_0 = 265,
     TOKEN_CONST_1 = 266,
     TOKEN_CONST_Z = 267,
     TOKEN_WIRE = 268,
     TOKEN_STRING = 269,
     TOKEN_BITSTRING = 270,
     TOKEN_REGULARID = 271,
     TOKEN_ESCAPEDID = 272,
     TOKEN_INTCONSTANT = 273,
     TOKEN_WYSIWYGID = 274
   };
#endif
/* Tokens.  */
#define TOKEN_MODULE 258
#define TOKEN_ENDMODULE 259
#define TOKEN_DEFPARAM 260
#define TOKEN_ASSIGN 261
#define TOKEN_INPUT 262
#define TOKEN_OUTPUT 263
#define TOKEN_INOUT 264
#define TOKEN_CONST_0 265
#define TOKEN_CONST_1 266
#define TOKEN_CONST_Z 267
#define TOKEN_WIRE 268
#define TOKEN_STRING 269
#define TOKEN_BITSTRING 270
#define TOKEN_REGULARID 271
#define TOKEN_ESCAPEDID 272
#define TOKEN_INTCONSTANT 273
#define TOKEN_WYSIWYGID 274




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 47 "vqm_parser.y"
{
	int value;
	char *string;
}
/* Line 1489 of yacc.c.  */
#line 92 "vqm_parser.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

