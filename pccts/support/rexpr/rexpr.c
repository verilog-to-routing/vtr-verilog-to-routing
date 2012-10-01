/*
 * This file contains code for
 *
 *      int rexpr(char *expr, char *s);
 *
 * which answers
 *
 *      1 if 's' is in the language described by the regular expression 'expr'
 *      0 if it is not
 *     -1 if the regular expression is invalid
 *
 * Language membership is determined by constructing a non-deterministic
 * finite automata (NFA) from the regular expression.  A depth-
 * first-search is performed on the NFA (graph) to check for a match of 's'.
 * Each non-epsilon arc consumes one character from 's'.  Backtracking is
 * performed to check all possible paths through the NFA.
 *
 * Regular expressions follow the meta-language:
 *
 * <regExpr>        ::= <andExpr> ( '|' <andExpr> )*
 *
 * <andExpr>        ::= <expr> ( <expr> )*
 *
 * <expr>           ::= {'~'} '[' <atomList> ']' <repeatSymbol>
 *                      | '(' <regExpr> ')' <repeatSymbol>
 *                      | '{' <regExpr> '}' <repeatSymbol>
 *                      | <atom> <repeatSymbol>
 *
 * <repeatSymbol>   ::= { '*' | '+' }
 *
 * <atomList>       ::= <atom> ( <atom> )*
 *                      | { <atomList> } <atom> '-' <atom> { <atomList> }
 *
 * <atom>           ::= Token[Atom]
 *
 * Notes:
 *		~	means complement the set in [..]. i.e. all characters not listed
 *		*	means match 0 or more times (can be on expression or atom)
 *		+	means match 1 or more times (can be on expression or atom)
 *		{}	optional
 *		()	grouping
 *		[]	set of atoms
 *		x-y	all characters from x to y (found only in [..])
 *		\xx the character with value xx
 *
 * Examples:
 *		[a-z]+
 *			match 1 or more lower-case letters (e.g. variable)
 *
 *		0x[0-9A-Fa-f]+
 *			match a hex number with 0x on front (e.g. 0xA1FF)
 *
 *		[0-9]+.[0-9]+{e[0-9]+}
 *			match a floating point number (e.g. 3.14e21)
 *
 * Code example:
 *		if ( rexpr("[a-zA-Z][a-zA-Z0-9]+", str) ) then str is keyword
 *
 * Terence Parr
 * Purdue University
 * April 1991
 */

#include <stdio.h>
#include <ctype.h>
#ifdef __STDC__
#include <stdlib.h>
#else
#include <malloc.h>
#endif
#include "rexpr.h"

#ifdef __STDC__
static int regExpr( GraphPtr g );
static int andExpr( GraphPtr g );
static int expr( GraphPtr g );
static int repeatSymbol( GraphPtr g );
static int atomList( char *p, int complement );
static int next( void );
static ArcPtr newGraphArc( void );
static NodePtr newNode( void );
static int ArcBetweenGraphNode( NodePtr i, NodePtr j, int label );
static Graph BuildNFA_atom( int label );
static Graph BuildNFA_AB( Graph A, Graph B );
static Graph BuildNFA_AorB( Graph A, Graph B );
static Graph BuildNFA_set( char *s );
static Graph BuildNFA_Astar( Graph A );
static Graph BuildNFA_Aplus( Graph A );
static Graph BuildNFA_Aoptional( Graph A );
#else
static int regExpr();
static int andExpr();
static int expr();
static int repeatSymbol();
static int atomList();
static int next();
static ArcPtr newGraphArc();
static NodePtr newNode();
static int ArcBetweenGraphNode();
static Graph BuildNFA_atom();
static Graph BuildNFA_AB();
static Graph BuildNFA_AorB();
static Graph BuildNFA_set();
static Graph BuildNFA_Astar();
static Graph BuildNFA_Aplus();
static Graph BuildNFA_Aoptional();
#endif

static char *_c;
static int token, tokchar;
static NodePtr accept;
static NodePtr freelist = NULL;

/*
 * return 1 if s in language described by expr
 *        0 if s is not
 *       -1 if expr is an invalid regular expression
 */
rexpr(expr, s)
char *expr, *s;
{
	NodePtr p,q;
	Graph nfa;
	int result;

	fprintf(stderr, "rexpr(%s,%s);\n", expr,s);
	freelist = NULL;
	_c = expr;
	next();
	if ( regExpr(&nfa) == -1 ) return -1;
	accept = nfa.right;
	result = match(nfa.left, s);
	/* free all your memory */
	p = q = freelist;
	while ( p!=NULL ) { q = p->track; free(p); p = q; }
	return result;
}

/*
 * do a depth-first-search on the NFA looking for a path from start to
 * accept state labelled with the characters of 's'.
 */
match(automaton, s)
NodePtr automaton;
char *s;
{
	ArcPtr p;
	
	if ( automaton == accept && *s == '\0' ) return 1;	/* match */

	for (p=automaton->arcs; p!=NULL; p=p->next)			/* try all arcs */
	{
		if ( p->label == Epsilon )
		{
			if ( match(p->target, s) ) return 1;
		}
		else if ( p->label == *s )
				if ( match(p->target, s+1) ) return 1;
	}
	return 0;
}

/*
 * <regExpr>        ::= <andExpr> ( '|' {<andExpr>} )*
 *
 * Return -1 if syntax error
 * Return  0 if none found
 * Return  1 if a regExrp was found
 */
static
regExpr(g)
GraphPtr g;
{
	Graph g1, g2;
	
	if ( andExpr(&g1) == -1 )
	{
		return -1;
	}
	
	while ( token == '|' )
	{
		int a;
		next();
		a = andExpr(&g2);
		if ( a == -1 ) return -1;	/* syntax error below */
		else if ( !a ) return 1;	/* empty alternative */
		g1 = BuildNFA_AorB(g1, g2);
	}
	
	if ( token!='\0' ) return -1;

	*g = g1;
	return 1;
}

/*
 * <andExpr>        ::= <expr> ( <expr> )*
 */
static
andExpr(g)
GraphPtr g;
{
	Graph g1, g2;
	
	if ( expr(&g1) == -1 )
	{
		return -1;
	}
	
	while ( token==Atom || token=='{' || token=='(' || token=='~' || token=='[' )
	{
		if (expr(&g2) == -1) return -1;
		g1 = BuildNFA_AB(g1, g2);
	}
	
	*g = g1;
	return 1;
}

/*
 * <expr>           ::=    {'~'} '[' <atomList> ']' <repeatSymbol>
 *                      | '(' <regExpr> ')' <repeatSymbol>
 *                      | '{' <regExpr> '}' <repeatSymbol>
 *                      | <atom> <repeatSymbol>
 */
static
expr(g)
GraphPtr g;
{
	int complement = 0;
	char s[257];    /* alloc space for string of char in [] */
	
	if ( token == '~' || token == '[' )
	{
		if ( token == '~' ) {complement = 1; next();}
		if ( token != '[' ) return -1;
		next();
		if ( atomList( s, complement ) == -1 ) return -1;
		*g = BuildNFA_set( s );
		if ( token != ']' ) return -1;
		next();
		repeatSymbol( g );
		return 1;
	}
	if ( token == '(' )
	{
		next();
		if ( regExpr( g ) == -1 ) return -1;
		if ( token != ')' ) return -1;
		next();
		repeatSymbol( g );
		return 1;
	}
	if ( token == '{' )
	{
		next();
		if ( regExpr( g ) == -1 ) return -1;
		if ( token != '}' ) return -1;
		next();
		/* S p e c i a l  C a s e   O p t i o n a l  {  } */
		if ( token != '*' && token != '+' )
		{
			*g = BuildNFA_Aoptional( *g );
		}
		repeatSymbol( g );
		return 1;
	}
	if ( token == Atom )
	{
		*g = BuildNFA_atom( tokchar );
		next();
		repeatSymbol( g );
		return 1;
	}
	
	return -1;
}

/*
 * <repeatSymbol>   ::= { '*' | '+' }
 */
static
repeatSymbol(g)
GraphPtr g;
{
	switch ( token )
	{
		case '*' : *g = BuildNFA_Astar( *g ); next(); break;
		case '+' : *g = BuildNFA_Aplus( *g ); next(); break;
	}
	return 1;
}

/*
 * <atomList>       ::= <atom> { <atom> }*
 *                      { <atomList> } <atom> '-' <atom> { <atomList> }
 *
 * a-b is same as ab
 * q-a is same as q
 */
static
atomList(p, complement)
char *p;
int complement;
{
	static unsigned char set[256];		/* no duplicates */
	int first, last, i;
	char *s = p;
	
	if ( token != Atom ) return -1;
	
	for (i=0; i<256; i++) set[i] = 0;
	while ( token == Atom )
	{
		if ( !set[tokchar] ) *s++ = tokchar;
		set[tokchar] = 1;    			/* Add atom to set */
		next();
		if ( token == '-' )         	/* have we found '-' */
		{
			first = *(s-1);             /* Get last char */
			next();
			if ( token != Atom ) return -1;
			else
			{
				last = tokchar;
			}
			for (i = first+1; i <= last; i++)
			{
				if ( !set[tokchar] ) *s++ = i;
				set[i] = 1;    			/* Add atom to set */
			}
			next();
		}
	}
	*s = '\0';
	if ( complement )
	{
		for (i=0; i<256; i++) set[i] = !set[i];
		for (i=1,s=p; i<256; i++) if ( set[i] ) *s++ = i;
		*s = '\0';
	}
	return 1;
}

/* a somewhat stupid lexical analyzer */
static
next()
{
	while ( *_c==' ' || *_c=='\t' || *_c=='\n' ) _c++;
	if ( *_c=='\\' )
	{
		_c++;
		if ( isdigit(*_c) )
		{
			int n=0;
			while ( isdigit(*_c) )
			{
				n = n*10 + (*_c++ - '0');
			}
			if ( n>255 ) n=255;
			tokchar = n;
		}
		else
		{
			switch (*_c)
			{
				case 'n' : tokchar = '\n'; break;
				case 't' : tokchar = '\t'; break;
				case 'r' : tokchar = '\r'; break;
				default  : tokchar = *_c;
			}
			_c++;
		}
		token = Atom;
	}
	else if ( isgraph(*_c) && *_c!='[' && *_c!='(' && *_c!='{' &&
			  *_c!='-' && *_c!='}' && *_c!=')' && *_c!=']' &&
			  *_c!='+' && *_c!='*' && *_c!='~' && *_c!='|' )
	{
		token = Atom;
		tokchar = *_c++;
	}
	else
	{
		token = tokchar = *_c++;
	}
}

/* N F A  B u i l d i n g  R o u t i n e s */

static
ArcPtr
newGraphArc()
{
	ArcPtr p;
	p = (ArcPtr) calloc(1, sizeof(Arc));
	if ( p==NULL ) {fprintf(stderr,"rexpr: out of memory\n"); exit(-1);}
	if ( freelist != NULL ) p->track = (ArcPtr) freelist;
	freelist = (NodePtr) p;
	return p;
}

static
NodePtr
newNode()
{
	NodePtr p;
	p = (NodePtr) calloc(1, sizeof(Node));
	if ( p==NULL ) {fprintf(stderr,"rexpr: out of memory\n"); exit(-1);}
	if ( freelist != NULL ) p->track = freelist;
	freelist = p;
	return p;
}

static
ArcBetweenGraphNodes(i, j, label)
NodePtr i, j;
int label;
{
	ArcPtr a;
	
	a = newGraphArc();
	if ( i->arcs == NULL ) i->arctail = i->arcs = a;
	else {(i->arctail)->next = a; i->arctail = a;}
	a->label = label;
	a->target = j;
}

static Graph
BuildNFA_atom(label)
int label;
{
	Graph g;
	
	g.left = newNode();
	g.right = newNode();
	ArcBetweenGraphNodes(g.left, g.right, label);
	return( g );
}

static Graph
BuildNFA_AB(A, B)
Graph A, B;
{
	Graph g;
	
	ArcBetweenGraphNodes(A.right, B.left, Epsilon);
	g.left = A.left;
	g.right = B.right;
	return( g );
}

static Graph
BuildNFA_AorB(A, B)
Graph A, B;
{
	Graph g;
	
	g.left = newNode();
	ArcBetweenGraphNodes(g.left, A.left, Epsilon);
	ArcBetweenGraphNodes(g.left, B.left, Epsilon);
	g.right = newNode();
	ArcBetweenGraphNodes(A.right, g.right, Epsilon);
	ArcBetweenGraphNodes(B.right, g.right, Epsilon);
	return( g );
}

static Graph
BuildNFA_set( s )
char *s;
{
	Graph g;
	
	if ( s == NULL ) return g;
	
	g.left = newNode();
	g.right = newNode();
	while ( *s != '\0' )
	{
		ArcBetweenGraphNodes(g.left, g.right, *s++);
	}
	return g;
}

static Graph
BuildNFA_Astar( A )
Graph A;
{
	Graph g;

	g.left = newNode();
	g.right = newNode();
	
	ArcBetweenGraphNodes(g.left, A.left, Epsilon);
	ArcBetweenGraphNodes(g.left, g.right, Epsilon);
	ArcBetweenGraphNodes(A.right, g.right, Epsilon);
	ArcBetweenGraphNodes(A.right, A.left, Epsilon);
	
	return( g );
}

static Graph
BuildNFA_Aplus( A )
Graph A;
{
	ArcBetweenGraphNodes(A.right, A.left, Epsilon);
	
	return( A );
}

static Graph
BuildNFA_Aoptional( A )
Graph A;
{
	Graph g;
	
	g.left = newNode();
	g.right = newNode();
	
	ArcBetweenGraphNodes(g.left, A.left, Epsilon);
	ArcBetweenGraphNodes(g.left, g.right, Epsilon);
	ArcBetweenGraphNodes(A.right, g.right, Epsilon);
	
	return( g );
}
