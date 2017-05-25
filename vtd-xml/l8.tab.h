/* 
 * Copyright (C) 2002-2015 XimpleWare, info@ximpleware.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
/*VTD-XML is protected by US patent 7133857, 7260652, an 7761459*/
/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     AXISNAME = 258,
     LITERAL = 259,
     NUMBER = 260,
     NAME = 261,
     FNAME = 262,
     NTEST = 263,
     OR = 264,
     AND = 265,
     EQ = 266,
     NE = 267,
     GT = 268,
     LT = 269,
     GE = 270,
     LE = 271,
     ADD = 272,
     SUB = 273,
     MULT = 274,
     DIV = 275,
     MOD = 276,
     LP = 277,
     RP = 278,
     DOLLAR = 279,
     UNION = 280,
     SLASH = 281,
     DSLASH = 282,
     COMMA = 283,
     ERROR = 284,
     AT = 285,
     DOT = 286,
     DDOT = 287,
     LB = 288,
     RB = 289,
     UMINUS = 290
   };
#endif
#define AXISNAME 258
#define LITERAL 259
#define NUMBER 260
#define NAME 261
#define FNAME 262
#define NTEST 263
#define OR 264
#define AND 265
#define EQ 266
#define NE 267
#define GT 268
#define LT 269
#define GE 270
#define LE 271
#define ADD 272
#define SUB 273
#define MULT 274
#define DIV 275
#define MOD 276
#define LP 277
#define RP 278
#define DOLLAR 279
#define UNION 280
#define SLASH 281
#define DSLASH 282
#define COMMA 283
#define ERROR 284
#define AT 285
#define DOT 286
#define DDOT 287
#define LB 288
#define RB 289
#define UMINUS 290




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 40 "l8.y"
typedef union YYSTYPE {
	UCSChar*   literal;
	axisType at;
	int	 integer;
	double	 number;
	struct nametype {
		UCSChar* prefix;
		UCSChar* qname;
		UCSChar* localname;
	} name;
	struct {
       		nodeTestType nt;
       		UCSChar*   arg;
	} ntest;
	funcName fname;
	expr *expression;
 	locationPathExpr *lpe;
 	unionExpr *une;
	pathExpr *pe;
	Step *s;
	aList *a;
	NodeTest *nodetest;
	Predicate* p;
} YYSTYPE;
/* Line 1248 of yacc.c.  */
#line 131 "l8.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern _thread YYSTYPE yylval;



