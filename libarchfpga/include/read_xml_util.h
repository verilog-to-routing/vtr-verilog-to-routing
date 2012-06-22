#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "util.h"
#include "ezxml.h"

extern ezxml_t FindElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required);
extern ezxml_t FindFirstElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required);
extern void CheckElement(INP ezxml_t Node, INP const char *Name);
extern void FreeNode(INOUTP ezxml_t Node);
extern const char * FindProperty(INP ezxml_t Parent, INP const char *Name,
		INP boolean);
extern boolean IsWhitespace(char c);
extern void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len);
extern char **GetNodeTokens(INP ezxml_t Node);
extern char **LookaheadNodeTokens(INP ezxml_t Node);
extern int CountChildren(INP ezxml_t Node, INP const char *Name,
		INP int min_count);
extern int GetIntProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP int default_value);
extern float GetFloatProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP float default_value);
extern boolean GetBooleanProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP boolean default_value);

#endif

