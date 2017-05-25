/*
* Copyright (C) 2002-2015 XimpleWare, info@ximpleware.com
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
/*VTD-XML is protected by US patent 7133857, 7260652, an 7761459*/
#include "xpath.h"
#include "textIter.h"
static UCSChar *axisName(axisType i);

static  inline Boolean isUnique_lpe(locationPathExpr *lpe, int i);
static 	int computeContextSize4Ancestor(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Ancestor2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4AncestorOrSelf(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4AncestorOrSelf2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Child(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Child2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4DDFP(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4DDFP2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4FollowingSibling(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4FollowingSibling2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Parent(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Parent2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4PrecedingSibling(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4PrecedingSibling2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Self(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static 	int computeContextSize4Self2(locationPathExpr *lpe,Predicate *p, VTDNav *vn);
static  int computeContextSize(locationPathExpr *lpe, Predicate *p, VTDNav *vn);
static  int process_ancestor_or_self(locationPathExpr *lpe, VTDNav *vn);
static	int process_ancestor_or_self2(locationPathExpr *lpe, VTDNav *vn);
static  int process_ancestor(locationPathExpr *lpe, VTDNav *vn);
static  int process_ancestor2(locationPathExpr *lpe, VTDNav *vn);
static  int process_attribute(locationPathExpr *lpe, VTDNav *vn);
static  int process_child(locationPathExpr *lpe, VTDNav *vn);
static  int process_child2(locationPathExpr *lpe, VTDNav *vn);

static  int process_DDFP(locationPathExpr *lpe, VTDNav *vn);
static  int process_DDFP2(locationPathExpr *lpe, VTDNav *vn);

static  int process_following_sibling(locationPathExpr *lpe, VTDNav *vn);
static  int process_following_sibling2(locationPathExpr *lpe, VTDNav *vn);

static  int process_parent(locationPathExpr *lpe, VTDNav *vn);
static  int process_parent2(locationPathExpr *lpe, VTDNav *vn);

static  int process_preceding_sibling(locationPathExpr *lpe, VTDNav *vn);
static  int process_preceding_sibling2(locationPathExpr *lpe, VTDNav *vn);

static  int process_self(locationPathExpr *lpe, VTDNav *vn);
static  int process_self2(locationPathExpr *lpe, VTDNav *vn);

static  int process_namespace(locationPathExpr *lpe, VTDNav *vn);
static  void selectNodeType(locationPathExpr *lpe, TextIter *ti);
static  void transition_child(locationPathExpr *lpe, VTDNav *vn);
static  void transition_DDFP(locationPathExpr *lpe, VTDNav *vn);

static void selectNodeType(locationPathExpr *lpe, TextIter *ti){
		if (lpe->currentStep->nt->testType == NT_TEXT )
			selectText(ti);
		else if (lpe->currentStep->nt->testType == NT_COMMENT )
			selectComment(ti);
		else if (lpe->currentStep->nt->testType == NT_PI0 )
			selectPI0(ti);
		else {
			selectPI1(ti,lpe->currentStep->nt->nodeName);
		}
		
	}


UCSChar *axisName(axisType i){
	switch(i){
			case AXIS_CHILD0: return (UCSChar *)L"child::";
			case AXIS_CHILD: return (UCSChar *)L"child::";
			case AXIS_DESCENDANT_OR_SELF0: return (UCSChar *)L"descendant-or-self::";
			case AXIS_DESCENDANT0: return (UCSChar *)L"descendant::";
			case AXIS_PRECEDING0: return (UCSChar *)L"preceding::";
			case AXIS_FOLLOWING0: return (UCSChar *)L"following::";
			case AXIS_DESCENDANT_OR_SELF: return (UCSChar *)L"descendant-or-self::";
			case AXIS_DESCENDANT: return (UCSChar *)L"descendant::";
			case AXIS_PRECEDING: return (UCSChar *)L"preceding::";
			case AXIS_FOLLOWING: return (UCSChar *)L"following::";
			case AXIS_PARENT: return (UCSChar *)L"parent::";
			case AXIS_ANCESTOR: return (UCSChar *)L"ancestor::";
			case AXIS_ANCESTOR_OR_SELF: return (UCSChar *)L"ancestor-or-self::";
			case AXIS_SELF: return (UCSChar *)L"self::";
			case AXIS_FOLLOWING_SIBLING: return (UCSChar *)L"following-sibling::";
			case AXIS_FOLLOWING_SIBLING0: return (UCSChar *)L"following-sibling::";
			case AXIS_PRECEDING_SIBLING: return (UCSChar *)L"preceding-sibling::";
			case AXIS_PRECEDING_SIBLING0: return (UCSChar *)L"preceding-sibling::";			
			case AXIS_ATTRIBUTE: return (UCSChar *)L"attribute::";
			default: return (UCSChar *)L"namespace::";	
	}
}


Boolean isUnique_lpe(locationPathExpr *lpe, int i){
	return isUniqueIntHash(lpe->ih,i);
}

int computeContextSize(locationPathExpr *lpe, Predicate *p, VTDNav *vn){
	Boolean b = FALSE;
	//Predicate *tp = NULL;
    int i = 0;
    AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
	UCSChar *helper = NULL;
	switch(lpe->currentStep->axis_type){
    	case AXIS_CHILD0:
			return computeContextSize4Child(lpe,p,vn);
		case AXIS_CHILD:
			return computeContextSize4Child2(lpe,p,vn);
		case AXIS_DESCENDANT_OR_SELF0:
		case AXIS_DESCENDANT0:
		case AXIS_PRECEDING0:
		case AXIS_FOLLOWING0:
			return computeContextSize4DDFP(lpe,p,vn);
		case AXIS_DESCENDANT_OR_SELF:
		case AXIS_DESCENDANT:
		case AXIS_PRECEDING:
		case AXIS_FOLLOWING:
			return computeContextSize4DDFP2(lpe,p,vn);

		case AXIS_PARENT:
			return computeContextSize4Parent2(lpe,p,vn);

		case AXIS_ANCESTOR:
			return computeContextSize4Ancestor2(lpe,p,vn);		

		case AXIS_ANCESTOR_OR_SELF:
			return computeContextSize4AncestorOrSelf2(lpe,p,vn);	

		case AXIS_SELF:
			return computeContextSize4Self2(lpe,p,vn);	

		case AXIS_FOLLOWING_SIBLING:
			return computeContextSize4FollowingSibling2(lpe,p,vn);
		case AXIS_FOLLOWING_SIBLING0:
			return computeContextSize4FollowingSibling(lpe,p,vn);
		case AXIS_PRECEDING_SIBLING:
			return computeContextSize4PrecedingSibling2(lpe,p,vn);
		case AXIS_PRECEDING_SIBLING0:
			return computeContextSize4PrecedingSibling(lpe,p,vn);

		case AXIS_ATTRIBUTE:
			if (ap==NULL)
					ap =  createAutoPilot(vn);
				else
					bind(ap,vn);
			//ap = createAutoPilot(vn);
			if (lpe->currentStep->nt->testType == NT_NODE)
				selectAttr(ap,(UCSChar *)L"*");
			else if (lpe->currentStep->nt->localName!=NULL)
				selectAttrNS(ap,lpe->currentStep->nt->URL,
				lpe->currentStep->nt->localName);
			else
				selectAttr(ap,lpe->currentStep->nt->nodeName);
			i = 0;
			while(iterateAttr2(ap)!=-1){
				if (evalPredicates2(lpe->currentStep, vn, p)){
					i++;
				}
			}
			resetP2_s(lpe->currentStep,vn,p);
			lpe->currentStep->o= ap;
			//freeAutoPilot(ap);
			return i;

		case AXIS_NAMESPACE:
			if (ap==NULL)
					ap = createAutoPilot(vn);
				else
					bind(ap,vn);
			if (lpe->currentStep->nt->testType == NT_NODE)
				selectNameSpace(ap,(UCSChar *)L"*");
			else
				selectNameSpace(ap,lpe->currentStep->nt->nodeName);
			i = 0;
			while(iterateNameSpace(ap)!=-1){
				if (evalPredicates2(lpe->currentStep, vn, p)){
					i++;
				}
			}
			resetP2_s(lpe->currentStep,vn,p);
			lpe->currentStep->o= ap;
			//freeAutoPilot(ap);
			return i;

		default:
			throwException2( xpath_eval_exception,"unknown state");
			return 0;
	}
}

static int process_ancestor_or_self(locationPathExpr *lpe, VTDNav *vn){
	Boolean b = FALSE, b1= FALSE;
	int result;
	//int contextSize;
	Predicate *t= NULL;

	switch ( lpe->state) {
		case  XPATH_EVAL_START:
			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe, t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				lpe->state = XPATH_EVAL_END;
				break;
			}

			lpe->state =  XPATH_EVAL_END;
			push2(vn);

			if (get_ft(lpe->currentStep)== TRUE){
				set_ft(lpe->currentStep,FALSE);
				if (eval_s(lpe->currentStep, vn)) {
					if (getNextStep(lpe->currentStep) != NULL) {
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
						break;
					} else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						if (vn->atTerminal)
							result = vn->LN;
						else
							result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
			}

			while (toElement(vn,PARENT)) {
				if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					if (lpe->currentStep->nextS != NULL) {
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
						break;
					} else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
			}

			if ( lpe->state ==  XPATH_EVAL_END) {
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep, vn);
				pop2(vn);
			}

			break;

		case  XPATH_EVAL_FORWARD:
			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe, t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				lpe->currentStep = lpe->currentStep->prevS;
				lpe->state = XPATH_EVAL_BACKWARD;
				break;
			}

			lpe->state =  XPATH_EVAL_BACKWARD;
			push2(vn);
			if (lpe->currentStep->ft) {
				lpe->currentStep->ft= FALSE;
				if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					if (lpe->currentStep->nextS != NULL) {
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
						break;
					} else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						if (vn->atTerminal)
							result = vn->LN;
						else
							result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
			}
			while (toElement(vn,PARENT)) {
				if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					if (lpe->currentStep->nextS != NULL) {
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
						break;
					} else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
			}

			if ( lpe->state ==  XPATH_EVAL_BACKWARD) {
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				lpe->currentStep->ft = TRUE;
				pop2(vn);
				lpe->currentStep = lpe->currentStep->prevS;
			}
			break;

		case  XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
	    	return -1;


		case  XPATH_EVAL_BACKWARD:
			b = FALSE;
			push2(vn);

			while (toElement(vn, PARENT)) {
				if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					if (lpe->currentStep->nextS != NULL) {
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
						b = TRUE;
						break;
					} else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
			}
			if (b == FALSE) {
				pop2(vn);
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				if (lpe->currentStep->prevS != NULL) {
					lpe->currentStep->ft = TRUE;
					lpe->state =  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				} else {
					lpe->state =  XPATH_EVAL_END;
				}
			}
			break;

		case  XPATH_EVAL_TERMINAL:
			while (toElement(vn, PARENT)) {
				if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					result = getCurrentIndex(vn);
					if ( isUnique_lpe(lpe, result))
						return result;
				}
			}
			pop2(vn);
			if (lpe->currentStep->hasPredicate)
				resetP_s(lpe->currentStep,vn);
			if (lpe->currentStep->prevS != NULL) {
				lpe->currentStep->ft = TRUE;
				lpe->state =  XPATH_EVAL_BACKWARD;
				lpe->currentStep = lpe->currentStep->prevS;
			}
			else {
				 lpe->state =  XPATH_EVAL_END;
			}
			break;


		default:
			throwException2(xpath_eval_exception,
					"unknown state");
		}
		return -2;
}
static int process_ancestor(locationPathExpr *lpe, VTDNav *vn){
	int result;
	Boolean b = FALSE, b1 = FALSE;
	//int contextSize;
	Predicate *t= NULL;

	switch(lpe->state){
			case XPATH_EVAL_START:
				t = lpe->currentStep->p;
				while (t != NULL) {
					if (t->requireContext) {
						int i = computeContextSize(lpe, t, vn);
						if (i == 0) {
							b1 = TRUE;
							break;
						} else
							setContextSize_p(t,i);
					}
					t = t->nextP;
				}
				if (b1) {
					lpe->state = XPATH_EVAL_END;
					break;
				}

				lpe->state = XPATH_EVAL_END;
				if (getCurrentDepth(vn) != -1) {
					push2(vn);

					while (toElement(vn,PARENT)) {
						if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
							if (lpe->currentStep->nextS != NULL) {
								lpe->state = XPATH_EVAL_FORWARD;
								lpe->currentStep = lpe->currentStep->nextS;
								break;
							} else {
								//vn.pop();
								lpe->state = XPATH_EVAL_TERMINAL;
								result = getCurrentIndex(vn);
								if (isUnique_lpe(lpe,result))
									return result;
							}
						}
					}
					if (lpe->state == XPATH_EVAL_END) {
						if (lpe->currentStep->hasPredicate)
							resetP_s(lpe->currentStep,vn);
						pop2(vn);
					}
				}
				break;

			case XPATH_EVAL_END:
				lpe->currentStep =NULL;
				// reset();
				return -1;

			case XPATH_EVAL_FORWARD:
				t = lpe->currentStep->p;
				while(t!=NULL){
					if (t->requireContext){
						int i = computeContextSize(lpe,t,vn);
						if (i==0){
							b1 = TRUE;
							break;
						}else
							setContextSize_p(t,i);
					}
					t = t->nextP;
				}
				if (b1){
					lpe->currentStep = lpe->currentStep->prevS;
					lpe->state = XPATH_EVAL_BACKWARD;
					break;
				}
				lpe->state =  XPATH_EVAL_BACKWARD;
				push2(vn);

				while(toElement(vn,P)){
					if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
						if (lpe->currentStep->nextS != NULL){
							lpe->state =  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
							break;
						}
						else {
							//vn.pop();
							lpe->state =  XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}
				}
				if ( lpe->state== XPATH_EVAL_BACKWARD){
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					pop2(vn);
					lpe->currentStep=lpe->currentStep->prevS;
				}
				break;

			case XPATH_EVAL_BACKWARD:
				b = FALSE;
				push2(vn);

				while (toElement(vn,PARENT)) {
					if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS!= NULL) {
							lpe->state =  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
							b = TRUE;
							break;
						} else {
							//vn.pop();
							lpe->state =  XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}
				}
				if (b==FALSE){
					pop2(vn);
					if (lpe->currentStep->prevS!=NULL) {
						if (lpe->currentStep->hasPredicate)
							resetP_s(lpe->currentStep,vn);
						lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
					else {
						lpe->state =  XPATH_EVAL_END;
					}
				}
				break;

			case XPATH_EVAL_TERMINAL:
				while (toElement(vn,PARENT)) {
					if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
				pop2(vn);

				if (lpe->currentStep->prevS!=NULL) {
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					lpe->state =  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				}
				else {
					lpe->state =  XPATH_EVAL_END;
				}
				break;

			default:
				throwException2(xpath_eval_exception,
					"unknown state");
	}
	return -2;

}
static int process_attribute(locationPathExpr *lpe, VTDNav *vn){
	AutoPilot *ap = NULL;
	Boolean b1 = FALSE;
	Predicate *t= NULL;
	int temp;
	switch(lpe->state){
		case  XPATH_EVAL_START:
		case  XPATH_EVAL_FORWARD:

			t = lpe->currentStep->p;
			while(t!=NULL){
				if (t->requireContext){
					int i = computeContextSize(lpe,t,vn);
					if (i==0){
						b1 = TRUE;
						break;
					}else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1){
				if (lpe->state == XPATH_EVAL_FORWARD){
					lpe->state= XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				}else
					lpe->state= XPATH_EVAL_END;
				break;
			}

			if (vn->atTerminal){
				if (lpe->state ==XPATH_EVAL_START)
					lpe->state = XPATH_EVAL_END;
				else {
					lpe->state = XPATH_EVAL_BACKWARD;
					lpe->currentStep  = lpe->currentStep->prevS;
				}
			} else {
				if (lpe->currentStep->ft == TRUE) {
					if (lpe->currentStep->o == NULL)
						lpe->currentStep->o = ap = createAutoPilot(vn);
					else {
						ap = lpe->currentStep->o;
						bind(ap,vn);
					}
					if (lpe->currentStep->nt->testType== NT_NODE)
						selectAttr(ap,L"*");
					else if (lpe->currentStep->nt->localName != NULL)
						selectAttrNS(ap,lpe->currentStep->nt->URL,
                                lpe->currentStep->nt->localName);
					else 
						selectAttr(ap,lpe->currentStep->nt->nodeName);
					lpe->currentStep->ft = FALSE;
				}
				if ( lpe->state==  XPATH_EVAL_START)
					lpe->state=  XPATH_EVAL_END;
				vn->atTerminal=TRUE;
				while( (temp = iterateAttr2(ap)) != -1){
					if (!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn)){
						break;
					}
				}
				if (temp == -1){
					lpe->currentStep->ft = TRUE;
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					vn->atTerminal=(FALSE);
					if ( lpe->state==  XPATH_EVAL_FORWARD){
						lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
				}else {

					if (lpe->currentStep->nextS != NULL){
						vn->LN = temp;
						lpe->state=  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
					}
					else {
						//vn.pop();
						lpe->state=  XPATH_EVAL_TERMINAL;
						if ( isUnique_lpe(lpe,temp)){
							vn->LN = temp;
							return temp;
						}
					}

				}
			}
			break;

		case  XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
			return -1;

		case  XPATH_EVAL_BACKWARD:
			ap = lpe->currentStep->o;
			//vn.push();
			while( (temp = iterateAttr2(ap)) != -1){
				if (!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn)){
					break;
				}
			}
			if (temp == -1) {
				lpe->currentStep->ft = TRUE;
				//freeAutoPilot(lpe->currentStep->o);
				//lpe->currentStep->o = NULL;
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				vn->atTerminal=(FALSE);
				if (lpe->currentStep->prevS != NULL) {
					lpe->state =  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				} else
					lpe->state =  XPATH_EVAL_END;
			} else {
				if (lpe->currentStep->nextS != NULL) {
					lpe->state =  XPATH_EVAL_FORWARD;
					lpe->currentStep = lpe->currentStep->nextS;
				} else {
					lpe->state =  XPATH_EVAL_TERMINAL;
					if ( isUnique_lpe(lpe,temp)){
						vn->LN = temp;
						return temp;
					}
				}
			}
			break;

		case  XPATH_EVAL_TERMINAL:
			ap = lpe->currentStep->o;
			while( (temp = iterateAttr2(ap)) != -1){
				if ((!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn))){
					break;
				}
			}
			if (temp != -1)
				if (isUnique_lpe(lpe,temp)){
					vn->LN = temp;
					return temp;
				}
				vn->atTerminal=(FALSE);
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				lpe->currentStep->ft = TRUE;
				if (lpe->currentStep->prevS == NULL) {
					//lpe->currentStep->ft = TRUE;
					//freeAutoPilot(lpe->currentStep->o);
					//lpe->currentStep->o = NULL;
					lpe->state=  XPATH_EVAL_END;
				} else {
					lpe->state=  XPATH_EVAL_BACKWARD;
					//lpe->currentStep->ft = TRUE;
					//freeAutoPilot(lpe->currentStep->o);
					//lpe->currentStep->o = NULL;
					lpe->currentStep = lpe->currentStep->prevS;
				}

				break;

		default:
			throwException2(xpath_eval_exception,
				"unknown state");
	}
	return -2;
}


static int process_namespace(locationPathExpr *lpe, VTDNav *vn){
	AutoPilot *ap = NULL;
	Boolean b1 = FALSE;
	Predicate *t= NULL;
	int temp;
	switch(lpe->state){
		case  XPATH_EVAL_START:
		case  XPATH_EVAL_FORWARD:

			t = lpe->currentStep->p;
			while(t!=NULL){
				if (t->requireContext){
					int i = computeContextSize(lpe,t,vn);
					if (i==0){
						b1 = TRUE;
						break;
					}else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1){
				if (lpe->state == XPATH_EVAL_FORWARD){
					lpe->state= XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				}else
					lpe->state= XPATH_EVAL_END;
				break;
			}

			if (vn->atTerminal){
				if (lpe->state ==XPATH_EVAL_START)
					lpe->state = XPATH_EVAL_END;
				else {
					lpe->state = XPATH_EVAL_BACKWARD;
					lpe->currentStep  = lpe->currentStep->prevS;
				}
			} else {
				if (lpe->currentStep->ft == TRUE) {
					if (lpe->currentStep->o == NULL)
						lpe->currentStep->o = ap = createAutoPilot(vn);
					else {
						ap = lpe->currentStep->o;
						bind(ap,vn);
					}
					if (lpe->currentStep->nt->testType== NT_NODE)
						selectNameSpace(ap,L"*");
					else 
						selectNameSpace(ap,lpe->currentStep->nt->nodeName);
					lpe->currentStep->ft = FALSE;
				}
				if ( lpe->state==  XPATH_EVAL_START)
					lpe->state=  XPATH_EVAL_END;
				push2(vn);
				//setAtTerminal(vn,TRUE);
				while( (temp = iterateNameSpace(ap)) != -1){
					if ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn)){
						break;
					}
				}
				if (temp == -1){
					lpe->currentStep->ft = TRUE;
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					vn->atTerminal=(FALSE);
					if ( lpe->state==  XPATH_EVAL_FORWARD){
						lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
				}else {
					vn->atTerminal = TRUE;
					if (lpe->currentStep->nextS != NULL){
						vn->LN = temp;
						lpe->state=  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
					}
					else {
						//vn.pop();
						lpe->state=  XPATH_EVAL_TERMINAL;
						if ( isUnique_lpe(lpe,temp)){
							vn->LN = temp;
							return temp;
						}
					}

				}
			}
			break;

		case  XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
			return -1;

		case  XPATH_EVAL_BACKWARD:
			ap = lpe->currentStep->o;
			//vn.push();
			while( (temp = iterateNameSpace(ap)) != -1){
				if ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn)){
					break;
				}
			}
			if (temp == -1) {
				pop2(vn);
				lpe->currentStep->ft = TRUE;
				//freeAutoPilot(lpe->currentStep->o);
				//lpe->currentStep->o = NULL;
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				vn->atTerminal=(FALSE);
				if (lpe->currentStep->prevS != NULL) {
					lpe->state =  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				} else
					lpe->state =  XPATH_EVAL_END;
			} else {
				if (lpe->currentStep->nextS != NULL) {
					lpe->state =  XPATH_EVAL_FORWARD;
					lpe->currentStep = lpe->currentStep->nextS;
				} else {
					lpe->state =  XPATH_EVAL_TERMINAL;
					if ( isUnique_lpe(lpe,temp)){
						vn->LN = temp;
						return temp;
					}
				}
			}
			break;

		case  XPATH_EVAL_TERMINAL:
			ap = lpe->currentStep->o;
			while( (temp = iterateNameSpace(ap)) != -1){
				if ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn)){
					break;
				}
			}
			if (temp != -1)
				if (isUnique_lpe(lpe,temp)){
					vn->LN = temp;
					return temp;
				}
				vn->atTerminal=(FALSE);
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				if (lpe->currentStep->prevS == NULL) {
					lpe->currentStep->ft = TRUE;
					//freeAutoPilot(lpe->currentStep->o);
					//lpe->currentStep->o = NULL;
					pop2(vn);
					lpe->state=  XPATH_EVAL_END;
				} else {
					lpe->state=  XPATH_EVAL_BACKWARD;
					pop2(vn);
					lpe->currentStep->ft = TRUE;
					//freeAutoPilot(lpe->currentStep->o);
					//lpe->currentStep->o = NULL;
					lpe->currentStep = lpe->currentStep->prevS;
				}

				break;

		default:
			throwException2(xpath_eval_exception,
				"unknown state");
	}
	return -2;
}
static int process_child(locationPathExpr *lpe, VTDNav *vn){
	int result;
	Boolean b = FALSE, b1 = FALSE;
	Predicate *t= NULL;

	switch(lpe->state){
				case XPATH_EVAL_START:
					//if (lpe->currentStep->nt->testType < NT_TEXT){
						/* first search for any predicate that
						// requires contextSize
						// if so, compute its context size
						// if size > 0
						// set context
						// if size ==0
						// immediately set the state to backward or end*/
						t = lpe->currentStep->p;
						while(t!=NULL){
							if (t->requireContext){
								int i = computeContextSize(lpe, t,vn);
								if (i==0){
									b1 = TRUE;
									break;
								}else
									setContextSize_p(t,i);
							}
							t = t->nextP;
						}
						if (b1){
							lpe->state= XPATH_EVAL_END;
							break;
						}

						b=toElement(vn,FIRST_CHILD);
						lpe->state=  XPATH_EVAL_END;
						if (b == TRUE){
						 do {
							 if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
								 if (lpe->currentStep->nextS != NULL){
									 //currentStep.position++;
									 lpe->state=  XPATH_EVAL_FORWARD;
									 lpe->currentStep = lpe->currentStep->nextS;
								 }
								 else {
									 lpe->state=  XPATH_EVAL_TERMINAL;
									 result = getCurrentIndex(vn);
									 if ( isUnique_lpe(lpe,result)){
										 return result;
									 }
								 }
								 break;
							 }
						 } while (toElement(vn,NS));
						 if (lpe->state == XPATH_EVAL_END)
							 toElement(vn,PARENT);
					 }
					
					break;
				case XPATH_EVAL_END:
					lpe->currentStep =NULL;
					// reset();
					return -1;

				case XPATH_EVAL_FORWARD:
					//if (lpe->currentStep->nt->testType < NT_TEXT){
						t = lpe->currentStep->p;
						while(t!=NULL){
							if (t->requireContext){
								int i = computeContextSize(lpe, t,vn);
								if (i==0){
									b1 = TRUE;
									break;
								}else
									setContextSize_p(t,i);
							}
							t = t->nextP;
						}
						if (b1){
							lpe->currentStep = lpe->currentStep->prevS;
							lpe->state= XPATH_EVAL_BACKWARD;
							break;
						}

						lpe->state =  XPATH_EVAL_BACKWARD;
						if (toElement(vn,FIRST_CHILD)) {
							do {
								if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
									if (lpe->currentStep->nextS != NULL) {
										lpe->state=  XPATH_EVAL_FORWARD;
										lpe->currentStep = lpe->currentStep->nextS;
									} else {
										lpe->state=  XPATH_EVAL_TERMINAL;
										result = getCurrentIndex(vn);
										if ( isUnique_lpe(lpe,result))
											return result;
									}
									goto forward;
								}
							} while (toElement(vn,NEXT_SIBLING));
							toElement(vn,PARENT);
							if (lpe->currentStep->hasPredicate)
								resetP_s(lpe->currentStep,vn);
							lpe->currentStep = lpe->currentStep->prevS;
						} else {
							//vn.toElement(VTDNav.P);
							lpe->currentStep = lpe->currentStep->prevS;
						}
forward:;

					break;

				case XPATH_EVAL_BACKWARD:
					//if (lpe->currentStep->nt->testType < NT_TEXT) {
						if(lpe->currentStep->out_of_range){
							lpe->currentStep->out_of_range = FALSE;
							if (lpe->currentStep->hasPredicate)
								resetP_s(lpe->currentStep,vn);
							transition_child(lpe,vn);
							break;
						}
						b = FALSE;
						while (toElement(vn,NEXT_SIBLING)) {
							if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
								b = TRUE;
								break;
							}
						}
						if (b) {
							lpe->state=  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
						}else {
							if (lpe->currentStep->hasPredicate)
								resetP_s(lpe->currentStep,vn);
							transition_child(lpe,vn);
						}
					
					break;

				case XPATH_EVAL_TERMINAL:
					if(lpe->currentStep->out_of_range){
							lpe->currentStep->out_of_range = FALSE;
							if (lpe->currentStep->hasPredicate)
								resetP_s(lpe->currentStep,vn);
							transition_child(lpe,vn);
							break;
						}
					//if (lpe->currentStep->nt->testType < NT_TEXT) {
						while (toElement(vn,NEXT_SIBLING)) {
							if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
								result = getCurrentIndex(vn);
								if ( isUnique_lpe(lpe,result))
									return result;
							}
						}
						if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
						transition_child(lpe,vn);
					
					break;

				default:
					throwException2(xpath_eval_exception,
						"unknown state");
	}
	return -2;
}
static int process_DDFP(locationPathExpr *lpe, VTDNav *vn){
	AutoPilot *ap;
	Boolean b = FALSE, b1 = FALSE;
	Predicate *t= NULL;
	int result;
	UCSChar *helper;
	switch(lpe->state){
			case XPATH_EVAL_START:
			case XPATH_EVAL_FORWARD:
				if (vn->atTerminal){
					if (lpe->state == XPATH_EVAL_START)
						lpe->state= XPATH_EVAL_END;
					else {
						// no need to set_ft to TRUE
						// no need to resetP
						lpe->state= XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
					break;
				}

				t = lpe->currentStep->p;
				while(t!=NULL){
					if ((t->requireContext)){
						int i = computeContextSize(lpe, t,vn);
						if (i==0){
							b1 = TRUE;
							break;
						}else
							setContextSize_p(t,i);
					}
					t = t->nextP;
				}
				if (b1){
					if (lpe->state ==XPATH_EVAL_START)
						lpe->state= XPATH_EVAL_END;
					else {
						lpe->currentStep = lpe->currentStep->prevS;
						lpe->state= XPATH_EVAL_BACKWARD;
					}
					break;
				}


				helper = NULL;
				if (lpe->currentStep->nt->testType == NT_NAMETEST){
					helper = lpe->currentStep->nt->nodeName;
				}else if (lpe->currentStep->nt->testType == NT_NODE){
					helper = L"*";
				}else
    				throwException2(xpath_eval_exception,
					"can't run descendant following, or following-sibling axis over comment(), pi(), and text()"); 
				if (lpe->currentStep->o == NULL)
					lpe->currentStep->o = ap = createAutoPilot(vn);
				else{
					ap = lpe->currentStep->o;
					bind(ap,vn);
				}
				if (lpe->currentStep->ft == TRUE) {

					if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF0 )
						if (lpe->currentStep->nt->testType == NT_NODE)
							setSpecial(ap,TRUE);
						else
							setSpecial(ap,FALSE);
					//currentStep.o = ap = createAutoPilot(vn);
					if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF0)
						selectElement(ap,helper);
					else if (lpe->currentStep->axis_type == AXIS_DESCENDANT0)
						selectElement_D(ap,helper);
					else if (lpe->currentStep->axis_type == AXIS_PRECEDING0)
						selectElement_P(ap,helper);
					else
						selectElement_F(ap,helper);
					lpe->currentStep->ft = FALSE;
				}
				if ( lpe->state==  XPATH_EVAL_START)
					lpe->state=  XPATH_EVAL_END;

				push2(vn); // not the most efficient. good for now
				//System.out.println("  --++ push in //");
				b = FALSE;
				while(iterateAP(ap)){
					if (!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn)){
						b = TRUE;
						break;
					}
				}
				if (b == FALSE) {
					pop2(vn);
					//System.out.println("  --++ pop in //");
					lpe->currentStep->ft = TRUE;
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					if ( lpe->state==  XPATH_EVAL_FORWARD){
						lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
				} else {
					if (lpe->currentStep->nextS != NULL){
						lpe->state =  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
					}
					else {
						//vn.pop();
						lpe->state =  XPATH_EVAL_TERMINAL;
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
				break;

			case XPATH_EVAL_END:
				lpe->currentStep = NULL;
				// reset();
				return -1;

			case XPATH_EVAL_BACKWARD:
				//currentStep = lpe->currentStep->prevS;
				if (lpe->currentStep->out_of_range){
					lpe->currentStep->out_of_range = FALSE;
					transition_DDFP(lpe,vn);
					break;
				}
				ap = lpe->currentStep->o;
				//vn.push();
				b = FALSE;
				while(iterateAP(ap)){
					if (!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn)){
						b = TRUE;
						break;
					}
				}
				if (b ) {
					if (lpe->currentStep->nextS != NULL) {
						//vn.push();
						//System.out.println("  --++ push in //");
						lpe->state=  XPATH_EVAL_FORWARD;
						lpe->currentStep = lpe->currentStep->nextS;
					} else {
						lpe->state=  XPATH_EVAL_TERMINAL;
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				} else {
					transition_DDFP(lpe,vn);
				}
				break;

			case XPATH_EVAL_TERMINAL:
				if (lpe->currentStep->out_of_range){
					lpe->currentStep->out_of_range = FALSE;
					transition_DDFP(lpe,vn);
					break;
				}
				ap = lpe->currentStep->o;
				b = FALSE;
				while (iterateAP(ap)) {
					if (!lpe->currentStep->hasPredicate || evalPredicates(lpe->currentStep,vn)) {
						b = TRUE;
						break;
					}
				}
				if (b ) {
					//if (evalPredicates(lpe->currentStep,vn)) {
					result = getCurrentIndex(vn);
					if (isUnique_lpe(lpe,result))
						return result;
					//}
				} else {
					transition_DDFP(lpe,vn);
				}
				break;

			default:
				throwException2(xpath_eval_exception,
					"unknown state");
	}
	return -2;
}
static int process_following_sibling(locationPathExpr *lpe, VTDNav *vn){

	Boolean b = FALSE, b1 = FALSE;
	//int contextSize;
	Predicate *t= NULL;
	int result;
	switch( lpe->state){
		  case  XPATH_EVAL_START:
		  case  XPATH_EVAL_FORWARD:

			  t = lpe->currentStep->p;
			  while(t!=NULL){
				  if (t->requireContext){
					  int i = computeContextSize(lpe, t,vn);
					  if (i==0){
						  b1 = TRUE;
						  break;
					  }else
						  setContextSize_p(t,i);
				  }
				  t = t->nextP;
			  }
			  if (b1){
				  if (lpe->state == XPATH_EVAL_FORWARD){
					  lpe->state= XPATH_EVAL_BACKWARD;
					  lpe->currentStep = lpe->currentStep->prevS;
				  }else
					  lpe->state= XPATH_EVAL_END;
				  break;
			  }
			  if ( lpe->state==  XPATH_EVAL_START)
				  lpe->state=  XPATH_EVAL_END;
			  else
				  lpe->state=  XPATH_EVAL_BACKWARD;
			  push2(vn);
			  while (toElement(vn,NEXT_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  if (lpe->currentStep->nextS!=NULL){
						  lpe->state=  XPATH_EVAL_FORWARD;
						  lpe->currentStep = lpe->currentStep->nextS;
						  break;
					  } else {
						  lpe->state=  XPATH_EVAL_TERMINAL;
						  result = getCurrentIndex(vn);
						  if ( isUnique_lpe(lpe,result))
							  return result;
					  }
				  }
			  }
			  if ( lpe->state==  XPATH_EVAL_END){	
				if(lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				pop2(vn);
			  }else if ( lpe->state==  XPATH_EVAL_BACKWARD){
				if(lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				pop2(vn);
				lpe->currentStep = lpe->currentStep->prevS;
			  }
			  break;

		  case  XPATH_EVAL_END:
			  lpe->currentStep = NULL;
			  // reset();
			  return -1;

		  case  XPATH_EVAL_BACKWARD:
			  while (toElement(vn,NEXT_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  if (lpe->currentStep->nextS!=NULL){
						  lpe->state=  XPATH_EVAL_FORWARD;
						  lpe->currentStep = lpe->currentStep->nextS;
						  b = TRUE;
						  break;
					  } else {
						  lpe->state=  XPATH_EVAL_TERMINAL;
						  result = getCurrentIndex(vn);
						  if ( isUnique_lpe(lpe,result))
							  return result;
					  }
				  }
			  }
			  if (b==FALSE){
				  pop2(vn);
				  if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				  if (lpe->currentStep->prevS==NULL){
					  lpe->state=  XPATH_EVAL_END;
				  }else{
					  lpe->state=  XPATH_EVAL_BACKWARD;
					  lpe->currentStep = lpe->currentStep->prevS;
				  }
			  }
			  break;

		  case  XPATH_EVAL_TERMINAL:
			  while (toElement(vn,NEXT_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  // lpe->state=  XPATH_EVAL_TERMINAL;
					  result = getCurrentIndex(vn);
					  if ( isUnique_lpe(lpe,result))
						  return result;
				  }
			  }
			  pop2(vn);
			  if (lpe->currentStep->hasPredicate)
				resetP_s(lpe->currentStep,vn);
			  if(lpe->currentStep->prevS!=NULL){
				  lpe->currentStep = lpe->currentStep->prevS;
				  lpe->state=  XPATH_EVAL_BACKWARD;
			  }else{
				  lpe->state=  XPATH_EVAL_END;
			  }
			  break;

		  default:
			  throwException2(xpath_eval_exception,
				  "unknown state");
	}
	return -2;
}

static int process_parent(locationPathExpr *lpe, VTDNav *vn){
	Boolean /*b = FALSE,*/ b1 = FALSE;
	Predicate *t= NULL;
	int result;
	switch ( lpe->state) {
			case  XPATH_EVAL_START:
			case  XPATH_EVAL_FORWARD:
				t = lpe->currentStep->p;
				while(t!=NULL){
					if (t->requireContext){
						int i = computeContextSize(lpe, t,vn);
						if (i==0){
							b1 = TRUE;
							break;
						}else
							setContextSize_p(t,i);
					}
					t = t->nextP;
				}
				if (b1){
					if (lpe->state == XPATH_EVAL_FORWARD){
						lpe->state= XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}else
						lpe->state= XPATH_EVAL_END;
					break;
				}

				if (getCurrentDepth(vn) == -1) {
					if ( lpe->state==  XPATH_EVAL_START)
						lpe->state=  XPATH_EVAL_END;
					else {
						//vn.pop();
						lpe->state=  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
				} else {
					push2(vn);
					toElement(vn,PARENT); // must return TRUE
					if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
						if (lpe->currentStep->nextS != NULL) {
							lpe->state=  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
						} else {
							lpe->state=  XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}else{
						pop2(vn);
						if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
						if ( lpe->state==  XPATH_EVAL_START)
							lpe->state=  XPATH_EVAL_END;
						else {
							lpe->state=  XPATH_EVAL_BACKWARD;
							lpe->currentStep = lpe->currentStep->prevS;
						}
					}
				}

				break;

			case  XPATH_EVAL_END:
				lpe->currentStep = NULL;
				// reset();
				return -1;

			case  XPATH_EVAL_BACKWARD:
			case  XPATH_EVAL_TERMINAL:
				if (lpe->currentStep->prevS == NULL) {
					pop2(vn);
					lpe->state=  XPATH_EVAL_END;
					break;
				}else {
					pop2(vn);
					lpe->state=  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
					break;
				}

			default:
				throwException2(xpath_eval_exception,
					"unknown state");

	}
	return -2;
}

static int process_preceding_sibling(locationPathExpr *lpe, VTDNav *vn){

	Boolean b = FALSE, b1 = FALSE;
	Predicate *t= NULL;
	int result;
	switch(lpe->state){
		  case  XPATH_EVAL_START:
		  case  XPATH_EVAL_FORWARD:
			  t = lpe->currentStep->p;
			  while(t!=NULL){
				  if (t->requireContext){
					  int i = computeContextSize(lpe,t,vn);
					  if (i==0){
						  b1 = TRUE;
						  break;
					  }else
						  setContextSize_p(t,i);
				  }
				  t = t->nextP;
			  }
			  if (b1){
				  if (lpe->state == XPATH_EVAL_FORWARD){
					  lpe->state= XPATH_EVAL_BACKWARD;
					  lpe->currentStep = lpe->currentStep->prevS;
				  }else
					  lpe->state= XPATH_EVAL_END;
				  break;
			  }
			  if ( lpe->state==  XPATH_EVAL_START)
				  lpe->state=  XPATH_EVAL_END;
			  else
				  lpe->state=  XPATH_EVAL_BACKWARD;
			  push2(vn);
			  while (toElement(vn,PREV_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  if (lpe->currentStep->nextS!=NULL){
						  lpe->state=  XPATH_EVAL_FORWARD;
						  lpe->currentStep = lpe->currentStep->nextS;
						  break;
					  } else {
						  lpe->state=  XPATH_EVAL_TERMINAL;
						  result = getCurrentIndex(vn);
						  if ( isUnique_lpe(lpe,result))
							  return result;
					  }
				  }
			  }
			 
			  if ( lpe->state==  XPATH_EVAL_END){
				  if (lpe->currentStep->hasPredicate)
					  resetP_s(lpe->currentStep,vn);
				  pop2(vn);
			  }else if ( lpe->state==  XPATH_EVAL_BACKWARD){
				  if (lpe->currentStep->hasPredicate)
					  resetP_s(lpe->currentStep,vn);
				  pop2(vn);
				  lpe->currentStep = lpe->currentStep->prevS;
			  }
			  break;

		  case  XPATH_EVAL_END:
			  lpe->currentStep = NULL;
			  // reset();
			  return -1;

		  case  XPATH_EVAL_BACKWARD:
			  while (toElement(vn,PREV_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  if (lpe->currentStep->nextS!=NULL){
						  lpe->state=  XPATH_EVAL_FORWARD;
						  lpe->currentStep = lpe->currentStep->nextS;
						  b = TRUE;
						  break;
					  } else {
						  lpe->state=  XPATH_EVAL_TERMINAL;
						  result = getCurrentIndex(vn);
						  if ( isUnique_lpe(lpe,result))
							  return result;
					  }
				  }
			  }
			  if (b==FALSE){
				  pop2(vn);
				  if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				  if (lpe->currentStep->prevS==NULL){
					  lpe->state=  XPATH_EVAL_END;
				  }else{
					  lpe->state=  XPATH_EVAL_BACKWARD;
					  lpe->currentStep = lpe->currentStep->prevS;
				  }
			  }
			  break;

		  case  XPATH_EVAL_TERMINAL:
			  while (toElement(vn,PREV_SIBLING)){
				  if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
					  // state =  XPATH_EVAL_TERMINAL;
					  result = getCurrentIndex(vn);
					  if ( isUnique_lpe(lpe,result))
						  return result;
				  }
			  }
			  pop2(vn);
			  if(lpe->currentStep->prevS!=NULL){
				  lpe->currentStep = lpe->currentStep->prevS;
				  lpe->state=  XPATH_EVAL_BACKWARD;
			  }else{
				  lpe->state=  XPATH_EVAL_END;
			  }
			  break;

		  default:
			  throwException2(xpath_eval_exception,
				  "unknown state");
	}
	return -2;
}
static int process_self(locationPathExpr *lpe, VTDNav *vn){
		Boolean /*b = FALSE,*/ b1 = FALSE;
	    //int contextSize;
	    Predicate *t= NULL;
	    int result;
		switch( lpe->state){
		  case  XPATH_EVAL_START:
		  case  XPATH_EVAL_FORWARD:
  	        t = lpe->currentStep->p;
	        while(t!=NULL){
				if ((t->requireContext)){
	                int i = computeContextSize(lpe,t,vn);
	                if (i==0){
	                    b1 = TRUE;
	                    break;
	                }else
	                    setContextSize_p(t,i);
	            }
	            t = t->nextP;
	        }
	        if (b1){
	            if (lpe->state == XPATH_EVAL_FORWARD){
	                lpe->state= XPATH_EVAL_BACKWARD;
	                lpe->currentStep = lpe->currentStep->prevS;
	            }else
	                lpe->state= XPATH_EVAL_END;
	            break;
	        }
		  	if ((lpe->currentStep->nt_eval || eval_nt(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  		if (lpe->currentStep->nextS!=NULL){
		  			 lpe->state=  XPATH_EVAL_FORWARD;
		  			lpe->currentStep = lpe->currentStep->nextS;
		  		}
		  		else{
		  			 lpe->state=  XPATH_EVAL_TERMINAL;
		  			 if (vn->atTerminal)
		  			     result = vn->LN;
		  			 else
		  			     result = getCurrentIndex(vn);
					if ( isUnique_lpe(lpe,result))
						return result;
		  		}
		  	}else {
				if(lpe->currentStep->hasPredicate)
		  			resetP_s(lpe->currentStep,vn);
		  		if ( lpe->state==  XPATH_EVAL_START)
		  			 lpe->state=  XPATH_EVAL_END;
		  		else
		  			 lpe->state=  XPATH_EVAL_BACKWARD;
		  	}
		    break;

		  case  XPATH_EVAL_END:
		  	lpe->currentStep = NULL;
		  	// reset();
		  	return -1;

		  case  XPATH_EVAL_BACKWARD:
		  case  XPATH_EVAL_TERMINAL:
		  	if (lpe->currentStep->prevS!=NULL){
	  			 lpe->state=  XPATH_EVAL_BACKWARD;
	  			lpe->currentStep= lpe->currentStep->prevS;
	  		}else{
	  			 lpe->state=  XPATH_EVAL_END;
	  		}
		  	break;

		  default:
			throwException2(xpath_eval_exception,"unknown state");
		}
	    return -2;
	}



NodeTest *createNodeTest(){
	NodeTest *nt = (NodeTest *)malloc(sizeof(NodeTest));
	if (nt==NULL){
		throwException2(out_of_mem,
			"NodeTest allocation failed ");
	}
	nt->nsEnabled = FALSE;
	nt->localName = NULL;
	nt->prefix = NULL;
	nt->URL = NULL;
	nt->nodeName = NULL;
	return nt;
}
void freeNodeTest(NodeTest *nt){
	if (nt==NULL) return;
	free(nt->localName);
	free(nt->nodeName);
	free(nt->prefix);
	//free(nt->URL);
	free(nt);
}
Boolean eval_nt(NodeTest *nt, VTDNav *vn){
	if (nt->testType == NT_NODE)
		return TRUE;

	switch(nt->type){
			case 0: return TRUE;
			case 1: return matchElement(vn,nt->nodeName);
			case 2: return matchElementNS(vn,nt->URL,nt->localName);
	}
	
	return FALSE;
}

Boolean eval_nt2(NodeTest *nt, VTDNav *vn){
	int t;
		switch(nt->testType){
		case NT_NAMETEST:
			if (vn->atTerminal)
		        return FALSE;
			switch(nt->type){
			case 0: return TRUE;
			case 1: return matchElement(vn,nt->nodeName);
			case 2: return matchElementNS(vn,nt->URL,nt->localName);
			}
		case NT_NODE:
			return TRUE;
		case NT_TEXT:
			if (!vn->atTerminal)
		        return FALSE;
			t = getTokenType(vn,vn->LN);
			if (t== TOKEN_CHARACTER_DATA
					|| t == TOKEN_CDATA_VAL){
				return TRUE;
			}
			return FALSE;
			
		case NT_PI0:
			if (!vn->atTerminal)
				return FALSE;
			if (getTokenType(vn,vn->LN)== TOKEN_PI_NAME){
				return TRUE;
			}
			return FALSE;
		case NT_PI1:
			if (!vn->atTerminal)
				return FALSE;
			if (getTokenType(vn,vn->LN)== TOKEN_PI_NAME){
				return matchTokenString(vn,vn->LN, nt->nodeName);
			}
			return FALSE;
			
		default: // comment
			if (!vn->atTerminal)
				return FALSE;
			if (getTokenType(vn,vn->LN)== TOKEN_COMMENT){
				return TRUE;
			}
			return FALSE;
		}
}

void setNodeName(NodeTest *nt, UCSChar *name){
	nt->nodeName = name;
	if (wcscmp(name,L"*")==0){
		nt->type=0;
	}else
		nt->type=1;
}

void setNodeNameNS(NodeTest *nt, UCSChar *p, UCSChar *ln){
	nt->localName = ln;
	nt->prefix = p;
	nt->type = 2;
}

void setTestType(NodeTest *nt, nodeTestType ntt){
	nt->testType = ntt;
}

void toString_nt(NodeTest *nt, UCSChar *string){
	switch (nt->testType){
		case NT_NAMETEST :
		    if (nt->localName == NULL)
		        wprintf(L"%ls",nt->nodeName);
		    else
				wprintf(L"%ls:%ls", nt->prefix,nt->localName);
			break;
		case NT_NODE: wprintf(L"node()");break;
		case NT_TEXT: wprintf(L"text()");break;
		case NT_PI0:  wprintf(L"processing-instruction()");break;
		case NT_PI1: wprintf(L"processing-instruction(");
			if (wcschr(nt->nodeName,'"')!=NULL){
				wprintf(L"'");
				wprintf(nt->nodeName);
				wprintf(L"'");
			}else{
				wprintf(L"\"");
				wprintf(nt->nodeName);
				wprintf(L"\"");
			}
			         wprintf(L")");
					 break;
		default:  wprintf(L"comment()");
	}
}


Predicate *createPredicate(){
	Predicate *p = (Predicate *)malloc(sizeof(Predicate));
	if (p==NULL){
		throwException2(out_of_mem,
			"Predicate allocation failed ");
	}
	p->nextP = NULL;
	p->e = NULL;
	p->count = 0;
	p->d = 0;
	p->requireContext = FALSE;
	p->type = COMPLEX_P;
	p->fe=NULL;
	p->s=NULL;
	return p;
}

void freePredicate(Predicate *p){
	if (p==NULL)
		return;
	if(p->e!=NULL)
		p->e->freeExpr(p->e);
	free(p);
}

Boolean eval2_p(Predicate *p, VTDNav *vn){
	Boolean b;
	p->count++; // increment the position
	p->e->setPosition(p->e,p->count);
	if (p->e->isNumerical(p->e)){
		b = (p->e->evalNumber(p->e,vn)== p->count);
	}
	else{
		b = p->e->evalBoolean(p->e,vn);
	}
	return b;
}

Boolean eval_p(Predicate *p, VTDNav *vn){
	Boolean b;
	p->count++; // increment the position
	switch(p->type){
		case SIMPLE_P:
		if (p->d<p->count)
				return FALSE;
			else if(p->d==p->count){
				if (p->s!=NULL){
					p->s->out_of_range=TRUE;
				}else
					p->fe->out_of_range=TRUE;
				
				return TRUE;	
			}
		default:
				p->e->setPosition(p->e,p->count);
				if (p->e->isNumerical(p->e)){
					b = (p->e->evalNumber(p->e,vn)== p->count);
				}
				else{
					b = p->e->evalBoolean(p->e,vn);
				}
				return b;
	}
}

void setIndex_p(Predicate *p, int index){
	if (index<=0){
		throwException2(xpath_eval_exception,
			"Invalid index number");
	}
	p->d = (double) index;
}

void setContextSize_p(Predicate *p, int size){
	p->e->setContextSize(p->e,size);
}

Boolean requireContextSize_p(Predicate *p){
	return p->e->requireContextSize(p->e);
}

void reset_p(Predicate *p, VTDNav* vn){
	p->count = 0;
	p->e->reset(p->e,vn); // is this really needed?
}

void toString_p(Predicate *p, UCSChar *string){
		//String s = "["+expr+"]";
		if (p->nextP==NULL){
			wprintf(L"[");
			p->e->toString(p->e,string);
			wprintf(L"]");
		} else {
			wprintf(L"[");
			p->e->toString(p->e,string);
			wprintf(L"]");
			toString_p(p->nextP,string);
		}
}

Step *createStep(){
	Step *s = malloc(sizeof(Step));
	if (s==NULL){
		throwException2(out_of_mem,
			"Step allocation failed ");
	}
	s->nextS = s->prevS = NULL;
	s->p  = s->pt = NULL;
	s->nt = NULL;
	s->ft = TRUE;
	//s->position = 1;
	s->o = NULL;
	s->hasPredicate = FALSE;
	s->nt_eval = FALSE;
	s->out_of_range=FALSE;
	return s;
}

void freeStep(Step *s){
	Predicate *tmp, *tmp2;
	if (s==NULL)return;
	if (s->p != NULL){
		tmp = s->p;
		tmp2 = tmp->nextP;
		while(tmp2!=NULL){
			freePredicate(tmp);
			tmp= tmp2;
			tmp2 = tmp2->nextP;
		}
		freePredicate(tmp);
	}
	if (s->nt->testType == NT_TEXT){
		if (s->o!=NULL)
			freeTextIter((TextIter *)s->o);
	}else
		freeAutoPilot(s->o);
	freeNodeTest(s->nt);
	free(s);
}

void reset_s(Step *s, VTDNav *vn){
	s->ft = TRUE;
	if (s->hasPredicate)
		resetP_s(s,vn);
	s->out_of_range = FALSE;
	//s->position =1 ;
}

void resetP_s(Step *s,VTDNav *vn){
		Predicate *temp = s->p;
		while(temp!=NULL){
			reset_p(temp,vn);
			temp = temp->nextP;
		}
}
void resetP2_s(Step *s,VTDNav *vn, Predicate *p1){
		Predicate *temp = s->p;
		while(temp!=p1){
			reset_p(temp,vn);
			temp = temp->nextP;
		}
}

NodeTest *getNodeTest(Step *s){
	return s->nt;
}

Step *getNextStep(Step *s){
	return s->nextS;
}

Boolean get_ft(Step *s){
	return s->ft;
}

void set_ft(Step *s, Boolean b){
	s->ft = b;
}

Step *getPrevStep(Step *s){
	return s->prevS;
}

void setNodeTest(Step *s,NodeTest *n){
	s->nt = n;
	if (s->axis_type == AXIS_CHILD && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_CHILD0;
		}else if (s->axis_type == AXIS_DESCENDANT && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_DESCENDANT0;
		}else if (s->axis_type == AXIS_DESCENDANT_OR_SELF && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_DESCENDANT_OR_SELF0;
		}else if (s->axis_type == AXIS_FOLLOWING && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_FOLLOWING0;
		}else if (s->axis_type == AXIS_PRECEDING && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_PRECEDING0;
		}else if (s->axis_type == AXIS_FOLLOWING_SIBLING && n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_FOLLOWING_SIBLING0;
		}else if (s->axis_type == AXIS_PRECEDING_SIBLING&& n->testType ==NT_NAMETEST ){
			s->axis_type = AXIS_PRECEDING_SIBLING0;
		}
		if (n->testType== NT_NODE 
				|| (n->testType==NT_NAMETEST && wcscmp(n->nodeName,L"*")==0)){
			s->nt_eval= TRUE;
		}
		
}

void setPredicate(Step *s,Predicate *p1){
	if (s->p == NULL){
			s->p = s->pt = p1;
		} else {
			s->pt->nextP = p1;
			s->pt = s->pt->nextP;			
		}
		setStep4Predicates(s);
		if (p1!=NULL) s->hasPredicate = TRUE;
}

Boolean eval_s(Step *s,VTDNav *vn){
	return eval_nt(s->nt,vn) && ((!s->hasPredicate) ||evalPredicates(s,vn));
}

Boolean eval_s2(Step *s,VTDNav *vn, Predicate *p){
	return eval_nt(s->nt,vn) && evalPredicates2(s,vn,p);
}

Boolean eval2_s(Step *s,VTDNav *vn){
	return eval_nt2(s->nt,vn) && ((!s->hasPredicate) || evalPredicates(s,vn));
}

Boolean eval2_s2(Step *s,VTDNav *vn, Predicate *p){
	return eval_nt2(s->nt,vn) && evalPredicates2(s,vn,p);
}

Boolean evalPredicates(Step *s,VTDNav *vn){
	Predicate *temp = s->p;
	while(temp!=NULL) {
		if (eval_p(temp,vn)== FALSE)
			return FALSE;
		temp = temp->nextP;
	}
	return TRUE;
}
Boolean evalPredicates2(Step *s,VTDNav *vn, Predicate *p){
	Predicate *temp = s->p;
	while(temp!=p) {
		if (eval_p(temp,vn)== FALSE)
			return FALSE;
		temp = temp->nextP;
	}
	return TRUE;
}

void setAxisType(Step *s,axisType st){
	s->axis_type = st;

}

void toString_s(Step *s, UCSChar *string){
	//String s;

	if (s->p == NULL){
		wprintf(axisName(s->axis_type));
	    toString_nt(s->nt,string);
	}
	else {
		wprintf(axisName(s->axis_type));
		toString_nt(s->nt,string);
		wprintf(L" ");
		toString_p(s->p, string);
	}
	if (s->nextS == NULL)
		return;
	else {
		wprintf(L"/");
		toString_s(s->nextS, string);
	}
}

void setStep4Predicates(Step *s){
	Predicate *temp = s->p;
	while(temp!=NULL){
		temp->s=s;
		temp = temp->nextP;
	}
}

locationPathExpr *createLocationPathExpr(){
	exception e;
	locationPathExpr *lpe = malloc(sizeof(locationPathExpr));
	if (lpe==NULL){
		throwException2(out_of_mem,
			"locationPathExpr allocation failed ");
		return NULL;
	}
	Try{
		lpe->ih = createIntHash();
	}Catch(e){
		free(lpe);
		Throw e;
	}
	lpe->freeExpr = (free_Expr) &freeLocationPathExpr;
	lpe->evalBoolean = (eval_Boolean)&evalBoolean_lpe;
	lpe->evalNodeSet = (eval_NodeSet)&evalNodeSet_lpe;
	lpe->evalNumber  = (eval_Number)&evalNumber_lpe;
	lpe->evalString  = (eval_String)&evalString_lpe;
	lpe->isNumerical = (is_Numerical)&isNumerical_lpe;
	lpe->isBoolean = (is_Boolean)&isBoolean_lpe;
	lpe->isString =  (is_String)&isString_lpe;
	lpe->isNodeSet = (is_NodeSet)&isNodeSet_lpe;
	lpe->requireContextSize = (require_ContextSize)&requireContextSize_lpe;
	lpe->setContextSize = (set_ContextSize)&setContextSize_lpe;
	lpe->setPosition = (set_Position)&setPosition_lpe;
	lpe->reset = (reset_)&reset_lpe;
	lpe->toString = (to_String)&toString_lpe;
	lpe->adjust = (adjust_)&adjust_lpe;
	lpe->isFinal = (isFinal_)&isFinal_lpe;
	lpe->markCacheable = (markCacheable_)&markCacheable_lpe;
	lpe->markCacheable2 = (markCacheable2_)&markCacheable2_lpe;
	lpe->clearCache = (clearCache_)&clearCache_lpe;
	lpe->getFuncOpCode = (getFuncOpCode_)&getFuncOpCode;
	lpe->state = XPATH_EVAL_START;
	lpe->s = NULL;
	lpe->pathType = RELATIVE_PATH;
	lpe->currentStep = NULL;
	return lpe;
}

void freeLocationPathExpr(locationPathExpr *lpe){
	Step *tmp, *tmp2;
	if (lpe ==NULL) return;
	if (lpe->s != NULL){
		tmp = lpe->s;
		tmp2 = tmp->nextS;
		while(tmp2!=NULL){
			freeStep(tmp);
			tmp= tmp2;
			tmp2 = tmp2->nextS;
		}
		freeStep(tmp);
	}
	freeIntHash(lpe->ih);
	free(lpe);
}

int	evalNodeSet_lpe (locationPathExpr *lpe,VTDNav *vn){
    int result;
	if (lpe->currentStep == NULL) {
		if ( lpe->pathType ==  ABSOLUTE_PATH){
			vn->context[0]=-1;
			vn->atTerminal = FALSE;
		}
		lpe->currentStep =  lpe->s;
		if (lpe->currentStep == NULL){
			if (  lpe->state ==  XPATH_EVAL_START){
				 lpe->state = XPATH_EVAL_END;
				return 0;
			}
			else{
				return -1;
			}
		}
	}

	while (TRUE) {
		switch (lpe->currentStep->axis_type) {
			case AXIS_CHILD0:
			    if ( (result = process_child(lpe,vn))!=-2)
				   return result;
			    break;
			case AXIS_CHILD:
			    if ( (result = process_child2(lpe,vn))!=-2)
				   return result;
			    break;
			case AXIS_DESCENDANT_OR_SELF0:
			case AXIS_DESCENDANT0:
			case AXIS_PRECEDING0:
			case AXIS_FOLLOWING0:
			    if ((result = process_DDFP(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_DESCENDANT_OR_SELF:
			case AXIS_DESCENDANT:
			case AXIS_PRECEDING:
			case AXIS_FOLLOWING:
			    if ((result = process_DDFP2(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_PARENT:
			    if ((result = process_parent(lpe, vn))!= -2)
			        return result;
			    break;
			case AXIS_ANCESTOR:
			    if ((result = process_ancestor2(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_ANCESTOR_OR_SELF:
			    if ((result = process_ancestor_or_self(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_SELF:
			    if ((result = process_self(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_FOLLOWING_SIBLING:
			    if ((result = process_following_sibling2(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_FOLLOWING_SIBLING0:
			    if ((result = process_following_sibling(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_PRECEDING_SIBLING:
			    if ((result = process_preceding_sibling2(lpe, vn))!= -2)
			        return result;
			    break;
			case AXIS_PRECEDING_SIBLING0:
			    if ((result = process_preceding_sibling(lpe, vn))!= -2)
			        return result;
			    break;
			case AXIS_ATTRIBUTE:
			    if ((result = process_attribute(lpe,vn))!= -2)
			        return result;
			    break;
			case AXIS_NAMESPACE:
			    if ((result = process_namespace(lpe,vn))!= -2)
			        return result;
				break;
			default:
				throwException2(xpath_eval_exception,
					"axis not supported");
			}
		}
}

double	evalNumber_lpe (locationPathExpr *lpe,VTDNav *vn){

	double d1 = 0.0;
	exception ee;
	double d=d1/d1;
	int a = -1,size;
	push2(vn);
	size = vn->contextBuf2->size;
	Try {
		a = evalNodeSet_lpe(lpe,vn);
		if (a != -1) {
			int t = getTokenType(vn,a);
			if (t == TOKEN_ATTR_NAME) {
				d = parseDouble(vn,a+1);
			} else if (t == TOKEN_STARTING_TAG || t ==TOKEN_DOCUMENT) {
				UCSChar *s =getXPathStringVal( vn,0), *s1;
				d  = wcstod(s,&s1);
				free( s);
			}else if (t == TOKEN_PI_NAME) {
				if (a+1 < vn->vtdSize || getTokenType(vn,a+1)==TOKEN_PI_VAL)
					//s = vn.toString(a+1); 	
					d = parseDouble(vn,a+1);                	
			}else 
				d = parseDouble(vn,a);
		}
	} Catch (ee) {

	}
	vn->contextBuf2->size = size;
	reset_lpe(lpe,vn);
	pop2(vn);
	//return s;
	return d;
}

UCSChar* evalString_lpe  (locationPathExpr *lpe,VTDNav *vn){
	exception ee;
	int a,size;
	UCSChar *s = NULL;	
	
	//int a = -1;
	push2(vn);
    size = vn->contextBuf2->size;
     
	Try {
         a = evalNodeSet_lpe(lpe,vn);
         if (a != -1) {
            	int t = getTokenType(vn,a);
                switch(t){
			 case TOKEN_STARTING_TAG:
			 case TOKEN_DOCUMENT:
				 s = getXPathStringVal(vn,0);
				 break;
			 case TOKEN_ATTR_NAME:
				 s = toString(vn,a + 1);
				 break;
			 case TOKEN_PI_NAME:
				 //if (a + 1 < vn.vtdSize
				 //		|| vn.getTokenType(a + 1) == VTDNav.TOKEN_PI_VAL)
				 s = toString(vn,a + 1);
				 break;
			 default:
				 s = toString(vn,a);
				 break;
			 }		
            }else
				return wcsdup(L"");
        } Catch (ee) {

        }
        vn->contextBuf2->size = size;
        reset_lpe(lpe,vn);
        pop2(vn);
        return s;
}

Boolean evalBoolean_lpe (locationPathExpr *lpe,VTDNav *vn){
	exception e;
	Boolean a = FALSE;
	int size;
	push2(vn);
	// record stack size
	size = vn->contextBuf2->size;
	Try{
		a = (evalNodeSet_lpe(lpe,vn) != -1);
	}Catch (e){
	}
	//rewind stack
	vn->contextBuf2->size = size;
	reset_lpe(lpe,vn);
	pop2(vn);
	return a;
}

Boolean isBoolean_lpe (locationPathExpr *lpe){
	return FALSE;
}

Boolean isNumerical_lpe (locationPathExpr *lpe){
	return FALSE;
}

Boolean isString_lpe (locationPathExpr *lpe){
	return FALSE;
}

Boolean isNodeSet_lpe (locationPathExpr *lpe){
	return TRUE;
}

Boolean requireContextSize_lpe(locationPathExpr *lpe){
	return FALSE;
}
void reset_lpe(locationPathExpr *lpe, VTDNav *vn){
	Step *temp = lpe->s;
	lpe->state = XPATH_EVAL_START;
	resetIntHash(lpe->ih);
	lpe->currentStep = NULL;
	while(temp!=NULL){
		reset_s(temp,vn);
		temp = temp->nextS;
	}
}
void	setContextSize_lpe(locationPathExpr *lpe,int s){
}
void	setPosition_lpe(locationPathExpr *lpe,int pos){
}
void    toString_lpe(locationPathExpr *lpe, UCSChar* string){

	Step *ts = lpe->s;
	if (lpe->pathType == ABSOLUTE_PATH){
		wprintf(L"/");
	}
	if (ts == NULL)
		return;
	else
		toString_s(ts,string);
}

void setStep(locationPathExpr *lpe, Step* st){
	lpe->s = st;
}


void clearCache_lpe(locationPathExpr *lpe){
	Step *temp = lpe->s;
	while(temp!=NULL){
		if (temp->p!=NULL){
			temp->p->e->clearCache(temp->p->e);
		}
		temp = temp->nextS;
	}
}

void markCacheable_lpe(locationPathExpr *lpe){
	Step *temp = lpe->s;
	while(temp!=NULL){
		if (temp->p!=NULL){
			if (temp->p->e!=NULL ){
				if (temp->p->e->isFinal(temp->p->e) && temp->p->e->isNodeSet(temp->p->e)){
					cachedExpr *ce = createCachedExpr(temp->p->e);
					temp->p->e = (expr *)ce;
				}
				temp->p->e->markCacheable2(temp->p->e);
			}
		}
		temp = temp->nextS;
	}
}

void markCacheable2_lpe(locationPathExpr *lpe){
	markCacheable_lpe(lpe);
}


Boolean isFinal_lpe(locationPathExpr *lpe){return (lpe->pathType ==  ABSOLUTE_PATH);}


int adjust_lpe(locationPathExpr *lpe, int n){
	int i;
	Step *temp;
	if (lpe->pathType == RELATIVE_PATH){
		i= min(6,determineHashWidth(n));//hashwidth 64
	} else {
		i=determineHashWidth(n);
	}
	if (lpe->ih!=NULL && i==lpe->ih->e)
	{}else {
		freeIntHash(lpe->ih);
		lpe->ih = createIntHash2(i);
	}
	temp = lpe->s;
	while(temp!=NULL){
		adjust_s(temp,n);
		temp = temp->nextS;
	}
	return i;
}

int adjust_p(Predicate *p, int n){
	return p->e->adjust(p->e,n);
}

int adjust_s(Step *s,int n){
	Predicate *temp = s->p;
	while(temp!=NULL){
		adjust_p(s->p,n);
		temp = temp->nextP;
	}
	return n;
}

void optimize(locationPathExpr *lpe){
	Step *ts = lpe->s;
	if (ts==NULL)
		return;
	while(ts->nextS!=NULL){
		ts = ts->nextS;
	}

	while(ts->prevS !=NULL){
		// logic of optmize here
		if (ts->axis_type == AXIS_CHILD0
			|| ts->axis_type == AXIS_CHILD
			|| ts->axis_type == AXIS_ATTRIBUTE){
				switch(ts->prevS->axis_type){
					 case AXIS_CHILD:
						 if (ts->prevS->nt->testType == NT_NODE){
							 ts->prevS->axis_type = AXIS_CHILD0;
							 ts->prevS->nt->testType = NT_NAMETEST;
							 ts->prevS->nt->type= 0;
							 ts->prevS->nt->nodeName = L"*";
						 }
						 break;
					 case AXIS_DESCENDANT: 
						 ts->prevS->axis_type = AXIS_DESCENDANT0;
						 break;
					 case AXIS_DESCENDANT_OR_SELF:						 
						 ts->prevS->axis_type = AXIS_DESCENDANT_OR_SELF0;
						 break;
					 case AXIS_PRECEDING:						 
						 ts->prevS->axis_type = AXIS_PRECEDING0;
						 break;
					 case AXIS_FOLLOWING:
						 ts->prevS->axis_type = AXIS_FOLLOWING0;
						 break;
					 case AXIS_FOLLOWING_SIBLING:
						 ts->prevS->axis_type = AXIS_FOLLOWING_SIBLING0;
						 ts->prevS->nt->testType = NT_NAMETEST;
						 ts->prevS->nt->type= 0;
						 ts->prevS->nt->nodeName = L"*";
						 break;
					 case AXIS_PRECEDING_SIBLING:
						 ts->prevS->axis_type = AXIS_PRECEDING_SIBLING0;
						 ts->prevS->nt->testType = NT_NAMETEST;
						 ts->prevS->nt->type= 0;
						 ts->prevS->nt->nodeName = L"*";
						 break;
				}
		}
		ts= ts->prevS;
	}
}

int computeContextSize4Ancestor(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		i = 0;
		while (toElement(vn,PARENT)) {
			if (eval_s2(lpe->currentStep, vn, p)) {
            	i++;
		    }	
		}				
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;	
}

int computeContextSize4Ancestor2(locationPathExpr *lpe, Predicate *p, VTDNav *vn){

		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		i = 0;
		while (toNode(vn,PARENT)) {
			if (eval2_s2(lpe->currentStep,vn, p)) {
           		i++;
			}
		}				
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int computeContextSize4AncestorOrSelf(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		i = 0;
		do {
			if (eval_s2(lpe->currentStep,vn, p)) {
            	i++;
		       }
		}while(toElement(vn,PARENT));
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int computeContextSize4AncestorOrSelf2(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		i = 0;
		do {
			if (eval2_s2(lpe->currentStep,vn, p)) {
            	i++;
		       }
		}while(toNode(vn,PARENT));
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}

intcomputeContextSize4Child( locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
    	Boolean b = toElement(vn,FIRST_CHILD);
    	if (b) {
    	    do {
				if (eval_s2(lpe->currentStep,vn, p)) {
                   	i++;
    	        }
    	    } while (toElement(vn,NEXT_SIBLING));	    		    
    	    toElement(vn,PARENT);
			resetP2_s(lpe->currentStep,vn,p);
    	    lpe->currentStep->out_of_range=FALSE;
    	    return i;
    	} else
    	    return 0;
}
int computeContextSize4Child(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
    	Boolean b =toNode(vn, FIRST_CHILD);
    	if (b) {
    	    do {
				if (eval2_s2(lpe->currentStep,vn, p)) {
                   	i++;
    	        }
    	    } while (toElement(vn,NEXT_SIBLING));	    		    
    	    toNode(vn,PARENT);
			resetP2_s(lpe->currentStep,vn,p);
    	    lpe->currentStep->out_of_range=FALSE;
    	    return i;
    	} else
    	    return 0;
}

int computeContextSize4Child2(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
    	Boolean b =toElement(vn, FIRST_CHILD);
    	if (b) {
    	    do {
				if (eval2_s2(lpe->currentStep,vn, p)) {
                   	i++;
    	        }
    	    } while (toNode(vn,NEXT_SIBLING));	    		    
    	    toNode(vn,PARENT);
			resetP2_s(lpe->currentStep,vn,p);
    	    lpe->currentStep->out_of_range=FALSE;
    	    return i;
    	} else
    	    return 0;
}

int computeContextSize4DDFP(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		UCSChar *helper = NULL;
	    int i=0;
	    AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		if (lpe->currentStep->nt->testType == NT_NODE){
		    helper = L"*";
		}else if (lpe->currentStep->nt->testType == NT_NAMETEST){
			helper = lpe->currentStep->nt->nodeName;
		}else
			throwException2(xpath_eval_exception,"can't run descendant "\
					"following, or following-sibling axis over comment(), pi(), and text()");
		if (ap==NULL)
			ap = createAutoPilot(vn);
		else
			bind(ap,vn);
		if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF0 )
			if (lpe->currentStep->nt->testType == NT_NODE)
				setSpecial(ap,TRUE);
			else
				setSpecial(ap,FALSE);
		//currentStep.o = ap = new AutoPilot(vn);
	    if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF0)
	        if (lpe->currentStep->nt->localName!=NULL)
	            selectElementNS(ap,lpe->currentStep->nt->URL,lpe->currentStep->nt->localName);
	        else 
	            selectElement(ap,helper);
		else if (lpe->currentStep->axis_type == AXIS_DESCENDANT0)
		    if (lpe->currentStep->nt->localName!=NULL)
		        selectElementNS_D(ap,lpe->currentStep->nt->URL,lpe->currentStep->nt->localName);
		    else 
		        selectElement_D(ap,helper);
		else if (lpe->currentStep->axis_type == AXIS_PRECEDING0)
		    if (lpe->currentStep->nt->localName!=NULL)
		        selectElementNS_P(ap,lpe->currentStep->nt->URL,lpe->currentStep->nt->localName);
		    else 
		        selectElement_P(ap,helper);
		else 
		    if (lpe->currentStep->nt->localName!=NULL)
		        selectElementNS_F(ap,lpe->currentStep->nt->URL,lpe->currentStep->nt->localName);
		    else 
		        selectElement_F(ap,helper);
	    push2(vn);
		while(iterateAP(ap)){
			if (evalPredicates2(lpe->currentStep,vn,p)){
				i++;
			}
		}
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int computeContextSize4DDFP2(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
	    AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		
		if (ap==NULL)
			ap = createAutoPilot(vn);
		else
			bind(ap,vn);
		
		//currentStep.o = ap = new AutoPilot(vn);
	    if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF)
	       selectNode(ap);
		else if (lpe->currentStep->axis_type == AXIS_DESCENDANT)
		   selectDescendantNode(ap);
		else if (lpe->currentStep->axis_type == AXIS_PRECEDING)
		   selectPrecedingNode(ap);
		else 
		   selectFollowingNode(ap);
	    push2(vn);
		while(iterateAP2(ap)){
			if (eval2_s2(lpe->currentStep,vn,p)){
				i++;
			}
		}
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int computeContextSize4FollowingSibling(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		//AutoPilot ap = (AutoPilot)currentStep.o;
		push2(vn);
		while(toElement(vn,NEXT_SIBLING)){
			if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
	    pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		//currentStep.o = ap;
		return i;
}


int computeContextSize4FollowingSibling2(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		//AutoPilot ap = (AutoPilot)currentStep.o;
		push2(vn);
		while(toNode(vn,NEXT_SIBLING)){
			if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
	    pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		//currentStep.o = ap;
		return i;
}

int computeContextSize4Parent(locationPathExpr* lpe,Predicate *p, VTDNav *vn){
		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		//i = 0;
		if (toElement(vn,PARENT)){
			if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int computeContextSize4Parent2(locationPathExpr* lpe,Predicate *p, VTDNav *vn){
		int i=0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		push2(vn);
		//i = 0;
		if (toNode(vn,PARENT)){
			if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}

int computeContextSize4PrecedingSibling(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		push2(vn);
		while(toElement(vn,PREV_SIBLING)){
		    if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		//currentStep.o = ap;
		return i;
}

int computeContextSize4PrecedingSibling2(locationPathExpr *lpe,Predicate *p, VTDNav *vn){
		int i=0;
		push2(vn);
		while(toNode(vn,PREV_SIBLING)){
		    if (eval_s2(lpe->currentStep,vn,p)){
		        i++;
		    }
		}			    
		pop2(vn);
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		//currentStep.o = ap;
		return i;
}



int computeContextSize4Self2(locationPathExpr *lpe, Predicate *p, VTDNav *vn){
		int i = 0;
		AutoPilot *ap = (AutoPilot *)lpe->currentStep->o;
		//if (vn->toNode(PARENT)){
		if (eval2_s2(lpe->currentStep,vn,p)){
		   i++;
		}
		//}
		resetP2_s(lpe->currentStep,vn,p);
		lpe->currentStep->out_of_range=FALSE;
		lpe->currentStep->o = ap;
		return i;
}


int process_ancestor2(locationPathExpr *lpe,VTDNav *vn){
	    int result;
	    Boolean b= FALSE, b1 = FALSE;
	    //int contextSize;
	    Predicate *t= NULL;
	    
	    switch(lpe->state){
	    	case XPATH_EVAL_START:
	    		
	    	    t = lpe->currentStep->p;
	    	    while (t != NULL) {
	    	        if (t->requireContext) {
	    	            int i = computeContextSize(lpe, t, vn);
	    	            if (i == 0) {
	    	                b1 = TRUE;
	    	                break;
	    	            } else
	    	                setContextSize_p(t,i);
	    	        }
	    	        t = t->nextP;
	    	    }
	    	    if (b1) {
	    	        lpe->state = XPATH_EVAL_END;
	    	        break;
	    	    }

	    	    lpe->state = XPATH_EVAL_END;
	    	    //if (vn.getCurrentDepth() != -1) {
	    	        push2(vn);

	    	        while (toNode(vn,PARENT)) {
						if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
	    	            		&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
	    	                if (lpe->currentStep->nextS != NULL) {
	    	                    lpe->state = XPATH_EVAL_FORWARD;
	    	                    lpe->currentStep = lpe->currentStep->nextS;
	    	                    break;
	    	                } else {
	    	                    //vn.pop();
	    	                    lpe->state = XPATH_EVAL_TERMINAL;
	    	                    result = getCurrentIndex(vn);
	    	                    if (isUnique_lpe(lpe,result))
	    	                        return result;
	    	                }
	    	            }
	    	        }
	    	        if (lpe->state == XPATH_EVAL_END) {
	    	        	if (lpe->currentStep->hasPredicate)
							resetP_s(lpe->currentStep,vn);
	    	           pop2( vn);
	    	        }
	    	   // }
	    	    break;
    	        
	    	case XPATH_EVAL_END:   
				lpe->currentStep =NULL;
				// reset();
			    return -1;
			    
	    	case XPATH_EVAL_FORWARD:	    	    
	    	     t = lpe->currentStep->p;
	    	     while(t!=NULL){
	    	        if (t->requireContext){
	    	             int i = computeContextSize(lpe,t,vn);
	    	             if (i==0){
	    	                 b1 = TRUE;
	    	                 break;
	    	             }else
	    	                 setContextSize_p(t,i);
	    	        }
	    	        t = t->nextP;
	    	    }
	    	    if (b1){
	    	        lpe->currentStep = lpe->currentStep->prevS;
	    	        lpe->state = XPATH_EVAL_BACKWARD;
	    	        break;
	    	    }
			    lpe->state =  XPATH_EVAL_BACKWARD;
			   	push2(vn);
					
			   	while(toNode(vn,PARENT)){
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
			   				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
			   			if (lpe->currentStep->nextS != NULL){
			   				lpe->state =  XPATH_EVAL_FORWARD;
			   				lpe->currentStep = lpe->currentStep->nextS;
			   				break;
			   			}
			   			else {
			   				//vn.pop();
			   				lpe->state =  XPATH_EVAL_TERMINAL;
			   				result =getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
			   			}
			   		}							
			   	}
			   	if ( lpe->state == XPATH_EVAL_BACKWARD){
					if (lpe->currentStep->hasPredicate)
				   		resetP_s(lpe->currentStep,vn);
					pop2(vn);
			   		lpe->currentStep=lpe->currentStep->prevS;
			   	}			    
			  	break;
	    	    
	    	case XPATH_EVAL_BACKWARD:
				b = FALSE;
				push2(vn);

				while (toNode(vn,PARENT)) {
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS!= NULL) {
							 lpe->state =  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
							b = TRUE;
							break;
						} else {
							//vn.pop();
							lpe->state =  XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}
				}
				if (b==FALSE){
					pop2(vn);
					if (lpe->currentStep->prevS!=NULL) {
						if (lpe->currentStep->hasPredicate)
							resetP_s(lpe->currentStep,vn);
						lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					}
					else {
						 lpe->state =  XPATH_EVAL_END;
					}
				}
				break;
				
	    	case XPATH_EVAL_TERMINAL:			
	    	    while (toNode(vn,PARENT)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					result = getCurrentIndex(vn);
					if ( isUnique_lpe(lpe,result))
						return result;
				}
			}
			pop2(vn);
			
			if (lpe->currentStep->prevS!=NULL) {
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				lpe->state =  XPATH_EVAL_BACKWARD;
				lpe->currentStep = lpe->currentStep->prevS;
			}
			else {
				 lpe->state =  XPATH_EVAL_END;
			}
			break;
		
		default:
			throwException2(xpath_eval_exception,"unknown state");
	    }
	    return -2;
}

int process_ancestor_or_self2(locationPathExpr *lpe,VTDNav *vn){
	    Boolean b = FALSE, b1 = FALSE;
	    Predicate *t= NULL;
	    int result;
		switch ( lpe->state) {
			case  XPATH_EVAL_START:
	    	    t = lpe->currentStep->p;
	    	    while (t != NULL) {
	    	        if (t->requireContext) {
	    	            int i = computeContextSize( lpe,t, vn);
	    	            if (i == 0) {
	    	                b1 = TRUE;
	    	                break;
	    	            } else
	    	                setContextSize_p(t,i);
	    	        }
	    	        t = t->nextP;
	    	    }
	    	    if (b1) {
	    	        lpe->state = XPATH_EVAL_END;
	    	        break;
	    	    }
				lpe->state =  XPATH_EVAL_END;
				push2(vn);
				
				if (lpe->currentStep->ft){						
					lpe->currentStep->ft = FALSE;
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS != NULL) {
							lpe->state =  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
							break;
						} else {
							//vn.pop();
							lpe->state =  XPATH_EVAL_TERMINAL;
							if (vn->atTerminal)
							    result = vn->LN;
							else 
							    result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}
				}
				
					while (toNode(vn,PARENT)) {
						if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
								&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
							if (lpe->currentStep->nextS != NULL) {
								 lpe->state =  XPATH_EVAL_FORWARD;
								lpe->currentStep = lpe->currentStep->nextS;
								break;
							} else {
								//vn.pop();
								 lpe->state =  XPATH_EVAL_TERMINAL;
								result = getCurrentIndex(vn);
								if ( isUnique_lpe(lpe,result))
									return result;
							}
						}
					}
				
				if ( lpe->state ==  XPATH_EVAL_END) {
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					pop2(vn);
				}

				break;
				
			case  XPATH_EVAL_FORWARD:
	    	     t = lpe->currentStep->p;
	    	     while(t!=NULL){
	    	        if (t->requireContext){
	    	             int i = computeContextSize(lpe,t,vn);
	    	             if (i==0){
	    	                 b1 = TRUE;
	    	                 break;
	    	             }else
	    	                 setContextSize_p(t,i);
	    	        }
	    	        t = t->nextP;
	    	    }
	    	    if (b1){
	    	        lpe->currentStep = lpe->currentStep->prevS;
	    	        lpe->state = XPATH_EVAL_BACKWARD;
	    	        break;
	    	    }
				 lpe->state =  XPATH_EVAL_BACKWARD;
					push2(vn);
					if (lpe->currentStep->ft ) {
						lpe->currentStep->ft = FALSE;
						
						if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
								&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
							if (lpe->currentStep->nextS != NULL) {
								 lpe->state =  XPATH_EVAL_FORWARD;
								lpe->currentStep = lpe->currentStep->nextS;
								break;
							} else {
								//vn.pop();
								 lpe->state =  XPATH_EVAL_TERMINAL;
								 if (vn->atTerminal)
								     result = vn->LN;
								 else 
								     result = getCurrentIndex(vn);
								if ( isUnique_lpe(lpe,result))
									return result;
							}
						}
					} 
						while (toNode(vn,PARENT)) {
							if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
									&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
								if (lpe->currentStep->nextS != NULL) {
									 lpe->state =  XPATH_EVAL_FORWARD;
									lpe->currentStep = lpe->currentStep->nextS;
									break;
								} else {
									//vn.pop();
									 lpe->state =  XPATH_EVAL_TERMINAL;
									result = getCurrentIndex(vn);
									if ( isUnique_lpe(lpe,result))
										return result;
								}
							}
						}
					
					if ( lpe->state ==  XPATH_EVAL_BACKWARD) {
						if (lpe->currentStep->hasPredicate)
							resetP_s(lpe->currentStep,vn);
						lpe->currentStep->ft = TRUE;
						pop2(vn);
						lpe->currentStep = lpe->currentStep->prevS;
					}
					break;
					
			case  XPATH_EVAL_END:
				lpe->currentStep = NULL;
				// reset();
		    	return -1;
				
			
			case  XPATH_EVAL_BACKWARD:
				//b = false;
				push2(vn);

				while (toNode(vn,PARENT)) {
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS != NULL) {
							 lpe->state =  XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
							b = TRUE;
							break;
						} else {
							//vn.pop();
							 lpe->state =  XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if ( isUnique_lpe(lpe,result))
								return result;
						}
					}
				}
				if (b == FALSE) {
					pop2(vn);
					if (lpe->currentStep->hasPredicate)
						resetP_s(lpe->currentStep,vn);
					if (lpe->currentStep->prevS != NULL) {
						lpe->currentStep->ft = TRUE;
						 lpe->state =  XPATH_EVAL_BACKWARD;
						lpe->currentStep = lpe->currentStep->prevS;
					} else {
						 lpe->state =  XPATH_EVAL_END;
					}
				}
				break;
			
			case  XPATH_EVAL_TERMINAL:
				while (toNode(vn,PARENT)) {
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
					}
				}
				pop2(vn);
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				if (lpe->currentStep->prevS!=NULL) {
					lpe->currentStep->ft = TRUE;
					 lpe->state =  XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				}
				else {
					 lpe->state =  XPATH_EVAL_END;
				}
				break;
				
			
			default:
				throwException2(xpath_eval_exception,"unknown state");
		}
	    return -2;

}


int process_child2(locationPathExpr *lpe,VTDNav *vn){
		int result;
	    Boolean b=FALSE;
		Boolean b1 = FALSE;
	    Predicate *t= NULL;
	    
	    switch(lpe->state){
		case XPATH_EVAL_START:
			// first search for any predicate that
			// requires contextSize
			// if so, compute its context size
			// if size > 0
			// set context
			// if size ==0
			// immediately set the state to backward or end
			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe,t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				lpe->state = XPATH_EVAL_END;
				break;
			}

			b = toNode(vn,FIRST_CHILD);
			lpe->state = XPATH_EVAL_END;
			if (b) {
				do {
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS != NULL) {
							// currentStep.position++;
							lpe->state = XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;							
						} else {
							lpe->state = XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if (isUnique_lpe(lpe,result)) {
								return result;
							}
						}
						break;
					}
				} while (toNode(vn,NEXT_SIBLING));
				if (lpe->state == XPATH_EVAL_END)
					toNode(vn,PARENT);
			}
			break;

		case XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
			return -1;

		case XPATH_EVAL_FORWARD:

			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe,t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				lpe->currentStep = lpe->currentStep->prevS;
				lpe->state = XPATH_EVAL_BACKWARD;
				break;
			}

			lpe->state = XPATH_EVAL_BACKWARD;
			 if (toNode(vn,FIRST_CHILD)) {
				do {
					if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
							&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
						if (lpe->currentStep->nextS != NULL) {
							lpe->state = XPATH_EVAL_FORWARD;
							lpe->currentStep = lpe->currentStep->nextS;
						} else {
							lpe->state = XPATH_EVAL_TERMINAL;
							result = getCurrentIndex(vn);
							if (isUnique_lpe(lpe,result))
								return result;
						}
						goto forward;
					}
				} while (toNode(vn,NEXT_SIBLING));
			
				toNode(vn,PARENT);
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				lpe->currentStep = lpe->currentStep->prevS;
			} else {
				// vn.toElement(VTDNav.P);
				lpe->currentStep = lpe->currentStep->prevS;
			}
forward:
			break;

		case XPATH_EVAL_BACKWARD:
			if (lpe->currentStep->out_of_range){
				lpe->currentStep->out_of_range = FALSE;
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				transition_child(lpe,vn);
				break;
			}

			// currentStep = currentStep.prevS;
			//b = false;
			while (toNode(vn,NEXT_SIBLING)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					b = TRUE;
					break;
				}
			}
			if (b) {
				lpe->state = XPATH_EVAL_FORWARD;
				lpe->currentStep = lpe->currentStep->nextS;
			} else{
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				transition_child(lpe,vn);
			}

			break;

		case XPATH_EVAL_TERMINAL:
			if (lpe->currentStep->out_of_range){
				lpe->currentStep->out_of_range = FALSE;
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				transition_child(lpe,vn);
				break;
			}
			while (toNode(vn,NEXT_SIBLING)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					// state = TERMINAL;
					result = getCurrentIndex(vn);
					if (isUnique_lpe(lpe,result))
						return result;
				}
			}
			if (lpe->currentStep->hasPredicate)
				resetP_s(lpe->currentStep,vn);
			transition_child(lpe,vn);

			break;

		default:
			throwException2(xpath_eval_exception,"unknown state");
		}
	    return -2;
}


int process_DDFP2(locationPathExpr *lpe,VTDNav *vn){
		AutoPilot *ap;
		Boolean b = FALSE, b1 = FALSE;
		Predicate *t = NULL;
		int result;
		//UCSChar *helper = NULL;
		switch (lpe->state) {
		case XPATH_EVAL_START:
		case XPATH_EVAL_FORWARD:
			/*if (vn.atTerminal) {
				if (lpe->state == START)
					state = END;
				else {
					// no need to set_ft to true
					// no need to resetP
					state = BACKWARD;
					currentStep = currentStep.prevS;
				}
				break;
			}*/

			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe,t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				if (lpe->state == XPATH_EVAL_START)
					lpe->state = XPATH_EVAL_END;
				else {
					lpe->currentStep = lpe->currentStep->prevS;
					lpe->state = XPATH_EVAL_BACKWARD;
				}
				break;
			}

			//UCSChar *helper = NULL;
			/*if (lpe->currentStep.nt.testType == NodeTest.NAMETEST) {
				helper = currentStep.nt.nodeName;
			} else if (currentStep.nt.testType == NodeTest.NODE) {
				helper = "*";
			} else
				throw new XPathEvalException(
						"can't run descendant "
								+ "following, or following-sibling axis over comment(), pi(), and text()");*/
			if (lpe->currentStep->o == NULL) {
				ap = createAutoPilot(vn);
				lpe->currentStep->o = ap;
			}
			else {
				ap = (AutoPilot *) lpe->currentStep->o;
				bind(ap,vn);
			}
			if (lpe->currentStep->ft) {

				/*if (lpe->currentStep.axis_type == AxisType.DESCENDANT_OR_SELF)
					if (lpe->currentStep.nt.testType == NodeTest.NODE)
						ap.setSpecial(true);
					else
						ap.setSpecial(false);*/
				// lpe->currentStep.o = ap = new AutoPilot(vn);
				if (lpe->currentStep->axis_type == AXIS_DESCENDANT_OR_SELF)
					selectNode(ap);
				else if (lpe->currentStep->axis_type == AXIS_DESCENDANT)
					selectDescendantNode(ap);
				else if (lpe->currentStep->axis_type == AXIS_PRECEDING)
					selectPrecedingNode(ap);
				else
					selectFollowingNode(ap);
				lpe->currentStep->ft = FALSE;
			}
			if (lpe->state == XPATH_EVAL_START)
				lpe->state = XPATH_EVAL_END;

			push2(vn); // not the most efficient. good for now
			// System.out.println("  --++ push in //");
			b = FALSE;
			while (iterateAP2(ap)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					b = TRUE;
					break;
				}
			}
			if (b == FALSE) {
				pop2(vn);
				// System.out.println("  --++ pop in //");
				lpe->currentStep->ft = TRUE;
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				if (lpe->state == XPATH_EVAL_FORWARD) {
					lpe->state = XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				}
			} else {
				if (lpe->currentStep->nextS != NULL) {
					lpe->state = XPATH_EVAL_FORWARD;
					lpe->currentStep = lpe->currentStep->nextS;
				} else {
					// vn.pop();
					lpe->state = XPATH_EVAL_TERMINAL;
					result = getCurrentIndex(vn);
					if (isUnique_lpe(lpe,result))
						return result;
				}
			}
			break;

		case XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
			return -1;

		case XPATH_EVAL_BACKWARD:
			if(lpe->currentStep->out_of_range){
				lpe->currentStep->out_of_range = FALSE;
				transition_DDFP(lpe,vn);
				break;
			}
			// lpe->currentStep = lpe->currentStep.prevS;
			ap = (AutoPilot *) lpe->currentStep->o;
			// vn.push();
			//b = false;
			while (iterateAP2(ap)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					b = TRUE;
					break;
				}
			}
			if (b ) {
				if (lpe->currentStep->nextS != NULL) {
					// vn.push();
					// System.out.println("  --++ push in //");
					lpe->state = XPATH_EVAL_FORWARD;
					lpe->currentStep = lpe->currentStep->nextS;
				} else {
					lpe->state = XPATH_EVAL_TERMINAL;
					result = getCurrentIndex(vn);
					if (isUnique_lpe(lpe,result))
						return result;
				}
			} else 
				transition_DDFP(lpe,vn);
			break;

		case XPATH_EVAL_TERMINAL:
			if(lpe->currentStep->out_of_range){
				lpe->currentStep->out_of_range = FALSE;
				transition_DDFP(lpe,vn);
				break;
			}
			ap = (AutoPilot *) lpe->currentStep->o;
			b = FALSE;
			while (iterateAP2(ap)) {
				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
					b = TRUE;
					break;
				}
			}
			if (b) {
				//if (currentStep.evalPredicates(vn)) {
				result = getCurrentIndex(vn);
				if (isUnique_lpe(lpe,result))
					return result;
				//}
			} else{ 
				transition_DDFP(lpe,vn);
			}
			break;

		default:
			throwException2(xpath_eval_exception,"unknown state");
		}
		return -2;
}



void transition_child(locationPathExpr *lpe,VTDNav *vn){
	toElement(vn,PARENT);
	if (lpe->currentStep->prevS != NULL){
		lpe->currentStep = lpe->currentStep->prevS;
		lpe->state = XPATH_EVAL_BACKWARD;
	}else{
		lpe->state = XPATH_EVAL_END;								
	}
}

void transition_DDFP(locationPathExpr *lpe,VTDNav *vn){
	pop2(vn);
	lpe->currentStep->ft = TRUE;
	if(lpe->currentStep->hasPredicate)
		resetP_s(lpe->currentStep,vn);
		//System.out.println("  --++ pop in //");
	if (lpe->currentStep->prevS != NULL) {
		lpe->state =  XPATH_EVAL_BACKWARD;
		lpe->currentStep = lpe->currentStep->prevS;
	} else
		lpe->state =  XPATH_EVAL_END;
}


int process_following_sibling2(locationPathExpr *lpe,VTDNav *vn){
	    Boolean b = FALSE, b1 = FALSE;
	    Predicate *t= NULL;
	    int result;
		switch( lpe->state){
		  case  XPATH_EVAL_START:
		  case  XPATH_EVAL_FORWARD:

  	        t = lpe->currentStep->p;
	        while(t!=NULL){
	            if (t->requireContext){
	                int i = computeContextSize(lpe,t,vn);
	                if (i==0){
	                    b1 = TRUE;
	                    break;
	                }else
	                    setContextSize_p(t,i);
	            }
	            t = t->nextP;
	        }
	        if (b1){
	            if (lpe->state == XPATH_EVAL_FORWARD){
	                lpe->state = XPATH_EVAL_BACKWARD;
	                lpe->currentStep = lpe->currentStep->prevS;
	            }else 
	                lpe->state = XPATH_EVAL_END;
	            break;
	        }
		  	if ( lpe->state ==  XPATH_EVAL_START)
		  		 lpe->state =  XPATH_EVAL_END;
		  	else
		  		 lpe->state =  XPATH_EVAL_BACKWARD;
		  	push2(vn);
		  	while (toNode(vn,NEXT_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			if (lpe->currentStep->nextS!=NULL){
		  				 lpe->state =  XPATH_EVAL_FORWARD;
		  				lpe->currentStep = lpe->currentStep->nextS;
		  				break;
		  			} else {
		  				 lpe->state =  XPATH_EVAL_TERMINAL;
		  				result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
		  			}
		  		}
		  	}
			if (lpe->state == XPATH_EVAL_END){
		  		if (lpe->currentStep->hasPredicate)
		  			resetP_s(lpe->currentStep,vn);	
		  		pop2(vn);
			}else if ( lpe->state ==  XPATH_EVAL_BACKWARD){
				if (lpe->currentStep->hasPredicate)
		  			resetP_s(lpe->currentStep,vn);	
		  		pop2(vn);
		  		lpe->currentStep = lpe->currentStep->prevS;				  		
		  	}
		    break;
		  	 
		  case  XPATH_EVAL_END:
		  	lpe->currentStep = NULL;
		  	// reset();
		  	return -1;
		  	
		  case  XPATH_EVAL_BACKWARD:
		  	while (toNode(vn,NEXT_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			if (lpe->currentStep->nextS!=NULL){
		  				 lpe->state =  XPATH_EVAL_FORWARD;
		  				lpe->currentStep = lpe->currentStep->nextS;
		  				b = TRUE;
		  				break;
		  			} else {
		  				 lpe->state =  XPATH_EVAL_TERMINAL;
		  				result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
		  			}
		  		}
		  	}
		    if (b==FALSE){
		    	pop2(vn);
				if (lpe->currentStep->hasPredicate)
			    	resetP_s(lpe->currentStep,vn);
		    	if (lpe->currentStep->prevS==NULL){
		    		 lpe->state =  XPATH_EVAL_END;
		    	}else{
		    		 lpe->state =  XPATH_EVAL_BACKWARD;
		    		lpe->currentStep = lpe->currentStep->prevS;
		    	}
		    }
		  	break;
		  
		  case  XPATH_EVAL_TERMINAL:
		  	while (toNode(vn,NEXT_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			// lpe->state =  TERMINAL;
		  			result = getCurrentIndex(vn);
					if ( isUnique_lpe(lpe,result))
						return result;
		  		}
		  	}
		  	pop2(vn);
		  	if (lpe->currentStep->hasPredicate)
		  		resetP_s(lpe->currentStep,vn);
		  	if(lpe->currentStep->prevS!=NULL){
		  		lpe->currentStep = lpe->currentStep->prevS;
		  		lpe->state =  XPATH_EVAL_BACKWARD;
		  	}else{
		  		lpe->state =  XPATH_EVAL_END;
		  	}
		  	break;

		  default:
			throwException2(xpath_eval_exception,"unknown state");
		}
	    return -2;
}


int process_parent2(locationPathExpr *lpe,VTDNav *vn){
		Boolean b1 = FALSE;
	    Predicate *t= NULL;
	    int result;
		switch ( lpe->state) {
			case  XPATH_EVAL_START:
			case  XPATH_EVAL_FORWARD:
    	        t = lpe->currentStep->p;
    	        while(t!=NULL){
    	            if (t->requireContext){
    	                int i = computeContextSize(lpe,t,vn);
    	                if (i==0){
    	                    b1 = TRUE;
    	                    break;
    	                }else
    	                    setContextSize_p(t,i);
    	            }
    	            t = t->nextP;
    	        }
    	        if (b1){
    	            if (lpe->state == XPATH_EVAL_FORWARD){
    	                lpe->state = XPATH_EVAL_BACKWARD;
    	                lpe->currentStep = lpe->currentStep->prevS;
    	            }else 
    	                lpe->state = XPATH_EVAL_END;
    	            break;
    	        }
    	        
    			if (getCurrentDepth(vn) == -1) {
    				if ( lpe->state ==  XPATH_EVAL_START)
    					 lpe->state =  XPATH_EVAL_END;
    				else {
    					//vn.pop();
    					 lpe->state =  XPATH_EVAL_BACKWARD;
    					lpe->currentStep = lpe->currentStep->prevS;
    				}
    			} else {
    				push2(vn);
    				toNode(vn,PARENT); // must return true
    				if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
    						&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
    				    if (lpe->currentStep->nextS != NULL) {
    					    lpe->state =  XPATH_EVAL_FORWARD;
    					   lpe->currentStep = lpe->currentStep->nextS;
    				    } else {
    					    lpe->state =  XPATH_EVAL_TERMINAL;
    					   result = getCurrentIndex(vn);
    						if ( isUnique_lpe(lpe,result))
    							return result;
    				    }
    				}else{
    					pop2(vn);
    					if (lpe->currentStep->hasPredicate)
    						resetP_s(lpe->currentStep,vn);
    					if ( lpe->state ==  XPATH_EVAL_START)
    						 lpe->state =  XPATH_EVAL_END;
    					else {								
    						 lpe->state =  XPATH_EVAL_BACKWARD;
    						lpe->currentStep = lpe->currentStep->prevS;
    					}
    				}
    			}

    			break;				
    			
    		case  XPATH_EVAL_END:
    			lpe->currentStep = NULL;
    			// reset();
    		    return -1;
    			
    		case  XPATH_EVAL_BACKWARD:
    		case  XPATH_EVAL_TERMINAL:
    			if (lpe->currentStep->prevS == NULL) {
    			    pop2(vn);
    				 lpe->state =  XPATH_EVAL_END;
    				break;
    			}else {
    				pop2(vn);
    				 lpe->state =  XPATH_EVAL_BACKWARD;
    				lpe->currentStep = lpe->currentStep->prevS;
    				break;
    			}
    			
    		default:
    			throwException2(xpath_eval_exception,"unknown state");
		
		}
	    return -2;
}

int process_preceding_sibling2(locationPathExpr *lpe,VTDNav *vn){
		Boolean b = FALSE, b1 = FALSE;
	    Predicate *t= NULL;
	    int result;
	    switch(lpe->state){
		  case   XPATH_EVAL_START:
		  case   XPATH_EVAL_FORWARD:
  	        t = lpe->currentStep->p;
	        while(t!=NULL){
	            if (t->requireContext){
	                int i = computeContextSize(lpe,t,vn);
	                if (i==0){
	                    b1 = TRUE;
	                    break;
	                }else
	                    setContextSize_p(t,i);
	            }
	            t = t->nextP;
	        }
	        if (b1){
	            if (lpe->state ==  XPATH_EVAL_FORWARD){
	                lpe->state =  XPATH_EVAL_BACKWARD;
	                lpe->currentStep = lpe->currentStep->prevS;
	            }else 
	                lpe->state =  XPATH_EVAL_END;
	            break;
	        }  
		  	if ( lpe->state ==   XPATH_EVAL_START)
		  		 lpe->state =   XPATH_EVAL_END;
		  	else
		  		 lpe->state =   XPATH_EVAL_BACKWARD;
		  	push2(vn);
		  	while (toNode(vn,PREV_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			if (lpe->currentStep->nextS!=NULL){
		  				 lpe->state =   XPATH_EVAL_FORWARD;
		  				lpe->currentStep = lpe->currentStep->nextS;
		  				break;
		  			} else {
		  				 lpe->state =   XPATH_EVAL_TERMINAL;
		  				result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
		  			}
		  		}
		  	}
			if ( lpe->state ==   XPATH_EVAL_BACKWARD){	
		  		if (lpe->currentStep->hasPredicate)
		  			resetP_s(lpe->currentStep,vn);
		  		pop2(vn);
			}else if ( lpe->state ==   XPATH_EVAL_BACKWARD){	
		  		if (lpe->currentStep->hasPredicate)
		  			resetP_s(lpe->currentStep,vn);
		  		pop2(vn);
				lpe->currentStep = lpe->currentStep->prevS;				  		
		  	}
		  	break;
		  	 
		  case   XPATH_EVAL_END:
		  	lpe->currentStep = NULL;
		  	// reset();
		  	return -1;
		  
		  case   XPATH_EVAL_BACKWARD:
		  	while (toNode(vn,PREV_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			if (lpe->currentStep->nextS!=NULL){
		  				 lpe->state =   XPATH_EVAL_FORWARD;
		  				lpe->currentStep = lpe->currentStep->nextS;
		  				b = TRUE;
		  				break;
		  			} else {
		  				 lpe->state =   XPATH_EVAL_TERMINAL;
		  				result = getCurrentIndex(vn);
						if ( isUnique_lpe(lpe,result))
							return result;
		  			}
		  		}
		  	}
		    if (b==FALSE){
		    	pop2(vn);
				if (lpe->currentStep->hasPredicate)
			    	resetP_s(lpe->currentStep,vn);
		    	if (lpe->currentStep->prevS==NULL){
		    		 lpe->state =   XPATH_EVAL_END;
		    	}else{
		    		 lpe->state =   XPATH_EVAL_BACKWARD;
		    		lpe->currentStep = lpe->currentStep->prevS;
		    	}
		    }
		  	break;
		  
		  case   XPATH_EVAL_TERMINAL:
		  	while (toNode(vn,PREV_SIBLING)){
		  		if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
		  				&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))){
		  			// state =  TERMINAL;
		  			result = getCurrentIndex(vn);
					if ( isUnique_lpe(lpe,result))
						return result;
		  		}
		  	}
		  	pop2(vn);
		  	if (lpe->currentStep->hasPredicate)
		  		resetP_s(lpe->currentStep,vn);
		  	if(lpe->currentStep->prevS!=NULL){
		  		lpe->currentStep = lpe->currentStep->prevS;
		  		 lpe->state =   XPATH_EVAL_BACKWARD;
		  	}else{
		  		 lpe->state =   XPATH_EVAL_END;
		  	}
		  	break;
		  
		  default:
			throwException2(xpath_eval_exception,"unknown state");
		}
	    return -2;
}


int process_self2(locationPathExpr *lpe,VTDNav *vn){
		Boolean b1 = FALSE;
		Predicate *t = NULL;
		int result;
		switch (lpe->state) {
		case XPATH_EVAL_START:
		case XPATH_EVAL_FORWARD:
			t = lpe->currentStep->p;
			while (t != NULL) {
				if (t->requireContext) {
					int i = computeContextSize(lpe,t, vn);
					if (i == 0) {
						b1 = TRUE;
						break;
					} else
						setContextSize_p(t,i);
				}
				t = t->nextP;
			}
			if (b1) {
				if (lpe->state == XPATH_EVAL_FORWARD) {
					lpe->state = XPATH_EVAL_BACKWARD;
					lpe->currentStep = lpe->currentStep->prevS;
				} else
					lpe->state = XPATH_EVAL_END;
				break;
			}
			if ((lpe->currentStep->nt_eval || eval_nt2(lpe->currentStep->nt,vn)) 
					&& ((!lpe->currentStep->hasPredicate) || evalPredicates(lpe->currentStep,vn))) {
				if (lpe->currentStep->nextS != NULL) {
					lpe->state = XPATH_EVAL_FORWARD;
					lpe->currentStep = lpe->currentStep->nextS;
				} else {
					lpe->state = XPATH_EVAL_TERMINAL;
					if (vn->atTerminal)
						result = vn->LN;
					else
						result = getCurrentIndex(vn);
					if (isUnique_lpe(lpe,result))
						return result;
				}
			} else {
				if (lpe->currentStep->hasPredicate)
					resetP_s(lpe->currentStep,vn);
				if (lpe->state == XPATH_EVAL_START)
					lpe->state = XPATH_EVAL_END;
				else
					lpe->state = XPATH_EVAL_BACKWARD;
			}
			break;

		case XPATH_EVAL_END:
			lpe->currentStep = NULL;
			// reset();
			return -1;

		case XPATH_EVAL_BACKWARD:
		case XPATH_EVAL_TERMINAL:
			if (lpe->currentStep->prevS != NULL) {
				lpe->state = XPATH_EVAL_BACKWARD;
				lpe->currentStep = lpe->currentStep->prevS;
			} else {
				lpe->state = XPATH_EVAL_END;
			}
			break;

		default:
			throwException2(xpath_eval_exception,"unknown state");
		}
		return -2;
}

