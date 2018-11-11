/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.0.4"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "./verilog_bison.y" /* yacc.c:339  */

/*
Copyright (c) 2009 Peter Andrew Jamieson (jamieson.peter@gmail.com)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "parse_making_ast.h"

#define PARSE {printf("here\n");}
extern int yylineno;
#ifndef YYLINENO
	#define YYLINENO yylineno
#endif

void yyerror(const char *str){	fprintf(stderr,"error in parsing: %s - on line number %d\n",str, yylineno);	exit(-1);}
int yywrap(){	return 1;}
int yyparse();
int yylex(void);



#line 111 "verilog_bison.tab.c" /* yacc.c:339  */

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    vSYMBOL_ID = 258,
    vINTEGRAL = 259,
    vUNSIGNED_DECIMAL = 260,
    vUNSIGNED_OCTAL = 261,
    vUNSIGNED_HEXADECIMAL = 262,
    vUNSIGNED_BINARY = 263,
    vSIGNED_DECIMAL = 264,
    vSIGNED_OCTAL = 265,
    vSIGNED_HEXADECIMAL = 266,
    vSIGNED_BINARY = 267,
    vDELAY_ID = 268,
    vALWAYS = 269,
    vINITIAL = 270,
    vSPECIFY = 271,
    vAND = 272,
    vASSIGN = 273,
    vBEGIN = 274,
    vCASE = 275,
    vDEFAULT = 276,
    vDEFINE = 277,
    vELSE = 278,
    vEND = 279,
    vENDCASE = 280,
    vENDMODULE = 281,
    vENDSPECIFY = 282,
    vENDFUNCTION = 283,
    vIF = 284,
    vINOUT = 285,
    vINPUT = 286,
    vMODULE = 287,
    vFUNCTION = 288,
    vNAND = 289,
    vNEGEDGE = 290,
    vNOR = 291,
    vNOT = 292,
    vOR = 293,
    vFOR = 294,
    vOUTPUT = 295,
    vPARAMETER = 296,
    vPOSEDGE = 297,
    vREG = 298,
    vWIRE = 299,
    vXNOR = 300,
    vXOR = 301,
    vDEFPARAM = 302,
    voANDAND = 303,
    voOROR = 304,
    voLTE = 305,
    voGTE = 306,
    voPAL = 307,
    voSLEFT = 308,
    voSRIGHT = 309,
    voEQUAL = 310,
    voNOTEQUAL = 311,
    voCASEEQUAL = 312,
    voCASENOTEQUAL = 313,
    voXNOR = 314,
    voNAND = 315,
    voNOR = 316,
    vWHILE = 317,
    vINTEGER = 318,
    vNOT_SUPPORT = 319,
    vPlusColon = 320,
    voPOWER = 321,
    UOR = 322,
    UAND = 323,
    UNOT = 324,
    UNAND = 325,
    UNOR = 326,
    UXNOR = 327,
    UXOR = 328,
    ULNOT = 329,
    UADD = 330,
    UMINUS = 331,
    LOWER_THAN_ELSE = 332
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 46 "./verilog_bison.y" /* yacc.c:355  */

	char *id_name;
	char *num_value;
	ast_node_t *node;

#line 232 "verilog_bison.tab.c" /* yacc.c:355  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);



/* Copy the second part of user declarations.  */

#line 249 "verilog_bison.tab.c" /* yacc.c:358  */

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE
# if (defined __GNUC__                                               \
      && (2 < __GNUC__ || (__GNUC__ == 2 && 96 <= __GNUC_MINOR__)))  \
     || defined __SUNPRO_C && 0x5110 <= __SUNPRO_C
#  define YY_ATTRIBUTE(Spec) __attribute__(Spec)
# else
#  define YY_ATTRIBUTE(Spec) /* empty */
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# define YY_ATTRIBUTE_PURE   YY_ATTRIBUTE ((__pure__))
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# define YY_ATTRIBUTE_UNUSED YY_ATTRIBUTE ((__unused__))
#endif

#if !defined _Noreturn \
     && (!defined __STDC_VERSION__ || __STDC_VERSION__ < 201112)
# if defined _MSC_VER && 1200 <= _MSC_VER
#  define _Noreturn __declspec (noreturn)
# else
#  define _Noreturn YY_ATTRIBUTE ((__noreturn__))
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN \
    _Pragma ("GCC diagnostic push") \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")\
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif


#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYSIZE_T yynewbytes;                                            \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / sizeof (*yyptr);                          \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, (Count) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYSIZE_T yyi;                         \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  9
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2464

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  104
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  64
/* YYNRULES -- Number of rules.  */
#define YYNRULES  206
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  455

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   332

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    86,     2,   102,     2,    77,    70,     2,
      78,    79,    75,    73,    99,    74,   100,    76,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    67,    98,
      71,   101,    72,    66,   103,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    82,     2,    83,    69,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    80,    68,    81,    85,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    84,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   124,   124,   128,   129,   130,   131,   135,   136,   137,
     138,   139,   140,   141,   142,   143,   148,   149,   150,   155,
     156,   160,   161,   165,   166,   167,   168,   169,   170,   171,
     172,   173,   174,   175,   176,   177,   178,   182,   186,   190,
     194,   195,   198,   199,   200,   201,   202,   203,   204,   208,
     212,   216,   220,   224,   228,   232,   233,   237,   241,   245,
     246,   250,   251,   252,   256,   257,   261,   262,   263,   264,
     266,   267,   271,   273,   276,   280,   284,   285,   290,   291,
     292,   293,   294,   295,   296,   300,   301,   305,   306,   310,
     311,   315,   316,   321,   322,   326,   331,   332,   336,   342,
     346,   347,   348,   349,   353,   354,   357,   358,   362,   363,
     367,   368,   372,   373,   378,   382,   386,   387,   388,   389,
     390,   391,   392,   393,   394,   398,   399,   403,   407,   408,
     412,   413,   417,   421,   422,   426,   427,   431,   435,   436,
     440,   441,   442,   447,   448,   449,   453,   454,   455,   459,
     460,   461,   462,   463,   464,   465,   466,   467,   468,   469,
     470,   471,   472,   473,   474,   475,   476,   477,   478,   479,
     480,   481,   482,   483,   484,   485,   486,   487,   488,   489,
     490,   491,   492,   493,   494,   495,   496,   497,   501,   502,
     503,   504,   505,   506,   507,   508,   509,   510,   511,   512,
     513,   514,   515,   519,   520,   524,   525
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "vSYMBOL_ID", "vINTEGRAL",
  "vUNSIGNED_DECIMAL", "vUNSIGNED_OCTAL", "vUNSIGNED_HEXADECIMAL",
  "vUNSIGNED_BINARY", "vSIGNED_DECIMAL", "vSIGNED_OCTAL",
  "vSIGNED_HEXADECIMAL", "vSIGNED_BINARY", "vDELAY_ID", "vALWAYS",
  "vINITIAL", "vSPECIFY", "vAND", "vASSIGN", "vBEGIN", "vCASE", "vDEFAULT",
  "vDEFINE", "vELSE", "vEND", "vENDCASE", "vENDMODULE", "vENDSPECIFY",
  "vENDFUNCTION", "vIF", "vINOUT", "vINPUT", "vMODULE", "vFUNCTION",
  "vNAND", "vNEGEDGE", "vNOR", "vNOT", "vOR", "vFOR", "vOUTPUT",
  "vPARAMETER", "vPOSEDGE", "vREG", "vWIRE", "vXNOR", "vXOR", "vDEFPARAM",
  "voANDAND", "voOROR", "voLTE", "voGTE", "voPAL", "voSLEFT", "voSRIGHT",
  "voEQUAL", "voNOTEQUAL", "voCASEEQUAL", "voCASENOTEQUAL", "voXNOR",
  "voNAND", "voNOR", "vWHILE", "vINTEGER", "vNOT_SUPPORT", "vPlusColon",
  "'?'", "':'", "'|'", "'^'", "'&'", "'<'", "'>'", "'+'", "'-'", "'*'",
  "'/'", "'%'", "'('", "')'", "'{'", "'}'", "'['", "']'", "voPOWER", "'~'",
  "'!'", "UOR", "UAND", "UNOT", "UNAND", "UNOR", "UXNOR", "UXOR", "ULNOT",
  "UADD", "UMINUS", "LOWER_THAN_ELSE", "';'", "','", "'.'", "'='", "'#'",
  "'@'", "$accept", "source_text", "items", "define", "module",
  "variable_define_list", "list_of_module_items", "module_item",
  "function_declaration", "initial_block", "specify_block",
  "list_of_function_items", "function_item", "function_input_declaration",
  "parameter_declaration", "defparam_declaration", "input_declaration",
  "output_declaration", "inout_declaration", "net_declaration",
  "integer_declaration", "function_output_variable",
  "function_id_and_output_variable", "variable_list",
  "integer_type_variable_list", "variable", "integer_type_variable",
  "continuous_assign", "list_of_blocking_assignment", "gate_declaration",
  "list_of_multiple_inputs_gate_declaration_instance",
  "list_of_single_input_gate_declaration_instance",
  "single_input_gate_instance", "multiple_inputs_gate_instance",
  "list_of_multiple_inputs_gate_connections", "module_instantiation",
  "list_of_module_instance", "function_instantiation", "function_instance",
  "module_instance", "list_of_function_connections",
  "list_of_module_connections", "module_connection",
  "list_of_module_parameters", "module_parameter", "always",
  "function_statement", "statement", "list_of_specify_statement",
  "specify_statement", "blocking_assignment", "non_blocking_assignment",
  "parallel_connection", "case_item_list", "case_items", "seq_block",
  "stmt_list", "delay_control", "event_expression_list",
  "event_expression", "expression", "primary", "probable_expression_list",
  "expression_list", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,   316,   317,   318,   319,   320,    63,    58,   124,    94,
      38,    60,    62,    43,    45,    42,    47,    37,    40,    41,
     123,   125,    91,    93,   321,   126,    33,   322,   323,   324,
     325,   326,   327,   328,   329,   330,   331,   332,    59,    44,
      46,    61,    35,    64
};
# endif

#define YYPACT_NINF -230

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-230)))

#define YYTABLE_NINF -207

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
      26,    37,    49,    57,    26,  -230,  -230,   314,   -16,  -230,
    -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,
    -230,    21,  -230,   -19,   -58,  1067,    -4,    35,    20,     4,
     552,    32,    11,   305,    16,    16,    17,    11,    11,    47,
      11,    16,    16,    16,    16,    11,    11,    16,   124,   959,
    -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,
    -230,  -230,  -230,  -230,  -230,  1067,  -230,    46,    80,    82,
      51,  -230,   -22,   552,    85,  -230,  -230,  -230,  -230,  -230,
    -230,  -230,  -230,  -230,   552,   106,   116,   122,   127,   875,
    -230,  -230,    84,   133,  -230,   -33,   305,    10,  -230,   155,
     875,    75,  -230,   129,  -230,   157,   175,   875,    71,  -230,
      78,  -230,   875,   158,  -230,   153,   163,   203,   875,   168,
    -230,   170,    81,    93,    99,   104,   172,   181,   119,   -36,
     197,  -230,  -230,  -230,   995,  1067,    23,   419,  -230,    20,
    -230,   890,  -230,   875,  -230,    66,   875,   875,   305,   875,
     -27,   875,   875,   875,   875,   875,   875,   875,   875,   875,
     875,   875,   875,  -230,  1105,  -230,   210,   186,  -230,  -230,
     747,   769,   214,   245,  -230,  -230,   875,  1137,  -230,    11,
    -230,   305,   305,  1444,  -230,    16,    16,  -230,  1476,   663,
    -230,  -230,   875,  1169,  -230,    47,  -230,  -230,  -230,  -230,
    -230,  -230,  -230,  -230,   875,   305,  -230,   124,  -230,  1031,
    -230,   296,   -57,  -230,  2252,   298,   -54,  -230,  2252,  -230,
     299,   300,   226,    25,  -230,  -230,  1407,  -230,  -230,  1508,
    1540,   208,  1572,   533,  -230,  -230,  -230,  -230,  -230,  -230,
    -230,  -230,  -230,  1604,  1068,  -230,  -230,   875,   875,   875,
     875,   875,   875,   875,   875,   875,   875,   875,   875,   875,
     875,   875,   875,   875,   875,   875,   875,   875,   875,   875,
     875,   875,  -230,   875,   875,  2252,   875,  2252,   206,   875,
    1201,   875,  -230,  -230,  -230,   875,  -230,  -230,   875,    16,
     617,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,  -230,
    1233,   875,  -230,  1636,  -230,  -230,  -230,   250,  -230,   533,
     251,   352,   419,  -230,  -230,  -230,   126,  -230,   126,   875,
     276,   853,   552,   875,   552,   -40,  -230,  -230,   875,  2316,
    2284,   112,   112,   173,   173,   316,   316,   316,   316,  2380,
     362,  2380,  1668,  2348,  2380,   362,   112,   112,    79,    79,
     277,   277,   277,   277,  1265,  2252,  2252,   305,  2252,   875,
    1297,  1700,  1737,   123,  -230,  -230,   875,  1774,   875,   875,
    -230,   875,   282,  -230,  -230,  -230,  1806,   875,   307,   724,
    -230,  1843,   340,  1361,  -230,  -230,   533,   284,   875,   274,
    1329,   875,   372,   378,  -230,  1875,  -230,  1907,  1944,  1976,
     204,   304,  2008,   552,  -230,  -230,   552,   552,   305,  -230,
     313,  2252,  -230,   875,   -35,  2252,    44,  -230,  -230,  -230,
    -230,  -230,  -230,   -32,   875,  -230,  -230,  -230,  -230,   317,
    -230,   -18,  -230,   875,   875,   875,  -230,  2045,   552,  -230,
    2252,  2077,  2252,   875,  -230,   875,  2109,  2146,  -230,   315,
     875,  2183,   875,  2215,  -230
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     0,     0,     0,     2,     6,     5,     0,     0,     1,
       4,     3,     7,     8,     9,    10,    11,    12,    13,    14,
      15,     0,    20,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      22,    32,    33,    36,    23,    35,    24,    25,    26,    27,
      28,    29,    30,    31,    34,     0,    19,     0,     0,     0,
       0,    97,     0,     0,   197,   188,   189,   190,   191,   192,
     193,   194,   195,   196,     0,     0,     0,     0,     0,     0,
     124,    38,     0,     0,   116,     0,     0,     0,   126,     0,
       0,     0,    86,     0,    77,     0,    66,     0,     0,    63,
       0,    59,     0,     0,    58,     0,     0,     0,     0,     0,
      88,     0,     0,     0,     0,     0,     0,     0,     0,    72,
       0,    65,    18,    21,     0,     0,     0,     0,    95,     0,
     141,     0,   114,     0,   139,     0,     0,     0,     0,     0,
     197,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   185,   203,   149,     0,     0,   117,   118,
       0,     0,     0,     0,    39,   125,     0,     0,    78,     0,
      75,     0,     0,     0,    54,     0,     0,    52,     0,     0,
      79,    80,     0,     0,    81,     0,    82,    53,    50,    56,
      55,    83,    84,    51,     0,     0,    57,     0,    16,     0,
     102,     0,     0,   107,   109,     0,     0,   111,   113,    96,
       0,     0,     0,     0,   145,   146,     0,   137,   138,     0,
       0,     0,     0,     0,    98,   157,   155,   156,   154,   159,
     153,   150,   151,     0,   203,   152,   158,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,     0,     0,   130,     0,   128,     0,     0,
       0,     0,    85,    76,    71,     0,    61,    62,     0,     0,
       0,    41,    43,    42,    47,    44,    45,    46,    48,   115,
       0,     0,    87,     0,    74,    64,    17,     0,   100,     0,
       0,     0,     0,   148,   147,   142,     0,   140,     0,     0,
     198,     0,     0,     0,     0,     0,   105,   186,     0,   183,
     182,   178,   179,   175,   174,   176,   177,   180,   181,   171,
     169,   170,     0,   168,   160,   167,   172,   173,   165,   166,
     162,   163,   164,   161,   204,   131,   129,     0,   132,     0,
       0,     0,     0,     0,    37,    40,     0,     0,     0,     0,
     106,     0,     0,   110,   143,   144,     0,     0,     0,     0,
     134,     0,   119,     0,   123,    99,     0,     0,     0,     0,
       0,     0,     0,     0,    49,     0,    90,     0,     0,     0,
       0,   200,     0,     0,   121,   133,     0,     0,     0,   104,
       0,   184,   127,     0,     0,    94,    67,    60,    89,    73,
     108,   112,   103,     0,     0,   199,   136,   135,   120,     0,
     187,     0,    92,     0,     0,     0,   101,     0,     0,    91,
      93,     0,    70,     0,   122,     0,     0,     0,   201,    68,
       0,     0,     0,     0,    69
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -230,  -230,  -230,   394,   395,  -230,   -29,   -47,  -230,  -230,
    -230,  -230,   111,  -230,  -178,  -177,  -230,  -230,  -230,  -174,
    -173,  -230,  -230,   -34,  -230,    39,   196,  -171,  -230,  -230,
     102,  -230,   212,   230,    -8,  -230,  -230,  -230,  -230,   271,
    -230,    40,  -229,  -230,   109,  -230,  -230,   -24,  -230,   335,
     -28,  -230,  -230,  -230,    62,  -230,  -230,  -230,  -230,  -175,
      83,   -30,   115,  -230
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     3,     4,     5,     6,    24,    49,    50,    51,    52,
      53,   290,   291,   292,    54,    55,    56,    57,    58,    59,
      60,   113,   114,   108,   130,   109,   131,    61,   103,    62,
     101,   119,   120,   102,   414,    63,    70,   163,   234,    71,
     325,   212,   213,   216,   217,    64,   298,   299,    97,    98,
      92,    93,   172,   379,   380,    94,   145,    73,   223,   224,
     214,   165,   166,   167
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      95,   110,   133,   105,   326,   104,    91,   122,   123,   124,
     125,   293,   294,   128,    99,   295,   296,   170,   297,   106,
     111,    26,   308,    68,    22,   311,   150,    75,    76,    77,
      78,    79,    80,    81,    82,    83,   134,   174,    66,   385,
       7,    27,   309,    95,   432,   312,   204,   436,     1,   142,
     117,   233,     8,   140,    95,   143,   141,     9,     2,   386,
     144,   439,    21,   316,   433,   205,   173,   309,   171,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    25,
     370,   433,   151,   152,   153,    84,    85,   133,    96,   100,
     227,   154,   155,   156,    65,    86,   157,   158,   107,   112,
      23,   159,   210,   160,   317,    87,   209,    72,   161,   162,
      96,   225,   293,   294,    67,    95,   295,   296,   105,   297,
     231,   228,    69,   211,   318,   118,   434,   129,    88,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,   115,
     116,   374,   121,   375,   135,   435,    89,   126,   127,   138,
     139,   105,   284,   283,   268,   269,   270,   409,   136,    95,
     137,   220,   133,   271,    90,   251,   252,   143,   221,   184,
     185,   186,   164,   178,   179,   304,   187,   185,   186,   197,
     185,   186,   168,   177,   146,   266,   267,   268,   269,   270,
     183,   198,   185,   186,   147,   188,   271,   199,   185,   186,
     148,   193,   200,   185,   186,   149,    89,   150,    75,    76,
      77,    78,    79,    80,    81,    82,    83,   203,   185,   186,
     218,   394,   185,   186,   286,   287,   226,   180,   181,   229,
     230,   169,   232,   176,   235,   236,   237,   238,   239,   240,
     241,   242,   243,   244,   245,   246,   266,   267,   268,   269,
     270,   190,   179,   275,   277,   363,   189,   271,   171,   280,
      95,   191,   179,   151,   152,   153,   194,   195,   196,   179,
     201,   179,   154,   155,   156,   300,   182,   157,   158,   202,
     179,   192,   159,   422,   160,   273,   225,   303,   225,   161,
     162,   272,    95,   278,    95,   206,   207,   279,   382,   307,
     384,   310,   313,   314,   211,   315,   323,   357,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    12,    13,
      14,    15,    16,    17,    18,    19,    20,   389,   369,   371,
     329,   330,   331,   332,   333,   334,   335,   336,   337,   338,
     339,   340,   341,   342,   343,   344,   345,   346,   347,   348,
     349,   350,   351,   352,   353,   372,   354,   355,   377,   356,
     400,   271,   358,   407,   360,   410,   249,   250,   361,   251,
     252,   362,   412,    95,   403,   416,    95,    95,   105,   426,
     429,   417,   427,   428,   367,    89,   424,   264,   265,   266,
     267,   268,   269,   270,   430,   218,   438,   450,    10,    11,
     271,   365,   376,   305,   381,   431,   383,   302,    95,   282,
     219,   164,   249,   250,   444,   251,   252,   253,   254,   255,
     256,   373,   150,    75,    76,    77,    78,    79,    80,    81,
      82,    83,   175,   264,   265,   266,   267,   268,   269,   270,
     423,   405,   390,   387,     0,     0,   271,     0,     0,   395,
       0,   397,   398,     0,   399,     0,     0,     0,     0,     0,
     402,     0,   381,     0,     0,     0,     0,     0,     0,     0,
       0,   411,     0,     0,   415,     0,     0,     0,   151,   152,
     153,     0,     0,     0,     0,     0,     0,   154,   155,   156,
       0,     0,   157,   158,     0,     0,   415,   159,     0,   160,
       0,     0,     0,     0,   161,   162,     0,   437,     0,     0,
       0,     0,     0,     0,     0,     0,   440,   441,   442,   215,
       0,     0,     0,     0,     0,     0,   446,     0,   447,     0,
       0,     0,     0,   451,     0,   453,   150,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,     0,     0,     0,     0,     0,
       0,    84,    85,     0,     0,     0,     0,     0,     0,     0,
       0,    86,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    87,   151,   152,   153,     0,     0,     0,     0,     0,
       0,   154,   155,   156,     0,     0,   157,   158,     0,     0,
       0,   159,     0,   160,    88,     0,     0,     0,   161,   162,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
       0,     0,    89,   211,     0,    33,    84,    85,     0,     0,
       0,     0,     0,     0,     0,   364,    86,     0,   289,     0,
      90,     0,     0,     0,     0,     0,    87,     0,    42,     0,
      43,    44,     0,     0,    47,     0,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,     0,     0,    88,
      48,    33,    84,    85,     0,     0,     0,     0,     0,     0,
       0,     0,    86,     0,   289,     0,     0,    89,     0,     0,
       0,     0,    87,     0,    42,     0,    43,    44,     0,     0,
      47,     0,     0,     0,     0,    90,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    88,    48,   150,    75,    76,
      77,    78,    79,    80,    81,    82,    83,     0,     0,     0,
       0,     0,     0,    89,     0,   378,     0,     0,     0,   404,
     150,    75,    76,    77,    78,    79,    80,    81,    82,    83,
     274,    90,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   150,    75,    76,    77,    78,    79,    80,    81,
      82,    83,   276,   151,   152,   153,     0,     0,     0,     0,
       0,     0,   154,   155,   156,     0,     0,   157,   158,     0,
       0,     0,   159,     0,   160,     0,   151,   152,   153,   161,
     162,     0,     0,     0,     0,   154,   155,   156,     0,     0,
     157,   158,     0,     0,     0,   159,     0,   160,   151,   152,
     153,     0,   161,   162,     0,     0,     0,   154,   155,   156,
       0,     0,   157,   158,     0,     0,     0,   159,     0,   160,
       0,     0,     0,     0,   161,   162,   150,    75,    76,    77,
      78,    79,    80,    81,    82,    83,     0,     0,     0,     0,
       0,     0,     0,     0,   378,     0,     0,     0,   150,    75,
      76,    77,    78,    79,    80,    81,    82,    83,     0,     0,
       0,     0,     0,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   151,   152,   153,     0,     0,     0,     0,     0,
       0,   154,   155,   156,     0,   220,   157,   158,     0,     0,
       0,   159,   221,   160,   151,   152,   153,     0,   161,   162,
       0,     0,     0,   154,   155,   156,     0,     0,   157,   158,
       0,     0,     0,   159,     0,   160,     0,     0,     0,     0,
     161,   162,    28,     0,     0,   222,     0,     0,     0,     0,
      89,     0,     0,    29,    30,    31,    32,    33,     0,     0,
       0,     0,     0,     0,     0,   132,     0,     0,     0,    34,
      35,     0,    36,    37,     0,    38,    39,    40,    28,    41,
      42,     0,    43,    44,    45,    46,    47,     0,     0,    29,
      30,    31,    32,    33,     0,     0,     0,     0,     0,     0,
       0,   208,    48,     0,     0,    34,    35,     0,    36,    37,
       0,    38,    39,    40,    28,    41,    42,     0,    43,    44,
      45,    46,    47,     0,     0,    29,    30,    31,    32,    33,
       0,     0,     0,     0,     0,     0,     0,   306,    48,     0,
       0,    34,    35,     0,    36,    37,     0,    38,    39,    40,
      28,    41,    42,     0,    43,    44,    45,    46,    47,     0,
       0,    29,    30,    31,    32,    33,     0,     0,     0,     0,
       0,     0,     0,     0,    48,     0,     0,    34,    35,     0,
      36,    37,     0,    38,    39,    40,     0,    41,    42,     0,
      43,    44,    45,    46,    47,     0,   247,   248,   249,   250,
       0,   251,   252,   253,   254,   255,   256,   257,   258,   259,
      48,     0,     0,     0,   260,     0,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,     0,     0,   328,     0,
       0,     0,   271,   247,   248,   249,   250,     0,   251,   252,
     253,   254,   255,   256,   257,   258,   259,  -206,     0,     0,
       0,   260,     0,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,     0,     0,   247,   248,   249,   250,   271,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,  -206,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,   247,   248,   249,
     250,   271,   251,   252,   253,   254,   255,   256,   257,   258,
     259,     0,     0,     0,     0,   260,   281,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,     0,     0,   247,
     248,   249,   250,   271,   251,   252,   253,   254,   255,   256,
     257,   258,   259,     0,     0,     0,     0,   260,   301,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,     0,
       0,   247,   248,   249,   250,   271,   251,   252,   253,   254,
     255,   256,   257,   258,   259,     0,     0,     0,     0,   260,
     359,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,     0,     0,   247,   248,   249,   250,   271,   251,   252,
     253,   254,   255,   256,   257,   258,   259,     0,     0,     0,
       0,   260,   366,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,     0,     0,   247,   248,   249,   250,   271,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,  -205,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,   247,   248,   249,
     250,   271,   251,   252,   253,   254,   255,   256,   257,   258,
     259,     0,     0,     0,     0,   260,   391,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,     0,     0,   247,
     248,   249,   250,   271,   251,   252,   253,   254,   255,   256,
     257,   258,   259,     0,     0,     0,     0,   260,   413,   261,
     262,   263,   264,   265,   266,   267,   268,   269,   270,     0,
       0,     0,     0,     0,     0,   271,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   247,   248,   249,   250,   408,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,   319,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,     0,     0,     0,
     320,   271,   247,   248,   249,   250,     0,   251,   252,   253,
     254,   255,   256,   257,   258,   259,     0,     0,     0,     0,
     260,   285,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,     0,     0,   247,   248,   249,   250,   271,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,   260,   288,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,     0,   247,   248,   249,   250,
     271,   251,   252,   253,   254,   255,   256,   257,   258,   259,
       0,     0,     0,     0,   260,     0,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,     0,   321,   247,   248,
     249,   250,   271,   251,   252,   253,   254,   255,   256,   257,
     258,   259,     0,     0,     0,     0,   260,     0,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,     0,   322,
     247,   248,   249,   250,   271,   251,   252,   253,   254,   255,
     256,   257,   258,   259,     0,     0,     0,     0,   260,     0,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
       0,   324,   247,   248,   249,   250,   271,   251,   252,   253,
     254,   255,   256,   257,   258,   259,     0,     0,     0,     0,
     260,     0,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,     0,   327,   247,   248,   249,   250,   271,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,   260,   368,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,     0,   247,   248,   249,   250,
     271,   251,   252,   253,   254,   255,   256,   257,   258,   259,
       0,     0,     0,     0,   260,   388,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,     0,     0,   247,   248,
     249,   250,   271,   251,   252,   253,   254,   255,   256,   257,
     258,   259,     0,     0,     0,     0,   260,     0,   261,   262,
     263,   264,   265,   266,   267,   268,   269,   270,     0,     0,
       0,     0,     0,   392,   271,   247,   248,   249,   250,     0,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,     0,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,     0,     0,     0,
     393,   271,   247,   248,   249,   250,     0,   251,   252,   253,
     254,   255,   256,   257,   258,   259,     0,     0,     0,     0,
     260,     0,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,     0,   396,   247,   248,   249,   250,   271,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,   260,     0,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,     0,     0,     0,     0,   401,
     271,   247,   248,   249,   250,     0,   251,   252,   253,   254,
     255,   256,   257,   258,   259,     0,     0,     0,     0,   260,
     406,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,     0,     0,   247,   248,   249,   250,   271,   251,   252,
     253,   254,   255,   256,   257,   258,   259,     0,     0,     0,
       0,   260,     0,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,     0,   418,   247,   248,   249,   250,   271,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,     0,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,     0,     0,     0,
     419,   271,   247,   248,   249,   250,     0,   251,   252,   253,
     254,   255,   256,   257,   258,   259,     0,     0,     0,     0,
     260,     0,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,     0,   420,   247,   248,   249,   250,   271,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,   260,     0,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,   421,   247,   248,   249,   250,
     271,   251,   252,   253,   254,   255,   256,   257,   258,   259,
       0,     0,     0,     0,   260,     0,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,     0,     0,     0,     0,
       0,   425,   271,   247,   248,   249,   250,     0,   251,   252,
     253,   254,   255,   256,   257,   258,   259,     0,     0,     0,
       0,   260,   443,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,     0,     0,   247,   248,   249,   250,   271,
     251,   252,   253,   254,   255,   256,   257,   258,   259,     0,
       0,     0,     0,   260,   445,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,     0,     0,   247,   248,   249,
     250,   271,   251,   252,   253,   254,   255,   256,   257,   258,
     259,     0,     0,     0,     0,   260,     0,   261,   262,   263,
     264,   265,   266,   267,   268,   269,   270,     0,     0,     0,
       0,     0,   448,   271,   247,   248,   249,   250,     0,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,   260,     0,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,     0,     0,     0,     0,   449,
     271,   247,   248,   249,   250,     0,   251,   252,   253,   254,
     255,   256,   257,   258,   259,     0,     0,     0,     0,   260,
     452,   261,   262,   263,   264,   265,   266,   267,   268,   269,
     270,     0,     0,   247,   248,   249,   250,   271,   251,   252,
     253,   254,   255,   256,   257,   258,   259,     0,     0,     0,
       0,   260,     0,   261,   262,   263,   264,   265,   266,   267,
     268,   269,   270,     0,     0,     0,     0,     0,   454,   271,
     247,   248,   249,   250,     0,   251,   252,   253,   254,   255,
     256,   257,   258,   259,     0,     0,     0,     0,   260,     0,
     261,   262,   263,   264,   265,   266,   267,   268,   269,   270,
       0,     0,   247,     0,   249,   250,   271,   251,   252,   253,
     254,   255,   256,   257,   258,   259,     0,     0,     0,     0,
       0,     0,   261,   262,   263,   264,   265,   266,   267,   268,
     269,   270,     0,     0,     0,     0,   249,   250,   271,   251,
     252,   253,   254,   255,   256,   257,   258,   259,     0,     0,
       0,     0,     0,     0,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,     0,     0,     0,     0,   249,   250,
     271,   251,   252,   253,   254,   255,   256,   257,   258,   259,
       0,     0,     0,     0,     0,     0,     0,   262,   263,   264,
     265,   266,   267,   268,   269,   270,     0,     0,     0,     0,
     249,   250,   271,   251,   252,   253,   254,   255,   256,     0,
     258,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     263,   264,   265,   266,   267,   268,   269,   270,     0,     0,
       0,     0,     0,     0,   271
};

static const yytype_int16 yycheck[] =
{
      30,    35,    49,    33,   233,    33,    30,    41,    42,    43,
      44,   189,   189,    47,     3,   189,   189,    50,   189,     3,
       3,    79,    79,     3,     3,    79,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    65,    27,     3,    79,
       3,    99,    99,    73,    79,    99,    82,    79,    22,    73,
       3,    78,     3,    75,    84,    82,    78,     0,    32,    99,
      84,    79,    78,    38,    99,   101,    96,    99,   101,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    98,
     309,    99,    59,    60,    61,    19,    20,   134,    78,    78,
      24,    68,    69,    70,    98,    29,    73,    74,    82,    82,
      79,    78,    79,    80,    79,    39,   135,   103,    85,    86,
      78,   141,   290,   290,    79,   145,   290,   290,   148,   290,
     148,   145,   102,   100,    99,    78,    82,     3,    62,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    37,
      38,   316,    40,   318,    98,   101,    80,    45,    46,    98,
      99,   181,   182,   181,    75,    76,    77,   386,    78,   189,
      78,    35,   209,    84,    98,    53,    54,    82,    42,    98,
      99,   100,    89,    98,    99,   205,    98,    99,   100,    98,
      99,   100,    98,   100,    78,    73,    74,    75,    76,    77,
     107,    98,    99,   100,    78,   112,    84,    98,    99,   100,
      78,   118,    98,    99,   100,    78,    80,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    98,    99,   100,
     137,    98,    99,   100,   185,   186,   143,    98,    99,   146,
     147,    98,   149,    78,   151,   152,   153,   154,   155,   156,
     157,   158,   159,   160,   161,   162,    73,    74,    75,    76,
      77,    98,    99,   170,   171,   289,    98,    84,   101,   176,
     290,    98,    99,    59,    60,    61,    98,    99,    98,    99,
      98,    99,    68,    69,    70,   192,   101,    73,    74,    98,
      99,    78,    78,    79,    80,    99,   316,   204,   318,    85,
      86,    81,   322,    79,   324,    98,    99,    52,   322,     3,
     324,     3,     3,     3,   100,    79,    98,   101,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,     4,     5,
       6,     7,     8,     9,    10,    11,    12,   357,    78,    78,
     247,   248,   249,   250,   251,   252,   253,   254,   255,   256,
     257,   258,   259,   260,   261,   262,   263,   264,   265,   266,
     267,   268,   269,   270,   271,     3,   273,   274,    82,   276,
      78,    84,   279,    23,   281,    81,    50,    51,   285,    53,
      54,   288,    98,   403,    67,     3,   406,   407,   408,   403,
     408,     3,   406,   407,   301,    80,    82,    71,    72,    73,
      74,    75,    76,    77,    81,   312,    79,    82,     4,     4,
      84,   290,   319,   207,   321,   413,   323,   195,   438,   179,
     139,   328,    50,    51,   438,    53,    54,    55,    56,    57,
      58,   312,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    97,    71,    72,    73,    74,    75,    76,    77,
     400,   379,   359,   328,    -1,    -1,    84,    -1,    -1,   366,
      -1,   368,   369,    -1,   371,    -1,    -1,    -1,    -1,    -1,
     377,    -1,   379,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   388,    -1,    -1,   391,    -1,    -1,    -1,    59,    60,
      61,    -1,    -1,    -1,    -1,    -1,    -1,    68,    69,    70,
      -1,    -1,    73,    74,    -1,    -1,   413,    78,    -1,    80,
      -1,    -1,    -1,    -1,    85,    86,    -1,   424,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   433,   434,   435,   100,
      -1,    -1,    -1,    -1,    -1,    -1,   443,    -1,   445,    -1,
      -1,    -1,    -1,   450,    -1,   452,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,    -1,
      -1,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    29,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    39,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    68,    69,    70,    -1,    -1,    73,    74,    -1,    -1,
      -1,    78,    -1,    80,    62,    -1,    -1,    -1,    85,    86,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      -1,    -1,    80,   100,    -1,    18,    19,    20,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    28,    29,    -1,    31,    -1,
      98,    -1,    -1,    -1,    -1,    -1,    39,    -1,    41,    -1,
      43,    44,    -1,    -1,    47,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    -1,    -1,    62,
      63,    18,    19,    20,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    29,    -1,    31,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    39,    -1,    41,    -1,    43,    44,    -1,    -1,
      47,    -1,    -1,    -1,    -1,    98,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    62,    63,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    21,    -1,    -1,    -1,    25,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    68,    69,    70,    -1,    -1,    73,    74,    -1,
      -1,    -1,    78,    -1,    80,    -1,    59,    60,    61,    85,
      86,    -1,    -1,    -1,    -1,    68,    69,    70,    -1,    -1,
      73,    74,    -1,    -1,    -1,    78,    -1,    80,    59,    60,
      61,    -1,    85,    86,    -1,    -1,    -1,    68,    69,    70,
      -1,    -1,    73,    74,    -1,    -1,    -1,    78,    -1,    80,
      -1,    -1,    -1,    -1,    85,    86,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    21,    -1,    -1,    -1,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    59,    60,    61,    -1,    -1,    -1,    -1,    -1,
      -1,    68,    69,    70,    -1,    35,    73,    74,    -1,    -1,
      -1,    78,    42,    80,    59,    60,    61,    -1,    85,    86,
      -1,    -1,    -1,    68,    69,    70,    -1,    -1,    73,    74,
      -1,    -1,    -1,    78,    -1,    80,    -1,    -1,    -1,    -1,
      85,    86,     3,    -1,    -1,    75,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    14,    15,    16,    17,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    26,    -1,    -1,    -1,    30,
      31,    -1,    33,    34,    -1,    36,    37,    38,     3,    40,
      41,    -1,    43,    44,    45,    46,    47,    -1,    -1,    14,
      15,    16,    17,    18,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    26,    63,    -1,    -1,    30,    31,    -1,    33,    34,
      -1,    36,    37,    38,     3,    40,    41,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    14,    15,    16,    17,    18,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    26,    63,    -1,
      -1,    30,    31,    -1,    33,    34,    -1,    36,    37,    38,
       3,    40,    41,    -1,    43,    44,    45,    46,    47,    -1,
      -1,    14,    15,    16,    17,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    63,    -1,    -1,    30,    31,    -1,
      33,    34,    -1,    36,    37,    38,    -1,    40,    41,    -1,
      43,    44,    45,    46,    47,    -1,    48,    49,    50,    51,
      -1,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      63,    -1,    -1,    -1,    66,    -1,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    80,    -1,
      -1,    -1,    84,    48,    49,    50,    51,    -1,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    99,    -1,    -1,
      -1,    66,    -1,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    -1,    48,    49,    50,    51,    84,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    99,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    48,    49,    50,
      51,    84,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    -1,    -1,    -1,    66,    99,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    -1,    -1,    48,
      49,    50,    51,    84,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    -1,    -1,    -1,    66,    99,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    -1,
      -1,    48,    49,    50,    51,    84,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    66,
      99,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    -1,    -1,    48,    49,    50,    51,    84,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    66,    99,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    -1,    48,    49,    50,    51,    84,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    99,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    48,    49,    50,
      51,    84,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    -1,    -1,    -1,    66,    99,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    -1,    -1,    48,
      49,    50,    51,    84,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    -1,    -1,    -1,    -1,    66,    99,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    -1,
      -1,    -1,    -1,    -1,    -1,    84,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    48,    49,    50,    51,    98,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,
      83,    84,    48,    49,    50,    51,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    48,    49,    50,    51,    84,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    48,    49,    50,    51,
      84,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    -1,    66,    -1,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    79,    48,    49,
      50,    51,    84,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    -1,    -1,    -1,    -1,    66,    -1,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    79,
      48,    49,    50,    51,    84,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    66,    -1,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    79,    48,    49,    50,    51,    84,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      66,    -1,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    79,    48,    49,    50,    51,    84,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    48,    49,    50,    51,
      84,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    -1,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    48,    49,
      50,    51,    84,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    -1,    -1,    -1,    -1,    66,    -1,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      -1,    -1,    -1,    83,    84,    48,    49,    50,    51,    -1,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    -1,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,
      83,    84,    48,    49,    50,    51,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      66,    -1,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    79,    48,    49,    50,    51,    84,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    66,    -1,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,    83,
      84,    48,    49,    50,    51,    -1,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    -1,    -1,    48,    49,    50,    51,    84,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    66,    -1,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    79,    48,    49,    50,    51,    84,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    -1,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,
      83,    84,    48,    49,    50,    51,    -1,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      66,    -1,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    79,    48,    49,    50,    51,    84,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    66,    -1,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    79,    48,    49,    50,    51,
      84,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    -1,    66,    -1,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    -1,    -1,
      -1,    83,    84,    48,    49,    50,    51,    -1,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    -1,    48,    49,    50,    51,    84,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    -1,
      -1,    -1,    -1,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    -1,    -1,    48,    49,    50,
      51,    84,    53,    54,    55,    56,    57,    58,    59,    60,
      61,    -1,    -1,    -1,    -1,    66,    -1,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    -1,    -1,    -1,
      -1,    -1,    83,    84,    48,    49,    50,    51,    -1,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    66,    -1,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    -1,    -1,    -1,    83,
      84,    48,    49,    50,    51,    -1,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    -1,    -1,    48,    49,    50,    51,    84,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    -1,    -1,    -1,
      -1,    66,    -1,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    -1,    -1,    -1,    -1,    -1,    83,    84,
      48,    49,    50,    51,    -1,    53,    54,    55,    56,    57,
      58,    59,    60,    61,    -1,    -1,    -1,    -1,    66,    -1,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      -1,    -1,    48,    -1,    50,    51,    84,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    -1,    -1,    -1,    -1,
      -1,    -1,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    -1,    -1,    -1,    -1,    50,    51,    84,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    -1,    -1,
      -1,    -1,    -1,    -1,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    -1,    -1,    -1,    -1,    50,    51,
      84,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    -1,    -1,    -1,    -1,
      50,    51,    84,    53,    54,    55,    56,    57,    58,    -1,
      60,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      70,    71,    72,    73,    74,    75,    76,    77,    -1,    -1,
      -1,    -1,    -1,    -1,    84
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    22,    32,   105,   106,   107,   108,     3,     3,     0,
     107,   108,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    78,     3,    79,   109,    98,    79,    99,     3,    14,
      15,    16,    17,    18,    30,    31,    33,    34,    36,    37,
      38,    40,    41,    43,    44,    45,    46,    47,    63,   110,
     111,   112,   113,   114,   118,   119,   120,   121,   122,   123,
     124,   131,   133,   139,   149,    98,     3,    79,     3,   102,
     140,   143,   103,   161,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    19,    20,    29,    39,    62,    80,
      98,   151,   154,   155,   159,   165,    78,   152,   153,     3,
      78,   134,   137,   132,   154,   165,     3,    82,   127,   129,
     127,     3,    82,   125,   126,   134,   134,     3,    78,   135,
     136,   134,   127,   127,   127,   127,   134,   134,   127,     3,
     128,   130,    26,   111,   110,    98,    78,    78,    98,    99,
      75,    78,   151,    82,   151,   160,    78,    78,    78,    78,
       3,    59,    60,    61,    68,    69,    70,    73,    74,    78,
      80,    85,    86,   141,   164,   165,   166,   167,    98,    98,
      50,   101,   156,   165,    27,   153,    78,   164,    98,    99,
      98,    99,   101,   164,    98,    99,   100,    98,   164,    98,
      98,    98,    78,   164,    98,    99,    98,    98,    98,    98,
      98,    98,    98,    98,    82,   101,    98,    99,    26,   110,
      79,   100,   145,   146,   164,   100,   147,   148,   164,   143,
      35,    42,    75,   162,   163,   165,   164,    24,   151,   164,
     164,   154,   164,    78,   142,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,    48,    49,    50,
      51,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      66,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    84,    81,    99,    13,   164,    13,   164,    79,    52,
     164,    99,   137,   154,   165,    67,   129,   129,    67,    31,
     115,   116,   117,   118,   119,   123,   124,   131,   150,   151,
     164,    99,   136,   164,   165,   130,    26,     3,    79,    99,
       3,    79,    99,     3,     3,    79,    38,    79,    99,    67,
      83,    79,    79,    98,    79,   144,   146,    79,    80,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   101,   164,    99,
     164,   164,   164,   127,    28,   116,    99,   164,    67,    78,
     146,    78,     3,   148,   163,   163,   164,    82,    21,   157,
     158,   164,   151,   164,   151,    79,    99,   166,    67,   165,
     164,    99,    83,    83,    98,   164,    79,   164,   164,   164,
      78,    83,   164,    67,    25,   158,    67,    23,    98,   146,
      81,   164,    98,    99,   138,   164,     3,     3,    79,    83,
      79,    79,    79,   145,    82,    83,   151,   151,   151,   154,
      81,   138,    79,    99,    82,   101,    79,   164,    79,    79,
     164,   164,   164,    67,   151,    67,   164,   164,    83,    83,
      82,   164,    67,   164,    83
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,   104,   105,   106,   106,   106,   106,   107,   107,   107,
     107,   107,   107,   107,   107,   107,   108,   108,   108,   109,
     109,   110,   110,   111,   111,   111,   111,   111,   111,   111,
     111,   111,   111,   111,   111,   111,   111,   112,   113,   114,
     115,   115,   116,   116,   116,   116,   116,   116,   116,   117,
     118,   119,   120,   121,   122,   123,   123,   124,   125,   126,
     126,   127,   127,   127,   128,   128,   129,   129,   129,   129,
     129,   129,   130,   130,   130,   131,   132,   132,   133,   133,
     133,   133,   133,   133,   133,   134,   134,   135,   135,   136,
     136,   137,   137,   138,   138,   139,   140,   140,   141,   142,
     143,   143,   143,   143,   144,   144,   145,   145,   146,   146,
     147,   147,   148,   148,   149,   150,   151,   151,   151,   151,
     151,   151,   151,   151,   151,   152,   152,   153,   154,   154,
     155,   155,   156,   157,   157,   158,   158,   159,   160,   160,
     161,   161,   161,   162,   162,   162,   163,   163,   163,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   164,   164,
     164,   164,   164,   164,   164,   164,   164,   164,   165,   165,
     165,   165,   165,   165,   165,   165,   165,   165,   165,   165,
     165,   165,   165,   166,   166,   167,   167
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     1,     2,     2,     1,     1,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     8,     9,     7,     3,
       1,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     5,     2,     3,
       2,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     1,     1,
       6,     3,     3,     1,     3,     1,     1,     6,    11,    16,
       8,     3,     1,     6,     3,     3,     3,     1,     3,     3,
       3,     3,     3,     3,     3,     3,     1,     3,     1,     6,
       5,     8,     7,     3,     1,     3,     3,     1,     2,     3,
       4,     8,     3,     7,     3,     1,     3,     1,     5,     1,
       3,     1,     5,     1,     3,     1,     1,     2,     2,     5,
       7,     6,     9,     5,     1,     2,     1,     6,     3,     4,
       3,     4,     3,     2,     1,     3,     3,     3,     2,     1,
       4,     2,     4,     3,     3,     1,     1,     2,     2,     1,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     5,     1,     3,     6,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     7,
       6,    11,     3,     1,     3,     3,     1
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                  \
do                                                              \
  if (yychar == YYEMPTY)                                        \
    {                                                           \
      yychar = (Token);                                         \
      yylval = (Value);                                         \
      YYPOPSTACK (yylen);                                       \
      yystate = *yyssp;                                         \
      goto yybackup;                                            \
    }                                                           \
  else                                                          \
    {                                                           \
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;                                                  \
    }                                                           \
while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*----------------------------------------.
| Print this symbol's value on YYOUTPUT.  |
`----------------------------------------*/

static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyo = yyoutput;
  YYUSE (yyo);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  YYUSE (yytype);
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyoutput, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yytype_int16 *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  unsigned long int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[yyssp[yyi + 1 - yynrhs]],
                       &(yyvsp[(yyi + 1) - (yynrhs)])
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
yystrlen (const char *yystr)
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            /* Fall through.  */
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (YY_NULLPTR, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYSIZE_T yysize1 = yysize + yytnamerr (YY_NULLPTR, yytname[yyx]);
                  if (! (yysize <= yysize1
                         && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                    return 2;
                  yysize = yysize1;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    YYSIZE_T yysize1 = yysize + yystrlen (yyformat);
    if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
      return 2;
    yysize = yysize1;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        YYSTYPE *yyvs1 = yyvs;
        yytype_int16 *yyss1 = yyss;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * sizeof (*yyssp),
                    &yyvs1, yysize * sizeof (*yyvsp),
                    &yystacksize);

        yyss = yyss1;
        yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yytype_int16 *yyss1 = yyss;
        union yyalloc *yyptr =
          (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

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
     '$$ = $1'.

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
#line 124 "./verilog_bison.y" /* yacc.c:1646  */
    {next_parsed_verilog_file((yyvsp[0].node));}
#line 2069 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 128 "./verilog_bison.y" /* yacc.c:1646  */
    {((yyvsp[-1].node) != NULL)? (yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node)): (yyval.node) = newList(FILE_ITEMS, (yyvsp[0].node));}
#line 2075 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 129 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2081 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 130 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(FILE_ITEMS, (yyvsp[0].node));}
#line 2087 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 131 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL;}
#line 2093 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 135 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), LONG_LONG, UNSIGNED, yylineno), yylineno);}
#line 2099 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 136 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), DEC, UNSIGNED, yylineno), yylineno);}
#line 2105 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 137 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), OCT, UNSIGNED, yylineno), yylineno);}
#line 2111 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 138 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), HEX, UNSIGNED, yylineno), yylineno);}
#line 2117 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 139 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), BIN, UNSIGNED, yylineno), yylineno);}
#line 2123 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 140 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), DEC, SIGNED, yylineno), yylineno);}
#line 2129 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 141 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), OCT, SIGNED, yylineno), yylineno);}
#line 2135 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 142 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), HEX, SIGNED, yylineno), yylineno);}
#line 2141 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 143 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL; newConstant((yyvsp[-1].id_name), newNumberNode((yyvsp[0].num_value), BIN, SIGNED, yylineno), yylineno);}
#line 2147 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 148 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModule((yyvsp[-6].id_name), (yyvsp[-4].node), (yyvsp[-1].node), yylineno);}
#line 2153 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 149 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModule((yyvsp[-7].id_name), (yyvsp[-5].node), (yyvsp[-1].node), yylineno);}
#line 2159 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 150 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModule((yyvsp[-5].id_name), NULL, (yyvsp[-1].node), yylineno);}
#line 2165 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 155 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), newVarDeclare((yyvsp[0].id_name), NULL, NULL, NULL, NULL, NULL, yylineno));}
#line 2171 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 156 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(VAR_DECLARE_LIST, newVarDeclare((yyvsp[0].id_name), NULL, NULL, NULL, NULL, NULL, yylineno));}
#line 2177 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 160 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node));}
#line 2183 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 161 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(MODULE_ITEMS, (yyvsp[0].node));}
#line 2189 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 165 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2195 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 166 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2201 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 167 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2207 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 168 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2213 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 169 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2219 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 170 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2225 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 171 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2231 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 172 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2237 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 173 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2243 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 174 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2249 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 175 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2255 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 176 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2261 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 177 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2267 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 178 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2273 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 182 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newFunction((yyvsp[-3].node), (yyvsp[-1].node), yylineno); }
#line 2279 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 186 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newInitial((yyvsp[0].node), yylineno); }
#line 2285 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 190 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2291 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 194 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node));}
#line 2297 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 195 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(FUNCTION_ITEMS, (yyvsp[0].node));}
#line 2303 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 198 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2309 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 199 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2315 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 200 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2321 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 201 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2327 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 202 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2333 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 203 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2339 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 204 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2345 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 208 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(FUNCTION, INPUT, (yyvsp[-1].node));}
#line 2351 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 212 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE,PARAMETER, (yyvsp[-1].node));}
#line 2357 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 216 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newDefparam(MODULE_PARAMETER_LIST, (yyvsp[-1].node), yylineno);}
#line 2363 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 220 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE,INPUT, (yyvsp[-1].node));}
#line 2369 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 224 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE,OUTPUT, (yyvsp[-1].node));}
#line 2375 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 228 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE,INOUT, (yyvsp[-1].node));}
#line 2381 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 232 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE, WIRE, (yyvsp[-1].node));}
#line 2387 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 233 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE, REG, (yyvsp[-1].node));}
#line 2393 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 237 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = markAndProcessSymbolListWith(MODULE,INTEGER, (yyvsp[-1].node));}
#line 2399 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 241 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(VAR_DECLARE_LIST, (yyvsp[0].node));}
#line 2405 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 245 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[0].id_name), NULL, NULL, NULL, NULL, NULL, yylineno);}
#line 2411 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 246 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[0].id_name), (yyvsp[-4].node), (yyvsp[-2].node), NULL, NULL, NULL, yylineno);}
#line 2417 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 250 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2423 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 251 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2429 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 252 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(VAR_DECLARE_LIST, (yyvsp[0].node));}
#line 2435 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 256 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2441 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 257 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(VAR_DECLARE_LIST, (yyvsp[0].node));}
#line 2447 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 261 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[0].id_name), NULL, NULL, NULL, NULL, NULL, yylineno);}
#line 2453 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 262 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[0].id_name), (yyvsp[-4].node), (yyvsp[-2].node), NULL, NULL, NULL, yylineno);}
#line 2459 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 68:
#line 263 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[-5].id_name), (yyvsp[-9].node), (yyvsp[-7].node), (yyvsp[-3].node), (yyvsp[-1].node), NULL, yylineno);}
#line 2465 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 69:
#line 264 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare2D((yyvsp[-10].id_name), (yyvsp[-14].node), (yyvsp[-12].node), (yyvsp[-8].node), (yyvsp[-6].node),(yyvsp[-3].node),(yyvsp[-1].node), NULL, yylineno);}
#line 2471 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 70:
#line 266 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[-2].id_name), (yyvsp[-6].node), (yyvsp[-4].node), NULL, NULL, (yyvsp[0].node), yylineno);}
#line 2477 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 71:
#line 267 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newVarDeclare((yyvsp[-2].id_name), NULL, NULL, NULL, NULL, (yyvsp[0].node), yylineno);}
#line 2483 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 72:
#line 271 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIntegerTypeVarDeclare((yyvsp[0].id_name), NULL, NULL, NULL, NULL, NULL, yylineno);}
#line 2489 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 73:
#line 273 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIntegerTypeVarDeclare((yyvsp[-5].id_name), NULL, NULL, (yyvsp[-3].node), (yyvsp[-1].node), NULL, yylineno);}
#line 2495 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 74:
#line 276 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIntegerTypeVarDeclare((yyvsp[-2].id_name), NULL, NULL, NULL, NULL, (yyvsp[0].node), yylineno);}
#line 2501 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 75:
#line 280 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2507 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 76:
#line 284 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2513 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 77:
#line 285 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(ASSIGN, (yyvsp[0].node));}
#line 2519 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 78:
#line 290 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_AND, (yyvsp[-1].node), yylineno);}
#line 2525 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 79:
#line 291 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_NAND, (yyvsp[-1].node), yylineno);}
#line 2531 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 80:
#line 292 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_NOR, (yyvsp[-1].node), yylineno);}
#line 2537 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 81:
#line 293 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_NOT, (yyvsp[-1].node), yylineno);}
#line 2543 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 82:
#line 294 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_OR, (yyvsp[-1].node), yylineno);}
#line 2549 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 83:
#line 295 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_XNOR, (yyvsp[-1].node), yylineno);}
#line 2555 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 84:
#line 296 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGate(BITWISE_XOR, (yyvsp[-1].node), yylineno);}
#line 2561 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 85:
#line 300 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2567 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 86:
#line 301 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(ONE_GATE_INSTANCE, (yyvsp[0].node));}
#line 2573 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 87:
#line 305 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2579 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 88:
#line 306 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(ONE_GATE_INSTANCE, (yyvsp[0].node));}
#line 2585 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 89:
#line 310 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGateInstance((yyvsp[-5].id_name), (yyvsp[-3].node), (yyvsp[-1].node), NULL, yylineno);}
#line 2591 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 90:
#line 311 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newGateInstance(NULL, (yyvsp[-3].node), (yyvsp[-1].node), NULL, yylineno);}
#line 2597 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 91:
#line 315 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newMultipleInputsGateInstance((yyvsp[-7].id_name), (yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-1].node), yylineno);}
#line 2603 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 92:
#line 316 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newMultipleInputsGateInstance(NULL, (yyvsp[-5].node), (yyvsp[-3].node), (yyvsp[-1].node), yylineno);}
#line 2609 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 93:
#line 321 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2615 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 94:
#line 322 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleConnection(NULL, (yyvsp[0].node), yylineno);}
#line 2621 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 95:
#line 326 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleInstance((yyvsp[-2].id_name), (yyvsp[-1].node), yylineno);}
#line 2627 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 96:
#line 331 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2633 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 97:
#line 332 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(ONE_MODULE_INSTANCE, (yyvsp[0].node));}
#line 2639 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 98:
#line 336 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newFunctionInstance((yyvsp[-1].id_name), (yyvsp[0].node), yylineno);}
#line 2645 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 99:
#line 342 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newFunctionNamedInstance((yyvsp[-1].node), NULL, yylineno);}
#line 2651 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 100:
#line 346 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleNamedInstance((yyvsp[-3].id_name), (yyvsp[-1].node), NULL, yylineno);}
#line 2657 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 101:
#line 347 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleNamedInstance((yyvsp[-3].id_name), (yyvsp[-1].node), (yyvsp[-5].node), yylineno); }
#line 2663 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 102:
#line 348 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleNamedInstance((yyvsp[-2].id_name), NULL, NULL, yylineno);}
#line 2669 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 103:
#line 349 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleNamedInstance((yyvsp[-2].id_name), NULL, (yyvsp[-4].node), yylineno);}
#line 2675 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 104:
#line 353 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2681 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 105:
#line 354 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newfunctionList(MODULE_CONNECT_LIST, (yyvsp[0].node));}
#line 2687 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 106:
#line 357 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2693 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 107:
#line 358 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(MODULE_CONNECT_LIST, (yyvsp[0].node));}
#line 2699 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 108:
#line 362 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleConnection((yyvsp[-3].id_name), (yyvsp[-1].node), yylineno);}
#line 2705 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 109:
#line 363 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleConnection(NULL, (yyvsp[0].node), yylineno);}
#line 2711 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 110:
#line 367 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2717 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 111:
#line 368 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(MODULE_PARAMETER_LIST, (yyvsp[0].node));}
#line 2723 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 112:
#line 372 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleParameter((yyvsp[-3].id_name), (yyvsp[-1].node), yylineno);}
#line 2729 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 113:
#line 373 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newModuleParameter(NULL, (yyvsp[0].node), yylineno);}
#line 2735 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 114:
#line 378 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newAlways((yyvsp[-1].node), (yyvsp[0].node), yylineno);}
#line 2741 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 115:
#line 382 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newAlways(NULL, (yyvsp[0].node), yylineno);}
#line 2747 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 116:
#line 386 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2753 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 117:
#line 387 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2759 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 118:
#line 388 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2765 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 119:
#line 389 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIf((yyvsp[-2].node), (yyvsp[0].node), NULL, yylineno);}
#line 2771 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 120:
#line 390 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIf((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2777 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 121:
#line 391 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newCase((yyvsp[-3].node), (yyvsp[-1].node), yylineno);}
#line 2783 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 122:
#line 392 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newFor((yyvsp[-6].node), (yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2789 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 123:
#line 393 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newWhile((yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2795 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 124:
#line 394 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL;}
#line 2801 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 125:
#line 398 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node));}
#line 2807 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 126:
#line 399 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(SPECIFY_PAL_CONNECT_LIST, (yyvsp[0].node));}
#line 2813 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 127:
#line 403 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newParallelConnection((yyvsp[-4].node), (yyvsp[-1].node), yylineno);}
#line 2819 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 128:
#line 407 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBlocking((yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2825 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 129:
#line 408 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBlocking((yyvsp[-3].node), (yyvsp[0].node), yylineno);}
#line 2831 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 130:
#line 412 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNonBlocking((yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2837 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 131:
#line 413 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNonBlocking((yyvsp[-3].node), (yyvsp[0].node), yylineno);}
#line 2843 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 132:
#line 417 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL;}
#line 2849 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 133:
#line 421 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node));}
#line 2855 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 134:
#line 422 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(CASE_LIST, (yyvsp[0].node));}
#line 2861 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 135:
#line 426 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newCaseItem((yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 2867 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 136:
#line 427 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newDefaultCase((yyvsp[0].node), yylineno);}
#line 2873 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 137:
#line 431 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2879 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 138:
#line 435 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-1].node), (yyvsp[0].node));}
#line 2885 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 139:
#line 436 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(BLOCK, (yyvsp[0].node));}
#line 2891 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 140:
#line 440 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 2897 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 141:
#line 441 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL;}
#line 2903 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 142:
#line 442 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = NULL;}
#line 2909 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 143:
#line 447 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2915 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 144:
#line 448 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node));}
#line 2921 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 145:
#line 449 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(DELAY_CONTROL, (yyvsp[0].node));}
#line 2927 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 146:
#line 453 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2933 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 147:
#line 454 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newPosedgeSymbol((yyvsp[0].id_name), yylineno);}
#line 2939 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 148:
#line 455 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNegedgeSymbol((yyvsp[0].id_name), yylineno);}
#line 2945 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 149:
#line 459 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 2951 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 150:
#line 460 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(ADD, (yyvsp[0].node), yylineno);}
#line 2957 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 151:
#line 461 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(MINUS, (yyvsp[0].node), yylineno);}
#line 2963 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 152:
#line 462 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_NOT, (yyvsp[0].node), yylineno);}
#line 2969 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 153:
#line 463 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_AND, (yyvsp[0].node), yylineno);}
#line 2975 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 154:
#line 464 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_OR, (yyvsp[0].node), yylineno);}
#line 2981 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 155:
#line 465 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_NAND, (yyvsp[0].node), yylineno);}
#line 2987 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 156:
#line 466 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_NOR, (yyvsp[0].node), yylineno);}
#line 2993 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 157:
#line 467 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_XNOR, (yyvsp[0].node), yylineno);}
#line 2999 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 158:
#line 468 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(LOGICAL_NOT, (yyvsp[0].node), yylineno);}
#line 3005 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 159:
#line 469 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newUnaryOperation(BITWISE_XOR, (yyvsp[0].node), yylineno);}
#line 3011 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 160:
#line 470 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_XOR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3017 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 161:
#line 471 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newExpandPower(MULTIPLY,(yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3023 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 162:
#line 472 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(MULTIPLY, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3029 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 163:
#line 473 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(DIVIDE, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3035 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 164:
#line 474 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(MODULO, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3041 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 165:
#line 475 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(ADD, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3047 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 166:
#line 476 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(MINUS, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3053 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 167:
#line 477 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_AND, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3059 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 168:
#line 478 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_OR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3065 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 169:
#line 479 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_NAND, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3071 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 170:
#line 480 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_NOR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3077 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 171:
#line 481 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(BITWISE_XNOR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3083 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 172:
#line 482 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(LT, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3089 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 173:
#line 483 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(GT, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3095 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 174:
#line 484 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(SR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3101 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 175:
#line 485 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(SL, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3107 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 176:
#line 486 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(LOGICAL_EQUAL, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3113 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 177:
#line 487 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(NOT_EQUAL, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3119 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 178:
#line 488 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(LTE, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3125 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 179:
#line 489 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(GTE, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3131 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 180:
#line 490 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(CASE_EQUAL, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3137 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 181:
#line 491 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(CASE_NOT_EQUAL, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3143 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 182:
#line 492 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(LOGICAL_OR, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3149 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 183:
#line 493 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newBinaryOperation(LOGICAL_AND, (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3155 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 184:
#line 494 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newIfQuestion((yyvsp[-4].node), (yyvsp[-2].node), (yyvsp[0].node), yylineno);}
#line 3161 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 185:
#line 495 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 3167 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 186:
#line 496 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node);}
#line 3173 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 187:
#line 497 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newListReplicate( (yyvsp[-4].node), (yyvsp[-2].node) ); }
#line 3179 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 188:
#line 501 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), LONG_LONG, UNSIGNED, yylineno);}
#line 3185 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 189:
#line 502 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), DEC, UNSIGNED, yylineno);}
#line 3191 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 190:
#line 503 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), OCT, UNSIGNED, yylineno);}
#line 3197 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 191:
#line 504 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), HEX, UNSIGNED, yylineno);}
#line 3203 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 192:
#line 505 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), BIN, UNSIGNED, yylineno);}
#line 3209 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 193:
#line 506 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), DEC, SIGNED, yylineno);}
#line 3215 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 194:
#line 507 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), OCT, SIGNED, yylineno);}
#line 3221 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 195:
#line 508 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), HEX, SIGNED, yylineno);}
#line 3227 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 196:
#line 509 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newNumberNode((yyvsp[0].num_value), BIN, SIGNED, yylineno);}
#line 3233 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 197:
#line 510 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newSymbolNode((yyvsp[0].id_name), yylineno);}
#line 3239 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 198:
#line 511 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newArrayRef((yyvsp[-3].id_name), (yyvsp[-1].node), yylineno);}
#line 3245 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 199:
#line 512 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newArrayRef2D((yyvsp[-6].id_name), (yyvsp[-4].node), (yyvsp[-1].node), yylineno);}
#line 3251 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 200:
#line 513 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newRangeRef((yyvsp[-5].id_name), (yyvsp[-3].node), (yyvsp[-1].node), yylineno);}
#line 3257 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 201:
#line 514 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newRangeRef2D((yyvsp[-10].id_name), (yyvsp[-8].node), (yyvsp[-6].node), (yyvsp[-3].node), (yyvsp[-1].node), yylineno);}
#line 3263 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 202:
#line 515 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[-1].node); ((yyvsp[-1].node))->types.concat.num_bit_strings = -1;}
#line 3269 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 203:
#line 519 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = (yyvsp[0].node);}
#line 3275 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 204:
#line 520 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node)); /* note this will be in order lsb = greatest to msb = 0 in the node child list */}
#line 3281 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 205:
#line 524 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList_entry((yyvsp[-2].node), (yyvsp[0].node)); /* note this will be in order lsb = greatest to msb = 0 in the node child list */}
#line 3287 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;

  case 206:
#line 525 "./verilog_bison.y" /* yacc.c:1646  */
    {(yyval.node) = newList(CONCATENATE, (yyvsp[0].node));}
#line 3293 "verilog_bison.tab.c" /* yacc.c:1646  */
    break;


#line 3297 "verilog_bison.tab.c" /* yacc.c:1646  */
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 528 "./verilog_bison.y" /* yacc.c:1906  */

