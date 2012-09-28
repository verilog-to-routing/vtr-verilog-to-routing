#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: antlr.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33MR7
 */
#define zzEOF_TOKEN 1
#define Eof 1
#define QuotedTerm 2
#define Action 34
#define Pred 35
#define PassAction 36
#define WildCard 83
#define LABEL 85
#define NonTerminal 93
#define TokenTerm 94
#define ID 139
#define INT 141

#ifdef __USE_PROTOS
void grammar(void);
#else
extern void grammar();
#endif

#ifdef __USE_PROTOS
void class_def(void);
#else
extern void class_def();
#endif

#ifdef __USE_PROTOS
void rule(void);
#else
extern void rule();
#endif

#ifdef __USE_PROTOS
void laction(void);
#else
extern void laction();
#endif

#ifdef __USE_PROTOS
void lmember(void);
#else
extern void lmember();
#endif

#ifdef __USE_PROTOS
void lprefix(void);
#else
extern void lprefix();
#endif

#ifdef __USE_PROTOS
void aLexclass(void);
#else
extern void aLexclass();
#endif

#ifdef __USE_PROTOS
void error(void);
#else
extern void error();
#endif

#ifdef __USE_PROTOS
void tclass(void);
#else
extern void tclass();
#endif

#ifdef __USE_PROTOS
void token(void);
#else
extern void token();
#endif

#ifdef __USE_PROTOS
void block( set *toksrefd, set *rulesrefd );
#else
extern void block();
#endif

#ifdef __USE_PROTOS
void alt( set *toksrefd, set *rulesrefd );
#else
extern void alt();
#endif

#ifdef __USE_PROTOS
extern  LabelEntry *  element_label(void);
#else
extern  LabelEntry *  element_label();
#endif

#ifdef __USE_PROTOS
extern  Node *  element( int not, int first_on_line, int use_def_MT_handler );
#else
extern  Node *  element();
#endif

#ifdef __USE_PROTOS
void default_exception_handler(void);
#else
extern void default_exception_handler();
#endif

#ifdef __USE_PROTOS
extern  ExceptionGroup *  exception_group(void);
#else
extern  ExceptionGroup *  exception_group();
#endif

#ifdef __USE_PROTOS
extern  ExceptionHandler *  exception_handler(void);
#else
extern  ExceptionHandler *  exception_handler();
#endif

#ifdef __USE_PROTOS
void enum_file( char *fname );
#else
extern void enum_file();
#endif

#ifdef __USE_PROTOS
void defines( char *fname );
#else
extern void defines();
#endif

#ifdef __USE_PROTOS
void enum_def( char *fname );
#else
extern void enum_def();
#endif

#endif
extern SetWordType zzerr1[];
extern SetWordType setwd1[];
extern SetWordType zzerr2[];
extern SetWordType zzerr3[];
extern SetWordType zzerr4[];
extern SetWordType zzerr5[];
extern SetWordType setwd2[];
extern SetWordType zzerr6[];
extern SetWordType zzerr7[];
extern SetWordType zzerr8[];
extern SetWordType zzerr9[];
extern SetWordType setwd3[];
extern SetWordType zzerr10[];
extern SetWordType zzerr11[];
extern SetWordType zzerr12[];
extern SetWordType zzerr13[];
extern SetWordType zzerr14[];
extern SetWordType zzerr15[];
extern SetWordType zzerr16[];
extern SetWordType zzerr17[];
extern SetWordType setwd4[];
extern SetWordType zzerr18[];
extern SetWordType zzerr19[];
extern SetWordType zzerr20[];
extern SetWordType zzerr21[];
extern SetWordType setwd5[];
