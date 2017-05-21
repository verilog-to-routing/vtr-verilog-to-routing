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
#include "textIter.h"
/* increment the index value to account for ultra long tokens */
static int increment(TextIter *ti, int sp);
/* Test whether a give token type is a TEXT.*/
static Boolean isText(TextIter *ti, int i);
static int handleDefault(TextIter *ti);
static int handleDocumentNode(TextIter *ti);
static int handleLevel0(TextIter *ti);
static int handleLevel1(TextIter *ti);
static int handleLevel2(TextIter *ti);
static int _handleLevel2(TextIter *ti);
static int handleLevel3(TextIter *ti);
static int handleLevel4(TextIter *ti);
static int handleDefault(TextIter *ti);


// create text iterator
TextIter *createTextIter(){
	TextIter *ti = NULL;
	ti = (TextIter *)malloc(sizeof(TextIter));
	
	if (ti == NULL){
		throwException2(out_of_mem,
			"createTextIter failed");
		return NULL;
	}
	ti->vn = NULL;
	ti->piName = NULL;
	ti->sel_type = 0;
	
	return ti;
}
/* free TextIter */
void freeTextIter(TextIter* ti){
	free(ti->piName);
	free(ti);
}
/* increment the index value to account for ultra long tokens */
int increment(TextIter *ti, int sp){
	int type = getTokenType(ti->vn,sp);
    int vtdSize = ti->vn->vtdBuffer->size;
    int i=sp+1;
    while(i<vtdSize && 
    	ti->depth == getTokenDepth(ti->vn,i) && 
		type == getTokenType(ti->vn,i)&&
			(getTokenOffset(ti->vn,i-1)+ (int)((longAt(ti->vn->vtdBuffer, i-1) & MASK_TOKEN_FULL_LEN)>>32) 
			        == getTokenOffset(ti->vn,i))){
		i++;
    }      	
    return i;
}

/* Test whether a give token type is a TEXT.*/
static Boolean isText(TextIter *ti, int index){
	exception e;
	int type = getTokenType(ti->vn,index);
	if (ti->sel_type == 0) {
		return (type == TOKEN_CHARACTER_DATA
			// || type == vn.TOKEN_COMMENT
			|| type == TOKEN_CDATA_VAL);
	}
	if (ti->sel_type == 1) {
		return (type == TOKEN_COMMENT);
	}

	if (ti->sel_type == 2)
		return (type == TOKEN_PI_NAME);
	Try {
		return (matchRawTokenString(ti->vn,index, ti->piName));
	} Catch(e){
		
	}
	return FALSE;
}


static int handleDefault(TextIter *ti){
	int sp = (ti->prevLocation != -1) ? increment(ti, ti->prevLocation): ti->index + 1;
	int d,type;
	if (sp>=ti->vn->vtdSize) return -1;
	d = getTokenDepth(ti->vn,sp);
	type = getTokenType(ti->vn,sp);
	while (d >= ti->depth
		&& !(d == ti->depth && type == TOKEN_STARTING_TAG)) {
			if (isText(ti,sp) == TRUE && d == ti->depth) {
				ti->prevLocation = sp;
				return sp;
			}
			sp++;
			if(sp >= ti->vn->vtdSize)
				return -1;

			d = getTokenDepth(ti->vn,sp);
			type = getTokenType(ti->vn,sp);                
	}
	return -1;
}

static int handleDocumentNode(TextIter *ti){
	int sp;
	if (ti->sel_type == 0)
		return -1;
	sp = (ti->prevLocation != -1) ? increment(ti,ti->prevLocation): ti->index + 1;
	if (sp>=ti->vn->vtdSize) return -1;
	//int d = vn.getTokenDepth(sp);
	//int type = vn.getTokenType(sp);
	//while (d == -1/*&& !(d == depth && type == VTDNav.TOKEN_STARTING_TAG)*/) {
	while(TRUE){    
		if (sp< ti->vn->rootIndex) {
			if (isText(ti,sp)){
				ti->prevLocation = sp;
				return sp;
			} else 
				sp++;
		} else { 
			// rewind to the end of document
			if (sp == ti->vn->rootIndex){
				sp = ti->vn->vtdSize-1;
				while(getTokenDepth(ti->vn,sp)==-1){
					sp--;
				}
				sp++;
			}        		 
			if (sp>=ti->vn->vtdSize){
				return -1;
			} else if (isText(ti,sp)){
				ti->prevLocation = sp;
				return sp;
			} else
				sp++;        		         		 
		}
	}
}
static int handleLevel0(TextIter *ti){
	//int curDepth = vn.context[0];
	int d, type,sp;
	sp = (ti->prevLocation != -1) ? increment(ti,ti->prevLocation): ti->index + 1;
	if (sp>=ti->vn->vtdSize) return -1;
	d = getTokenDepth(ti->vn,sp);
	type = getTokenType(ti->vn,sp);
	while (d >= ti->depth
		&& !(d == ti->depth && type == TOKEN_STARTING_TAG)) {
			if (isText(ti,sp) == TRUE && d == ti->depth) {
				ti->prevLocation = sp;
				return sp;
			}
			sp++;
			if(sp >= ti->vn->vtdSize)
				return -1;

			d = getTokenDepth(ti->vn,sp);
			type = getTokenType(ti->vn,sp);                
	}
	return -1;
}
static int handleLevel1(TextIter *ti){

	   	int sp,size;
        if (ti->prevLocation != -1) {
            sp = increment(ti,ti->prevLocation) ;
        } else {
            // fetch lclower and lcupper
			int i;
            ti->lcLower = lower32At(ti->vn->l1Buffer, ti->vn->l1index);
            if (ti->lcLower != -1) {
				ti->lcUpper = ti->vn->l2Buffer->size - 1;
                size = ti->vn->l1Buffer->size;
                for (i = ti->vn->l1index + 1; i < size ; i++) {
                    int temp = lower32At(ti->vn->l1Buffer,i);
                    if (temp != 0xffffffff) {
                        ti->lcUpper = temp - 1;
                        break;
                    }
                }
            }
            sp = ti->index + 1;
        } // check for l2lower and l2upper

        if (ti->lcLower != -1) { // have at least one child element
			int temp1 = upper32At(ti->vn->l2Buffer,ti->lcLower);
            int temp2 = upper32At(ti->vn->l2Buffer,ti->lcUpper);
            ti->lcIndex = (ti->lcIndex != -1) ? ti->lcIndex : ti->lcLower;
            while (sp < ti->vn->vtdSize) {
                int s = upper32At(ti->vn->l2Buffer,ti->lcIndex);
                if (sp >= temp1 && sp < temp2) {
                    if (sp == s) {
                        ti->lcIndex++;
						sp = upper32At(ti->vn->l2Buffer, ti->lcIndex) - 1;
                        //boolean b = false;
                        while (getTokenDepth(ti->vn,sp) == 1) {
                        	//b = true;
                            sp--;
                        }
                        //if (b)
                        sp++;
                        //continue; 
                    }
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==1 ) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else if (sp < temp1) {
                    if (isText(ti,sp) == TRUE) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else {
                    //if (sp == temp2) { // last child element
                    //} else
                    
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp) == 1){
                    	//System.out.println("depth ->"+vn.getTokenDepth(sp));
                        ti->prevLocation = sp;
                        return sp;
                    } else if ((getTokenType(ti->vn,sp)==TOKEN_STARTING_TAG
                            && getTokenDepth(ti->vn,sp) < 2 ) || getTokenDepth(ti->vn,sp)<1) {
                        break;
                    }
                    sp++;
                }                    
            }
            //prevLocation = vtdSize-1;
            return -1;
        } else { // no child element
			int d,type;
            if (sp>=ti->vn->vtdSize) return -1;
            d = getTokenDepth(ti->vn,sp);
            type = getTokenType(ti->vn,sp);
            while (sp < ti->vn->vtdSize
                && d >= 1
                && !(d == 1 && type == TOKEN_STARTING_TAG)) {
                if (isText(ti,sp) == TRUE) {
                    ti->prevLocation = sp;
                    return sp;
                }
                sp++;
                d = getTokenDepth(ti->vn,sp);
                type = getTokenType(ti->vn,sp);
                
            }
            //prevLocation = vtdSize-1;
            return -1;
        }
}
static int handleLevel2(TextIter *ti){
	    	int sp;
        if (ti->prevLocation != -1) {
            sp = increment(ti,ti->prevLocation);
        } else {
            // fetch lclower and lcupper
            ti->lcLower = lower32At(ti->vn->l2Buffer,ti->vn->l2index);
            if (ti->lcLower != -1) {
				int i,size;
                ti->lcUpper =  ti->vn->l3Buffer->size - 1;
                size = ti->vn->l2Buffer->size;
                for (i = ti->vn->l2index + 1; i < size ; i++) {
                    int temp = lower32At(ti->vn->l2Buffer,i);
                    if (temp != 0xffffffff) {
                        ti->lcUpper = temp - 1;
                        break;
                    }
                }
            }
            sp = ti->index + 1;
        } // check for l3lower and l3upper

        if (ti->lcLower != -1) { // at least one child element
            int temp1 = intAt(ti->vn->l3Buffer,ti->lcLower);
            int temp2 = intAt(ti->vn->l3Buffer,ti->lcUpper);
            ti->lcIndex = (ti->lcIndex != -1) ? ti->lcIndex : ti->lcLower;
            while (sp < ti->vn->vtdSize) {
                int s = intAt(ti->vn->l3Buffer,ti->lcIndex);
                //int s = vn.l2Buffer.upper32At(lcIndex);
                if (sp >= temp1 && sp < temp2) {
                    if (sp == s) {
                        ti->lcIndex++;
                        sp = intAt(ti->vn->l3Buffer,ti->lcIndex) - 1;
                        //boolean b = false;
                        while (getTokenDepth(ti->vn,sp) == 2) {
                            sp--;
                          //  b = true;
                        }
                        //if (b)
                        	sp++;
                        //continue;
                    }
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else if (sp < temp1) {
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else {
                    //if (sp == temp2) { // last child element
                    //} else                 
                    if ( isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp) == 2) {
                        ti->prevLocation = sp;
                        return sp;
                    } else if ((getTokenType(ti->vn,sp)==TOKEN_STARTING_TAG
                            && getTokenDepth(ti->vn,sp) < 3 ) || getTokenDepth(ti->vn,sp)<2) {
                        break;
                    }
                    sp++;
                }
            }
            //prevLocation = vtdSize-1;
            return -1;
        } else { // no child elements
			int d, type;
            if (sp>=ti->vn->vtdSize) return -1;
            d = getTokenDepth(ti->vn,sp);
            type = getTokenType(ti->vn,sp);
            while (sp < ti->vn->vtdSize
                && d >= 2
                && !(d == 2 && type == TOKEN_STARTING_TAG)) {
                // the last condition indicates the start of the next sibling element
                if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                    ti->prevLocation = sp;
                    return sp;
                }
                sp++;
                d = getTokenDepth(ti->vn,sp);
                type = getTokenType(ti->vn,sp);
                
            }
            //prevLocation = vtdSize-1;
            return -1;
        }
}

//static int handleDefault(TextIter *ti){
//         //int curDepth = vn.context[0];
//		int d, type;
//        int sp = (ti->prevLocation != -1) ? increment(ti,ti->prevLocation): ti->index + 1;
//        if (sp>=ti->vn->vtdSize) return -1;
//        d = getTokenDepth(ti->vn,sp);
//        type = getTokenType(ti->vn,sp);
//        while (d >= ti->depth
//            && !(d == ti->depth && type == TOKEN_STARTING_TAG)) {
//            if (isText(ti,sp) == TRUE && d == ti->depth) {
//                ti->prevLocation = sp;
//                return sp;
//            }
//            sp++;
//            if(sp >= ti->vn->vtdSize)
//              return -1;
//
//            d = getTokenDepth(ti->vn,sp);
//            type = getTokenType(ti->vn,sp);                
//        }
//        return -1;
//}

/* Obtain the current navigation position and element info from VTDNav.
 * So one can instantiate it once and use it for many different elements */
void touch(TextIter *ti, VTDNav *v){
	/*if (v == NULL || ti==NULL){
		e.et = invalid_argument;
		e.msg = "Touch failed VTDNav instance can't be null";
		Throw e;
	}*/     

	 ti->depth = v->context[0];
	 if (ti->depth == -1)
		 ti->index = 0;
	 else 
         ti->index = (ti->depth != 0) ? v->context[ti->depth] : v->rootIndex;

     ti->vn = v;
     ti->prevLocation = -1;
     ti->lcIndex = -1;
     ti->lcUpper = -1;
     ti->lcLower = -1;
}

/* Get the index vals for the text nodes in document order.*/
int getNext(TextIter *ti){
	if (ti->vn == NULL)
		throwException2(invalid_argument, "VTDNav instance can't be null");
	if (ti->vn->shallowDepth)
		switch (ti->depth) {
		case -1: 
			return handleDocumentNode(ti);
		case 0 :
			return handleLevel0(ti);
		case 1 :
			return handleLevel1(ti);
		case 2 :
			return handleLevel2(ti);
		default :
			return handleDefault(ti);
	}
	else{
		switch (ti->depth) {
			case -1:
				return handleDocumentNode(ti);
			case 0:
				return handleLevel0(ti);
			case 1:
				return handleLevel1(ti);
			case 2:
				return _handleLevel2(ti);
			case 3:
				return handleLevel3(ti);
			case 4:
				return handleLevel4(ti);
			default:
				return handleDefault(ti);
		}
	}
}


/* Ask textIter to return character data or CDATA nodes*/
void selectText(TextIter *ti){
	ti->sel_type = 0;
}
/*  Ask textIter to return comment nodes*/
void selectComment(TextIter *ti){
	ti->sel_type = 1;
}
/* Ask TextIter to return processing instruction name 
 * no value */
void selectPI0(TextIter *ti){
	ti->sel_type = 2;
}
/* Ask TextIter to return processing instruction of 
given name */
void selectPI1(TextIter *ti, UCSChar *s){
	ti->sel_type =3;
	ti->piName = s;
}


static int _handleLevel2(TextIter *ti){
       	int sp,i,temp1, temp2,d,size;
		tokenType type;
       	VTDNav_L5* vnl = (VTDNav_L5 *)ti->vn;
        if (ti->prevLocation != -1) {
            sp = increment(ti,ti->prevLocation);
        } else {
            // fetch lclower and lcupper
            ti->lcLower = lower32At(vnl->l2Buffer,vnl->l2index);
            if (ti->lcLower != -1) {
                ti->lcUpper = vnl->l3Buffer->size - 1;
                size = vnl->l2Buffer->size;
                for (i = vnl->l2index + 1; i < size ; i++) {
                    int temp = lower32At(vnl->l2Buffer,i);
                    if (temp != -1) {
                        ti->lcUpper = temp - 1;
                        break;
                    }
                }
            }
            sp = ti->index + 1;
        } // check for l3lower and l3upper

        if (ti->lcLower != -1) { // at least one child element
            temp1 = upper32At(vnl->_l3Buffer,ti->lcLower);
            temp2 = upper32At(vnl->_l3Buffer,ti->lcUpper);
            ti->lcIndex = (ti->lcIndex != -1) ? ti->lcIndex : ti->lcLower;
            while (sp < vnl->vtdSize) {
                int s = upper32At(vnl->_l3Buffer,ti->lcIndex);
                //int s = vn.l2Buffer.upper32At(lcIndex);
                if (sp >= temp1 && sp < temp2) {
                    if (sp == s) {
                        ti->lcIndex++;
                        sp = upper32At(vnl->_l3Buffer,ti->lcIndex) - 1;
                        //boolean b = false;
						while (getTokenDepth(ti->vn,sp) == 2) {
                            sp--;
                          //  b = true;
                        }
                        //if (b)
                        	sp++;
                        //continue;
                    }
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else if (sp < temp1) {
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else {
                    //if (sp == temp2) { // last child element
                    //} else                 
                    if ( isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp) == 2) {
                        ti->prevLocation = sp;
                        return sp;
                    } else if ((getTokenType(ti->vn,sp)==TOKEN_STARTING_TAG
                            && getTokenDepth(ti->vn,sp) < 3 ) || getTokenDepth(ti->vn,sp)<2) {
                        break;
                    }
                    sp++;
                }
            }
            //prevLocation = vtdSize-1;
            return -1;
        } else { // no child elements
            if (sp>=vnl->vtdSize) return -1;
            d = getTokenDepth(ti->vn,sp);
            type = getTokenType(ti->vn,sp);
            while (sp < vnl->vtdSize
                && d >= 2
                && !(d == 2 && type == TOKEN_STARTING_TAG)) {
                // the last condition indicates the start of the next sibling element
                if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==2) {
                    ti->prevLocation = sp;
                    return sp;
                }
                sp++;
                d = getTokenDepth(ti->vn,sp);
                type = getTokenType(ti->vn,sp);                
            }
            //prevLocation = vtdSize-1;
            return -1;
        }
}

static int handleLevel3(TextIter *ti){
    	
       	int sp,d,temp1,temp2,i,size;
		tokenType type;
       	VTDNav_L5* vnl = (VTDNav_L5*)ti->vn;
        if (ti->prevLocation != -1) {
            sp = increment(ti,ti->prevLocation);
        } else {
            // fetch lclower and lcupper
            ti->lcLower = lower32At(vnl->_l3Buffer,vnl->l3index);
            if (ti->lcLower != -1) {
                ti->lcUpper = vnl->l4Buffer->size - 1;
                size = vnl->_l3Buffer->size;
                for (i = vnl->l3index + 1; i < size ; i++) {
                    int temp = lower32At(vnl->_l3Buffer,i);
                    if (temp != -1) {
                        ti->lcUpper = temp - 1;
                        break;
                    }
                }
            }
            sp = ti->index + 1;
        } // check for l3lower and l3upper
        if (ti->lcLower != -1) { // at least one child element
            temp1 = upper32At(vnl->l4Buffer,ti->lcLower);
            temp2 = upper32At(vnl->l4Buffer,ti->lcUpper);
            ti->lcIndex = (ti->lcIndex != -1) ? ti->lcIndex : ti->lcLower;
            while (sp < vnl->vtdSize) {
                int s = upper32At(vnl->l4Buffer,ti->lcIndex);
                //int s = vn.l2Buffer.upper32At(lcIndex);
                if (sp >= temp1 && sp < temp2) {
                    if (sp == s) {
                        ti->lcIndex++;
                        sp = upper32At(vnl->l4Buffer,ti->lcIndex) - 1;
                        //boolean b = false;
                        while (getTokenDepth(ti->vn,sp) == 2) {
                            sp--;
                          //  b = true;
                        }
                        //if (b)
                        	sp++;
                        //continue;
                    }
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==3) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else if (sp < temp1) {
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==3) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else {
                    //if (sp == temp2) { // last child element
                    //} else                 
                    if ( isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp) == 3) {
                        ti->prevLocation = sp;
                        return sp;
                    } else if ((getTokenType(ti->vn,sp)==TOKEN_STARTING_TAG
                            &&getTokenDepth(ti->vn,sp) < 4 ) ||getTokenDepth(ti->vn,sp)<3) {
                        break;
                    }
                    sp++;
                }
            }
            //prevLocation = vtdSize-1;
            return -1;
        } else { // no child elements
            if (sp>= vnl->vtdSize) return -1;
            d = getTokenDepth(ti->vn,sp);
            type = getTokenType(ti->vn,sp);
            while (sp < vnl->vtdSize
                && d >= 3
                && !(d == 3 && type == TOKEN_STARTING_TAG)) {
                // the last condition indicates the start of the next sibling element
                if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==3) {
                    ti->prevLocation = sp;
                    return sp;
                }
                sp++;
                d = getTokenDepth(ti->vn,sp);
                type =getTokenType( ti->vn,sp);
                
            }
            //prevLocation = vtdSize-1;
            return -1;
        }
}

static int handleLevel4(TextIter *ti){
   		int sp,temp1,temp2,d,i,size;
		tokenType type;
    	VTDNav_L5* vnl = (VTDNav_L5 *)ti->vn;
        if (ti->prevLocation != -1) {
            sp = increment(ti, ti->prevLocation);
        } else {
            // fetch lclower and lcupper
            ti->lcLower = lower32At(vnl->l4Buffer,vnl->l4index);
            if (ti->lcLower != -1) {
                ti->lcUpper = vnl->l5Buffer->size - 1; //5
                size = vnl->l4Buffer->size; //4
                for (i = vnl->l4index + 1; i < size ; i++) {//4
                    int temp = lower32At(vnl->l4Buffer,i); //4
                    if (temp != -1) {
                        ti->lcUpper = temp - 1;
                        break;
                    }
                }
            }
            sp = ti->index + 1;
        } // check for l3lower and l3upper

        if (ti->lcLower != -1) { // at least one child element
            temp1 = intAt(vnl->l5Buffer,ti->lcLower);
            temp2 = intAt(vnl->l5Buffer,ti->lcUpper);
            ti->lcIndex = (ti->lcIndex != -1) ? ti->lcIndex : ti->lcLower;
            while (sp < ti->vn->vtdSize) {
                int s = intAt(vnl->l5Buffer,ti->lcIndex);
                //int s = vn.l2Buffer.upper32At(lcIndex);
                if (sp >= temp1 && sp < temp2) {
                    if (sp == s) {
                        ti->lcIndex++;
                        sp = intAt(vnl->l5Buffer,ti->lcIndex) - 1;
                        //boolean b = false;
                        while (getTokenDepth(ti->vn,sp) == 4) {
                            sp--;
                          //  b = true;
                        }
                        //if (b)
                        	sp++;
                        //continue;
                    }
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==4) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else if (sp < temp1) {
                    if (isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp)==4) {
                        ti->prevLocation = sp;
                        return sp;
                    }
                    sp++;
                } else {
                    //if (sp == temp2) { // last child element
                    //} else                 
                    if ( isText(ti,sp) == TRUE && getTokenDepth(ti->vn,sp) == 4) {
                        ti->prevLocation = sp;
                        return sp;
                    } else if ((getTokenType(ti->vn,sp)== TOKEN_STARTING_TAG
                            && getTokenDepth(ti->vn,sp) < 5 ) || getTokenDepth(ti->vn,sp)<4) {
                        break;
                    }
                    sp++;
                }
            }
            //prevLocation = vtdSize-1;
            return -1;
        } else { // no child elements
            if (sp>=vnl->vtdSize) return -1;
            d = getTokenDepth(ti->vn,sp);
            type = getTokenType(ti->vn,sp);
            while (sp < vnl->vtdSize
                && d >= 4
                && !(d == 4 && type == TOKEN_STARTING_TAG)) {
                // the last condition indicates the start of the next sibling element
                if (isText(ti,sp) == TRUE &&getTokenDepth(ti->vn,sp)== 4) {
                    ti->prevLocation = sp;
                    return sp;
                }
                sp++;
                d = getTokenDepth(ti->vn,sp);
                type = getTokenType(ti->vn,sp);
                
            }
            //prevLocation = vtdSize-1;
            return -1;
        }
}

