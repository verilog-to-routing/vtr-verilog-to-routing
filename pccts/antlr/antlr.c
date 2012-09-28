/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR7
 */
#include <stdio.h>
#define ANTLR_VERSION	133MR7

#include "config.h"
#include "set.h"
#include <ctype.h>
#include "syn.h"
#include "hash.h"
#include "generic.h"
#define zzcr_attr(attr,tok,t)
#define zzSET_SIZE 20
#include "antlr.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"
#ifndef PURIFY
#define PURIFY(r,s)
#endif
ANTLR_INFO

#ifdef __USE_PROTOS
static void chkToken(char *, char *, char *, int);
#else
static void chkToken();
#endif

#ifdef __USE_PROTOS
static int isDLGmaxToken(char *Token);				     /* MR3 */
#else
static int isDLGmaxToken();				                             /* MR3 */
#endif

static int class_nest_level = 0;
extern int inAlt;
extern set attribsRefdFromAction;
extern int UsedOldStyleAttrib;
extern int UsedNewStyleLabel;

void
#ifdef __USE_PROTOS
grammar(void)
#else
grammar()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	Graph g;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( 1 ) {
			if ( !((setwd1[LA(1)]&0x1))) break;
			if ( (LA(1)==88) ) {
				zzmatch(88); zzCONSUME;
				zzmatch(Action);
				
				if ( HdrAction==NULL ) {
					HdrAction = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
					require(HdrAction!=NULL, "rule grammar: cannot allocate header action");
					strcpy(HdrAction, LATEXT(1));
				}
				else warn("additional #header statement ignored");
 zzCONSUME;

			}
			else {
				if ( (LA(1)==89) ) {
					zzmatch(89); zzCONSUME;
					zzmatch(QuotedTerm);
					
					if ( GenCC ) {
						warn("#parser meta-op incompatible with -CC; ignored");
					}
					else {
						if ( strcmp(ParserName,"zzparser")==0 ) {
							ParserName=StripQuotes(mystrdup(LATEXT(1)));
							if ( RulePrefix[0]!='\0' )
							{
								warn("#parser meta-op incompatible with '-gp prefix'; '-gp' ignored");
								RulePrefix[0]='\0';
							}
						}
						else warn("additional #parser statement ignored");
					}
 zzCONSUME;

				}
				else {
					if ( (LA(1)==90) ) {
						zzmatch(90); zzCONSUME;
						zzmatch(QuotedTerm);
						{
							char *fname;
							zzantlr_state st; FILE *f; struct zzdlg_state dst;
							UserTokenDefsFile = mystrdup(LATEXT(1));
							zzsave_antlr_state(&st);
							zzsave_dlg_state(&dst);
							fname = mystrdup(LATEXT(1));
							f = fopen(StripQuotes(fname), "r");
							if ( f==NULL ) {warn(eMsg1("cannot open token defs file '%s'", fname+1));}
							else {
								ANTLRm(enum_file(fname+1), f, PARSE_ENUM_FILE);
								UserDefdTokens = 1;
							}
							zzrestore_antlr_state(&st);
							zzrestore_dlg_state(&dst);
						}
 zzCONSUME;

					}
					else break; /* MR6 code for exiting loop "for sure" */
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( 1 ) {
			if ( !((setwd1[LA(1)]&0x2))) break;
			if ( (LA(1)==Action) ) {
				zzmatch(Action);
				{
					UserAction *ua = newUserAction(LATEXT(1));
					ua->file = action_file; ua->line = action_line;
					if ( class_nest_level>0 ) list_add(&class_before_actions, ua);
					else list_add(&BeforeActions, ua);
				}
 zzCONSUME;

			}
			else {
				if ( (LA(1)==101) ) {
					laction();
				}
				else {
					if ( (LA(1)==102) ) {
						lmember();
					}
					else {
						if ( (LA(1)==103) ) {
							lprefix();
						}
						else {
							if ( (LA(1)==104) ) {
								aLexclass();
							}
							else {
								if ( (LA(1)==107) ) {
									token();
								}
								else {
									if ( (LA(1)==105) ) {
										error();
									}
									else {
										if ( (LA(1)==106) ) {
											tclass();
										}
										else {
											if ( (LA(1)==124) ) {
												default_exception_handler();
											}
											else {
												if ( (LA(1)==92) ) {
													class_def();
												}
												else {
													if ( (LA(1)==91) ) {
														zzmatch(91);
														
														if ( class_nest_level==0 )
														warn("missing class definition for trailing '}'");
														class_nest_level--;
 zzCONSUME;

													}
													else break; /* MR6 code for exiting loop "for sure" */
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	rule();
	g=zzaArg(zztasp1,3); SynDiag = (Junction *) zzaArg(zztasp1,3 ).left;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( 1 ) {
			if ( !((setwd1[LA(1)]&0x4))) break;
			if ( (LA(1)==NonTerminal) ) {
				rule();
				if ( zzaArg(zztasp2,1 ).left!=NULL ) {g.right = NULL; g = Or(g, zzaArg(zztasp2,1));}
			}
			else {
				if ( (LA(1)==104) ) {
					aLexclass();
				}
				else {
					if ( (LA(1)==107) ) {
						token();
					}
					else {
						if ( (LA(1)==105) ) {
							error();
						}
						else {
							if ( (LA(1)==106) ) {
								tclass();
							}
							else {
								if ( (LA(1)==92) ) {
									class_def();
								}
								else {
									if ( (LA(1)==91) ) {
										zzmatch(91);
										
										if ( class_nest_level==0 )
										warn("missing class definition for trailing '}'");
										class_nest_level--;
 zzCONSUME;

									}
									else break; /* MR6 code for exiting loop "for sure" */
								}
							}
						}
					}
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( 1 ) {
			if ( !((setwd1[LA(1)]&0x8))) break;
			if ( (LA(1)==Action) ) {
				zzmatch(Action);
				{
					UserAction *ua = newUserAction(LATEXT(1));
					ua->file = action_file; ua->line = action_line;
					if ( class_nest_level>0 ) list_add(&class_after_actions, ua);
					else list_add(&AfterActions, ua);
				}
 zzCONSUME;

			}
			else {
				if ( (LA(1)==101) ) {
					laction();
				}
				else {
					if ( (LA(1)==102) ) {
						lmember();
					}
					else {
						if ( (LA(1)==103) ) {
							lprefix();
						}
						else {
							if ( (LA(1)==105) ) {
								error();
							}
							else {
								if ( (LA(1)==106) ) {
									tclass();
								}
								else {
									if ( (LA(1)==92) ) {
										class_def();
									}
									else {
										if ( (LA(1)==91) ) {
											zzmatch(91);
											
											if ( class_nest_level==0 )
											warn("missing class definition for trailing '}'");
											class_nest_level--;
 zzCONSUME;

										}
										else break; /* MR6 code for exiting loop "for sure" */
									}
								}
							}
						}
					}
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(Eof); zzCONSUME;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x10);
	}
}

void
#ifdef __USE_PROTOS
class_def(void)
#else
class_def()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	int go=1; char name[MaxRuleName+1];
	zzmatch(92); zzCONSUME;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==NonTerminal) ) {
			zzmatch(NonTerminal);
			if(go) strncpy(name,LATEXT(1),MaxRuleName);
 zzCONSUME;

		}
		else {
			if ( (LA(1)==TokenTerm) ) {
				zzmatch(TokenTerm);
				if(go) strncpy(name,LATEXT(1),MaxRuleName);
 zzCONSUME;

			}
			else {zzFAIL(1,zzerr1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
		zzEXIT(zztasp2);
		}
	}
	
	if ( CurrentClassName[0]!='\0' && strcmp(CurrentClassName,name)!=0
	&& GenCC ) {
		err("only one grammar class allowed in this release");
		go = 0;
	}
	else strcpy(CurrentClassName, name);
	if ( !GenCC ) { err("class meta-op used without C++ option"); }
	zzmatch(95);
	
	no_classes_found = 0;
	if ( class_nest_level>=1 ) {warn("cannot have nested classes");}
	else class_nest_level++;
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x20);
	}
}

void
#ifdef __USE_PROTOS
rule(void)
#else
rule()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	
	ListNode *ex_groups = NULL;
	ExceptionGroup *eg;
	RuleEntry *q; Junction *p; Graph r; int f, l; ECnode *e;
	set toksrefd, rulesrefd;
	char *pdecl=NULL, *ret=NULL, *a; CurRetDef = CurParmDef = NULL;
	CurExGroups = NULL;
	CurElementLabels = NULL;
	/* We want a new element label hash table for each rule */
	if ( Elabel!=NULL ) killHashTable(Elabel);
	Elabel = newHashTable();
	attribsRefdFromAction = empty;
	zzmatch(NonTerminal);
	q=NULL;
	if ( hash_get(Rname, LATEXT(1))!=NULL ) {
		err(eMsg1("duplicate rule definition: '%s'",LATEXT(1)));
		CannotContinue=TRUE;
	}
	else
	{
		q = (RuleEntry *)hash_add(Rname,
		LATEXT(1),
		(Entry *)newRuleEntry(LATEXT(1)));
		CurRule = q->str;
	}
	CurRuleNode = q;
	f = CurFile; l = zzline;
	NumRules++;
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==96) ) {
			zzmatch(96);
			if ( q!=NULL ) q->noAST = TRUE;
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		;
		if ( (setwd1[LA(1)]&0x40) ) {
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==97) ) {
					zzmatch(97); zzCONSUME;
				}
				zzEXIT(zztasp3);
				}
			}
			zzmatch(PassAction);
			pdecl = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(pdecl!=NULL, "rule rule: cannot allocate param decl");
			strcpy(pdecl, LATEXT(1));
			CurParmDef = pdecl;
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==98) ) {
			zzmatch(98); zzCONSUME;
			zzmatch(PassAction);
			ret = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(ret!=NULL, "rule rule: cannot allocate ret type");
			strcpy(ret, LATEXT(1));
			CurRetDef = ret;
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==QuotedTerm) ) {
			zzmatch(QuotedTerm);
			if ( q!=NULL ) q->egroup=mystrdup(LATEXT(1));
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	
	if ( GenEClasseForRules && q!=NULL ) {
		e = newECnode;
		require(e!=NULL, "cannot allocate error class node");
		if ( q->egroup == NULL ) {a = q->str; a[0] = (char)toupper(a[0]);}
		else a = q->egroup;
		if ( Tnum( a ) == 0 )
		{
			e->tok = addTname( a );
			list_add(&eclasses, (char *)e);
			if ( q->egroup == NULL ) a[0] = (char)tolower(a[0]);
			/* refers to itself */
			list_add(&(e->elist), mystrdup(q->str));
		}
		else {
			warn(eMsg1("default errclass for '%s' would conflict with token/errclass/tokclass",a));
			if ( q->egroup == NULL ) a[0] = (char)tolower(a[0]);
			free((char *)e);
		}
	}
	BlkLevel++;
	zzmatch(99);
	inAlt=1;
 zzCONSUME;

	block( &toksrefd, &rulesrefd );
	r = makeBlk(zzaArg(zztasp1,7),0);
	CurRuleBlk = (Junction *)r.left;
	CurRuleBlk->blockid = CurBlockID;
	CurRuleBlk->jtype = RuleBlk;
	if ( q!=NULL ) CurRuleBlk->rname = q->str;
	CurRuleBlk->file = f;
	CurRuleBlk->line = l;
	CurRuleBlk->pdecl = pdecl;
	CurRuleBlk->ret = ret;
	CurRuleBlk->lock = makelocks();
	CurRuleBlk->pred_lock = makelocks();
	CurRuleBlk->tokrefs = toksrefd;
	CurRuleBlk->rulerefs = rulesrefd;
	p = newJunction();	/* add EndRule Node */
	((Junction *)r.right)->p1 = (Node *)p;
	r.right = (Node *) p;
	p->jtype = EndRule;
	p->lock = makelocks();
	p->pred_lock = makelocks();
	CurRuleBlk->end = p;
	if ( q!=NULL ) q->rulenum = NumRules;
	zzaArg(zztasp1,7) = r;
	--BlkLevel;
	altFixup();leFixup();egFixup();
	zzmatch(100);
	inAlt=0;
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==Action) ) {
			zzmatch(Action);
			a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(a!=NULL, "rule rule: cannot allocate error action");
			strcpy(a, LATEXT(1));
			CurRuleBlk->erraction = a;
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==124) ) {
			 eg  = exception_group();

			if ( eg!=NULL ) {
				list_add(&CurExGroups, (void *)eg);
				if ( eg->label=='\0' ) q->has_rule_exception = 1;
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	if ( q==NULL ) zzaArg(zztasp1,0 ).left = NULL; else zzaArg(zztasp1,0) = zzaArg(zztasp1,7);
	CurRuleNode = NULL;
	CurRuleBlk->exceptions = CurExGroups;
	CurRuleBlk->el_labels = CurElementLabels;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x80);
	}
}

void
#ifdef __USE_PROTOS
laction(void)
#else
laction()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *a;
	zzmatch(101); zzCONSUME;
	zzmatch(Action);
	
	a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
	require(a!=NULL, "rule laction: cannot allocate action");
	strcpy(a, LATEXT(1));
	list_add(&LexActions, a);
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x1);
	}
}

void
#ifdef __USE_PROTOS
lmember(void)
#else
lmember()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *a;
	zzmatch(102); zzCONSUME;
	zzmatch(Action);
	
	/* MR1 */		if (! GenCC) {
		/* MR1 */		  err("Use #lexmember only in C++ mode (to insert code in DLG class header");
		/* MR1 */	        } else {
		/* MR1 */		  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
		/* MR1 */		  require(a!=NULL, "rule lmember: cannot allocate action");
		/* MR1 */		  strcpy(a, LATEXT(1));
		/* MR1 */		  list_add(&LexMemberActions, a);
		/* MR1 */		};
	/* MR1 */		
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x2);
	}
}

void
#ifdef __USE_PROTOS
lprefix(void)
#else
lprefix()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *a;
	zzmatch(103); zzCONSUME;
	zzmatch(Action);
	
	/* MR1 */		if (! GenCC) {
		/* MR1 */		  err("Use #lexprefixr only in C++ mode (to insert code in DLG class header");
		/* MR1 */	        } else {
		/* MR1 */		  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
		/* MR1 */		  require(a!=NULL, "rule lprefix: cannot allocate action");
		/* MR1 */		  strcpy(a, LATEXT(1));
		/* MR1 */		  list_add(&LexPrefixActions, a);
		/* MR1 */		};
	/* MR1 */		
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x4);
	}
}

void
#ifdef __USE_PROTOS
aLexclass(void)
#else
aLexclass()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	zzmatch(104); zzCONSUME;
	zzmatch(TokenTerm);
	lexclass(mystrdup(LATEXT(1)));
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x8);
	}
}

void
#ifdef __USE_PROTOS
error(void)
#else
error()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *t=NULL; ECnode *e; int go=1; TermEntry *p;
	zzmatch(105); zzCONSUME;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		;
		if ( (LA(1)==TokenTerm) ) {
			zzmatch(TokenTerm);
			t=mystrdup(LATEXT(1));
 zzCONSUME;

		}
		else {
			if ( (LA(1)==QuotedTerm) ) {
				zzmatch(QuotedTerm);
				t=mystrdup(LATEXT(1));
 zzCONSUME;

			}
			else {zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
		zzEXIT(zztasp2);
		}
	}
	e = newECnode;
	require(e!=NULL, "cannot allocate error class node");
	e->lexclass = CurrentLexClass;
	if ( Tnum( (t=StripQuotes(t)) ) == 0 )
	{
		if ( hash_get(Texpr, t) != NULL )
		warn(eMsg1("errclass name conflicts with regular expression  '%s'",t));
		e->tok = addTname( t );
		set_orel(e->tok, &imag_tokens);
		require((p=(TermEntry *)hash_get(Tname, t)) != NULL,
		"hash table mechanism is broken");
		p->classname = 1;	/* entry is errclass name, not token */
		list_add(&eclasses, (char *)e);
	}
	else
	{
	warn(eMsg1("redefinition of errclass or conflict w/token or tokclass '%s'; ignored",t));
	free( (char *)e );
	go=0;
}
	zzmatch(95); zzCONSUME;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==NonTerminal) ) {
			zzmatch(NonTerminal);
			if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

		}
		else {
			if ( (LA(1)==TokenTerm) ) {
				zzmatch(TokenTerm);
				if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

			}
			else {
				if ( (LA(1)==QuotedTerm) ) {
					zzmatch(QuotedTerm);
					if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

				}
				else {zzFAIL(1,zzerr3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
		}
		zzEXIT(zztasp2);
		}
	}
	if ( go ) list_add(&(e->elist), t);
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (setwd2[LA(1)]&0x10) ) {
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==NonTerminal) ) {
					zzmatch(NonTerminal);
					if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

				}
				else {
					if ( (LA(1)==TokenTerm) ) {
						zzmatch(TokenTerm);
						if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

					}
					else {
						if ( (LA(1)==QuotedTerm) ) {
							zzmatch(QuotedTerm);
							if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

						}
						else {zzFAIL(1,zzerr4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
				zzEXIT(zztasp3);
				}
			}
			if ( go ) list_add(&(e->elist), t);
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(91); zzCONSUME;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x20);
	}
}

void
#ifdef __USE_PROTOS
tclass(void)
#else
tclass()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *t=NULL; TCnode *e; int go=1,tok; TermEntry *p, *term;
	zzmatch(106); zzCONSUME;
	zzmatch(TokenTerm);
	t=mystrdup(LATEXT(1));
 zzCONSUME;

	e = newTCnode;
	require(e!=NULL, "cannot allocate token class node");
	e->lexclass = CurrentLexClass;
	if ( Tnum( t ) == 0 )
	{
		e->tok = addTname( t );
		set_orel(e->tok, &imag_tokens);
		set_orel(e->tok, &tokclasses);
		require((p=(TermEntry *)hash_get(Tname, t)) != NULL,
		"hash table mechanism is broken");
		p->classname = 1;	/* entry is class name, not token */
		p->tclass = e;		/* save ptr to this tclass def */
		list_add(&tclasses, (char *)e);
	}
	else
	{
	warn(eMsg1("redefinition of tokclass or conflict w/token '%s'; ignored",t));
	free( (char *)e );
	go=0;
}
	zzmatch(95); zzCONSUME;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (setwd2[LA(1)]&0x40) ) {
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==TokenTerm) ) {
					zzmatch(TokenTerm);
					if ( go ) {
						term = (TermEntry *) hash_get(Tname, LATEXT(1));
						if ( term==NULL && UserDefdTokens ) {
							err("implicit token definition not allowed with #tokdefs");
							go = 0;
						}
						else {t=mystrdup(LATEXT(1)); tok=addTname(LATEXT(1));}
					}
 zzCONSUME;

				}
				else {
					if ( (LA(1)==QuotedTerm) ) {
						zzmatch(QuotedTerm);
						if ( go ) {
							term = (TermEntry *) hash_get(Texpr, LATEXT(1));
							if ( term==NULL && UserDefdTokens ) {
								err("implicit token definition not allowed with #tokdefs");
								go = 0;
							}
							else {t=mystrdup(LATEXT(1)); tok=addTexpr(LATEXT(1));}
						}
 zzCONSUME;

					}
					else {zzFAIL(1,zzerr5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
				zzEXIT(zztasp3);
				}
			}
			if ( go ) list_add(&(e->tlist), t);
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(91); zzCONSUME;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x80);
	}
}

void
#ifdef __USE_PROTOS
token(void)
#else
token()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	char *t=NULL, *e=NULL, *a=NULL; int tnum=0;
	zzmatch(107);
	tokenActionActive=1;
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==TokenTerm) ) {
			zzmatch(TokenTerm);
			t=mystrdup(LATEXT(1));
 zzCONSUME;

			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==108) ) {
					zzmatch(108); zzCONSUME;
					zzmatch(109);
					tnum = atoi(LATEXT(1));
 zzCONSUME;

				}
				zzEXIT(zztasp3);
				}
			}
		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==QuotedTerm) ) {
			zzmatch(QuotedTerm);
			e=mystrdup(LATEXT(1));
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==Action) ) {
			zzmatch(Action);
			
			a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(a!=NULL, "rule token: cannot allocate action");
			strcpy(a, LATEXT(1));
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	chkToken(t, e, a, tnum);
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x1);
	}
}

void
#ifdef __USE_PROTOS
block( set *toksrefd, set *rulesrefd )
#else
block(toksrefd,rulesrefd)
 set *toksrefd;
set *rulesrefd ;
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	
	Graph g, b;
	set saveblah;
	int saveinalt = inAlt;
	ExceptionGroup *eg;
	* toksrefd = empty;
	* rulesrefd = empty;
	set_clr(AST_nodes_refd_in_actions);
	CurBlockID++;
	CurAltNum = 1;
	saveblah = attribsRefdFromAction;
	attribsRefdFromAction = empty;
	alt( toksrefd,rulesrefd );
	b = g = zzaArg(zztasp1,1);
	
	if ( ((Junction *)g.left)->p1->ntype == nAction )
	{
		if ( !((ActionNode *)(((Junction *)g.left)->p1))->is_predicate )
		{
			((ActionNode *)(((Junction *)g.left)->p1))->init_action = TRUE;
		}
	}
	((Junction *)g.left)->blockid = CurBlockID;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==124) ) {
			 eg  = exception_group();

			
			if ( eg!=NULL ) {
				/* MR7 *****       	eg->altID = makeAltID(CurBlockID,CurAltNum);        *****/
				/* MR7 *****		CurAltStart->exception_label = eg->altID;           *****/
				list_add(&CurExGroups, (void *)eg);
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	CurAltNum++;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==110) ) {
			zzmatch(110);
			inAlt=1;
 zzCONSUME;

			alt( toksrefd,rulesrefd );
			g = Or(g, zzaArg(zztasp2,2));
			
			((Junction *)g.left)->blockid = CurBlockID;
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				while ( (LA(1)==124) ) {
					 eg  = exception_group();

					
					if ( eg!=NULL ) {
						/* MR7 *****       	eg->altID = makeAltID(CurBlockID,CurAltNum);        *****/
						/* MR7 *****		CurAltStart->exception_label = eg->altID;           *****/
						list_add(&CurExGroups, (void *)eg);
					}
					zzLOOP(zztasp3);
				}
				zzEXIT(zztasp3);
				}
			}
			CurAltNum++;
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzaArg(zztasp1,0) = b;
	attribsRefdFromAction = saveblah; inAlt = saveinalt;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x2);
	}
}

void
#ifdef __USE_PROTOS
alt( set *toksrefd, set *rulesrefd )
#else
alt(toksrefd,rulesrefd)
 set *toksrefd;
set *rulesrefd ;
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	int n=0,ne=0; Graph g; int e_num=0, not=0; Node *node; set elems, dif;
	int first_on_line = 1, use_def_MT_handler = 0;
	g.left=NULL; g.right=NULL;
	
			CurAltStart = NULL;
	elems = empty;
	inAlt = 1;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==84) ) {
			zzmatch(84);
			use_def_MT_handler = 1;
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		int tok;
		while ( (setwd3[LA(1)]&0x4) ) {
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				not=0;
				if ( (LA(1)==111) ) {
					zzmatch(111);
					not=1;
 zzCONSUME;

				}
				zzEXIT(zztasp3);
				}
			}
			 node  = element( not, first_on_line, use_def_MT_handler );

			if ( node!=NULL && node->ntype!=nAction ) first_on_line = 0;
			
			if ( zzaArg(zztasp2,2 ).left!=NULL ) {
				g = Cat(g, zzaArg(zztasp2,2));
				n++;
				if ( node!=NULL ) {
					if ( node->ntype!=nAction ) e_num++;
					/* record record number of all rule and token refs */
					if ( node->ntype==nToken ) {
						TokNode *tk = (TokNode *)((Junction *)zzaArg(zztasp2,2 ).left)->p1;
						tk->elnum = e_num;
						set_orel(e_num, &elems);
					}
					else if ( node->ntype==nRuleRef ) {
						RuleRefNode *rn = (RuleRefNode *)((Junction *)zzaArg(zztasp2,2 ).left)->p1;
						rn->elnum = e_num;
						set_orel(e_num,  rulesrefd);
					}
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	if ( n == 0 ) g = emptyAlt();
	zzaArg(zztasp1,0) = g;
	/* We want to reduce number of LT(i) calls and the number of
	* local attribute variables in C++ mode (for moment, later we'll
	* do for C also).  However, if trees are being built, they
	* require most of the attrib variables to create the tree nodes
	* with; therefore, we gen a token ptr for each token ref in C++
	*/
	if ( GenCC && !GenAST )
	{
	/* This now free's the temp set -ATG 5/6/95 */
	set temp;
	temp = set_and(elems, attribsRefdFromAction);
	set_orin( toksrefd, temp);
	set_free(temp);
}
else set_orin( toksrefd, elems);
if ( GenCC ) {
	dif = set_dif(attribsRefdFromAction, elems);
	if ( set_deg(dif)>0 )
	err("one or more $i in action(s) refer to non-token elements");
	set_free(dif);
}
set_free(elems);
set_free(attribsRefdFromAction);
inAlt = 0;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x8);
	}
}

 LabelEntry *
#ifdef __USE_PROTOS
element_label(void)
#else
element_label()
#endif
{
	 LabelEntry * 	 _retv;
	zzRULE;
	zzBLOCK(zztasp1);
	PURIFY(_retv,sizeof( LabelEntry * 	))
	zzMake0;
	{
	TermEntry *t=NULL; LabelEntry *l=NULL; RuleEntry *r=NULL; char *lab;
	zzmatch(LABEL);
	lab = mystrdup(LATEXT(1));
 zzCONSUME;

	
	UsedNewStyleLabel = 1;
	if ( UsedOldStyleAttrib ) err("cannot mix with new-style labels with old-style $i");
	t = (TermEntry *) hash_get(Tname, lab);
	if ( t==NULL ) t = (TermEntry *) hash_get(Texpr, lab);
	if ( t==NULL ) r = (RuleEntry *) hash_get(Rname, lab);
	if ( t!=NULL ) {
		err(eMsg1("label definition clashes with token/tokclass definition: '%s'", lab));
		_retv = NULL;
	}
	else if ( r!=NULL ) {
		err(eMsg1("label definition clashes with rule definition: '%s'", lab));
		_retv = NULL;
	}
	else {
		/* we don't clash with anybody else */
		l = (LabelEntry *) hash_get(Elabel, lab);
		if ( l==NULL ) {	/* ok to add new element label */
		l = (LabelEntry *)hash_add(Elabel,
		lab,
		(Entry *)newLabelEntry(lab));
		/* add to list of element labels for this rule */
		list_add(&CurElementLabels, (void *)lab);
		/* MR7 */       leAdd(l);  /* list of labels waiting for exception group definitions */
		_retv = l;
	}
	else {
	err(eMsg1("label definitions must be unique per rule: '%s'", lab));
	_retv = NULL;
}
}
	zzmatch(99); zzCONSUME;
	zzEXIT(zztasp1);
	return _retv;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x10);
	return _retv;
	}
}

 Node *
#ifdef __USE_PROTOS
element( int not, int first_on_line, int use_def_MT_handler )
#else
element(not,first_on_line,use_def_MT_handler)
 int not;
int first_on_line;
int use_def_MT_handler ;
#endif
{
	 Node * 	 _retv;
	zzRULE;
	zzBLOCK(zztasp1);
	PURIFY(_retv,sizeof( Node * 	))
	zzMake0;
	{
	
	Attrib blk;
	Predicate *pred = NULL;
	int local_use_def_MT_handler=0;
	ActionNode *act;
	RuleRefNode *rr;
	set toksrefd, rulesrefd;
	TermEntry *term;
	TokNode *p=NULL; RuleRefNode *q; int approx=0;
	LabelEntry *label=NULL;
	_retv = NULL;
	if ( (setwd3[LA(1)]&0x20) ) {
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			if ( (LA(1)==LABEL) ) {
				 label  = element_label();

			}
			zzEXIT(zztasp2);
			}
		}
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			if ( (LA(1)==TokenTerm) ) {
				zzmatch(TokenTerm);
				
				term = (TermEntry *) hash_get(Tname, LATEXT(1));
				if ( term==NULL && UserDefdTokens ) {
					err("implicit token definition not allowed with #tokdefs");
					zzaRet.left = zzaRet.right = NULL;
				}
				else {
					zzaRet = buildToken(LATEXT(1));
					p=((TokNode *)((Junction *)zzaRet.left)->p1);
					term = (TermEntry *) hash_get(Tname, LATEXT(1));
					require( term!= NULL, "hash table mechanism is broken");
					p->tclass = term->tclass;
					p->complement =  not;
					if ( label!=NULL ) {
						p->el_label = label->str;
						label->elem = (Node *)p;
					}
				}
 zzCONSUME;

				{
					zzBLOCK(zztasp3);
					zzMake0;
					{
					if ( (LA(1)==112) ) {
						zzmatch(112); zzCONSUME;
						{
							zzBLOCK(zztasp4);
							zzMake0;
							{
							if ( (LA(1)==QuotedTerm) ) {
								zzmatch(QuotedTerm);
								if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

							}
							else {
								if ( (LA(1)==TokenTerm) ) {
									zzmatch(TokenTerm);
									if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

								}
								else {zzFAIL(1,zzerr6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
							}
							zzEXIT(zztasp4);
							}
						}
					}
					zzEXIT(zztasp3);
					}
				}
				
				if ( p!=NULL && (p->upper_range!=0 || p->tclass ||  not) )
				list_add(&MetaTokenNodes, (void *)p);
				{
					zzBLOCK(zztasp3);
					zzMake0;
					{
					if ( (LA(1)==113) ) {
						zzmatch(113);
						if ( p!=NULL ) p->astnode=ASTroot;
 zzCONSUME;

					}
					else {
						if ( (setwd3[LA(1)]&0x40) ) {
							if ( p!=NULL ) p->astnode=ASTchild;
						}
						else {
							if ( (LA(1)==96) ) {
								zzmatch(96);
								if ( p!=NULL ) p->astnode=ASTexclude;
 zzCONSUME;

							}
							else {zzFAIL(1,zzerr7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
						}
					}
					zzEXIT(zztasp3);
					}
				}
				{
					zzBLOCK(zztasp3);
					zzMake0;
					{
					if ( (LA(1)==84) ) {
						zzmatch(84);
						local_use_def_MT_handler = 1;
 zzCONSUME;

					}
					zzEXIT(zztasp3);
					}
				}
				
				if ( p!=NULL &&  first_on_line ) {
					CurAltStart = (Junction *)zzaRet.left;
					altAdd(CurAltStart);                                 /* MR7 */
					p->altstart = CurAltStart;
				}
				if ( p!=NULL )
				p->use_def_MT_handler =  use_def_MT_handler || local_use_def_MT_handler;
				_retv = (Node *)p;
			}
			else {
				if ( (LA(1)==QuotedTerm) ) {
					zzmatch(QuotedTerm);
					
					term = (TermEntry *) hash_get(Texpr, LATEXT(1));
					if ( term==NULL && UserDefdTokens ) {
						err("implicit token definition not allowed with #tokdefs");
						zzaRet.left = zzaRet.right = NULL;
					}
					else {
						zzaRet = buildToken(LATEXT(1)); p=((TokNode *)((Junction *)zzaRet.left)->p1);
						p->complement =  not;
						if ( label!=NULL ) {
							p->el_label = label->str;
							label->elem = (Node *)p;
						}
					}
 zzCONSUME;

					{
						zzBLOCK(zztasp3);
						zzMake0;
						{
						if ( (LA(1)==112) ) {
							zzmatch(112); zzCONSUME;
							{
								zzBLOCK(zztasp4);
								zzMake0;
								{
								if ( (LA(1)==QuotedTerm) ) {
									zzmatch(QuotedTerm);
									if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

								}
								else {
									if ( (LA(1)==TokenTerm) ) {
										zzmatch(TokenTerm);
										if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

									}
									else {zzFAIL(1,zzerr8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
								}
								zzEXIT(zztasp4);
								}
							}
						}
						zzEXIT(zztasp3);
						}
					}
					{
						zzBLOCK(zztasp3);
						zzMake0;
						{
						if ( (LA(1)==113) ) {
							zzmatch(113);
							if ( p!=NULL ) p->astnode=ASTroot;
 zzCONSUME;

						}
						else {
							if ( (setwd3[LA(1)]&0x80) ) {
								if ( p!=NULL ) p->astnode=ASTchild;
							}
							else {
								if ( (LA(1)==96) ) {
									zzmatch(96);
									if ( p!=NULL ) p->astnode=ASTexclude;
 zzCONSUME;

								}
								else {zzFAIL(1,zzerr9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
							}
						}
						zzEXIT(zztasp3);
						}
					}
					{
						zzBLOCK(zztasp3);
						zzMake0;
						{
						if ( (LA(1)==84) ) {
							zzmatch(84);
							local_use_def_MT_handler = 1;
 zzCONSUME;

						}
						zzEXIT(zztasp3);
						}
					}
					
					if ( p!=NULL && (p->upper_range!=0 || p->tclass ||  not) )
					list_add(&MetaTokenNodes, (void *)p);
					
					if (  first_on_line ) {
						CurAltStart = (Junction *)zzaRet.left;
						altAdd(CurAltStart);                                 /* MR7 */
						p->altstart = CurAltStart;
					}
					if ( p!=NULL )
					p->use_def_MT_handler =  use_def_MT_handler || local_use_def_MT_handler;
					_retv = (Node *)p;
				}
				else {
					if ( (LA(1)==WildCard) ) {
						if (  not ) warn("~ WILDCARD is an undefined operation (implies 'nothing')");
						zzmatch(WildCard);
						zzaRet = buildWildCard(LATEXT(1)); p=((TokNode *)((Junction *)zzaRet.left)->p1);
 zzCONSUME;

						{
							zzBLOCK(zztasp3);
							zzMake0;
							{
							if ( (LA(1)==113) ) {
								zzmatch(113);
								p->astnode=ASTroot;
 zzCONSUME;

							}
							else {
								if ( (setwd4[LA(1)]&0x1) ) {
									p->astnode=ASTchild;
								}
								else {
									if ( (LA(1)==96) ) {
										zzmatch(96);
										p->astnode=ASTexclude;
 zzCONSUME;

									}
									else {zzFAIL(1,zzerr10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
								}
							}
							zzEXIT(zztasp3);
							}
						}
						list_add(&MetaTokenNodes, (void *)p);
						
						if (  first_on_line ) {
							CurAltStart = (Junction *)zzaRet.left;
							altAdd(CurAltStart);                                 /* MR7 */
							p->altstart = CurAltStart;
							if ( label!=NULL ) {
								p->el_label = label->str;
								label->elem = (Node *)p;
							}
						}
						_retv = (Node *)p;
					}
					else {
						if ( (LA(1)==NonTerminal) ) {
							if (  not ) warn("~ NONTERMINAL is an undefined operation");
							zzmatch(NonTerminal);
							zzaRet = buildRuleRef(LATEXT(1));
 zzCONSUME;

							{
								zzBLOCK(zztasp3);
								zzMake0;
								{
								if ( (LA(1)==96) ) {
									zzmatch(96);
									q = (RuleRefNode *) ((Junction *)zzaRet.left)->p1;
									q->astnode=ASTexclude;
 zzCONSUME;

								}
								zzEXIT(zztasp3);
								}
							}
							{
								zzBLOCK(zztasp3);
								zzMake0;
								{
								if ( (setwd4[LA(1)]&0x2) ) {
									{
										zzBLOCK(zztasp4);
										zzMake0;
										{
										if ( (LA(1)==97) ) {
											zzmatch(97); zzCONSUME;
										}
										zzEXIT(zztasp4);
										}
									}
									zzmatch(PassAction);
									addParm(((Junction *)zzaRet.left)->p1, LATEXT(1));
 zzCONSUME;

								}
								zzEXIT(zztasp3);
								}
							}
							rr=(RuleRefNode *) ((Junction *)zzaRet.left)->p1;
							{
								zzBLOCK(zztasp3);
								zzMake0;
								{
								char *a;
								if ( (LA(1)==98) ) {
									zzmatch(98); zzCONSUME;
									zzmatch(PassAction);
									
									a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
									require(a!=NULL, "rule element: cannot allocate assignment");
									strcpy(a, LATEXT(1));
									rr->assign = a;
 zzCONSUME;

								}
								zzEXIT(zztasp3);
								}
							}
							
							if ( label!=NULL ) {
								rr->el_label = label->str;
								label->elem = (Node *)rr;
							}
							if (  first_on_line ) {
								CurAltStart = (Junction *)zzaRet.left;
								altAdd(CurAltStart);                                 /* MR7 */
								((RuleRefNode *)((Junction *)zzaRet.left)->p1)->altstart = CurAltStart;
							}
							_retv = (Node *)rr;
						}
						else {zzFAIL(1,zzerr11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
					}
				}
			}
			zzEXIT(zztasp2);
			}
		}
	}
	else {
		if ( (LA(1)==Action) ) {
			if (  not )	warn("~ ACTION is an undefined operation");
			zzmatch(Action);
			zzaArg(zztasp1,0) = buildAction(LATEXT(1),action_file,action_line, 0);
 zzCONSUME;

			if (  first_on_line ) {                                /* MR7 */
				CurAltStart = (Junction *)zzaArg(zztasp1,0 ).left;                   /* MR7 */
				altAdd(CurAltStart);                                 /* MR7 */
			};
			_retv = (Node *) ((Junction *)zzaArg(zztasp1,0 ).left)->p1;
		}
		else {
			if ( (LA(1)==Pred) ) {
				if (  not )	warn("~ SEMANTIC-PREDICATE is an undefined operation");
				zzmatch(Pred);
				zzaArg(zztasp1,0) = buildAction(LATEXT(1),action_file,action_line, 1);
 zzCONSUME;

				act = (ActionNode *) ((Junction *)zzaArg(zztasp1,0 ).left)->p1;
				{
					zzBLOCK(zztasp2);
					zzMake0;
					{
					char *a;
					if ( (LA(1)==PassAction) ) {
						zzmatch(PassAction);
						
						a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
						require(a!=NULL, "rule element: cannot allocate predicate fail action");
						strcpy(a, LATEXT(1));
						act->pred_fail = a;
 zzCONSUME;

					}
					zzEXIT(zztasp2);
					}
				}
				if (  first_on_line ) {                                /* MR7 */
					CurAltStart = (Junction *)zzaArg(zztasp1,0 ).left;                   /* MR7 */
					altAdd(CurAltStart);                                 /* MR7 */
				};
				_retv = (Node *)act;
			}
			else {
				if ( (setwd4[LA(1)]&0x4) ) {
					if (  not )	warn("~ BLOCK is an undefined operation");
					BlkLevel++;
					{
						zzBLOCK(zztasp2);
						zzMake0;
						{
						if ( (LA(1)==114) ) {
							zzmatch(114); zzCONSUME;
							{
								zzBLOCK(zztasp3);
								zzMake0;
								{
								if ( (LA(1)==115) ) {
									zzmatch(115);
									approx=LL_k;
 zzCONSUME;

								}
								else {
									if ( (LA(1)==116) ) {
										zzmatch(116);
										approx = 1;
 zzCONSUME;

									}
									else {
										if ( (LA(1)==117) ) {
											zzmatch(117);
											approx = 2;
 zzCONSUME;

										}
										else {zzFAIL(1,zzerr12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
									}
								}
								zzEXIT(zztasp3);
								}
							}
						}
						zzEXIT(zztasp2);
						}
					}
					{
						zzBLOCK(zztasp2);
						zzMake0;
						{
						if ( (LA(1)==118) ) {
							zzmatch(118); zzCONSUME;
							block( &toksrefd,&rulesrefd );
							zzmatch(119);
							blk = zzaRet = zzaArg(zztasp2,2); --BlkLevel;
 zzCONSUME;

							{
								zzBLOCK(zztasp3);
								zzMake0;
								{
								if ( (LA(1)==120) ) {
									zzmatch(120);
									zzaRet = makeLoop(zzaRet,approx);
 zzCONSUME;

								}
								else {
									if ( (LA(1)==121) ) {
										zzmatch(121);
										zzaRet = makePlus(zzaRet,approx);
 zzCONSUME;

									}
									else {
										if ( (LA(1)==122) ) {
											zzmatch(122); zzCONSUME;
											{
												zzBLOCK(zztasp4);
												zzMake0;
												{
												if ( (LA(1)==123) ) {
													zzmatch(123); zzCONSUME;
													zzmatch(Pred);
													zzaRet = buildAction(LATEXT(1),action_file,action_line,1);
 zzCONSUME;

													act = (ActionNode *) ((Junction *)zzaRet.left)->p1;
													{
														zzBLOCK(zztasp5);
														zzMake0;
														{
														char *a;
														if ( (LA(1)==PassAction) ) {
															zzmatch(PassAction);
															
															a = (char *)calloc(strlen(LATEXT(1))+1, sizeof(char));
															require(a!=NULL, "rule element: cannot allocate predicate fail action");
															strcpy(a, LATEXT(1));
															act->pred_fail = a;
 zzCONSUME;

														}
														zzEXIT(zztasp5);
														}
													}
													if ( first_on_line) {                      /* MR7 */
														CurAltStart=(Junction *)zzaRet.left;         /* MR7 */
														altAdd(CurAltStart);                     /* MR7 */
													};
													_retv = (Node *)act;
													
													pred = computePredicateFromContextGuard(blk);
													if ( pred==NULL )
													err("invalid or missing context guard");
													else {
														pred->expr = act->action;
														pred->source = act;
														act->guardpred = pred;
													}
												}
												else {
													if ( (setwd4[LA(1)]&0x8) ) {
														zzaRet = makeBlk(zzaRet,approx);
														FoundGuessBlk = 1;
														((Junction *) ((Junction *)zzaRet.left)->p1)->guess=1;
														if ( ! first_on_line ) {
															err("(...)? predicate must be first element of production");
														}
													}
													else {zzFAIL(1,zzerr13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
												}
												zzEXIT(zztasp4);
												}
											}
										}
										else {
											if ( (setwd4[LA(1)]&0x10) ) {
												zzaRet = makeBlk(zzaRet,approx);
											}
											else {zzFAIL(1,zzerr14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
										}
									}
								}
								zzEXIT(zztasp3);
								}
							}
							
							if ( pred==NULL ) {
								((Junction *)((Junction *)zzaRet.left)->p1)->blockid = CurBlockID;
								((Junction *)((Junction *)zzaRet.left)->p1)->tokrefs = toksrefd;
								((Junction *)((Junction *)zzaRet.left)->p1)->rulerefs = rulesrefd;
								if (  first_on_line ) {                         /* MR7 */
									CurAltStart = (Junction *)((Junction *)((Junction *)zzaRet.left)->p1);  /* MR7 */
									altAdd(CurAltStart);                         /* MR7 */
								};                                              /* MR7 */
								_retv = (Node *) ((Junction *)zzaRet.left)->p1;
							}
						}
						else {
							if ( (LA(1)==95) ) {
								zzmatch(95); zzCONSUME;
								block( &toksrefd,&rulesrefd );
								zzaRet = makeOpt(zzaArg(zztasp2,2),approx); --BlkLevel;
								zzmatch(91);
								
								((Junction *)((Junction *)zzaRet.left)->p1)->blockid = CurBlockID;
								((Junction *)((Junction *)zzaRet.left)->p1)->tokrefs = toksrefd;
								((Junction *)((Junction *)zzaRet.left)->p1)->rulerefs = rulesrefd;
 zzCONSUME;

								if (  first_on_line ) {                            /* MR7 */
									CurAltStart = (Junction *) ((Junction *)((Junction *)zzaRet.left)->p1);  /* MR7 */
									altAdd(CurAltStart);                             /* MR7 */
								};
								_retv = (Node *) ((Junction *)zzaRet.left)->p1;
							}
							else {zzFAIL(1,zzerr15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
						}
						zzEXIT(zztasp2);
						}
					}
				}
				else {
					if ( (LA(1)==120) ) {
						zzmatch(120);
						warn("don't you want a ')' with that '*'?"); CannotContinue=TRUE;
 zzCONSUME;

					}
					else {
						if ( (LA(1)==121) ) {
							zzmatch(121);
							warn("don't you want a ')' with that '+'?"); CannotContinue=TRUE;
 zzCONSUME;

						}
						else {
							if ( (LA(1)==98) ) {
								zzmatch(98);
								warn("'>' can only appear after a nonterminal"); CannotContinue=TRUE;
 zzCONSUME;

							}
							else {
								if ( (LA(1)==PassAction) ) {
									zzmatch(PassAction);
									warn("[...] out of context 'rule > [...]'");
									CannotContinue=TRUE;
 zzCONSUME;

								}
								else {zzFAIL(1,zzerr16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
							}
						}
					}
				}
			}
		}
	}
	zzEXIT(zztasp1);
	return _retv;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd4, 0x20);
	return _retv;
	}
}

void
#ifdef __USE_PROTOS
default_exception_handler(void)
#else
default_exception_handler()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	 DefaultExGroup  = exception_group();

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd4, 0x40);
	}
}

 ExceptionGroup *
#ifdef __USE_PROTOS
exception_group(void)
#else
exception_group()
#endif
{
	 ExceptionGroup * 	 _retv;
	zzRULE;
	zzBLOCK(zztasp1);
	PURIFY(_retv,sizeof( ExceptionGroup * 	))
	zzMake0;
	{
	ExceptionHandler *h; LabelEntry *label=NULL;	  /* MR6 */
	FoundException = 1; FoundExceptionGroup = 1;
	zzmatch(124);
	_retv = (ExceptionGroup *)calloc(1, sizeof(ExceptionGroup));
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		char *p;
		if ( (LA(1)==PassAction) ) {
			zzmatch(PassAction);
			
			p = LATEXT(1)+1;
			p[strlen(p)-1] = '\0';		/* kill trailing space */
			label = (LabelEntry *) hash_get(Elabel, LATEXT(1)+1);
			if ( label==NULL )
			{
				err(eMsg1("unknown label in exception handler: '%s'", LATEXT(1)+1));
			}
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==126) ) {
			 h  = exception_handler();

			list_add(&(_retv->handlers), (void *)h);
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==125) ) {
			zzmatch(125); zzCONSUME;
			zzmatch(99); zzCONSUME;
			zzmatch(Action);
			{
				ExceptionHandler *eh = (ExceptionHandler *)
				calloc(1, sizeof(ExceptionHandler));
				char *a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
				require(eh!=NULL, "exception: cannot allocate handler");
				require(a!=NULL, "exception: cannot allocate action");
				strcpy(a, LATEXT(1));
				eh->action = a;
				eh->signalname = (char *) calloc(strlen("default")+1, sizeof(char));
				require(eh->signalname!=NULL, "exception: cannot allocate sig name");
				strcpy(eh->signalname, "default");
				list_add(&(_retv->handlers), (void *)eh);
			}
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	
	if ( label!=NULL ) {
		/* Record ex group in sym tab for this label */
		if ( label->ex_group!=NULL ) {
			err(eMsg1("duplicate exception handler for label '%s'",label->str));
		} else {
			label->ex_group = _retv;
			/* Label the exception group itself */
			_retv->label = label->str;
			/* Make the labelled element pt to the exception also */
			/* MR6 */	  if (label->elem == NULL) {
				/* MR6 */	     err(eMsg1("reference in exception handler to undefined label '%s'",label->str));
				/* MR6 */	  } else {
				switch ( label->elem->ntype ) {
					case nRuleRef :
					{
						RuleRefNode *r = (RuleRefNode *)label->elem;
						r->ex_group = _retv;
						break;
					}
					case nToken :
					{
						TokNode *t = (TokNode *)label->elem;
						t->ex_group = _retv;
						break;
					}
				} /* end switch */
				/* MR6 */	  }; /* end test on label->elem */
		} /* end test on label->ex_group */
		
		} /* end test on exception label */
	
/* MR7 */
	/* MR7 */   if (BlkLevel == 1 && label == NULL) {
		/* MR7 */     _retv->forRule=1;
		/* MR7 */   } else if (label == NULL) {
		/* MR7 */     _retv->altID = makeAltID(CurBlockID,CurAltNum);
		/* MR7 */     egAdd(_retv);
		/* MR7 */   } else {
		/* MR7 */     _retv->labelEntry=label;
		/* MR7 */   };
	/* MR7 */
	/* MR7 */	    /* You may want to remove this exc from the rule list  */
	/* MR7 */		/* and handle at the labeled element site.             */
	/* MR7 */
	/* MR7 */   if (label != NULL) {
		/* MR7 */     _retv = NULL;
		/* MR7 */   };
	
		
	zzEXIT(zztasp1);
	return _retv;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd4, 0x80);
	return _retv;
	}
}

 ExceptionHandler *
#ifdef __USE_PROTOS
exception_handler(void)
#else
exception_handler()
#endif
{
	 ExceptionHandler * 	 _retv;
	zzRULE;
	zzBLOCK(zztasp1);
	PURIFY(_retv,sizeof( ExceptionHandler * 	))
	zzMake0;
	{
	char *a;
	zzmatch(126);
	
	_retv = (ExceptionHandler *)calloc(1, sizeof(ExceptionHandler));
	require(_retv!=NULL, "exception: cannot allocate handler");
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==NonTerminal) ) {
			zzmatch(NonTerminal);
			
			_retv->signalname = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(_retv->signalname!=NULL, "exception: cannot allocate sig name");
			strcpy(_retv->signalname, LATEXT(1));
 zzCONSUME;

		}
		else {
			if ( (LA(1)==TokenTerm) ) {
				zzmatch(TokenTerm);
				
				_retv->signalname = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
				require(_retv->signalname!=NULL, "exception: cannot allocate sig name");
				strcpy(_retv->signalname, LATEXT(1));
 zzCONSUME;

			}
			else {zzFAIL(1,zzerr17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(99); zzCONSUME;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		_retv->action = NULL;
		if ( (LA(1)==Action) ) {
			zzmatch(Action);
			
			_retv->action = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
			require(_retv->action!=NULL, "exception: cannot allocate action");
			strcpy(_retv->action, LATEXT(1));
 zzCONSUME;

		}
		zzEXIT(zztasp2);
		}
	}
	zzEXIT(zztasp1);
	return _retv;
fail:
	zzEXIT(zztasp1);
	CannotContinue=TRUE;
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd5, 0x1);
	return _retv;
	}
}

void
#ifdef __USE_PROTOS
enum_file( char *fname )
#else
enum_file(fname)
 char *fname ;
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	if ( (setwd5[LA(1)]&0x2) ) {
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			if ( (LA(1)==134) ) {
				zzmatch(134); zzCONSUME;
				zzmatch(ID); zzCONSUME;
				{
					zzBLOCK(zztasp3);
					zzMake0;
					{
					if ( (LA(1)==140) ) {
						zzmatch(140); zzCONSUME;
						zzmatch(ID); zzCONSUME;
					}
					zzEXIT(zztasp3);
					}
				}
			}
			zzEXIT(zztasp2);
			}
		}
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			if ( (LA(1)==142) ) {
				{
					zzBLOCK(zztasp3);
					int zzcnt=1;
					zzMake0;
					{
					do {
						enum_def(  fname );
						zzLOOP(zztasp3);
					} while ( (LA(1)==142) );
					zzEXIT(zztasp3);
					}
				}
			}
			else {
				if ( (LA(1)==140) ) {
					defines(  fname );
				}
				else {zzFAIL(1,zzerr18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
			}
			zzEXIT(zztasp2);
			}
		}
	}
	else {
		if ( (LA(1)==Eof) ) {
		}
		else {zzFAIL(1,zzerr19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd5, 0x4);
	}
}

void
#ifdef __USE_PROTOS
defines( char *fname )
#else
defines(fname)
 char *fname ;
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	int v; int maxt=(-1); char *t;
	{
		zzBLOCK(zztasp2);
		int zzcnt=1;
		zzMake0;
		{
		do {
			zzmatch(140); zzCONSUME;
			zzmatch(ID);
			t = mystrdup(LATEXT(1));
 zzCONSUME;

			zzmatch(INT);
			
			v = atoi(LATEXT(1));
			/*			fprintf(stderr, "#token %s=%d\n", t, v);*/
			
	/* MR2 Andreas Magnusson (Andreas.Magnusson@mailbox.swipnet.se) */
			/* MR2 Fix to bug introduced by 1.33MR1 for #tokdefs            */
			/* MR2 Don't let #tokdefs be confused by 			*/
			/* MR2   DLGminToken and DLGmaxToken     			*/
			
			if ( ! isDLGmaxToken(t)) {		/* MR2 */
			TokenNum = v;
			if ( v>maxt ) maxt=v;
			if ( Tnum( t ) == 0 ) {
			addForcedTname( t, v );
		} else {
		warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
	};
};
 zzCONSUME;

			zzLOOP(zztasp2);
		} while ( (LA(1)==140) );
		zzEXIT(zztasp2);
		}
	}
	TokenNum = maxt + 1;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd5, 0x8);
	}
}

void
#ifdef __USE_PROTOS
enum_def( char *fname )
#else
enum_def(fname)
 char *fname ;
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	int v= 0; int maxt=(-1); char *t;
	zzmatch(142); zzCONSUME;
	zzmatch(ID); zzCONSUME;
	zzmatch(143); zzCONSUME;
	zzmatch(ID);
	t = mystrdup(LATEXT(1));
 zzCONSUME;

	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==144) ) {
			zzmatch(144); zzCONSUME;
			zzmatch(INT);
			v=atoi(LATEXT(1));
 zzCONSUME;

		}
		else {
			if ( (setwd5[LA(1)]&0x10) ) {
				v++;
			}
			else {zzFAIL(1,zzerr20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
		zzEXIT(zztasp2);
		}
	}
	
	/*			fprintf(stderr, "#token %s=%d\n", t, v);*/
	TokenNum = v;
	if ( v>maxt ) maxt=v;				/* MR3 */
	if ( Tnum( t ) == 0 ) addForcedTname( t, v );
	else {
		warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
	}
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==145) ) {
			zzmatch(145); zzCONSUME;
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==ID)&&(isDLGmaxToken(LATEXT(1))) ) {
					if (!(isDLGmaxToken(LATEXT(1)))) {zzfailed_pred("  isDLGmaxToken(LATEXT(1))");}
					zzmatch(ID); zzCONSUME;
					{
						zzBLOCK(zztasp4);
						zzMake0;
						{
						if ( (LA(1)==144) ) {
							zzmatch(144); zzCONSUME;
							zzmatch(INT); zzCONSUME;
						}
						zzEXIT(zztasp4);
						}
					}
				}
				else {
					if ( (LA(1)==ID) ) {
						zzmatch(ID);
						t = mystrdup(LATEXT(1));
 zzCONSUME;

						{
							zzBLOCK(zztasp4);
							zzMake0;
							{
							if ( (LA(1)==144) ) {
								zzmatch(144); zzCONSUME;
								zzmatch(INT);
								v=atoi(LATEXT(1));
 zzCONSUME;

							}
							else {
								if ( (setwd5[LA(1)]&0x20) ) {
									v++;
								}
								else {zzFAIL(1,zzerr21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
							}
							zzEXIT(zztasp4);
							}
						}
						
						/*					fprintf(stderr, "#token %s=%d\n", t, v);*/
						TokenNum = v;
						if ( v>maxt ) maxt=v;				/* MR3 */
						if ( Tnum( t ) == 0 ) addForcedTname( t, v );
						else {
							warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
						}
					}
				}
				zzEXIT(zztasp3);
				}
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(146); zzCONSUME;
	zzmatch(147);
	TokenNum = maxt + 1;
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd5, 0x40);
	}
}


/* MR2 Andreas Magnusson (Andreas.Magnusson@mailbox.swipnet.se) */
/* MR2 Fix to bug introduced by 1.33MR1 for #tokdefs            */
/* MR2 Don't let #tokdefs be confused by 			*/
/* MR2   DLGminToken and DLGmaxToken     			*/

/* semantic check on DLGminToken and DLGmaxmaxToken in #tokdefs */

#ifdef __USE_PROTOS
static int isDLGmaxToken(char *Token)
#else
static int isDLGmaxToken(Token)
char *	Token;
#endif
{
static char checkStr1[] = "DLGmaxToken";
static char checkStr2[] = "DLGminToken";

   if (strcmp(Token, checkStr1) == 0)
return 1;
else if (strcmp(Token, checkStr2) == 0)
return 1;
else
return 0;
}

/* semantics of #token */
static void
#ifdef __USE_PROTOS
chkToken(char *t, char *e, char *a, int tnum)
#else
chkToken(t,e,a,tnum)
char *t, *e, *a;
int tnum;
#endif
{
TermEntry *p;

	/* check to see that they don't try to redefine a token as a token class */
if ( t!=NULL ) {
p = (TermEntry *) hash_get(Tname, t);
if ( p!=NULL && p->classname ) {
	err(eMsg1("redefinition of #tokclass '%s' to #token not allowed; ignored",t));
	if ( a!=NULL ) free((char *)a);
	return;
}
}

	if ( t==NULL && e==NULL ) {			/* none found */
err("#token requires at least token name or rexpr");
}
else if ( t!=NULL && e!=NULL ) {	/* both found */
if ( UserDefdTokens ) {			/* if #tokdefs, must not define new */
	p = (TermEntry *) hash_get(Tname, t);
	if ( p==NULL ) {
		err(eMsg1("#token definition '%s' not allowed with #tokdefs; ignored",t));
		return;
	}
}
Tklink(t, e);
if ( a!=NULL ) {
	if ( hasAction(e) ) {
		err(eMsg1("redefinition of action for %s; ignored",e));
	}
	else setHasAction(e, a);
}
}
else if ( t!=NULL ) {				/* only one found */
if ( UserDefdTokens ) {
	err(eMsg1("#token definition '%s' not allowed with #tokdefs; ignored",t));
	return;
}
if ( Tnum( t ) == 0 ) addTname( t );
else {
	err(eMsg1("redefinition of token %s; ignored",t));
}
if ( a!=NULL ) {
	err(eMsg1("action cannot be attached to a token name (%s); ignored",t));
	free((char *)a);
}
}
else if ( e!=NULL ) {
if ( Tnum( e ) == 0 ) addTexpr( e );
else {
	if ( hasAction(e) ) {
		err(eMsg1("redefinition of action for expr %s; ignored",e));
	}
	else if ( a==NULL ) {
		err(eMsg1("redefinition of expr %s; ignored",e));
	}
}
if ( a!=NULL ) setHasAction(e, a);
}

	/* if a token type number was specified, then add the token ID and 'tnum'
* pair to the ForcedTokens list.  (only applies if an id was given)
*/
if ( t!=NULL && tnum>0 )
{
if ( set_el(tnum, reserved_positions) )
{
	err(eMsgd("a token has already been forced to token number %d; ignored", tnum));
}
else
{
	list_add(&ForcedTokens, newForcedToken(t,tnum));
	set_orel(tnum, &reserved_positions);
}
}
}

static int
#ifdef __USE_PROTOS
match_token(char *s, char **nxt)
#else
match_token(s,nxt)
char *s;
char **nxt;
#endif
{
	if ( !(*s>='A' && *s<='Z') ) return 0;
	s++;
	while ( (*s>='a' && *s<='z') ||
	(*s>='A' && *s<='Z') ||
	(*s>='0' && *s<='9') ||
	*s=='_' )
	{
		s++;
	}
	if ( *s!=' ' && *s!='}' ) return 0;
	*nxt = s;
	return 1;
}

static int
#ifdef __USE_PROTOS
match_rexpr(char *s, char **nxt)
#else
match_rexpr(s,nxt)
char *s;
char **nxt;
#endif
{
	if ( *s!='"' ) return 0;
	s++;
	while ( *s!='"' )
	{
		if ( *s=='\n' )
		warn("eoln found in regular expression");
		if ( *s=='\\' ) s++;
		s++;
	}
	*nxt = s+1;
	return 1;
}

/*
* Walk a string "{ A .. Z }" where A..Z is a space separated list
* of token references (either labels or reg exprs).  Return a
* string "inlineX_set" for some unique integer X.  Basically,
* we pretend as if we had seen "#tokclass inlineX { A .. Z }"
* on the input stream outside of an action.
*/
char *
#ifdef __USE_PROTOS
inline_set(char *s)
#else
inline_set(s)
char *s;
#endif
{
	char *nxt;
	fprintf(stderr, "found consumeUntil( {...} )\n");
	while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
	if ( *s!='{' )
	{
		err("malformed consumeUntil( {...} ); missing '{'");
		return "bad_set";
	}
	s++;
	while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
	while ( *s!='}' )
	{
		if ( match_token(s,&nxt) ) fprintf(stderr, "found token %s\n", s);
		else if ( match_rexpr(s,&nxt) ) fprintf(stderr, "found rexpr %s\n", s);
		else {
			err("invalid element in consumeUntil( {...} )");
			return "bad_set";
		}
		s = nxt;
		while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
	}
	return "inlineX_set";
}

/* ANTLR-specific syntax error message generator
* (define USER_ZZSYN when compiling so don't get 2 definitions)
*/
void
#ifdef __USE_PROTOS
zzsyn(char *text, int tok, char *egroup, SetWordType *eset, int etok,
int k, char *bad_text)
#else
zzsyn(text, tok, egroup, eset, etok, k, bad_text)
char *text, *egroup, *bad_text;
int tok;
int etok;
int k;
SetWordType *eset;
#endif
{
fprintf(stderr, ErrHdr, FileStr[CurFile]!=NULL?FileStr[CurFile]:"stdin", zzline);
fprintf(stderr, " syntax error at \"%s\"", (tok==zzEOF_TOKEN)?"EOF":text);
if ( !etok && !eset ) {fprintf(stderr, "\n"); return;}
if ( k==1 ) fprintf(stderr, " missing");
else
{
fprintf(stderr, "; \"%s\" not", bad_text);
if ( zzset_deg(eset)>1 ) fprintf(stderr, " in");
}
if ( zzset_deg(eset)>0 ) zzedecode(eset);
else fprintf(stderr, " %s", zztokens[etok]);
if ( strlen(egroup) > (size_t)0 ) fprintf(stderr, " in %s", egroup);
fprintf(stderr, "\n");
}
