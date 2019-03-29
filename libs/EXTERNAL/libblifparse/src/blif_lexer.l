%{
    /*
     * Include Files
     */
    #include "blifparse.hpp"
    #include "blif_common.hpp"
    #include "blif_error.hpp"

    #include "blif_lexer.hpp"

%}

/*
 * Options
 */

/* track line numbers*/
%option yylineno 

/* No lexing accross files */
%option noyywrap

/* unistd.h doesn't exist on windows */
%option nounistd

/* Avoid unused yyunput function warning */
%option nounput

/* Avoid unused yyinput function warning */
%option noinput

/* isatty() doesn't exist on windows */
%option never-interactive

/* no default rule to echo unrecongaized tokens to output */
%option nodefault

/*%option bison-bridge*/
%option reentrant

/*
 * Use a prefix to avoid name clashes with other
 * flex lexers
 */
%option prefix="blifparse_" 

/* Common character classes */
ID_SET [^ \t\r\n\\=]
BACK_SLASH [\\]
WS [ \t]
ENDL (\n|\n\r|\r\n)

/* Special Parser States */
%x LATCH
%x NAMES
%x SO_COVER

/*
 * Symbol Definitions
 */
%%
<*>#.*{ENDL}                    { /* ignore comments, but forward EOL for end of line comments */
                                  return blifparse::Parser::make_EOL();
                                }
^{WS}*{ENDL}                    { /* Ignore blank lines. */ }
\\{ENDL}{WS}*{ENDL}             { 
                                  /* 
                                   * Do forward end of line if the last line was a continuation. 
                                   *
                                   * Some times line continuations are followed by blank lines (which 
                                   * are otherwise ignored). In these cases we *do* want to 
                                   * forward EOL, so the parser knows the continued line has finished 
                                   */ 
                                  return blifparse::Parser::make_EOL(); 
                                }
<*>\\{ENDL}                     { /* line continuation (don't forward EOL to parser) */ }
{ENDL}                          { 
                                  return blifparse::Parser::make_EOL(); 
                                }
<*>{WS}+                        { /* skip white space */ }
<*>\.names                      { 
                                  /*
                                   * To process the single output cover rows of the names directly as symbols
                                   * (rather than as strings) we use a special lexer state.
                                   */
                                  BEGIN(NAMES);
                                  return blifparse::Parser::make_DOT_NAMES(); 
                                }
<*>\.latch                      { 
                                  /*
                                   * The initial state value of a latch is ambiguous (it chould be 
                                   * interpreted as a string or logic value string). So we use
                                   * a special lexer state to capture it.
                                   */
                                  BEGIN(LATCH); 
                                  return blifparse::Parser::make_DOT_LATCH(); 
                                }
<*>\.model                      { BEGIN(INITIAL); return blifparse::Parser::make_DOT_MODEL(); }
<*>\.subckt                     { BEGIN(INITIAL); return blifparse::Parser::make_DOT_SUBCKT(); }
<*>\.inputs                     { BEGIN(INITIAL); return blifparse::Parser::make_DOT_INPUTS(); }
<*>\.outputs                    { BEGIN(INITIAL); return blifparse::Parser::make_DOT_OUTPUTS(); }
<*>\.end                        { BEGIN(INITIAL); return blifparse::Parser::make_DOT_END(); }
<*>\.blackbox                   { BEGIN(INITIAL); return blifparse::Parser::make_DOT_BLACKBOX(); }

<*>\.conn                       { BEGIN(INITIAL); return blifparse::Parser::make_DOT_CONN(); /*BLIF extension */}
<*>\.attr                       { BEGIN(INITIAL); return blifparse::Parser::make_DOT_ATTR(); /*BLIF extension */}
<*>\.param                      { BEGIN(INITIAL); return blifparse::Parser::make_DOT_PARAM(); /*BLIF extension */}
<*>\.cname                      { BEGIN(INITIAL); return blifparse::Parser::make_DOT_CNAME(); /*BLIF extension */}

=                               { return blifparse::Parser::make_EQ();}
<LATCH>fe                       { return blifparse::Parser::make_LATCH_FE(); }
<LATCH>re                       { return blifparse::Parser::make_LATCH_RE(); }
<LATCH>ah                       { return blifparse::Parser::make_LATCH_AH(); }
<LATCH>al                       { return blifparse::Parser::make_LATCH_AL(); }
<LATCH>as                       { return blifparse::Parser::make_LATCH_AS(); }
<LATCH>NIL                      { return blifparse::Parser::make_NIL(); }
<LATCH>0                        { return blifparse::Parser::make_LOGIC_FALSE(); }
<LATCH>1                        { return blifparse::Parser::make_LOGIC_TRUE(); }
<LATCH>2                        { return blifparse::Parser::make_LATCH_INIT_2(); }
<LATCH>3                        { return blifparse::Parser::make_LATCH_INIT_3(); }
<LATCH>{ENDL}                   { 
                                  /*
                                   * Latches are only every defined on a single line,
                                   * so when we see the end of a line while in the LATCH
                                   * state we can go back to the regular (INITIAL) state.
                                   */
                                  BEGIN(INITIAL); return blifparse::Parser::make_EOL(); 
                                }
<SO_COVER>0                     { return blifparse::Parser::make_LOGIC_FALSE(); }
<SO_COVER>1                     { return blifparse::Parser::make_LOGIC_TRUE(); }
<SO_COVER>\-                    { return blifparse::Parser::make_LOGIC_DONT_CARE(); }
<SO_COVER>{ENDL}                { return blifparse::Parser::make_EOL(); }
<NAMES>{ENDL}                   { 
                                  /*
                                   * Once we reach the end of a line in NAMES state (i.e. the end of a .names line)
                                   * we expect the truth table (in single output cover format) to follow, so we enter
                                   * the SO_COVER state.
                                   */
                                  BEGIN(SO_COVER); 
                                  return blifparse::Parser::make_EOL(); 
                                }
<INITIAL,NAMES,LATCH>(({ID_SET}|{BACK_SLASH})*{ID_SET}) {
                                    /*
                                     * We allow all sorts of characters in regular strings.
                                     * However we need to be careful about line continuations
                                     * in particular, it is possible that we could have a string 
                                     * followed by a continuation with no space for this reason, 
                                     * we do not allow a continuation (backslash, \\ in escaped 
                                     * form in the regex) in the last character of the string.
                                     */
                                    return blifparse::Parser::make_STRING(blifparse_get_text(yyscanner)); 
                                }
<<EOF>>                         { /* If the file has no blank line at the end there will
                                     not be the expected EOL following the last command. 
                                     So first time through, return EOL, and subsequently 
                                     return 0 (which indicated end of file). This ensures
                                     there will always be an EOL provided to the parser. 
                                     However it may also generate a stray EOL if the last
                                     line IS blank - so the parser must handle those correclty. */
                                  static bool once; return (once = !once) ? blifparse::Parser::make_EOL() : blifparse::Parser::make_EOF();
                                }
<*>.                            { blifparse::blif_error_wrap(callback, blifparse_get_lineno(yyscanner), blifparse_get_text(yyscanner), "Unrecognized character"); }
%%
