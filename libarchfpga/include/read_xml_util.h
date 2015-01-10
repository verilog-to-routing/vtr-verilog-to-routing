#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "util.h"
#include "ezxml.h"

#ifdef __cplusplus 
extern "C" {
#endif
ezxml_t FindElement(INP ezxml_t Parent, INP const char *Name,
		INP bool Required);
ezxml_t FindFirstElement(INP ezxml_t Parent, INP const char *Name,
		INP bool Required);
void CheckElement(INP ezxml_t Node, INP const char *Name);
void FreeNode(INOUTP ezxml_t Node);
const char * FindProperty(INP ezxml_t Parent, INP const char *Name,
		INP bool);
bool IsWhitespace(char c);
void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len);
char **GetNodeTokens(INP ezxml_t Node);
char **LookaheadNodeTokens(INP ezxml_t Node);
int CountChildren(INP ezxml_t Node, INP const char *Name,
		INP int min_count);
int GetIntProperty(INP ezxml_t Parent, INP char *Name,
		INP bool Required, INP int default_value);
float GetFloatProperty(INP ezxml_t Parent, INP char *Name,
		INP bool Required, INP float default_value);
bool GetboolProperty(INP ezxml_t Parent, INP char *Name,
		INP bool Required, INP bool default_value);
#ifdef __cplusplus 
}
#endif

#endif

