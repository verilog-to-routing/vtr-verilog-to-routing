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
#include "bookMark.h"
#include "vtdNav.h"

BookMark *createBookMark(){
	BookMark *bm = (BookMark*) malloc(sizeof(BookMark));
	if (bm==NULL) {
		throwException2(out_of_mem,
			"BookMark allocation failed ");
	}
	bm->ba = NULL;
	bm->ba_len = -1;
	bm->vn1 = NULL;
	return bm;
}
BookMark *createBookMark2(VTDNav *vn){
	BookMark *bm = (BookMark*) malloc(sizeof(BookMark));
	if (bm==NULL) {
		throwException2(out_of_mem,
			"BookMark allocation failed ");
	}
	bm->ba = NULL;
	bm->ba_len = -1;
	bm->vn1 = NULL;

	bind4BookMark(bm,vn);
	recordCursorPosition2(bm);

	return bm;
}

void freeBookMark(BookMark *bm){
	free(bm->ba);
	free(bm);
}
void unbind4BookMark(BookMark *bm){
	bm->vn1 = NULL;
}
void bind4BookMark(BookMark *bm, VTDNav *vn){
		if (vn==NULL)
            throwException2(invalid_argument,"vn can't be null");
        bm->vn1 = vn;
		if (vn->shallowDepth){
			if (bm->ba == NULL || vn->nestingLevel+8 != bm->ba_len){
				if (vn->nestingLevel+8 != bm->ba_len){
					free(bm->ba);
				}
				bm->ba = malloc(sizeof(int)*(vn->nestingLevel + 8)); 
				if (bm->ba == NULL){
					throwException2(out_of_mem,
						"BookMark.ba allocation failed ");
				}
			}
		}else{
			if (bm->ba == NULL || vn->nestingLevel+8 != bm->ba_len){
				if (vn->nestingLevel+14 != bm->ba_len){
					free(bm->ba);
				}
				bm->ba = malloc(sizeof(int)*(vn->nestingLevel + 14)); 
				if (bm->ba == NULL){
					throwException2(out_of_mem,
						"BookMark.ba allocation failed ");
				}
			}
		}
        bm->ba[0]= -2 ; // this would never happen in a VTDNav obj's context
}
VTDNav* getNav4BookMark(BookMark *bm){
	return bm->vn1;
}
Boolean setCursorPosition(BookMark *bm, VTDNav *vn){
	int i=0;
	if (bm->vn1 != vn || bm->ba == NULL || bm->ba[0] == -2)
		return FALSE;
	for (i = 0; i < vn->nestingLevel; i++) {
		vn->context[i] = bm->ba[i];
	}

	if (vn->shallowDepth){
		vn->l1index = bm->ba[vn->nestingLevel];
		vn->l2index = bm->ba[vn->nestingLevel + 1];
		vn->l3index = bm->ba[vn->nestingLevel + 2];
		vn->l2lower = bm->ba[vn->nestingLevel + 3];
		vn->l2upper = bm->ba[vn->nestingLevel + 4];
		vn->l3lower = bm->ba[vn->nestingLevel + 5];
		vn->l3upper = bm->ba[vn->nestingLevel + 6];
		if (bm->ba[vn->nestingLevel+7] < 0){
			vn->atTerminal = TRUE;		    
		} else
			vn->atTerminal = FALSE;

		vn->LN = bm->ba[vn->nestingLevel+7] & 0x7fffffff;
	}else{
		VTDNav_L5 *vnl = (VTDNav_L5 *)vn;
		vnl->l1index = bm->ba[vn->nestingLevel];
		vnl->l2index = bm->ba[vn->nestingLevel + 1];
		vnl->l3index = bm->ba[vn->nestingLevel + 2];
		vnl->l4index = bm->ba[vn->nestingLevel + 3];
		vnl->l5index = bm->ba[vn->nestingLevel + 4];
		vnl->l2lower = bm->ba[vn->nestingLevel + 5];
		vnl->l2upper = bm->ba[vn->nestingLevel + 6];
		vnl->l3lower = bm->ba[vn->nestingLevel + 7];
		vnl->l3upper = bm->ba[vn->nestingLevel + 8];
		vnl->l4lower = bm->ba[vn->nestingLevel + 9];
		vnl->l4upper = bm->ba[vn->nestingLevel + 10];
		vnl->l4lower = bm->ba[vn->nestingLevel + 11];
		vnl->l4upper = bm->ba[vn->nestingLevel + 12];
		if (bm->ba[vn->nestingLevel+13] < 0){
			vn->atTerminal = TRUE;		    
		} else
			vn->atTerminal = FALSE;

		vn->LN = bm->ba[vn->nestingLevel+13] & 0x7fffffff;
	}
	return TRUE;
}
Boolean setCursorPosition2(BookMark *bm){
	return setCursorPosition(bm,bm->vn1);
}
/**
* Record the cursor position
* This method is implemented to be lenient on loading in
* that it can load nodes from any VTDNav object
* if vn is null, return false
*/
Boolean recordCursorPosition(BookMark *bm, VTDNav *vn){
	int i;
	if (vn == NULL)
		return FALSE;
	if (vn== bm->vn1){
	}else {
		bind4BookMark(bm,vn);
	}
	for (i = 0; i < vn->nestingLevel; i++) {
		bm->ba[i] = bm->vn1->context[i];
	}

	if (vn->shallowDepth){
		bm->ba[vn->nestingLevel]= vn->l1index ;
		bm->ba[vn->nestingLevel + 1]= vn->l2index ;
		bm->ba[vn->nestingLevel + 2]= vn->l3index ;
		bm->ba[vn->nestingLevel + 3]= vn->l2lower ;
		bm->ba[vn->nestingLevel + 4]= vn->l2upper ;
		bm->ba[vn->nestingLevel + 5]= vn->l3lower ;
		bm->ba[vn->nestingLevel + 6]= vn->l3upper ;
		//ba[vn.nestingLevel + 7]=(vn.atTerminal == true)?1:0;
		bm->ba[vn->nestingLevel + 7]= 
			(vn->atTerminal == TRUE)? 
			(vn->LN | 0x80000000) : vn->LN ;
	}
	else{
		VTDNav_L5 *vnl = (VTDNav_L5 *)vn;
		bm->ba[vn->nestingLevel]= vnl->l1index ;
		bm->ba[vn->nestingLevel + 1]= vnl->l2index ;
		bm->ba[vn->nestingLevel + 2]= vnl->l3index ;
		bm->ba[vn->nestingLevel + 3]= vnl->l4index ;
		bm->ba[vn->nestingLevel + 4]= vnl->l5index ;
		bm->ba[vn->nestingLevel + 5]= vnl->l2lower ;
		bm->ba[vn->nestingLevel + 6]= vnl->l2upper ;
		bm->ba[vn->nestingLevel + 7]= vnl->l3lower ;
		bm->ba[vn->nestingLevel + 8]= vnl->l3upper ;
		bm->ba[vn->nestingLevel + 9]= vnl->l4lower ;
		bm->ba[vn->nestingLevel + 10]= vnl->l4upper ;
		bm->ba[vn->nestingLevel + 11]= vnl->l5lower ;
		bm->ba[vn->nestingLevel + 12]= vnl->l5upper ;
		bm->ba[vn->nestingLevel + 13]= 
			(vn->atTerminal == TRUE)? 
			(vn->LN | 0x80000000) : vn->LN ;

	}
	return TRUE;
}

/**
* Record cursor position of the VTDNav object as embedded in the
* bookmark
*/
Boolean recordCursorPosition2(BookMark *bm){
	return recordCursorPosition(bm,bm->vn1);
}

/**
* Compare the bookmarks to ensure they represent the same
* node in the same VTDnav instance
*/
Boolean equal4BookMark(BookMark *bm1, BookMark *bm2){
	if (bm1 == bm2)
		return TRUE;
	return deepEqual4BookMark(bm1,bm2);
}
/**
* Returns the hash code which is a unique integer for every node
*/
int hashCode4BookMark(BookMark *bm){
	if (bm->ba == NULL || bm->vn1==NULL || bm->ba[0]==-2)
		return -2;
	if (bm->vn1->atTerminal)
		return bm->vn1->LN;
	if (bm->ba[0]==1)
		return bm->vn1->rootIndex;
	return bm->ba[bm->ba[0]];
}
/**
* Compare the bookmarks to ensure they represent the same
* node in the same VTDnav instance
*/
   
Boolean deepEqual4BookMark(BookMark *bm1, BookMark *bm2){
	if (bm2->vn1 == bm1->vn1){
		if (bm2->ba[bm2->ba[0]]==bm1->ba[bm1->ba[0]]){
			if (bm1->vn1->shallowDepth){
				if (bm1->ba[bm1->vn1->nestingLevel+7] < 0){
					if (bm1->ba[bm1->vn1->nestingLevel+7]
					!= bm2->ba[bm1->vn1->nestingLevel+7])
						return FALSE;
				}
			}else{
				if (bm1->ba[bm1->vn1->nestingLevel+13] < 0){
					if (bm1->ba[bm1->vn1->nestingLevel+13]
					!= bm2->ba[bm1->vn1->nestingLevel+13])
						return FALSE;
				}
			}
			return TRUE;
		}
	}
	return FALSE;
}