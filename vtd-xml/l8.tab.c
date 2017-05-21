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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "l8.y"

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
#include "customTypes.h"
#include <string.h>
#include <stdio.h>
#include "xpath.h"
#include "helper.h"

_thread Step *tmpStep;
_thread Step *tmpStep2;
_thread NodeTest *tmpNt;
_thread locationPathExpr *tmpLPExpr ;
_thread aList *tmpAl;
_thread Predicate *tmpP;

static _thread expr *x;
static _thread NsList *xpathNSList = NULL;
static _thread ExprList *xpathExprList = NULL;
extern _thread int isName;
extern _thread int colonPosition;
_thread expr* ex;

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

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
/* Line 191 of yacc.c.  */
#line 209 "l8.tab.c"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 221 "l8.tab.c"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  43
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   105

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  36
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  30
/* YYNRULES -- Number of rules. */
#define YYNRULES  67
/* YYNRULES -- Number of states. */
#define YYNSTATES  98

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     5,     7,    11,    13,    17,    19,    23,
      27,    29,    33,    37,    41,    45,    47,    51,    55,    57,
      61,    65,    69,    71,    74,    76,    80,    82,    84,    88,
      92,    94,    97,    99,   103,   105,   107,   109,   111,   116,
     117,   119,   123,   125,   127,   129,   131,   134,   136,   138,
     142,   144,   148,   150,   152,   154,   155,   158,   160,   162,
     163,   165,   167,   169,   172,   176,   180,   183
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      37,     0,    -1,    38,    -1,    39,    -1,    38,     9,    39,
      -1,    40,    -1,    39,    10,    40,    -1,    41,    -1,    40,
      11,    41,    -1,    40,    12,    41,    -1,    42,    -1,    41,
      14,    42,    -1,    41,    13,    42,    -1,    41,    16,    42,
      -1,    41,    15,    42,    -1,    43,    -1,    42,    17,    43,
      -1,    42,    18,    43,    -1,    44,    -1,    43,    19,    44,
      -1,    43,    20,    44,    -1,    43,    21,    44,    -1,    45,
      -1,    18,    44,    -1,    46,    -1,    46,    25,    45,    -1,
      52,    -1,    47,    -1,    47,    26,    54,    -1,    47,    27,
      54,    -1,    48,    -1,    47,    63,    -1,    64,    -1,    22,
      37,    23,    -1,     4,    -1,     5,    -1,    49,    -1,    29,
      -1,    65,    22,    50,    23,    -1,    -1,    51,    -1,    51,
      28,    50,    -1,    37,    -1,    54,    -1,    53,    -1,    26,
      -1,    26,    54,    -1,    61,    -1,    55,    -1,    55,    26,
      54,    -1,    62,    -1,    58,    56,    57,    -1,    60,    -1,
       6,    -1,     8,    -1,    -1,    63,    57,    -1,     3,    -1,
      59,    -1,    -1,    30,    -1,    31,    -1,    32,    -1,    27,
      54,    -1,    55,    27,    54,    -1,    33,    37,    34,    -1,
      24,     6,    -1,     7,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    94,    94,    97,    98,   111,   112,   124,   125,   135,
     147,   148,   158,   168,   178,   190,   191,   201,   213,   214,
     224,   234,   246,   247,   260,   270,   287,   288,   289,   307,
     335,   336,   349,   350,   351,   360,   368,   369,   377,   389,
     390,   399,   412,   416,   428,   443,   444,   445,   448,   449,
     453,   457,   481,   485,   509,   531,   532,   537,   538,   541,
     542,   545,   559,   577,   595,   617,   629,   648
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "AXISNAME", "LITERAL", "NUMBER", "NAME", 
  "FNAME", "NTEST", "OR", "AND", "EQ", "NE", "GT", "LT", "GE", "LE", 
  "ADD", "SUB", "MULT", "DIV", "MOD", "LP", "RP", "DOLLAR", "UNION", 
  "SLASH", "DSLASH", "COMMA", "ERROR", "AT", "DOT", "DDOT", "LB", "RB", 
  "UMINUS", "$accept", "Expr", "OrExpr", "AndExpr", "EqualityExpr", 
  "RelationalExpr", "AdditiveExpr", "MultiplicativeExpr", "UnaryExpr", 
  "UnionExpr", "PathExpr", "FilterExpr", "PrimaryExpr", "FunctionCall", 
  "ArgumentList", "Argument", "LocationPath", "AbsoluteLocationPath", 
  "RelativeLocationPath", "Step", "NodeTest", "PredicateList", 
  "AxisSpecifier", "AbbreviatedAxisSpecifier", "AbbreviatedStep", 
  "AbbreviatedAbsoluteLocationPath", "AbbreviatedRelativeLocationPath", 
  "Predicate", "VariableReference", "FunctionName", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    36,    37,    38,    38,    39,    39,    40,    40,    40,
      41,    41,    41,    41,    41,    42,    42,    42,    43,    43,
      43,    43,    44,    44,    45,    45,    46,    46,    46,    46,
      47,    47,    48,    48,    48,    48,    48,    48,    49,    50,
      50,    50,    51,    52,    52,    53,    53,    53,    54,    54,
      54,    55,    55,    56,    56,    57,    57,    58,    58,    59,
      59,    60,    60,    61,    62,    63,    64,    65
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     3,     1,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     3,     1,     2,     1,     3,     1,     1,     3,     3,
       1,     2,     1,     3,     1,     1,     1,     1,     4,     0,
       1,     3,     1,     1,     1,     1,     2,     1,     1,     3,
       1,     3,     1,     1,     1,     0,     2,     1,     1,     0,
       1,     1,     1,     2,     3,     3,     2,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
      59,    57,    34,    35,    67,    59,    59,     0,    45,    59,
      37,    60,    61,    62,     0,     2,     3,     5,     7,    10,
      15,    18,    22,    24,    27,    30,    36,    26,    44,    43,
      48,     0,    58,    52,    47,    50,    32,     0,    23,     0,
      66,    46,    63,     1,    59,    59,    59,    59,    59,    59,
      59,    59,    59,    59,    59,    59,    59,    59,    59,    59,
      59,    31,    59,    59,    53,    54,    55,    59,    33,     4,
       6,     8,     9,    12,    11,    14,    13,    16,    17,    19,
      20,    21,    25,    28,    29,     0,    49,    64,    51,    55,
      42,     0,    40,    65,    56,    38,    59,    41
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,    90,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    91,    92,    27,    28,    29,    30,
      66,    88,    31,    32,    33,    34,    35,    89,    36,    37
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -17
static const yysigned_char yypact[] =
{
      43,   -17,   -17,   -17,   -17,    43,    43,     1,     9,    13,
     -17,   -17,   -17,   -17,    14,    22,    24,    15,     7,    36,
     -16,   -17,   -17,    17,    -8,   -17,   -17,   -17,   -17,   -17,
      66,    74,   -17,   -17,   -17,   -17,   -17,    27,   -17,    45,
     -17,   -17,   -17,   -17,    43,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    43,    59,    13,    13,
      43,   -17,    13,    13,   -17,   -17,    38,     6,   -17,    24,
      15,     7,     7,    36,    36,    36,    36,   -16,   -16,   -17,
     -17,   -17,   -17,   -17,   -17,    50,   -17,   -17,   -17,    38,
     -17,    64,    70,   -17,   -17,   -17,     6,   -17
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -17,     0,   -17,    55,    56,    48,    28,    44,     3,    46,
     -17,   -17,   -17,   -17,     4,   -17,   -17,   -17,    -7,   -17,
     -17,    16,   -17,   -17,   -17,   -17,   -17,    78,   -17,   -17
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -60
static const yysigned_char yytable[] =
{
      14,    41,    42,    54,    55,    56,    39,    40,    38,     1,
       2,     3,     1,     4,    43,   -59,     1,   -59,    58,    59,
      48,    49,    50,    51,     5,    60,    46,    47,     6,   -39,
       7,    44,     8,     9,    45,    10,    11,    12,    13,    11,
      12,    13,    57,    11,    12,    13,     1,     2,     3,    67,
       4,    83,    84,    52,    53,    86,    87,    79,    80,    81,
      85,     5,     1,     2,     3,     6,     4,     7,    68,     8,
       9,    60,    10,    11,    12,    13,    73,    74,    75,    76,
      64,     6,    65,     7,    93,     8,     9,    95,    10,    11,
      12,    13,    62,    63,    71,    72,    77,    78,    96,    69,
      97,    70,    61,    82,     0,    94
};

static const yysigned_char yycheck[] =
{
       0,     8,     9,    19,    20,    21,     6,     6,     5,     3,
       4,     5,     3,     7,     0,     6,     3,     8,    26,    27,
      13,    14,    15,    16,    18,    33,    11,    12,    22,    23,
      24,     9,    26,    27,    10,    29,    30,    31,    32,    30,
      31,    32,    25,    30,    31,    32,     3,     4,     5,    22,
       7,    58,    59,    17,    18,    62,    63,    54,    55,    56,
      60,    18,     3,     4,     5,    22,     7,    24,    23,    26,
      27,    33,    29,    30,    31,    32,    48,    49,    50,    51,
       6,    22,     8,    24,    34,    26,    27,    23,    29,    30,
      31,    32,    26,    27,    46,    47,    52,    53,    28,    44,
      96,    45,    24,    57,    -1,    89
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,     3,     4,     5,     7,    18,    22,    24,    26,    27,
      29,    30,    31,    32,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    52,    53,    54,
      55,    58,    59,    60,    61,    62,    64,    65,    44,    37,
       6,    54,    54,     0,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    25,    26,    27,
      33,    63,    26,    27,     6,     8,    56,    22,    23,    39,
      40,    41,    41,    42,    42,    42,    42,    43,    43,    44,
      44,    44,    45,    54,    54,    37,    54,    54,    57,    63,
      37,    50,    51,    34,    57,    23,    28,    50
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
_thread int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
_thread int yychar;

/* The semantic value of the lookahead symbol.  */
_thread YYSTYPE yylval;

/* Number of syntax errors so far.  */
_thread int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  exception e;
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 94 "l8.y"
    { x=yyval.expression = yyvsp[0].expression;;}
    break;

  case 3:
#line 97 "l8.y"
    {yyval.expression = yyvsp[0].expression;;}
    break;

  case 4:
#line 98 "l8.y"
    {
								Try {
										yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression, OP_OR,yyvsp[0].expression);
										addObj(yyval.expression);
									}
								Catch(e){
									//freeAllObj();
									 YYABORT;
								} 
		
								;}
    break;

  case 5:
#line 111 "l8.y"
    { yyval.expression = yyvsp[0].expression;;}
    break;

  case 6:
#line 112 "l8.y"
    {
	 							Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_AND,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 7:
#line 124 "l8.y"
    { yyval.expression = yyvsp[0].expression;;}
    break;

  case 8:
#line 125 "l8.y"
    { 
								Try {

									if (yyvsp[0].expression->isFinal(yyvsp[0].expression) && yyvsp[0].expression->isString(yyvsp[0].expression)) {
										if (yyvsp[-2].expression->getFuncOpCode(yyvsp[-2].expression) == FN_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[-2].expression;
											tmp->opCode = FN_MATCH_NAME;
											addArg_fne(tmp,yyvsp[0].expression);
											yyval.expression = tmp;
										}
										else if (getFuncOpCode_fne(yyvsp[-2].expression) == FN_LOCAL_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[-2].expression;
											tmp->opCode = FN_MATCH_LOCAL_NAME;
											addArg_fne(tmp, yyvsp[0].expression);
											yyval.expression = tmp;
										}
										else {
											yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression, OP_EQ, yyvsp[0].expression);
											
										}
										
									}
									else if (yyvsp[-2].expression->isFinal(yyvsp[-2].expression) && yyvsp[-2].expression->isString(yyvsp[-2].expression)) {
										if (yyvsp[0].expression->getFuncOpCode(yyvsp[0].expression) == FN_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[0].expression;
											tmp->opCode = FN_MATCH_NAME;
											addArg_fne(tmp, yyvsp[-2].expression);
											yyval.expression = tmp;
										}
										else if (yyvsp[0].expression->getFuncOpCode(yyvsp[0].expression) == FN_LOCAL_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[0].expression;
											tmp->opCode = FN_MATCH_LOCAL_NAME;
											addArg_fne(tmp, yyvsp[-2].expression);
											yyval.expression = tmp;
										}
										else {
											yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression, OP_EQ, yyvsp[0].expression);
											//addObj(yyval.expression);
										}
									}
									else {
										yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_EQ,yyvsp[0].expression);
										//addObj(yyval.expression);
										}
									
									addObj(yyval.expression);
	 								
								}
								
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 9:
#line 135 "l8.y"
    {
			 					Try {
									if (yyvsp[0].expression->isFinal(yyvsp[0].expression) && yyvsp[0].expression->isString(yyvsp[0].expression)) {
										if (yyvsp[-2].expression->getFuncOpCode(yyvsp[-2].expression) == FN_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[-2].expression;
											tmp->opCode = FN_NOT_MATCH_NAME;
											addArg_fne(tmp,yyvsp[0].expression);
											yyval.expression = tmp;
										}
										else if (getFuncOpCode_fne(yyvsp[-2].expression) == FN_LOCAL_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[-2].expression;
											tmp->opCode = FN_NOT_MATCH_LOCAL_NAME;
											addArg_fne(tmp, yyvsp[0].expression);
											yyval.expression = tmp;
										}
										else {
											yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression, OP_NE, yyvsp[0].expression);

										}

									}
									else if (yyvsp[-2].expression->isFinal(yyvsp[-2].expression) && yyvsp[-2].expression->isString(yyvsp[-2].expression)) {
										if (yyvsp[0].expression->getFuncOpCode(yyvsp[0].expression) == FN_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[0].expression;
											tmp->opCode = FN_NOT_MATCH_NAME;
											addArg_fne(tmp, yyvsp[-2].expression);
											yyval.expression = tmp;
										}
										else if (yyvsp[0].expression->getFuncOpCode(yyvsp[0].expression) == FN_LOCAL_NAME) {
											funcExpr *tmp = (funcExpr *)yyvsp[0].expression;
											tmp->opCode = FN_NOT_MATCH_LOCAL_NAME;
											addArg_fne(tmp, yyvsp[-2].expression);
											yyval.expression = tmp;
										}
										else {
											yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression, OP_NE, yyvsp[0].expression);
											//addObj(yyval.expression);
										}
									}
									else {
										yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_NE,yyvsp[0].expression);
										//addObj(yyval.expression);
									}

									addObj(yyval.expression);
	 									//yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_NE,yyvsp[0].expression);
	 									//addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 10:
#line 147 "l8.y"
    { yyval.expression = yyvsp[0].expression; ;}
    break;

  case 11:
#line 148 "l8.y"
    {
			 					Try {
	 									yyval.expression =(expr *) createBinaryExpr(yyvsp[-2].expression,OP_LT,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 12:
#line 158 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_GT,yyvsp[0].expression);
	 									addObj(yyval.expression);	 									
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 13:
#line 168 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_LE,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 14:
#line 178 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_GE,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 15:
#line 190 "l8.y"
    {yyval.expression  = yyvsp[0].expression; ;}
    break;

  case 16:
#line 191 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_ADD,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 17:
#line 201 "l8.y"
    {
			 					Try {
	 									yyval.expression =(expr *) createBinaryExpr(yyvsp[-2].expression,OP_SUB,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 18:
#line 213 "l8.y"
    {yyval.expression = yyvsp[0].expression; ;}
    break;

  case 19:
#line 214 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_MULT,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 20:
#line 224 "l8.y"
    {
			 					Try {
	 									yyval.expression =(expr *) createBinaryExpr(yyvsp[-2].expression,OP_DIV,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 21:
#line 234 "l8.y"
    {
			 					Try {
	 									yyval.expression = (expr *)createBinaryExpr(yyvsp[-2].expression,OP_MOD,yyvsp[0].expression);
	 									addObj(yyval.expression);
	 								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 22:
#line 246 "l8.y"
	  { if (yyvsp[0].une->next == NULL) {
		  yyval.expression = (expr *)yyvsp[0].une->fe;
		  yyvsp[0].une->fe = NULL;
		  yyvsp[0].une->freeExpr(yyvsp[0].une);
	    }
	    else 
		{
		   yyval.expression = (expr *)yyvsp[0].une;
		}
	  ;}
    break;

  case 23:
#line 247 "l8.y"
    {
			 					Try {
									
	 									yyval.expression = (expr *)createUnaryExpr(OP_NE,yyvsp[0].expression);
	 									addObj(yyval.expression);
								}
	 							Catch(e){
	 								//freeAllObj();
									YYABORT;
								}
								;}
    break;

  case 24:
#line 260 "l8.y"
    { 
									Try {
										yyval.une = createUnionExpr((expr *)yyvsp[0].pe);
										addObj(yyval.une);
									}
									Catch(e) {
										YYABORT;
									 }
								
								;}
    break;

  case 25:
#line 270 "l8.y"
    {
   										Try {
   										   yyval.une = createUnionExpr((expr *)yyvsp[-2].pe);
   										   addObj(yyval.une);
   										   yyval.une->next = yyvsp[0].une;
   										}
   										Catch(e){
   											YYABORT;   											
   										}
   										
   		                         /*freeAllObj();*/
   		                         //YYABORT;
   		                         
   		                         ;}
    break;

  case 26:
#line 287 "l8.y"
    { yyval.pe = (pathExpr *) yyvsp[0].lpe;  ;}
    break;

  case 27:
#line 288 "l8.y"
    { yyval.pe = (pathExpr *)yyvsp[0].expression;;}
    break;

  case 28:
#line 289 "l8.y"
    {  														
  														Try{
															//printf("creating locationi path \n");
  															tmpLPExpr = createLocationPathExpr();
  															addObj(tmpLPExpr);
  															addObj(tmpLPExpr->ih);
  															addObj(tmpLPExpr->ih->storage);
  															setStep(tmpLPExpr, yyvsp[0].s);
  															yyval.pe = createPathExpr(yyvsp[-2].expression, tmpLPExpr);
  															addObj(yyval.pe);
  															addObj(yyval.pe->ih);
  															addObj(yyval.pe->ih->storage);
  															tmpLPExpr = NULL;
  														}Catch(e){  														
  															//freeAllObj();
  															YYABORT;
  														}  		
  													;}
    break;

  case 29:
#line 307 "l8.y"
    {
														Try {
															//printf("creating step \n");
															tmpStep = createStep();
															addObj(tmpStep);
															setAxisType(tmpStep,AXIS_DESCENDANT_OR_SELF);
															tmpNt = createNodeTest();
															addObj(tmpNt);
															setTestType(tmpNt, NT_NODE);
															setNodeTest(tmpStep, tmpNt);
															tmpStep->nextS = yyvsp[0].s;
															yyvsp[0].s->prevS = tmpStep;
															tmpLPExpr = createLocationPathExpr();
															addObj(tmpLPExpr);
															addObj(tmpLPExpr->ih);
															addObj(tmpLPExpr->ih->storage);
															setStep(tmpLPExpr, tmpStep);
															yyval.pe = createPathExpr(yyvsp[-2].expression, tmpLPExpr);
															addObj(yyval.pe);
															addObj(yyval.pe->ih);	
															addObj(yyval.pe->ih->storage);															
														} Catch (e){
															//freeAllObj();		
															YYABORT;																											
														}								
													;}
    break;

  case 30:
#line 335 "l8.y"
    { yyval.expression = yyvsp[0].expression;;}
    break;

  case 31:
#line 336 "l8.y"
    {Try {
  										yyval.expression = (expr *)createFilterExpr(yyvsp[-1].expression,yyvsp[0].p);
  										addObj(yyval.expression);
  										}
  									Catch(e){
  										//freeAllObj();
  										YYABORT;
  									}
  								;}
    break;

  case 32:
#line 349 "l8.y"
    {yyval.expression = yyvsp[0].expression;;}
    break;

  case 33:
#line 350 "l8.y"
    { yyval.expression = yyvsp[-1].expression;;}
    break;

  case 34:
#line 351 "l8.y"
    { Try {
							yyval.expression = (expr *)createLiteralExpr(yyvsp[0].literal);
							addObj(yyval.expression);
							addObj(yyvsp[0].literal);
						} Catch (e) {
							//freeAllObj();
							YYABORT;	
						}						
					  ;}
    break;

  case 35:
#line 360 "l8.y"
    { Try {
						    yyval.expression = (expr *)createNumberExpr(yyvsp[0].number);
						    addObj(yyval.expression);
					   } Catch (e) {
							//freeAllObj();
							YYABORT;
					   }
					 ;}
    break;

  case 36:
#line 368 "l8.y"
    { yyval.expression = yyvsp[0].expression;;}
    break;

  case 37:
#line 369 "l8.y"
    {
						//freeAllObj();
						//printf(" Invalid char encoutered \n");
						YYABORT;
					;}
    break;

  case 38:
#line 377 "l8.y"
    { Try {
															yyval.expression = (expr *)createFuncExpr(yyvsp[-3].fname, yyvsp[-1].a);
															if  (!checkArgumentCount(((funcExpr *)yyval.expression)))
																throwException2( xpath_parse_exception,"Invalid argument for functional expression");
															addObj(yyval.expression);
													   }
													   Catch(e){
															//freeAllObj();
															YYABORT;
													   }

													 ;}
    break;

  case 39:
#line 389 "l8.y"
    { yyval.a = NULL;;}
    break;

  case 40:
#line 390 "l8.y"
    {	Try {
	     							yyval.a = createAlist();
	     							addObj(yyval.a);
	     							yyval.a->e = yyvsp[0].expression;  								     	
	     						}Catch (e){
	     							//freeAllObj();
	     							YYABORT;
	     						}	
	     					;}
    break;

  case 41:
#line 399 "l8.y"
    {
	     											Try {	     											
	     												yyval.a = createAlist();
	     												addObj(yyval.a);
	     												yyval.a->e = yyvsp[-2].expression;
	     												yyval.a->next = yyvsp[0].a;	     												
	     											} Catch (e){
	     												//freeAllObj();
	     												YYABORT;	     													     												
	     											}	     	
	     									  ;}
    break;

  case 42:
#line 412 "l8.y"
    {yyval.expression = yyvsp[0].expression;;}
    break;

  case 43:
#line 416 "l8.y"
    { Try {
													yyval.lpe = createLocationPathExpr();
													addObj(yyval.lpe);
													addObj(yyval.lpe->ih);
													addObj(yyval.lpe->ih->storage);
													setStep(yyval.lpe, yyvsp[0].s);
													optimize(yyval.lpe);
												  }
											  Catch (e) {
													//freeAllObj();
													YYABORT;
											  }
											;}
    break;

  case 44:
#line 428 "l8.y"
    {		Try { // printf("absolute locationpath \n");
													yyval.lpe = createLocationPathExpr();
													addObj(yyval.lpe);
													addObj(yyval.lpe->ih);
													yyval.lpe->pathType = ABSOLUTE_PATH;
													setStep(yyval.lpe, yyvsp[0].s);
													optimize(yyval.lpe);
												  }
											  Catch (e) {
													//freeLocationPathExpr($$);
													//freeAllObj();
													YYABORT;
											  }
									;}
    break;

  case 45:
#line 443 "l8.y"
    {yyval.s = NULL;;}
    break;

  case 46:
#line 444 "l8.y"
    {/*printf("slash detected \n");*/ yyval.s = yyvsp[0].s;;}
    break;

  case 47:
#line 445 "l8.y"
    {yyval.s = yyvsp[0].s;;}
    break;

  case 48:
#line 448 "l8.y"
    {yyval.s = yyvsp[0].s;;}
    break;

  case 49:
#line 449 "l8.y"
    { yyvsp[-2].s->nextS = yyvsp[0].s;
												yyvsp[0].s->prevS = yyvsp[-2].s;
												yyval.s = yyvsp[-2].s;		
												;}
    break;

  case 50:
#line 453 "l8.y"
    {yyval.s = yyvsp[0].s;;}
    break;

  case 51:
#line 457 "l8.y"
    {
														Try {
															yyval.s = createStep();
															addObj(yyval.s);
															setAxisType(yyval.s, yyvsp[-2].at);
															if ( (yyvsp[-2].at== AXIS_ATTRIBUTE
	        												|| yyvsp[-2].at == AXIS_NAMESPACE) && 
      														(yyvsp[-1].nodetest->testType>1)){
      																printf("%s axis can't operate on comment(), pi(), or text()\n", getAxisString(yyvsp[-2].at));
      																throwException2(xpath_parse_exception," attr|namespace axis can't operate on comment(), pi(), or text()");
      	         											}
															setNodeTest(yyval.s, yyvsp[-1].nodetest);
															setPredicate(yyval.s, yyvsp[0].p);
															}
														Catch(e){
															//freeAllObj();
															YYABORT;
														}
														;}
    break;

  case 52:
#line 481 "l8.y"
    {yyval.s = yyvsp[0].s;;}
    break;

  case 53:
#line 485 "l8.y"
    { Try {
										//printf("create Node test2");
										yyval.nodetest = createNodeTest();
										addObj(yyval.nodetest);
										setTestType(yyval.nodetest,NT_NAMETEST);
										//wprintf(L"$1.qname %ls\n",yyval.name.qname);
										setNodeName(yyval.nodetest,yyvsp[0].name.qname);
										//addObj($1.qname);
										if (yyvsp[0].name.localname!=NULL){
											setNodeNameNS(yyval.nodetest,yyvsp[0].name.prefix,yyvsp[0].name.localname);
											//addObj($1.prefix);
											//addObj($1.localname);
											// the part for URL mapping goes here
											yyval.nodetest->URL = lookup(xpathNSList,yyvsp[0].name.prefix);
											if (yyval.nodetest->URL == NULL){
												YYABORT;
											}
										}
										yyvsp[0].name.prefix = yyvsp[0].name.localname = yyvsp[0].name.qname = NULL;
										
									  } Catch(e){
										//freeAllObj();
										YYABORT;
									  }
									;}
    break;

  case 54:
#line 509 "l8.y"
    { Try{
									//printf("create node test\n");
	 								yyval.nodetest = createNodeTest();
	 								addObj(yyval.nodetest);
	 								if (yyvsp[0].ntest.nt !=3)
	 									setTestType(yyval.nodetest,yyvsp[0].ntest.nt);
	 								else {
	 									if (yyvsp[0].ntest.arg == NULL)
	 										setTestType(yyval.nodetest, yyvsp[0].ntest.nt);
	 									else{
	 										setTestType(yyval.nodetest,4);
	 										setNodeName(yyval.nodetest, yyvsp[0].ntest.arg);
	 									}
	 								}
	 								//setTestType($$,$1.nt);
	 								}
	 								Catch(e){
	 									//freeAllObj();
	 									YYABORT;
	 								}	 								
	 							;}
    break;

  case 55:
#line 531 "l8.y"
    { yyval.p = NULL;;}
    break;

  case 56:
#line 532 "l8.y"
    { yyvsp[-1].p->nextP = yyvsp[0].p;
	     									yyval.p = yyvsp[-1].p;	
	     								  ;}
    break;

  case 57:
#line 537 "l8.y"
    {yyval.at  = yyvsp[0].at;;}
    break;

  case 58:
#line 538 "l8.y"
    {yyval.at  = yyvsp[0].at;;}
    break;

  case 59:
#line 541 "l8.y"
    { /*printf("abbreviated child axis \n");*/yyval.at  = AXIS_CHILD;;}
    break;

  case 60:
#line 542 "l8.y"
    {yyval.at = AXIS_ATTRIBUTE;;}
    break;

  case 61:
#line 545 "l8.y"
    {Try{
								yyval.s = createStep();
								addObj(yyval.s);
								tmpNt = createNodeTest();
								addObj(tmpNt);
								setTestType(tmpNt, NT_NODE);
								setAxisType(yyval.s,AXIS_SELF);
								setNodeTest(yyval.s,tmpNt);
							}
							Catch(e){
								//freeAllObj();
								YYABORT;
							}
						;}
    break;

  case 62:
#line 559 "l8.y"
    {
					Try{

						//printf("create step\n");
						yyval.s = createStep();
						addObj(yyval.s);
						tmpNt = createNodeTest();
						addObj(tmpNt);
						setTestType(tmpNt, NT_NODE);
						setAxisType(yyval.s,AXIS_PARENT);
						setNodeTest(yyval.s,tmpNt);
					}
					Catch(e){
						//freeAllObj();
						YYABORT;
					}
		
					;}
    break;

  case 63:
#line 577 "l8.y"
    {
																Try{
																	yyval.s = createStep();
																	addObj(yyval.s);
																	setAxisType(yyval.s,AXIS_DESCENDANT_OR_SELF);
																	tmpNt = createNodeTest();
																	addObj(tmpNt);
																	setTestType(tmpNt, NT_NODE);
																	setNodeTest(yyval.s,tmpNt);
																	yyval.s->nextS = yyvsp[0].s;
																	yyvsp[0].s->prevS = yyval.s;																
																}Catch(e){
																	//freeAllObj();
																	YYABORT;																	
																}
																;}
    break;

  case 64:
#line 595 "l8.y"
    {
																	Try{
																		yyval.s= createStep();
																		addObj(yyval.s);
																		setAxisType(yyval.s,AXIS_DESCENDANT_OR_SELF);
																		tmpNt = createNodeTest();
																		addObj(tmpNt);
																		setTestType(tmpNt,NT_NODE);
																		setNodeTest(yyval.s, tmpNt);
																		yyvsp[-2].s->nextS = yyval.s;
																		yyval.s->prevS = yyvsp[-2].s;
																		yyval.s->nextS = yyvsp[0].s;
																		yyvsp[0].s->prevS = yyval.s;
																		yyval.s = yyvsp[-2].s;
																	}Catch(e){
																		//freeAllObj();
																		YYABORT;	
																	}
																	;}
    break;

  case 65:
#line 617 "l8.y"
    {
							   Try {
									yyval.p = createPredicate();
									addObj(yyval.p);
									yyval.p->e = yyvsp[-1].expression;
									if ((yyval.p)->e->isFinal((yyval.p)->e) && (yyval.p)->e->isNumerical((yyval.p)->e)){
										yyval.p->d = yyval.p->e->evalNumber(yyval.p->e,NULL);
										if (yyval.p->d <1){
											throwException2(xpath_parse_exception,"Invalid index number <1");
										}
										yyval.p->type = SIMPLE_P;
									}
								} Catch(e){
									//freeAllObj();
									YYABORT;
								}
							;}
    break;

  case 66:
#line 629 "l8.y"
    {
								Try {
								    //addObj($2);
								    ex = getExprFromList(xpathExprList,yyvsp[0].name.qname);

									//printf("here !!!\n");
								    if (ex==NULL) {
										YYABORT;
								    }
								    yyval.expression = (expr *)createVariableExpr(yyvsp[0].name.qname, ex);
									//$$ = createPredicate();
									addObj(yyval.expression);
									//$$->e = $2;
								} Catch(e){
									//freeAllObj();
									YYABORT;
								}

							;}
    break;

  case 67:
#line 648 "l8.y"
    {yyval.fname  = yyvsp[0].fname;;}
    break;


    }

/* Line 991 of yacc.c.  */
#line 1943 "l8.tab.c"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 651 "l8.y"

extern _thread unsigned short *xpathInput;
extern _thread unsigned short *xpathInputPtr;
extern _thread unsigned short *xpathInputLimit;
extern _thread int num_chars;
extern _thread int isName;
expr *xpathParse(UCSChar *input, NsList *nl, ExprList *el){
	int l = wcslen(input);
	int i = 0;
	num_chars = 0;	
	xpathNSList = nl;
	xpathExprList = el;
	XMLChar_init();
	xpathInputPtr = xpathInput = (unsigned short *)malloc((l+1)<<1);
	// copy the string from wchar_t to unsigned short
	for (i=0;i<l;i++){
		xpathInput[i] = (unsigned short)input[i];
	}
	xpathInputLimit = (unsigned short *) (xpathInput + ((l)<<1));
	isName = 1;
	colonPosition = -1;
	if (yyparse() == 0){
		//isName = 1;
		//wprintf(L"yyparse return 0\n");
		free(xpathInput);
		xpathInput = xpathInputPtr = xpathInputLimit = NULL;
		resetObj();
		return x;
	} else {
		//wprintf(L"yyparse YYABORT\n");
		wprintf(L"xpath syntax error after or around the end of \n     %.*s\n", num_chars, input);
		free(xpathInput);
		xpathInput = xpathInputPtr = xpathInputLimit = NULL;
		//isName = 1;
		freeAllObj();
		return NULL;
	}
	
}


