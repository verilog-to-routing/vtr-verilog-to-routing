#ifndef READLINE_H
#define READLINE_H

#ifdef __cplusplus 
extern "C" {
#endif

char **ReadLineTokens(FILE * InFile, int *LineNum);
int CountTokens(const char **Tokens);
void FreeTokens(char ***TokensPtr);

#ifdef __cplusplus 
}
#endif

#endif

