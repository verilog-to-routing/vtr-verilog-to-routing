#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "read_xml_util.h"
#include "read_xml_arch_file.h"

using namespace std;
using namespace pugiutil;

/* Convert bool to ReqOpt enum */ 
extern ReqOpt BoolToReqOpt(bool b) {
	if (b) {
		return REQUIRED;	
	}
	return OPTIONAL;
}

/* Returns true if character is whatspace between tokens */
bool IsWhitespace(char c) {
	switch (c) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return true;
	default:
		return false;
	}
}


/* Count tokens and length in all the Text children nodes of current node. */
extern void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len) {
	bool InToken;
	*Num = 0;
	*Len = 0;
	InToken = false;
	while (*Str) {
		if (IsWhitespace(*Str)) {
			InToken = false;
		}

		else

		{
			if (!InToken) {
				++(*Num);
			}
			++(*Len);
			InToken = true;
		}
		++Str; /* Advance pointer */
	}
}

/* Returns a token list of the text nodes of a given node. This
 * unlinks all the text nodes from the document */
extern char **
GetNodeTokens(INP pugi::xml_node Node, const pugiloc::loc_data& loc_data) {
	int Count, Len;
	char *Dst;
	const char *Cur;
	bool InToken;
	char **Tokens;

	/* Count the tokens and find length of token data */
	CountTokensInString(Node.child_value(), &Count, &Len);

	/* Error out if no tokens found */
	if (Count < 1) {
		return NULL;
	}
	Len = (sizeof(char) * Len) + /* Length of actual data */
	(sizeof(char) * Count); /* Null terminators */

	/* Alloc the pointer list and data list. Count the final 
	 * empty string we will use as list terminator */
	Tokens = (char **) my_malloc(sizeof(char *) * (Count + 1));
	Dst = (char *) my_malloc(sizeof(char) * Len);
	Count = 0;

	/* Copy data to tokens */
	Cur = Node.child_value();
	InToken = false;
	while (*Cur) {
		if (IsWhitespace(*Cur)) {
			if (InToken) {
				*Dst = '\0';
				++Dst;
			}
			InToken = false;
		}

		else {
			if (!InToken) {
				Tokens[Count] = Dst;
				++Count;
			}
			*Dst = *Cur;
			++Dst;
			InToken = true;
		}
		++Cur;
	}
	if (InToken) { /* Null term final token */
		*Dst = '\0';
		++Dst;
	}
	Tokens[Count] = NULL; /* End of tokens marker is a NULL */

	/* Return the list */
	return Tokens;
}

/* Returns a token list of the text nodes of a given node. This
 * does not unlink the text nodes from the document */
extern char **
LookaheadNodeTokens( INP pugi::xml_node Node, const pugiloc::loc_data& loc_data) {
	int Count, Len;
	char *Dst;
	const char *Cur;
	bool InToken;
	char **Tokens;

	/* Count the tokens and find length of token data */
	CountTokensInString(Node.child_value(), &Count, &Len);

	/* Error out if no tokens found */
	if (Count < 1) {
		return NULL;
	}
	Len = (sizeof(char) * Len) + /* Length of actual data */
	(sizeof(char) * Count); /* Null terminators */

	/* Alloc the pointer list and data list. Count the final 
	 * empty string we will use as list terminator */
	Tokens = (char **) my_malloc(sizeof(char *) * (Count + 1));
	Dst = (char *) my_malloc(sizeof(char) * Len);
	Count = 0;

	/* Copy data to tokens */
	Cur = Node.child_value();
	InToken = false;
	while (*Cur) {
		if (IsWhitespace(*Cur)) {
			if (InToken) {
				*Dst = '\0';
				++Dst;
			}
			InToken = false;
		}

		else {
			if (!InToken) {
				Tokens[Count] = Dst;
				++Count;
			}
			*Dst = *Cur;
			++Dst;
			InToken = true;
		}
		++Cur;
	}
	if (InToken) { /* Null term final token */
		*Dst = '\0';
		++Dst;
	}
	Tokens[Count] = NULL; /* End of tokens marker is a NULL */

	/* Return the list */
	return Tokens;
}

