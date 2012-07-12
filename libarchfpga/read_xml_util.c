#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "ezxml.h"
#include "read_xml_util.h"

/* Finds child element with given name and returns it. Errors out if
 * more than one instance exists. */
ezxml_t FindElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required) {
	ezxml_t Cur;

	/* Find the first node of correct name */
	Cur = ezxml_child(Parent, Name);

	/* Error out if node isn't found but they required it */
	if (Required) {
		if (NULL == Cur) {
			vpr_printf(TIO_MESSAGE_ERROR,
			"[LINE %d] Element '%s' not found within element '%s'.\n",
					Parent->line, Name, Parent->name);
			exit(1);
		}
	}

	/* Look at next tag with same name and error out if exists */
	if (Cur != NULL && Cur->next) {
		vpr_printf(
				TIO_MESSAGE_ERROR, "[LINE %d] Element '%s' found twice within element '%s'.\n",
				Parent->line, Name, Parent->name);
		exit(1);
	}
	return Cur;
}
/* Finds child element with given name and returns it. */
ezxml_t FindFirstElement(INP ezxml_t Parent, INP const char *Name,
		INP boolean Required) {
	ezxml_t Cur;

	/* Find the first node of correct name */
	Cur = ezxml_child(Parent, Name);

	/* Error out if node isn't found but they required it */
	if (Required) {
		if (NULL == Cur) {
			vpr_printf(TIO_MESSAGE_ERROR,
			"[LINE %d] Element '%s' not found within element '%s'.\n",
					Parent->line, Name, Parent->name);
			exit(1);
		}
	}

	return Cur;
}

/* Checks the node is an element with name equal to one given */
void CheckElement(INP ezxml_t Node, INP const char *Name) {
	assert(Node != NULL && Name != NULL);
	if (0 != strcmp(Node->name, Name)) {
		vpr_printf(TIO_MESSAGE_ERROR,
		"[LINE %d] Element '%s' within element '%s' does match expected "
		"element type of '%s'\n", Node->line, Node->name, Node->parent->name,
				Name);
		exit(1);
	}
}

/* Checks that the node has no remaining child nodes or property nodes,
 * then unlinks the node from the tree which updates pointers and
 * frees the memory */
void FreeNode(INOUTP ezxml_t Node) {
	ezxml_t Cur;
	char *Txt;

	/* Shouldn't have unprocessed properties */
	if (Node->attr[0]) {
		vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Node '%s' has invalid property %s=\"%s\".\n",
				Node->line, Node->name, Node->attr[0], Node->attr[1]);
		exit(1);
	}

	/* Shouldn't have non-whitespace text */
	Txt = Node->txt;
	while (*Txt) {
		if (!IsWhitespace(*Txt)) {
			vpr_printf(TIO_MESSAGE_ERROR,
			"[LINE %d] Node '%s' has unexpected text '%s' within it.\n",
					Node->line, Node->name, Node->txt);
			exit(1);
		}
		++Txt;
	}

	/* We shouldn't have child items left */
	Cur = Node->child;
	if (Cur) {
		vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Node '%s' has invalid child node '%s'.\n",
				Node->line, Node->name, Cur->name);
		exit(1);
	}

	/* Now actually unlink and free the node */
	ezxml_remove(Node);
}

/* Returns TRUE if character is whatspace between tokens */
boolean IsWhitespace(char c) {
	switch (c) {
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return TRUE;
	default:
		return FALSE;
	}
}

const char *
FindProperty(INP ezxml_t Parent, INP const char *Name, INP boolean Required) {
	const char *Res;

	Res = ezxml_attr(Parent, Name);
	if (Required) {
		if (NULL == Res) {
			vpr_printf(TIO_MESSAGE_ERROR,
			"[Line %d] Required property '%s' not found for element '%s'.\n",
					Parent->line, Name, Parent->name);
			exit(1);
		}
	}
	return Res;
}

/* Count tokens and length in all the Text children nodes of current node. */
extern void CountTokensInString(INP const char *Str, OUTP int *Num,
		OUTP int *Len) {
	boolean InToken;
	*Num = 0;
	*Len = 0;
	InToken = FALSE;
	while (*Str) {
		if (IsWhitespace(*Str)) {
			InToken = FALSE;
		}

		else

		{
			if (!InToken) {
				++(*Num);
			}
			++(*Len);
			InToken = TRUE;
		}
		++Str; /* Advance pointer */
	}
}

/* Returns a token list of the text nodes of a given node. This
 * unlinks all the text nodes from the document */
extern char **
GetNodeTokens(INP ezxml_t Node) {
	int Count, Len;
	char *Cur, *Dst;
	boolean InToken;
	char **Tokens;

	/* Count the tokens and find length of token data */
	CountTokensInString(Node->txt, &Count, &Len);

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
	Cur = Node->txt;
	InToken = FALSE;
	while (*Cur) {
		if (IsWhitespace(*Cur)) {
			if (InToken) {
				*Dst = '\0';
				++Dst;
			}
			InToken = FALSE;
		}

		else {
			if (!InToken) {
				Tokens[Count] = Dst;
				++Count;
			}
			*Dst = *Cur;
			++Dst;
			InToken = TRUE;
		}
		++Cur;
	}
	if (InToken) { /* Null term final token */
		*Dst = '\0';
		++Dst;
	}
	ezxml_set_txt(Node, "");
	Tokens[Count] = NULL; /* End of tokens marker is a NULL */

	/* Return the list */
	return Tokens;
}

/* Returns a token list of the text nodes of a given node. This
 * does not unlink the text nodes from the document */
extern char **
LookaheadNodeTokens(INP ezxml_t Node) {
	int Count, Len;
	char *Cur, *Dst;
	boolean InToken;
	char **Tokens;

	/* Count the tokens and find length of token data */
	CountTokensInString(Node->txt, &Count, &Len);

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
	Cur = Node->txt;
	InToken = FALSE;
	while (*Cur) {
		if (IsWhitespace(*Cur)) {
			if (InToken) {
				*Dst = '\0';
				++Dst;
			}
			InToken = FALSE;
		}

		else {
			if (!InToken) {
				Tokens[Count] = Dst;
				++Count;
			}
			*Dst = *Cur;
			++Dst;
			InToken = TRUE;
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

/* Find integer attribute matching Name in XML tag Parent and return it if exists.  
 Removes attribute from Parent */
extern int GetIntProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP int default_value) {
	const char * Prop;
	int property_value;

	property_value = default_value;
	Prop = FindProperty(Parent, Name, Required);
	if (Prop) {
		property_value = my_atoi(Prop);
		ezxml_set_attr(Parent, Name, NULL);
	}
	return property_value;
}

/* Find floating-point attribute matching Name in XML tag Parent and return it if exists.  
 Removes attribute from Parent */
extern float GetFloatProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP float default_value) {

	const char * Prop;
	float property_value;

	property_value = default_value;
	Prop = FindProperty(Parent, Name, Required);
	if (Prop) {
		property_value = (float)atof(Prop);
		ezxml_set_attr(Parent, Name, NULL);
	}
	return property_value;
}

/* Find boolean attribute matching Name in XML tag Parent and return it if exists.  
 Removes attribute from Parent */
extern boolean GetBooleanProperty(INP ezxml_t Parent, INP char *Name,
		INP boolean Required, INP boolean default_value) {

	const char * Prop;
	boolean property_value;

	property_value = default_value;
	Prop = FindProperty(Parent, Name, Required);
	if (Prop) {
		if ((strcmp(Prop, "false") == 0) || (strcmp(Prop, "FALSE") == 0)
				|| (strcmp(Prop, "False") == 0)) {
			property_value = FALSE;
		} else if ((strcmp(Prop, "true") == 0) || (strcmp(Prop, "TRUE") == 0)
				|| (strcmp(Prop, "True") == 0)) {
			property_value = TRUE;
		} else {
			vpr_printf(
					TIO_MESSAGE_ERROR, "[LINE %d] Unknown value %s for boolean attribute %s in %s",
					Parent->line, Prop, Name, Parent->name);
			exit(1);
		}
		ezxml_set_attr(Parent, Name, NULL);
	}
	return property_value;
}

/* Counts number of child elements in a container element. 
 * Name is the name of the child element. 
 * Errors if no occurances found. */
extern int CountChildren(INP ezxml_t Node, INP const char *Name,
		INP int min_count) {
	ezxml_t Cur, sibling;
	int Count;

	Count = 0;
	Cur = Node->child;
	sibling = NULL;

	if (Cur) {
		sibling = Cur->sibling;
	}

	while (Cur) {
		if (strcmp(Cur->name, Name) == 0) {
			++Count;
		}
		Cur = Cur->next;
		if (Cur == NULL) {
			Cur = sibling;
			if (Cur != NULL) {
				sibling = Cur->sibling;
			}
		}
	}
	/* Error if no occurances found */
	if (Count < min_count) {
		vpr_printf(TIO_MESSAGE_ERROR, "[Line %d] Expected node '%s' to have %d "
		"child elements, but none found.\n", Node->line, Node->name, min_count);
		exit(1);
	}
	return Count;
}

