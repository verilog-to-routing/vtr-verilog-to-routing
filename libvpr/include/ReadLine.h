#ifndef READLINE_H
#define READLINE_H

extern char **ReadLineTokens(INOUTP FILE * InFile, INOUTP int *LineNum);
extern int CountTokens(INP char **Tokens);
extern void FreeTokens(INOUTP char ***TokensPtr);

#endif

