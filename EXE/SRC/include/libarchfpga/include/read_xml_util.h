#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "util.h"
#include "ezxml.h"

#ifdef __cplusplus 
extern "C" {
#endif
ezxml_t FindElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required);
ezxml_t FindFirstElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required);
void CheckElement(INP ezxml_t Node, INP const char *Name);
void FreeNode(INOUTP ezxml_t Node);
const char * FindProperty(INP ezxml_t Parent, INP const char *Name,
		INP boolean);
boolean IsWhitespace(char c);
void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len);
char **GetNodeTokens(INP ezxml_t Node);
char **LookaheadNodeTokens(INP ezxml_t Node);
int CountChildren(INP ezxml_t Node, INP const char *Name,
		INP int min_count);
int GetIntProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP int default_value);
float GetFloatProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP float default_value);
boolean GetBooleanProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP boolean default_value);
#ifdef __cplusplus 
}
#endif

#endif

