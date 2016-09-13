/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

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
#line 14 "src/sdc_parse.y" /* yacc.c:1909  */

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


#line 100 "src/sdc_parse.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_SRC_SDC_PARSE_TAB_H_INCLUDED  */
