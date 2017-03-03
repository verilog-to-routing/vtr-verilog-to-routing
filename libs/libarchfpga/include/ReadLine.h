#ifndef READLINE_H
#define READLINE_H

#ifdef __cplusplus 
extern "C" {
#endif

char **ReadLineTokens(INOUTP FILE * InFile, INOUTP int *LineNum);
int CountTokens(INP char **Tokens);
void FreeTokens(INOUTP char ***TokensPtr);

#ifdef __cplusplus 
}
#endif

#endif

