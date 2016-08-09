#ifndef READ_XML_UTIL_H
#define READ_XML_UTIL_H

#include "util.h"
#include "pugixml.hpp"
#include "pugixml_util.hpp"

#ifdef __cplusplus 
extern "C" {
#endif

pugiutil::ReqOpt BoolToReqOpt(bool b);
bool IsWhitespace(char c);
void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len);
char **GetNodeTokens(INP pugi::xml_node Node, const pugiloc::loc_data& loc_data);
char **LookaheadNodeTokens(INP pugi::xml_node Node, const pugiloc::loc_data& loc_data);

#ifdef __cplusplus 
}
#endif

#endif

