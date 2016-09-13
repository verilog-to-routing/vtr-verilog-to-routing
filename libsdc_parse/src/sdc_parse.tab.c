/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

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
#define YYBISON_VERSION "3.0.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* Copy the first part of user declarations.  */
#line 1 "src/sdc_parse.y" /* yacc.c:339  */


#include <stdio.h>
#include "assert.h"

#include "sdc_common.h"


int yyerror(const char *msg);
extern int yylex(void);


#line 79 "src/sdc_parse.tab.c" /* yacc.c:339  */

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
# define YYERROR_VERBOSE 1
#endif

/* In a future release of Bison, this section will be replaced
   by #include "sdc_parse.tab.h".  */
#ifndef YY_YY_SRC_SDC_PARSE_TAB_H_INCLUDED
# define YY_YY_SRC_SDC_PARSE_TAB_H_INCLUDED
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
    CMD_CREATE_CLOCK = 258,
    CMD_SET_CLOCK_GROUPS = 259,
    CMD_SET_FALSE_PATH = 260,
    CMD_SET_MAX_DELAY = 261,
    CMD_SET_MULTICYCLE_PATH = 262,
    CMD_SET_INPUT_DELAY = 263,
    CMD_SET_OUTPUT_DELAY = 264,
    CMD_GET_PORTS = 265,
    CMD_GET_CLOCKS = 266,
    ARG_PERIOD = 267,
    ARG_WAVEFORM = 268,
    ARG_NAME = 269,
    ARG_EXCLUSIVE = 270,
    ARG_GROUP = 271,
    ARG_FROM = 272,
    ARG_TO = 273,
    ARG_SETUP = 274,
    ARG_CLOCK = 275,
    ARG_MAX = 276,
    EOL = 277,
    STRING = 278,
    ESCAPED_STRING = 279,
    CHAR = 280,
    FLOAT_NUMBER = 281,
    INT_NUMBER = 282
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 14 "src/sdc_parse.y" /* yacc.c:355  */

    char* strVal;
    double floatVal;
    int intVal;

    t_sdc_commands* sdc_commands;

    t_sdc_create_clock* create_clock;
    t_sdc_set_io_delay* set_io_delay;
    t_sdc_set_clock_groups* set_clock_groups;
    t_sdc_set_false_path* set_false_path;
    t_sdc_set_max_delay* set_max_delay;
    t_sdc_set_multicycle_path* set_multicycle_path;

    t_sdc_string_group* string_group;


#line 165 "src/sdc_parse.tab.c" /* yacc.c:355  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SRC_SDC_PARSE_TAB_H_INCLUDED  */

/* Copy the second part of user declarations.  */

#line 180 "src/sdc_parse.tab.c" /* yacc.c:358  */

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
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   221

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  32
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  16
/* YYNRULES -- Number of rules.  */
#define YYNRULES  67
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  129

/* YYTRANSLATE[YYX] -- Symbol number corresponding to YYX as returned
   by yylex, with out-of-bounds checking.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX)                                                \
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, without out-of-bounds checking.  */
static const yytype_uint8 yytranslate[] =
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
       2,    30,     2,    31,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    28,     2,    29,     2,     2,     2,     2,
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
      25,    26,    27
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_uint8 yyrline[] =
{
       0,    87,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    98,    99,   100,   101,   102,   103,   106,   107,   108,
     109,   112,   113,   114,   115,   118,   119,   120,   121,   122,
     125,   126,   127,   128,   129,   130,   131,   134,   135,   136,
     137,   138,   139,   140,   141,   144,   145,   146,   147,   148,
     149,   150,   151,   152,   155,   156,   157,   160,   161,   162,
     165,   166,   168,   169,   172,   173,   176,   179
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 1
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "CMD_CREATE_CLOCK",
  "CMD_SET_CLOCK_GROUPS", "CMD_SET_FALSE_PATH", "CMD_SET_MAX_DELAY",
  "CMD_SET_MULTICYCLE_PATH", "CMD_SET_INPUT_DELAY", "CMD_SET_OUTPUT_DELAY",
  "CMD_GET_PORTS", "CMD_GET_CLOCKS", "ARG_PERIOD", "ARG_WAVEFORM",
  "ARG_NAME", "ARG_EXCLUSIVE", "ARG_GROUP", "ARG_FROM", "ARG_TO",
  "ARG_SETUP", "ARG_CLOCK", "ARG_MAX", "EOL", "STRING", "ESCAPED_STRING",
  "CHAR", "FLOAT_NUMBER", "INT_NUMBER", "'{'", "'}'", "'['", "']'",
  "$accept", "sdc_commands", "cmd_create_clock", "cmd_set_input_delay",
  "cmd_set_output_delay", "cmd_set_clock_groups", "cmd_set_false_path",
  "cmd_set_max_delay", "cmd_set_multicycle_path", "cmd_get_ports",
  "cmd_get_clocks", "stringGroup", "string", "number", "float_number",
  "int_number", YY_NULLPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   123,   125,
      91,    93
};
# endif

#define YYPACT_NINF -67

#define yypact_value_is_default(Yystate) \
  (!!((Yystate) == (-67)))

#define YYTABLE_NINF -1

#define yytable_value_is_error(Yytable_value) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -67,    68,   -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,
     -67,    84,    13,    93,    -9,   199,    14,    82,    -4,   -27,
      23,   -67,   -67,   -67,   -67,   -67,    23,    -4,   -67,    -6,
      23,    -4,   -67,    -6,   -67,    35,   -67,    56,   138,   -67,
     141,   149,   -67,   -67,   -67,   -67,   -67,   -67,   152,   160,
     -67,   -67,   -67,   -67,    -4,   -67,    32,   -67,   -67,   -67,
      -3,   -67,   -67,    94,   -67,    -1,   -67,   -67,    -1,   -67,
     -67,    -1,   -67,   -67,    -1,   -67,   -67,    -1,   -67,   -67,
      -1,   -67,   -67,    -1,   -67,    -4,   -67,   -67,   -67,   -67,
     -67,   -67,    87,   -67,    96,   162,   105,   169,   107,   171,
     116,   173,   118,   180,   127,   182,   129,    15,   184,   -67,
     -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,
     -67,   -67,   -67,   -67,   -67,   -67,   -67,   191,   -67
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,    11,    25,    30,    37,    45,    17,    21,
      10,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     3,    62,    63,    60,    16,     0,     0,     4,     0,
       0,     0,     5,     0,    26,     0,     6,     0,     0,     7,
       0,     0,     8,    66,    67,    38,    64,    65,     0,     0,
      47,     9,    46,    12,     0,    13,     0,    18,    19,    54,
       0,    22,    23,     0,    60,     0,    29,    60,     0,    35,
      60,     0,    36,    60,     0,    43,    60,     0,    44,    60,
       0,    52,    60,     0,    53,     0,    15,    61,    60,    20,
      56,    24,     0,    57,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    28,
      60,    27,    59,    33,    31,    34,    32,    41,    39,    42,
      40,    50,    48,    51,    49,    14,    55,     0,    58
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,   -67,    17,
     -66,   -22,   -11,   -15,   -67,     1
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
      -1,     1,    11,    12,    13,    14,    15,    16,    17,    60,
      94,    56,    87,    45,    46,    47
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_uint8 yytable[] =
{
      25,    54,    96,    53,    59,    98,    34,    35,   100,    55,
      93,   102,    58,    36,   104,    57,    62,   106,    52,    61,
      22,    23,    43,    44,    66,    88,    69,    72,    89,    75,
      78,    40,    41,    26,    27,    28,    42,    81,    84,    85,
      43,    44,    92,    29,   125,    95,    22,    23,    97,    90,
      63,    99,    90,     0,   101,    22,    23,   103,    22,    23,
     105,    86,     0,    64,     0,    65,   108,     0,     2,     0,
     107,     3,     4,     5,     6,     7,     8,     9,     0,    22,
      23,     0,     0,   112,    67,   112,    68,   112,   127,   112,
      10,   112,     0,   112,     0,   112,    18,    19,    20,    48,
      49,    50,     0,     0,    51,     0,    21,    22,    23,    44,
      22,    23,    24,    30,    31,    32,   109,    22,    23,    22,
      23,     0,    88,    33,   110,    91,     0,   111,    22,    23,
      22,    23,     0,   110,     0,   110,   114,     0,   116,    22,
      23,    22,    23,     0,   110,     0,   110,   118,     0,   120,
      22,    23,    22,    23,     0,   110,     0,   110,   122,     0,
     124,    22,    23,     0,    22,    23,    70,     0,    71,    73,
       0,    74,    22,    23,     0,    22,    23,    76,     0,    77,
      79,     0,    80,    22,    23,    22,    23,     0,    82,     0,
      83,   113,    22,    23,    22,    23,    22,    23,   115,     0,
     117,     0,   119,    22,    23,    22,    23,    22,    23,   121,
       0,   123,     0,   126,    22,    23,    37,    38,     0,     0,
     128,    39
};

static const yytype_int8 yycheck[] =
{
      11,    28,    68,    18,    10,    71,    15,    16,    74,    20,
      11,    77,    27,    22,    80,    26,    31,    83,    17,    30,
      23,    24,    26,    27,    35,    28,    37,    38,    31,    40,
      41,    17,    18,    20,    21,    22,    22,    48,    49,    54,
      26,    27,    64,    30,    29,    67,    23,    24,    70,    60,
      33,    73,    63,    -1,    76,    23,    24,    79,    23,    24,
      82,    29,    -1,    28,    -1,    30,    88,    -1,     0,    -1,
      85,     3,     4,     5,     6,     7,     8,     9,    -1,    23,
      24,    -1,    -1,    94,    28,    96,    30,    98,   110,   100,
      22,   102,    -1,   104,    -1,   106,    12,    13,    14,    17,
      18,    19,    -1,    -1,    22,    -1,    22,    23,    24,    27,
      23,    24,    28,    20,    21,    22,    29,    23,    24,    23,
      24,    -1,    28,    30,    28,    31,    -1,    31,    23,    24,
      23,    24,    -1,    28,    -1,    28,    31,    -1,    31,    23,
      24,    23,    24,    -1,    28,    -1,    28,    31,    -1,    31,
      23,    24,    23,    24,    -1,    28,    -1,    28,    31,    -1,
      31,    23,    24,    -1,    23,    24,    28,    -1,    30,    28,
      -1,    30,    23,    24,    -1,    23,    24,    28,    -1,    30,
      28,    -1,    30,    23,    24,    23,    24,    -1,    28,    -1,
      30,    29,    23,    24,    23,    24,    23,    24,    29,    -1,
      29,    -1,    29,    23,    24,    23,    24,    23,    24,    29,
      -1,    29,    -1,    29,    23,    24,    17,    18,    -1,    -1,
      29,    22
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    33,     0,     3,     4,     5,     6,     7,     8,     9,
      22,    34,    35,    36,    37,    38,    39,    40,    12,    13,
      14,    22,    23,    24,    28,    44,    20,    21,    22,    30,
      20,    21,    22,    30,    15,    16,    22,    17,    18,    22,
      17,    18,    22,    26,    27,    45,    46,    47,    17,    18,
      19,    22,    47,    45,    28,    44,    43,    44,    45,    10,
      41,    44,    45,    41,    28,    30,    44,    28,    30,    44,
      28,    30,    44,    28,    30,    44,    28,    30,    44,    28,
      30,    44,    28,    30,    44,    45,    29,    44,    28,    31,
      44,    31,    43,    11,    42,    43,    42,    43,    42,    43,
      42,    43,    42,    43,    42,    43,    42,    45,    43,    29,
      28,    31,    44,    29,    31,    29,    31,    29,    31,    29,
      31,    29,    31,    29,    31,    29,    29,    43,    29
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    32,    33,    33,    33,    33,    33,    33,    33,    33,
      33,    34,    34,    34,    34,    34,    34,    35,    35,    35,
      35,    36,    36,    36,    36,    37,    37,    37,    37,    37,
      38,    38,    38,    38,    38,    38,    38,    39,    39,    39,
      39,    39,    39,    39,    39,    40,    40,    40,    40,    40,
      40,    40,    40,    40,    41,    41,    41,    42,    42,    42,
      43,    43,    44,    44,    45,    45,    46,    47
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     3,     3,     3,     3,     3,     3,     3,
       2,     1,     3,     3,     6,     4,     2,     1,     3,     3,
       4,     1,     3,     3,     4,     1,     2,     5,     5,     3,
       1,     5,     5,     5,     5,     3,     3,     1,     2,     5,
       5,     5,     5,     3,     3,     1,     2,     2,     5,     5,
       5,     5,     3,     3,     1,     4,     2,     1,     4,     2,
       0,     2,     1,     1,     1,     1,     1,     1
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
#line 87 "src/sdc_parse.y" /* yacc.c:1646  */
    { g_sdc_commands = alloc_sdc_commands(); (yyval.sdc_commands) = g_sdc_commands; }
#line 1361 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 3:
#line 88 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_create_clock((yyvsp[-2].sdc_commands), (yyvsp[-1].create_clock)); }
#line 1367 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 4:
#line 89 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_io_delay((yyvsp[-2].sdc_commands), (yyvsp[-1].set_io_delay)); }
#line 1373 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 5:
#line 90 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_io_delay((yyvsp[-2].sdc_commands), (yyvsp[-1].set_io_delay)); }
#line 1379 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 6:
#line 91 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_clock_groups((yyvsp[-2].sdc_commands), (yyvsp[-1].set_clock_groups)); }
#line 1385 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 7:
#line 92 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_false_path((yyvsp[-2].sdc_commands), (yyvsp[-1].set_false_path)); }
#line 1391 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 8:
#line 93 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_max_delay((yyvsp[-2].sdc_commands), (yyvsp[-1].set_max_delay)); }
#line 1397 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 9:
#line 94 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.sdc_commands) = add_sdc_set_multicycle_path((yyvsp[-2].sdc_commands), (yyvsp[-1].set_multicycle_path)); }
#line 1403 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 10:
#line 95 "src/sdc_parse.y" /* yacc.c:1646  */
    { /* Eat stray EOL symbols */ (yyval.sdc_commands) = (yyvsp[-1].sdc_commands); }
#line 1409 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 11:
#line 98 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = alloc_sdc_create_clock(); }
#line 1415 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 12:
#line 99 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = sdc_create_clock_set_period((yyvsp[-2].create_clock), (yyvsp[0].floatVal)); }
#line 1421 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 13:
#line 100 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = sdc_create_clock_set_name((yyvsp[-2].create_clock), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1427 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 14:
#line 101 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = sdc_create_clock_set_waveform((yyvsp[-5].create_clock), (yyvsp[-2].floatVal), (yyvsp[-1].floatVal)); }
#line 1433 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 15:
#line 102 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = sdc_create_clock_add_targets((yyvsp[-3].create_clock), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1439 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 16:
#line 103 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.create_clock) = sdc_create_clock_add_targets((yyvsp[-1].create_clock), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal))); free((yyvsp[0].strVal)); }
#line 1445 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 17:
#line 106 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = alloc_sdc_set_io_delay(SDC_INPUT_DELAY); }
#line 1451 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 18:
#line 107 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_clock((yyvsp[-2].set_io_delay), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1457 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 19:
#line 108 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_max_value((yyvsp[-2].set_io_delay), (yyvsp[0].floatVal)); }
#line 1463 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 20:
#line 109 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_ports((yyvsp[-3].set_io_delay), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1469 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 21:
#line 112 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = alloc_sdc_set_io_delay(SDC_OUTPUT_DELAY); }
#line 1475 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 22:
#line 113 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_clock((yyvsp[-2].set_io_delay), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1481 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 23:
#line 114 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_max_value((yyvsp[-2].set_io_delay), (yyvsp[0].floatVal)); }
#line 1487 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 24:
#line 115 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_io_delay) = sdc_set_io_delay_set_ports((yyvsp[-3].set_io_delay), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1493 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 25:
#line 118 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_clock_groups) = alloc_sdc_set_clock_groups(); }
#line 1499 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 26:
#line 119 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_clock_groups) = sdc_set_clock_groups_set_type((yyvsp[-1].set_clock_groups), SDC_CG_EXCLUSIVE); }
#line 1505 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 27:
#line 120 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_clock_groups) = sdc_set_clock_groups_add_group((yyvsp[-4].set_clock_groups), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1511 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 28:
#line 121 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_clock_groups) = sdc_set_clock_groups_add_group((yyvsp[-4].set_clock_groups), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group));}
#line 1517 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 29:
#line 122 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_clock_groups) = sdc_set_clock_groups_add_group((yyvsp[-2].set_clock_groups), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal))); free((yyvsp[0].strVal)); }
#line 1523 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 30:
#line 125 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = alloc_sdc_set_false_path(); }
#line 1529 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 31:
#line 126 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-4].set_false_path), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1535 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 32:
#line 127 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-4].set_false_path), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1541 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 33:
#line 128 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-4].set_false_path), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1547 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 34:
#line 129 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-4].set_false_path), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1553 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 35:
#line 130 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-2].set_false_path), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_FROM); free((yyvsp[0].strVal)); }
#line 1559 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 36:
#line 131 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_false_path) = sdc_set_false_path_add_to_from_group((yyvsp[-2].set_false_path), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_TO  ); free((yyvsp[0].strVal)); }
#line 1565 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 37:
#line 134 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = alloc_sdc_set_max_delay(); }
#line 1571 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 38:
#line 135 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_set_max_delay_value((yyvsp[-1].set_max_delay), (yyvsp[0].floatVal)); }
#line 1577 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 39:
#line 136 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-4].set_max_delay), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1583 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 40:
#line 137 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-4].set_max_delay), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1589 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 41:
#line 138 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-4].set_max_delay), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1595 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 42:
#line 139 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-4].set_max_delay), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1601 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 43:
#line 140 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-2].set_max_delay), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_FROM); free((yyvsp[0].strVal)); }
#line 1607 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 44:
#line 141 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_max_delay) = sdc_set_max_delay_add_to_from_group((yyvsp[-2].set_max_delay), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_TO  ); free((yyvsp[0].strVal)); }
#line 1613 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 45:
#line 144 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = alloc_sdc_set_multicycle_path(); }
#line 1619 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 46:
#line 145 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_set_mcp_value((yyvsp[-1].set_multicycle_path), (yyvsp[0].intVal)); }
#line 1625 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 47:
#line 146 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_set_type((yyvsp[-1].set_multicycle_path), SDC_MCP_SETUP); }
#line 1631 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 48:
#line 147 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-4].set_multicycle_path), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1637 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 49:
#line 148 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-4].set_multicycle_path), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1643 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 50:
#line 149 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-4].set_multicycle_path), (yyvsp[-1].string_group), SDC_FROM); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1649 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 51:
#line 150 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-4].set_multicycle_path), (yyvsp[-1].string_group), SDC_TO  ); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1655 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 52:
#line 151 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-2].set_multicycle_path), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_FROM); free((yyvsp[0].strVal)); }
#line 1661 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 53:
#line 152 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.set_multicycle_path) = sdc_set_multicycle_path_add_to_from_group((yyvsp[-2].set_multicycle_path), make_sdc_string_group(SDC_STRING, (yyvsp[0].strVal)), SDC_TO  ); free((yyvsp[0].strVal)); }
#line 1667 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 54:
#line 155 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = alloc_sdc_string_group(SDC_PORT); }
#line 1673 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 55:
#line 156 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = sdc_string_group_add_strings((yyvsp[-3].string_group), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1679 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 56:
#line 157 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = sdc_string_group_add_string((yyvsp[-1].string_group), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1685 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 57:
#line 160 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = alloc_sdc_string_group(SDC_CLOCK); }
#line 1691 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 58:
#line 161 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = sdc_string_group_add_strings((yyvsp[-3].string_group), (yyvsp[-1].string_group)); free_sdc_string_group((yyvsp[-1].string_group)); }
#line 1697 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 59:
#line 162 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = sdc_string_group_add_string((yyvsp[-1].string_group), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1703 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 60:
#line 165 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = alloc_sdc_string_group(SDC_STRING); }
#line 1709 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 61:
#line 166 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.string_group) = sdc_string_group_add_string((yyvsp[-1].string_group), (yyvsp[0].strVal)); free((yyvsp[0].strVal)); }
#line 1715 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 62:
#line 168 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.strVal) = (yyvsp[0].strVal); }
#line 1721 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 63:
#line 169 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.strVal) = (yyvsp[0].strVal); }
#line 1727 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 64:
#line 172 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.floatVal) = (yyvsp[0].floatVal); }
#line 1733 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 65:
#line 173 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.floatVal) = (yyvsp[0].intVal); }
#line 1739 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 66:
#line 176 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.floatVal) = (yyvsp[0].floatVal); }
#line 1745 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;

  case 67:
#line 179 "src/sdc_parse.y" /* yacc.c:1646  */
    { (yyval.intVal) = (yyvsp[0].intVal); }
#line 1751 "src/sdc_parse.tab.c" /* yacc.c:1646  */
    break;


#line 1755 "src/sdc_parse.tab.c" /* yacc.c:1646  */
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
#line 182 "src/sdc_parse.y" /* yacc.c:1906  */



int yyerror(const char *msg) {
    sdc_error(yylineno, yytext, "%s\n", msg);
    return 1;
}
