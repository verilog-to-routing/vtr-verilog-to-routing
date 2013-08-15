/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-1994
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR6
 */
#include <stdio.h>
#define ANTLR_VERSION	133MR6

#include <ctype.h>
#include "dlg.h"
#ifdef MEMCHK
#include "trax.h"
#endif
#define zzSET_SIZE 8
#include "antlr.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"
#ifndef PURIFY
#define PURIFY(r,s)
#endif
ANTLR_INFO

int	action_no = 0;	   /* keep track of actions outputed */
int	nfa_allocated = 0; /* keeps track of number of nfa nodes */
nfa_node **nfa_array = NULL;/* root of binary tree that stores nfa array */
nfa_node nfa_model_node;   /* model to initialize new nodes */
set	used_chars;	   /* used to label trans. arcs */
set	used_classes;	   /* classes or chars used to label trans. arcs */
set	normal_chars;	   /* mask to get rid elements that aren't used
in set */
int	flag_paren = FALSE;
int	flag_brace = FALSE;
int	mode_counter = 0;  /* keep track of number of %%names */



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
	p_head(); p_class_hdr(); func_action = FALSE;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (setwd1[LA(1)]&0x1) ) {
			{
				zzBLOCK(zztasp3);
				zzMake0;
				{
				if ( (LA(1)==LEXACTION) ) {
					zzmatch(LEXACTION); zzCONSUME;
				}
				else {
					if ( (LA(1)==LEXMEMBER) ) {
						zzmatch(LEXMEMBER); zzCONSUME;
					}
					else {
						if ( (LA(1)==LEXPREFIX) ) {
							zzmatch(LEXPREFIX); zzCONSUME;
						}
						else {
							if ( (LA(1)==PARSERCLASS) ) {
								zzmatch(PARSERCLASS); zzCONSUME;
							}
						}
					}
				}
				zzEXIT(zztasp3);
				}
			}
			zzmatch(ACTION); zzCONSUME;
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	if ( gen_cpp ) p_includes();
	start_states();
	func_action = FALSE; p_tables(); p_tail();
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==ACTION) ) {
			zzmatch(ACTION); zzCONSUME;
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(1);
	if (firstLexMember != 0) p_class_def1();
 zzCONSUME;

	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x2);
	}
}

void
#ifdef __USE_PROTOS
start_states(void)
#else
start_states()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==PER_PER) ) {
			zzmatch(PER_PER); zzCONSUME;
			do_conversion();
		}
		else {
			if ( (LA(1)==NAME_PER_PER) ) {
				zzmatch(NAME_PER_PER); zzCONSUME;
				do_conversion();
				{
					zzBLOCK(zztasp3);
					zzMake0;
					{
					while ( (LA(1)==NAME_PER_PER) ) {
						zzmatch(NAME_PER_PER); zzCONSUME;
						do_conversion();
						zzLOOP(zztasp3);
					}
					zzEXIT(zztasp3);
					}
				}
			}
			else {zzFAIL(1,zzerr1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
		zzEXIT(zztasp2);
		}
	}
	zzmatch(PER_PER); zzCONSUME;
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x4);
	}
}

void
#ifdef __USE_PROTOS
do_conversion(void)
#else
do_conversion()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	new_automaton_mode(); func_action = TRUE;
	rule_list();
	
	dfa_class_nop[mode_counter] =
	relabel(zzaArg(zztasp1,1 ).l,comp_level);
	if (comp_level)
	p_shift_table(mode_counter);
	dfa_basep[mode_counter] = dfa_allocated+1;
	make_dfa_model_node(dfa_class_nop[mode_counter]);
	nfa_to_dfa(zzaArg(zztasp1,1 ).l);
	++mode_counter;
	func_action = FALSE;
#ifdef HASH_STAT
	fprint_hash_stats(stderr);
#endif
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x8);
	}
}

void
#ifdef __USE_PROTOS
rule_list(void)
#else
rule_list()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	if ( (setwd1[LA(1)]&0x10) ) {
		rule();
		zzaRet.l=zzaArg(zztasp1,1 ).l; zzaRet.r=zzaArg(zztasp1,1 ).r;
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			while ( (setwd1[LA(1)]&0x20) ) {
				rule();
				{nfa_node *t1;
					t1 = new_nfa_node();
					(t1)->trans[0]=zzaRet.l;
					(t1)->trans[1]=zzaArg(zztasp2,1 ).l;
					/* all accept nodes "dead ends" */
					zzaRet.l=t1; zzaRet.r=NULL;
				}
				zzLOOP(zztasp2);
			}
			zzEXIT(zztasp2);
			}
		}
	}
	else {
		if ( (setwd1[LA(1)]&0x40) ) {
			zzaRet.l = new_nfa_node(); zzaRet.r = NULL;
			warning("no regular expressions", zzline);
		}
		else {zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd1, 0x80);
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
	if ( (setwd2[LA(1)]&0x1) ) {
		reg_expr();
		zzmatch(ACTION);
		zzaRet.l=zzaArg(zztasp1,1 ).l; zzaRet.r=zzaArg(zztasp1,1 ).r; (zzaArg(zztasp1,1 ).r)->accept=action_no;
 zzCONSUME;

	}
	else {
		if ( (LA(1)==ACTION) ) {
			zzmatch(ACTION);
			zzaRet.l = NULL; zzaRet.r = NULL;
			error("no expression for action  ", zzline);
 zzCONSUME;

		}
		else {zzFAIL(1,zzerr3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x2);
	}
}

void
#ifdef __USE_PROTOS
reg_expr(void)
#else
reg_expr()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	and_expr();
	zzaRet.l=zzaArg(zztasp1,1 ).l; zzaRet.r=zzaArg(zztasp1,1 ).r;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (LA(1)==OR) ) {
			zzmatch(OR); zzCONSUME;
			and_expr();
			{nfa_node *t1, *t2;
				t1 = new_nfa_node(); t2 = new_nfa_node();
				(t1)->trans[0]=zzaRet.l;
				(t1)->trans[1]=zzaArg(zztasp2,2 ).l;
				(zzaRet.r)->trans[1]=t2;
				(zzaArg(zztasp2,2 ).r)->trans[1]=t2;
				zzaRet.l=t1; zzaRet.r=t2;
			}
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x4);
	}
}

void
#ifdef __USE_PROTOS
and_expr(void)
#else
and_expr()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	repeat_expr();
	zzaRet.l=zzaArg(zztasp1,1 ).l; zzaRet.r=zzaArg(zztasp1,1 ).r;
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (setwd2[LA(1)]&0x8) ) {
			repeat_expr();
			(zzaRet.r)->trans[1]=zzaArg(zztasp2,1 ).l; zzaRet.r=zzaArg(zztasp2,1 ).r;
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x10);
	}
}

void
#ifdef __USE_PROTOS
repeat_expr(void)
#else
repeat_expr()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	if ( (setwd2[LA(1)]&0x20) ) {
		expr();
		zzaRet.l=zzaArg(zztasp1,1 ).l; zzaRet.r=zzaArg(zztasp1,1 ).r;
		{
			zzBLOCK(zztasp2);
			zzMake0;
			{
			if ( (LA(1)==ZERO_MORE) ) {
				zzmatch(ZERO_MORE);
				{	nfa_node *t1,*t2;
					(zzaRet.r)->trans[0] = zzaRet.l;
					t1 = new_nfa_node(); t2 = new_nfa_node();
					t1->trans[0]=zzaRet.l;
					t1->trans[1]=t2;
					(zzaRet.r)->trans[1]=t2;
					zzaRet.l=t1;zzaRet.r=t2;
				}
 zzCONSUME;

			}
			else {
				if ( (LA(1)==ONE_MORE) ) {
					zzmatch(ONE_MORE);
					(zzaRet.r)->trans[0] = zzaRet.l;
 zzCONSUME;

				}
			}
			zzEXIT(zztasp2);
			}
		}
	}
	else {
		if ( (LA(1)==ZERO_MORE) ) {
			zzmatch(ZERO_MORE);
			error("no expression for *", zzline);
 zzCONSUME;

		}
		else {
			if ( (LA(1)==ONE_MORE) ) {
				zzmatch(ONE_MORE);
				error("no expression for +", zzline);
 zzCONSUME;

			}
			else {zzFAIL(1,zzerr4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd2, 0x40);
	}
}

void
#ifdef __USE_PROTOS
expr(void)
#else
expr()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	zzaRet.l = new_nfa_node(); zzaRet.r = new_nfa_node();
	if ( (LA(1)==L_BRACK) ) {
		zzmatch(L_BRACK); zzCONSUME;
		atom_list();
		zzmatch(R_BRACK);
		
		(zzaRet.l)->trans[0] = zzaRet.r;
		(zzaRet.l)->label = set_dup(zzaArg(zztasp1,2 ).label);
		set_orin(&used_chars,(zzaRet.l)->label);
 zzCONSUME;

	}
	else {
		if ( (LA(1)==NOT) ) {
			zzmatch(NOT); zzCONSUME;
			zzmatch(L_BRACK); zzCONSUME;
			atom_list();
			zzmatch(R_BRACK);
			
			(zzaRet.l)->trans[0] = zzaRet.r;
			(zzaRet.l)->label = set_dif(normal_chars,zzaArg(zztasp1,3 ).label);
			set_orin(&used_chars,(zzaRet.l)->label);
 zzCONSUME;

		}
		else {
			if ( (LA(1)==L_PAR) ) {
				zzmatch(L_PAR); zzCONSUME;
				reg_expr();
				zzmatch(R_PAR);
				
				(zzaRet.l)->trans[0] = zzaArg(zztasp1,2 ).l;
				(zzaArg(zztasp1,2 ).r)->trans[1] = zzaRet.r;
 zzCONSUME;

			}
			else {
				if ( (LA(1)==L_BRACE) ) {
					zzmatch(L_BRACE); zzCONSUME;
					reg_expr();
					zzmatch(R_BRACE);
					
					(zzaRet.l)->trans[0] = zzaArg(zztasp1,2 ).l;
					(zzaRet.l)->trans[1] = zzaRet.r;
					(zzaArg(zztasp1,2 ).r)->trans[1] = zzaRet.r;
 zzCONSUME;

				}
				else {
					if ( (setwd2[LA(1)]&0x80) ) {
						atom();
						
						(zzaRet.l)->trans[0] = zzaRet.r;
						(zzaRet.l)->label = set_dup(zzaArg(zztasp1,1 ).label);
						set_orin(&used_chars,(zzaRet.l)->label);
					}
					else {zzFAIL(1,zzerr5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
				}
			}
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x1);
	}
}

void
#ifdef __USE_PROTOS
atom_list(void)
#else
atom_list()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	set_free(zzaRet.label);
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		while ( (setwd3[LA(1)]&0x2) ) {
			near_atom();
			set_orin(&(zzaRet.label),zzaArg(zztasp2,1 ).label);
			zzLOOP(zztasp2);
		}
		zzEXIT(zztasp2);
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x4);
	}
}

void
#ifdef __USE_PROTOS
near_atom(void)
#else
near_atom()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	register int i;
	register int i_prime;
	anychar();
	zzaRet.letter=zzaArg(zztasp1,1 ).letter; zzaRet.label=set_of(zzaArg(zztasp1,1 ).letter);
	i_prime = zzaArg(zztasp1,1 ).letter + MIN_CHAR;
	if (case_insensitive && islower(i_prime))
	set_orel(toupper(i_prime)-MIN_CHAR,
	&(zzaRet.label));
	if (case_insensitive && isupper(i_prime))
	set_orel(tolower(i_prime)-MIN_CHAR,
	&(zzaRet.label));
	{
		zzBLOCK(zztasp2);
		zzMake0;
		{
		if ( (LA(1)==RANGE) ) {
			zzmatch(RANGE); zzCONSUME;
			anychar();
			if (case_insensitive){
				i_prime = zzaRet.letter+MIN_CHAR;
				zzaRet.letter = (islower(i_prime) ?
				toupper(i_prime) : i_prime)-MIN_CHAR;
				i_prime = zzaArg(zztasp2,2 ).letter+MIN_CHAR;
				zzaArg(zztasp2,2 ).letter = (islower(i_prime) ?
				toupper(i_prime) : i_prime)-MIN_CHAR;
			}
			/* check to see if range okay */
			if (zzaRet.letter > zzaArg(zztasp2,2 ).letter){
				error("invalid range  ", zzline);
			}
			for (i=zzaRet.letter; i<= (int)zzaArg(zztasp2,2 ).letter; ++i){
				set_orel(i,&(zzaRet.label));
				i_prime = i+MIN_CHAR;
				if (case_insensitive && islower(i_prime))
				set_orel(toupper(i_prime)-MIN_CHAR,
				&(zzaRet.label));
				if (case_insensitive && isupper(i_prime))
				set_orel(tolower(i_prime)-MIN_CHAR,
				&(zzaRet.label));
			}
		}
		zzEXIT(zztasp2);
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x8);
	}
}

void
#ifdef __USE_PROTOS
atom(void)
#else
atom()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	register int i_prime;
	anychar();
	zzaRet.label = set_of(zzaArg(zztasp1,1 ).letter);
	i_prime = zzaArg(zztasp1,1 ).letter + MIN_CHAR;
	if (case_insensitive && islower(i_prime))
	set_orel(toupper(i_prime)-MIN_CHAR,
	&(zzaRet.label));
	if (case_insensitive && isupper(i_prime))
	set_orel(tolower(i_prime)-MIN_CHAR,
	&(zzaRet.label));
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x10);
	}
}

void
#ifdef __USE_PROTOS
anychar(void)
#else
anychar()
#endif
{
	zzRULE;
	zzBLOCK(zztasp1);
	zzMake0;
	{
	if ( (LA(1)==REGCHAR) ) {
		zzmatch(REGCHAR);
		zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

	}
	else {
		if ( (LA(1)==OCTAL_VALUE) ) {
			zzmatch(OCTAL_VALUE);
			zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

		}
		else {
			if ( (LA(1)==HEX_VALUE) ) {
				zzmatch(HEX_VALUE);
				zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

			}
			else {
				if ( (LA(1)==DEC_VALUE) ) {
					zzmatch(DEC_VALUE);
					zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

				}
				else {
					if ( (LA(1)==TAB) ) {
						zzmatch(TAB);
						zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

					}
					else {
						if ( (LA(1)==NL) ) {
							zzmatch(NL);
							zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

						}
						else {
							if ( (LA(1)==CR) ) {
								zzmatch(CR);
								zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

							}
							else {
								if ( (LA(1)==BS) ) {
									zzmatch(BS);
									zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

								}
								else {
									if ( (LA(1)==LIT) ) {
										zzmatch(LIT);
										zzaRet.letter = zzaArg(zztasp1,1 ).letter - MIN_CHAR;
 zzCONSUME;

									}
									else {
										if ( (LA(1)==L_EOF) ) {
											zzmatch(L_EOF);
											zzaRet.letter = 0;
 zzCONSUME;

										}
										else {zzFAIL(1,zzerr6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	zzEXIT(zztasp1);
	return;
fail:
	zzEXIT(zztasp1);
	/* empty action */
	zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
	zzresynch(setwd3, 0x20);
	}
}

/* adds a new nfa to the binary tree and returns a pointer to it */
nfa_node *new_nfa_node()
{
	register nfa_node *t;
	static int nfa_size=0;	/* elements nfa_array[] can hold */
	
	++nfa_allocated;
	if (nfa_size<=nfa_allocated){
		/* need to redo array */
		if (!nfa_array){
			/* need some to do inital allocation */
			nfa_size=nfa_allocated+NFA_MIN;
			nfa_array=(nfa_node **) malloc(sizeof(nfa_node*)*
			nfa_size);
		}else{
			/* need more space */
			nfa_size=2*(nfa_allocated+1);
			nfa_array=(nfa_node **) realloc(nfa_array,
			sizeof(nfa_node*)*nfa_size);
		}
	}
	/* fill out entry in array */
	t = (nfa_node*) malloc(sizeof(nfa_node));
	nfa_array[nfa_allocated] = t;
	*t = nfa_model_node;
	t->node_no = nfa_allocated;
	return t;
}


/* initialize the model node used to fill in newly made nfa_nodes */
void
make_nfa_model_node()
{
	nfa_model_node.node_no = -1; /* impossible value for real nfa node */
	nfa_model_node.nfa_set = 0;
	nfa_model_node.accept = 0;   /* error state default*/
	nfa_model_node.trans[0] = NULL;
	nfa_model_node.trans[1] = NULL;
	nfa_model_node.label = empty;
}

#ifdef DEBUG

/* print out the pointer value and the node_number */
fprint_dfa_pair(f, p)
FILE *f;
nfa_node *p;
{
	if (p){
		fprintf(f, "%x (%d)", p, p->node_no);
	}else{
		fprintf(f, "(nil)");
	}
}

/* print out interest information on a set */
fprint_set(f,s)
FILE *f;
set s;
{
	unsigned int *x;
	
	fprintf(f, "n = %d,", s.n);
	if (s.setword){
		fprintf(f, "setword = %x,   ", s.setword);
		/* print out all the elements in the set */
		x = set_pdq(s);
		while (*x!=nil){
			fprintf(f, "%d ", *x);
			++x;
		}
	}else{
		fprintf(f, "setword = (nil)");
	}
}

/* code to be able to dump out the nfas
return 0 if okay dump
return 1 if screwed up
*/
int dump_nfas(first_node, last_node)
int first_node;
int last_node;
{
	register int i;
	nfa_node *t;
	
	for (i=first_node; i<=last_node; ++i){
		t = NFA(i);
		if (!t) break;
		fprintf(stderr, "nfa_node %d {\n", t->node_no);
		fprintf(stderr, "\n\tnfa_set = %d\n", t->nfa_set);
		fprintf(stderr, "\taccept\t=\t%d\n", t->accept);
		fprintf(stderr, "\ttrans\t=\t(");
		fprint_dfa_pair(stderr, t->trans[0]);
		fprintf(stderr, ",");
		fprint_dfa_pair(stderr, t->trans[1]);
		fprintf(stderr, ")\n");
		fprintf(stderr, "\tlabel\t=\t{ ");
		fprint_set(stderr, t->label);
		fprintf(stderr, "\t}\n");
		fprintf(stderr, "}\n\n");
	}
	return 0;
}
#endif

/* DLG-specific syntax error message generator
* (define USER_ZZSYN when compiling so don't get 2 definitions)
*/
void
#ifdef __USE_PROTOS
zzsyn(char *text, int tok, char *egroup, SetWordType *eset, int etok, int k, char *bad_text)
#else
zzsyn(text, tok, egroup, eset, etok, k, bad_text)
char *text, *egroup, *bad_text;
int tok;
int etok;
int k;
SetWordType *eset;
#endif
{
fprintf(stderr, ErrHdr, file_str[0]!=NULL?file_str[0]:"stdin", zzline);
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
