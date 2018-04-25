/*
 * Copyright (c) 2006 Peter Tummeltshammer for the Spiral project (www.spiral.net)
 * Copyright (c) 2006 Carnegie Mellon University
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
   kmult.c - Merging several Addition chains into one, petertu, 17.9.2003
*/

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "table.h"
#include "chains.h"
#include "time.h"

typedef int          boolean;

#define True  1
#define False 0
#define MAX_AUs 6   //maximum amount of adds/subs in one DAG
#define MAX_DAGs 35  //maximum number of DAGs being merged
#define MAX_CONSTBITWIDTH 20  //maximum bitwidth for constants

//constants for precalculating the designs' size
#define ADDSIZE    67.5625
#define SUBSIZE    75.25
#define ADDSUBSIZE 97.5
#define MUXSIZE    28.65625
#define MUXPLUS    14.34375


static int VL_INPUTBITWIDTH = 16;
static int VL_CONSTBITWIDTH = 8;
static int VL_FRACTIONBITWIDTH = 8;
static int USE_RANDOM_DAGs = False;
static int USE_TABLE = False;
static int USE_RANDOM_DAGORDER = False;
static int VERBOSE = False;
static int OPTIMIZATION = 0;
static int RANDOM_SEARCH_SIZE = 10000;  // maximum number of rand. combinations
                                        // being searched in function reorderdags


#define max(x,y) (x <= y ? y : x)
#define min(x,y) (x <= y ? x : y)
#define even(x) (((x) & 1) == 0)

#define RType(R) (R) //((R) & 7)
typedef enum { RI, RT, RA, RS, RAS, RM, RLAST } rtype_t;
//             0   1   2   3   4    5   6

/* definition of special dag for parsing results */
typedef struct _dagnode{
  rtype_t type;
  int dagindex;
  int au_type[MAX_DAGs+1];
  int fundamental;
  int fundamentals[MAX_DAGs];
  int shift;
  int connections;
  boolean visited;
  struct _dagnode *leftnode;
  struct _dagnode *rightnode;
  struct _dagnode *mux[MAX_DAGs+1];
  struct _dagnode *copy;
  struct _costobject *nodecost;
} dagnode;

/* definition of return object for getcost */
typedef struct _costobject{
  int shift;
  int fundamental;
} costobject;

static char RString[6][10];
static char Colors[17][15];

/* Print realization
   =================
*/

static int maxShift, maxIndex;
#define       max_subs 100
static int n_subs;
static int sum_subs[RLAST];
static int   subs[max_subs];
static boolean temp_subs;
static dagnode   *t_subs[max_subs];


/* builddag-functions */
static inline int t_index_subs(int x) {
    int i;

    for (i = 0; i < n_subs; ++i)
        if (t_subs[i]->fundamental == x)
            return (i);
    return (-1);
}

dagnode * create_dagnode(rtype_t type, int index, int fund, int shift,
             dagnode *lft, dagnode *rt) {
    dagnode *res = (dagnode*) malloc(sizeof(dagnode));
    res->type = type;
    res->dagindex = index;
    res->fundamental = fund;
    res->shift = shift;
    res->connections = 1;
    res->visited = False;
    res->leftnode = lft;
    res->rightnode = rt;
    for(int i = 0;i < MAX_DAGs;i++){
        res->au_type[i] = -1;
        res->mux[i] = NULL;
        res->fundamentals[i] = -1;
    }
    res->fundamentals[index] = fund;
    res->copy = NULL;
    res->nodecost = NULL;
    if(type!=RI && type!=RT)
    res->au_type[index] = type;
    ++sum_subs[type];
    /* save new dag in common subexpression table */
    t_subs[n_subs++] = res;
    return res;
}


std::map<reg_t, dagnode*> REGS;
int REGNUM = 0;

int tmpreg_k() {
    REGS[REGNUM] = NULL;
    return REGNUM++;
}

dagnode *lookup_cse(int x) {
    int curr_subs;
    if ((curr_subs = t_index_subs(x)) >= 0) {
        dagnode *mydag = t_subs[curr_subs];
    mydag->connections++;
        return mydag;
    }
    else return NULL;
}

  //void icode_chain(chain *c, IcodeList *iclist, reg_t dest, reg_t src) {
static dagnode *ttR1(int x, int index, reg_t dest, reg_t src) {
    dagnode *d;
    int sh;

    if(x == 1)
        return (REGS[dest] = REGS[src]);

    if((sh = compute_shr(x)) != 0) /* not an odd number */
    return (REGS[dest] =
        create_dagnode(RT, index, x, sh, ttR1((x>>sh), index, tmpreg_k(), src), NULL));

    else {
    oplist_t oplist;
    chain * c;
    if(USE_TABLE) {
        c = get_chain_table(x);
				if(c==NULL) { //Table too small - backup solution
					set_chains(&x,1,1);
					c = get_chain(x,0);
					delete_chain();
				}
        assert(c!=NULL && "table-returned chain is NULL!");
        if(VERBOSE) c->output(cout);
    } else {
        c = get_chain(x,index);
        assert(c!=NULL && "returned chain is NULL!");
        if(VERBOSE) c->output(cout);
    }
    c->get_ops(oplist, dest, src, tmpreg_k);
    dagnode *ddest = NULL;

    for(oplist_t::iterator i = oplist.begin(); i != oplist.end(); ++i) {
        op * o = *i; /* dereference the iterator */
        dagnode  *r1, *r2;
        int xr1, xr2;

        r1  = REGS[o->r1];
        xr1 = r1->fundamental;
        if(o->type == op::ADD || o->type == op::SUB) {
        r2  = REGS[o->r2];
        xr2 = r2->fundamental;
        }
        switch(o->type) {
        case op::ADD:  d = create_dagnode(RA, index, xr1+xr2, 0, r1, r2); break;
        case op::SUB:  d = create_dagnode(RS, index, xr1-xr2, 0, r1, r2); break;
        case op::SHL:  d = create_dagnode(RT, index, (xr1 << o->sh), o->sh, r1, NULL); break;
        case op::CMUL: d = ttR1(o->c, index, o->res, o->r1); break;
        case op::SHR: fprintf(stderr, "Offending number=%d\n", x);
                  assert("Right-shifts are not supported");
        default: assert(0);
        }
        REGS[o->res] = d;
        if(dest == o->res) ddest = d;
    }
    if(ddest == 0)
        assert(0 && "Something got screwed up: 'dest' register was never written into");
    return ddest;
    }
}


dagnode *builddag(int x, int dagindex) {
    n_subs = 0;
    for(int i=0; i<RLAST; ++i)
        sum_subs[i] = 0;
    reg_t dest = tmpreg_k();
    reg_t src = tmpreg_k();
    REGS[src] = create_dagnode(RI, dagindex, 1, 0, NULL, NULL);
    return ttR1(x, dagindex, dest, src);
}


void checkdag(dagnode *mydag) {
    int i;

    {
//      printf("No: %d, ",mydag->dagindex);
      printf("Address: %d, ",mydag);
      printf("Type: %d, ",mydag->type);
      printf("Index: %d, ",mydag->fundamental);
      printf("Shift: %d, ",mydag->shift);
      printf("Connections: %d,",mydag->connections);
      printf("Visited: %d\n",mydag->visited);
      fflush(stdout);
    }

    if (mydag->leftnode != NULL)
      checkdag(mydag->leftnode);
    if (mydag->rightnode != NULL)
      checkdag(mydag->rightnode);
    if (mydag->type == RM)
        for(i=0;i<MAX_DAGs;i++)
        if(mydag->mux[i] != NULL)
            checkdag(mydag->mux[i]);
}


int indexNumber = 0; //has to be global :(
void setindices(dagnode *mydag) {
    int i;
    if(mydag->visited == False) {
        mydag->visited = True;
        mydag->dagindex = indexNumber++;
        if (mydag->leftnode != NULL)
        		setindices(mydag->leftnode);
        if (mydag->rightnode != NULL)
        		setindices(mydag->rightnode);
		    if (mydag->type == RM)
		        for(i=0;i<MAX_DAGs;i++)
		            if(mydag->mux[i] != NULL)
		            	setindices(mydag->mux[i]);

		}

}


/* Sets the visited-state to false for all Nodes */
void resetvisitedstate(dagnode *mydag) {
    int i;
    if (mydag->visited == True) {
        mydag->visited = False;
        if (mydag->leftnode != NULL)
        resetvisitedstate(mydag->leftnode);
    if (mydag->rightnode != NULL)
        resetvisitedstate(mydag->rightnode);
    if (mydag->type == RM)
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
            resetvisitedstate(mydag->mux[i]);
    }
}

/* Returnes a copy of mydag */
dagnode *copydag(dagnode *mydag) {
    int i;
    dagnode *leftdag = NULL;
    dagnode *rightdag = NULL;
    dagnode *returndag;

    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            leftdag =  copydag(mydag->leftnode);
        if (mydag->rightnode != NULL)
            rightdag =  copydag(mydag->rightnode);

        returndag  = (struct _dagnode *) malloc(sizeof(struct _dagnode));
        returndag->type = mydag->type;
        returndag->dagindex = mydag->dagindex;
        returndag->fundamental = mydag->fundamental;
        returndag->shift = mydag->shift;
        returndag->connections = mydag->connections;
        returndag->visited = False;
        returndag->leftnode = leftdag;
        returndag->rightnode = rightdag;
        for(i = 0;i < MAX_DAGs;i++) {
            returndag->au_type[i] = mydag->au_type[i];
            returndag->mux[i] = NULL;
            returndag->fundamentals[i] = mydag->fundamentals[i];
        }
        mydag->copy = returndag;
        mydag->nodecost = NULL;
        return returndag;
    }
    return mydag->copy;
}




/* Deletes a DAG */
void killdag(dagnode *mydag) {
    int i;
    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            killdag(mydag->leftnode);
        if (mydag->rightnode != NULL)
            killdag(mydag->rightnode);
        if (mydag->type == RM)
            for(i=0;i<MAX_DAGs;i++)
                if(mydag->mux[i] != NULL)
                    killdag(mydag->mux[i]);
        free(mydag);
    }
}

int countmuxes(dagnode *mydag) {
int result = 0;
int i;
    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            result += countmuxes(mydag->leftnode);
        if (mydag->rightnode != NULL)
            result += countmuxes(mydag->rightnode);
        if (mydag->type == RM)
            result--;
            for(i=0;i<MAX_DAGs;i++)
                if(mydag->mux[i] != NULL) {
                    result += countmuxes(mydag->mux[i]);
                    result++;
                }
    }
    return result;
}


/* removes useless muxes from DAG */
dagnode *removeuselessmuxes(dagnode *mydag) {
    int i,j,muxmakessense,shift;
    dagnode *goon, *target = NULL;
    dagnode *tempdag = NULL;
    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            mydag->leftnode = removeuselessmuxes(mydag->leftnode);
        if (mydag->rightnode != NULL)
            mydag->rightnode = removeuselessmuxes(mydag->rightnode);
        if (mydag->type == RM) {
/*            for(i=0;i<MAX_DAGs;i++)
                if(mydag->mux[i] != NULL)
                    for(j=0;j<i;j++)
                        if(mydag->mux[j] != NULL)
                            if((mydag->mux[i]->type == RT) && (mydag->mux[j]->type == RT)){
                                if((mydag->mux[i]->shift == mydag->mux[j]->shift) &&
                                   (mydag->mux[i]->leftnode == mydag->mux[j]->leftnode))
                                    mydag->mux[j] = NULL;
                              } else
                                if(mydag->mux[i] == mydag->mux[j])
                                    mydag->mux[j] = NULL;
  */
            muxmakessense = 0;
            for(i=0;i<MAX_DAGs;i++)
                if(mydag->mux[i] != NULL)
                    if(target == NULL) {
                    		if(mydag->mux[i]->type == RT) {
                    			shift = mydag->mux[i]->shift;
                        	target = mydag->mux[i]->leftnode;
                        } else {
                        	shift = 0;
                        	target = mydag->mux[i];
                        }
                    } else {
                   			if(mydag->mux[i]->type == RT) {
                   				if(shift != mydag->mux[i]->shift || target != mydag->mux[i]->leftnode)
                   					muxmakessense = 1;
                   			} else {
                   				if(shift != 0 || target != mydag->mux[i])
                   					muxmakessense = 1;
                   			}
                   	}

            if(muxmakessense == 0) {
                tempdag = mydag;
            		for(i=0;i<MAX_DAGs;i++)
                	if(mydag->mux[i] != NULL)
                		mydag = mydag->mux[i];
                free(tempdag);
                removeuselessmuxes(mydag);
            } else
                for(i=0;i<MAX_DAGs;i++)
                    if(mydag->mux[i] != NULL)
                        mydag->mux[i] = removeuselessmuxes(mydag->mux[i]);
        }
    }
    return mydag;
}



/* recursively draws the DAG */
void rdd(dagnode *mydag,FILE * out) {
    int i,j;
    bool useful;
    if(mydag->visited == False) {
        mydag->visited = True;
    if (mydag->type == RM) {
        fprintf(out,"  node%d [shape=polygon,sides=4,distortion=.7];\n",mydag->dagindex);
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
                if(mydag->mux[i]->type == RT) {
                		useful = true;
                		for(j=0;j<i;j++)
                			if(mydag->mux[j] != NULL)
                				if(mydag->mux[j]->type == RT)
                					if(mydag->mux[j]->shift == mydag->mux[i]->shift && mydag->mux[j]->leftnode == mydag->mux[i]->leftnode)
                						useful = false;
                		if(useful)
                				if(mydag->mux[i]->shift == MAX_CONSTBITWIDTH+5) { //replacing for 0
                						fprintf(out,"  node%d [shape=plaintext,label=\"0\"];\n",mydag->mux[i]->dagindex);
                						fprintf(out,"  node%d -> node%d;\n",mydag->mux[i]->dagindex, mydag->dagindex);
                				}
                				else {
                    				fprintf(out,"  node%d -> node%d [label=\"%d\"];\n",mydag->mux[i]->leftnode->dagindex, mydag->dagindex,mydag->mux[i]->shift);
                    				mydag->mux[i]->visited = True;
                    				rdd(mydag->mux[i]->leftnode,out);
                  			}
                } else {
                		useful = true;
                		for(j=0;j<i;j++)
                			if(mydag->mux[j] != NULL)
                					if(mydag->mux[j] == mydag->mux[i])
                						useful = false;
                		if(useful)
                    	fprintf(out,"  node%d -> node%d;\n",mydag->mux[i]->dagindex, mydag->dagindex);
                    rdd(mydag->mux[i],out);
                }
    }
    if(mydag->type == RT && mydag->shift == MAX_CONSTBITWIDTH+5) {//0 was the only constant specified
    		fprintf(out,"  node%d [shape=plaintext,label=\"0\"];\n",mydag->dagindex);
    		return;
    }
    if (mydag->leftnode != NULL) {
        if(mydag->leftnode->type == RT) {
            fprintf(out,"  node%d -> node%d [label=\"%d\"];\n",mydag->leftnode->leftnode->dagindex, mydag->dagindex,mydag->leftnode->shift);
            mydag->leftnode->visited = True;
            rdd(mydag->leftnode->leftnode,out);
        } else {
            fprintf(out,"  node%d -> node%d;\n",mydag->leftnode->dagindex, mydag->dagindex);
            rdd(mydag->leftnode,out);
        }
    }
    if (mydag->rightnode != NULL) {
        if(mydag->rightnode->type == RT) {
            fprintf(out,"  node%d -> node%d [label=\"%d\"];\n",mydag->rightnode->leftnode->dagindex, mydag->dagindex,mydag->rightnode->shift);
            mydag->rightnode->visited = true;
            rdd(mydag->rightnode->leftnode,out);
        } else {
            fprintf(out,"  node%d -> node%d;\n",mydag->rightnode->dagindex, mydag->dagindex);
            rdd(mydag->rightnode,out);
        }
    }
    if(mydag->type == RT){
        fprintf(out,"  node%d [label=\"%s %d\"];\n",mydag->dagindex,RString[mydag->type],mydag->shift);
    } else {
        if(mydag->type == RA || mydag->type == RS || mydag->type == RAS) {
            fprintf(out,"  node%d [label=\"%s",mydag->dagindex,RString[mydag->type]);
            //for(i=0;i<MAX_DAGs;i++)
            //    if(mydag->fundamentals[i] != -1)
            //        fprintf(out,", f%d=%d",i,mydag->fundamentals[i]);
            fprintf(out,"\"];\n");
        } else
            fprintf(out,"  node%d [label=\"%s\"];\n",mydag->dagindex,RString[mydag->type]);

        }
    }
}


/* draws a DAG */
void drawdag(dagnode *mydag,char *filename) {
    FILE * graphfilepointer = NULL;
    sprintf(RString[0],"Input");
    sprintf(RString[1],"<< ");
    sprintf(RString[2],"Add");
    sprintf(RString[3],"Sub");
    sprintf(RString[4],"AddSub");
    sprintf(RString[5],"Mux");

    resetvisitedstate(mydag);

    graphfilepointer = fopen(filename,"w");
		fprintf(graphfilepointer,"/*\n");
		fprintf(graphfilepointer," * This source file contains a graphical description of a MCM IP core\n");
		fprintf(graphfilepointer," * automatically generated by the SPIRAL Multiple Constant Multiplication\n");
		fprintf(graphfilepointer," * IP Generator Version 1.0:\n");
		fprintf(graphfilepointer," * http://www.spiral.net\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * It can be visualized using Graphviz, a graph visualization tool: \n");
		fprintf(graphfilepointer," * http://www.graphviz.org\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * Redistributions of any form whatsoever must retain and/or include the\n");
		fprintf(graphfilepointer," * following acknowledgment, notices and disclaimer:\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * This product includes software developed by Carnegie Mellon University.\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * Copyright (c) 2003 by Peter Tummeltshammer for the SPIRAL Project,\n");
		fprintf(graphfilepointer," * Carnegie Mellon University\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * For more information, see the SPIRAL project website at:\n");
		fprintf(graphfilepointer," *   http://www.spiral.net\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * You may not use the name \"Carnegie Mellon University\" or derivations\n");
		fprintf(graphfilepointer," * thereof to endorse or promote products derived from this software.\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * If you modify the software you must place a notice on or within any\n");
		fprintf(graphfilepointer," * modified version provided or made available to any third party stating\n");
		fprintf(graphfilepointer," * that you have modified the software.  The notice shall include at least\n");
		fprintf(graphfilepointer," * your name, address, phone number, email address and the date and purpose\n");
		fprintf(graphfilepointer," * of the modification.\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," * THE SOFTWARE IS PROVIDED \"AS-IS\" WITHOUT ANY WARRANTY OF ANY KIND, EITHER\n");
		fprintf(graphfilepointer," * EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY\n");
		fprintf(graphfilepointer," * THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY\n");
		fprintf(graphfilepointer," * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,\n");
		fprintf(graphfilepointer," * TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY\n");
		fprintf(graphfilepointer," * BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,\n");
		fprintf(graphfilepointer," * SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN\n");
		fprintf(graphfilepointer," * ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,\n");
		fprintf(graphfilepointer," * CONTRACT, TORT OR OTHERWISE).\n");
		fprintf(graphfilepointer," *\n");
		fprintf(graphfilepointer," */\n\n");
    time_t mytime;
    time(&mytime);
    fprintf(graphfilepointer,"// File generated:  %s\n",ctime(&mytime));
    fprintf(graphfilepointer,"digraph DAG {\n");
    rdd(mydag,graphfilepointer);
    resetvisitedstate(mydag);
    fprintf(graphfilepointer,"}\n");
    fflush(graphfilepointer);
    fclose(graphfilepointer);

}


void getoperators(dagnode *mydag,dagnode **operators, int *opcount) {
    int i;
    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            getoperators(mydag->leftnode,operators,opcount);
        if (mydag->rightnode != NULL)
            getoperators(mydag->rightnode,operators,opcount);
        if (mydag->type == RM)
            for(i=0;i<MAX_DAGs;i++)
        if(mydag->mux[i] != NULL)
            getoperators(mydag->mux[i],operators,opcount);
        if (mydag->type == RS || mydag->type == RA || mydag->type == RAS)
            operators[(*opcount)++] = mydag;
    }
}

boolean in_so_far(int number, int *so_far, int index) {
    int i;
    for(i = 0;i < index;i++)
        if(so_far[i] == number)
        return True;
    return False;
}

boolean correct_order(int number, int *so_far, int index) {
    int i;
    for(i = 0;i < index;i++)
        if(so_far[i] > number)
        return False;
    return True;
}

boolean rel_order(int number, int *so_far, int *relativeorder,int index) {
    int i;
    for(i = 0;i < index;i++)
        if(so_far[i] > number)
        if(relativeorder[number] != relativeorder[so_far[i]])
            return False;
    return True;
}
int indexof(dagnode *mydag, dagnode **op) {
    int i;
    if(mydag->type == RI)
    	return -1;
    for(i = 0;i < MAX_AUs;i++)
        if(mydag == op[i])
        return(i);
    fprintf(stderr,"Error in function indexof!\n");
    exit(1);
}

/*
// recusrively calculates the coarse cost for
// fusing two specific nodes
double rcc(dagnode *mydag0, dagnode *mydag1, dagnode **op0, dagnode **op_perm) {
    int i;
    double currcost;
    double mincost = 99999.0;

    if (mydag0->type == RM) {
        for(i=0;i<MAX_DAGs;i++) {
        if(mydag0->mux[i] != NULL) {
            currcost = rcc(mydag0->mux[i], mydag1, op0, op_perm);
            if(currcost < mincost)
            mincost = currcost;
        }
    }
    return mincost;
    }
    if (mydag1->type == RM) {
        for(i=0;i<MAX_DAGs;i++) {
        if(mydag1->mux[i] != NULL) {
            currcost = rcc(mydag0, mydag1->mux[i], op0, op_perm);
        if(currcost < mincost)
            mincost = currcost;
        }
    }
    return mincost;
    }

    if (mydag0->type == RT && mydag1->type == RT &&
    mydag0->shift == mydag1->shift)
        return rcc(mydag0->leftnode, mydag1->leftnode, op0, op_perm);
    if (mydag0->type == RI && mydag1->type == RI)
        return 0;
    if((mydag0->type == RA || mydag0->type == RS || mydag0->type == RAS) &&
       (mydag1->type == RA || mydag1->type == RS || mydag1->type == RAS)) {
        if(indexof(mydag0,op0) == indexof(mydag1,op_perm))
        return 0;
    }
    return 1;
}


// calculates the coarse cost for
// fusing two nodes

double calccost(int *perm,int n, dagnode **op0, dagnode **op1,dagnode *mydag0, dagnode *mydag1,int currdagindex) {
    int i;
    double cost = 0;
    dagnode *op_perm[MAX_AUs+1];
    for(i = 0;i < n;i++) {
        if(perm[i] == -1)
        op_perm[i] = NULL;
        else
        op_perm[i] = op1[perm[i]];
    }
    for(i = 0;i < n;i++) {
        if(op_perm[i] != NULL) {
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RA)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RS) {
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
        cost+= 1; //for the Addsub
      }
      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RA) {
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
        cost+= 1; //for the Addsub
      }
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RAS)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));

      if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RA)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));

      if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RAS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));

      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
    }
    }
    cost+= rcc(mydag0, mydag1,op0,op_perm);
    return cost;
}
*/



// recusrively calculates the cost for
// fusing two specific nodes
double rcc(dagnode *mydag0, dagnode *mydag1, dagnode **op0, dagnode **op_perm) {//, costobject *returncost) {
	  int maxfundamental = 0;
	  int minshift = 99999;
	  int muxcount = 0;
	  int permshift,permfundamental;
	  bool foundperm = false;
	  double origmuxsize = 0.0;

	  if(mydag1->type == RM) {
	  	dagnode *tempdag = mydag1;
	  	mydag1 = mydag0;
	  	mydag0 = tempdag;
	  	dagnode **tempop = op_perm;
	  	op_perm = op0;
	  	op0 = tempop;
	  }
	  if(mydag1->type == RT) {
	  	permshift = mydag1->shift;
	  	permfundamental = mydag1->leftnode->fundamental;
	  } else {
	  	permshift = 0;
	  	permfundamental = mydag1->fundamental;
	  }

		if(mydag0->type == RM) {
      for(int i=0;i<MAX_DAGs;i++) {
        if(mydag0->mux[i] != NULL) {
        	muxcount++;
        	if(mydag0->mux[i]->type == RT) {
        		if(mydag0->mux[i]->shift < minshift) {minshift = mydag0->mux[i]->shift;}
        		if(mydag0->mux[i]->leftnode->fundamentals[i] > maxfundamental) {maxfundamental = mydag0->mux[i]->leftnode->fundamentals[i];}
        		if(mydag1->type == RT && mydag1->shift == mydag0->mux[i]->shift &&
        			 indexof(mydag1->leftnode,op_perm) == indexof(mydag0->mux[i]->leftnode,op0)) {foundperm = true;}
        	} else {
        		minshift = 0;
        		if(mydag0->mux[i]->fundamentals[i] > maxfundamental) {maxfundamental = mydag0->mux[i]->fundamentals[i];}
        		if(mydag1->type != RT  && indexof(mydag1,op_perm) == indexof(mydag0->mux[i],op0)) {foundperm = true;}
        	}
        }
      }
      assert(muxcount > 1);
    	origmuxsize = (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * MUXSIZE +
       			        (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * (muxcount - 2) * MUXPLUS;

			maxfundamental = max(maxfundamental,permfundamental);
			minshift = min(minshift,permshift);
			if(!foundperm){muxcount++;}
//			returncost->fundamental = maxfundamental;
//			returncost->shift = minshift;

			return (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * MUXSIZE +
       			 (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * (muxcount - 2) * MUXPLUS - origmuxsize;
		} else {
		  if(mydag0->type == RT) {
		  	minshift = mydag0->shift;
		  	maxfundamental = mydag0->leftnode->fundamental;
    		if(mydag1->type == RT && mydag1->shift == mydag0->shift &&
    			 indexof(mydag1->leftnode,op_perm) == indexof(mydag0->leftnode,op0)) {foundperm = true;}
		  } else {
		  	minshift = 0;
		  	maxfundamental = mydag0->fundamental;
		  	if(mydag1->type != RT  && indexof(mydag1,op_perm) == indexof(mydag0,op0)) {foundperm = true;}
	  	}
			maxfundamental = max(maxfundamental,permfundamental);
			minshift = min(minshift,permshift);
//			returncost->fundamental = maxfundamental;
//			returncost->shift = minshift;
			if(foundperm) {return 0.0;}
			else {
				return (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * MUXSIZE;
			}

		}


}


// calculates the coarse cost for
// fusing two nodes
double calccost(int *perm,int n, dagnode **op0, dagnode **op1,dagnode *mydag0, dagnode *mydag1,int currdagindex) {
    int i;
    double cost = 0;
    dagnode *op_perm[MAX_AUs+1];
    for(i = 0;i < n;i++) {
        if(perm[i] == -1)
        op_perm[i] = NULL;
        else
        op_perm[i] = op1[perm[i]];
    }
    for(i = 0;i < n;i++) {
        if(op_perm[i] != NULL) {
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RA)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RS) {
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
        cost+= VL_INPUTBITWIDTH*(ADDSUBSIZE-SUBSIZE); //for the Addsub
      }
      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RA) {
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));
        cost+= VL_INPUTBITWIDTH*(ADDSUBSIZE-SUBSIZE); //for the Addsub
      }
      if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RAS)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));

      if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RA)
        cost+= min((rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm)),
               (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm)));

      if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RAS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));

      if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RS)
        cost+= (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
            rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
    }
    }
    cost+= rcc(mydag0, mydag1,op0,op_perm);
    return cost;
}

/*
 * recursively calculates the cost for
 * merging two DAGs
 */
void rmt(int *so_far, int curr_index, int n, int k, int empties,dagnode **op0, dagnode **op1,
     dagnode *mydag0, dagnode *mydag1,double *best, int *best_so_far,int currdagindex,int *relativeorderdag1) {
    int i;
    double cost;

    if(curr_index == n) {
        cost = calccost(so_far,n,op0,op1,mydag0,mydag1,currdagindex);

        if(cost < *best) {
        *best = cost;
        for(i=0;i < MAX_AUs+1;i++)
            best_so_far[i] = so_far[i];
        }
        return;
    }
    if(empties > 0) {
        so_far[curr_index] = -1;
        rmt(so_far,curr_index + 1,n,k,empties - 1,op0,op1,mydag0, mydag1,best,best_so_far,currdagindex,relativeorderdag1);
    }
    for(i=0;i < k;i++){
        if(in_so_far(i,so_far,curr_index) == False) {
            if(rel_order(i,so_far,relativeorderdag1,curr_index) == True) {
                so_far[curr_index] = i;
                rmt(so_far,curr_index + 1,n,k,empties,op0,op1,mydag0,mydag1,best,best_so_far,currdagindex,relativeorderdag1);
            }
        }
    }
}

/* Returns the origin of a DAG */
dagnode *getRI(dagnode *mydag) {
    int i;
    if(mydag->type == RI)
        return mydag;
    if (mydag->leftnode != NULL)
        return getRI(mydag->leftnode);
    if (mydag->rightnode != NULL)
        return getRI(mydag->rightnode);
    if (mydag->type == RM)
        for(i=0;i<MAX_DAGs;i++)
        if(mydag->mux[i] != NULL)
            return getRI(mydag->mux[i]);

    fprintf(stderr,"Error in function getRI!\n");
    exit(1);
}

/* Inserts a new Multiplexer during DAGfusion */
dagnode *insertmux(int n,dagnode *mydag0, dagnode *mydag1, dagnode **op0, dagnode **op_perm) {
    int i,j;
    dagnode *tempdag = NULL;
    dagnode *mymux;
    dagnode *myshift;

    if(mydag0->type == RM) {
        if(mydag1->type == RT) {
            myshift  = (struct _dagnode *) malloc(sizeof(struct _dagnode));
            myshift->type = RT;
            myshift->fundamental = 0;
            myshift->dagindex = mydag1->dagindex;
            myshift->shift = mydag1->shift;
            myshift->connections = 1;
            myshift->visited = False;
            myshift->rightnode = NULL;

            if(mydag1->leftnode->type == RI)
                myshift->leftnode = getRI(mydag0);
            else
                myshift->leftnode = op0[indexof(mydag1->leftnode,op_perm)];
            for(i = 0;i < MAX_DAGs;i++){
                myshift->au_type[i] = -1;
                myshift->mux[i] = NULL;
                myshift->fundamentals[i] = -1;
            }
            mydag0->mux[mydag1->dagindex] = myshift;
        }
        if(mydag1->type == RA || mydag1->type == RS || mydag1->type == RAS)
            mydag0->mux[mydag1->dagindex] = op0[indexof(mydag1,op_perm)];
        if(mydag1->type == RI)
            mydag0->mux[mydag1->dagindex] = getRI(mydag0);
        return mydag0;
    }

    mymux  = (struct _dagnode *) malloc(sizeof(struct _dagnode));
    mymux->type = RM;
    mymux->fundamental = 0;
    mymux->dagindex = -1;
    mymux->shift = 0;
    mymux->connections = 1;
    mymux->visited = False;
    mymux->leftnode = NULL;
    mymux->rightnode = NULL;
    for(i = 0;i < MAX_DAGs;i++){
        mymux->au_type[i] = -1;
        mymux->mux[i] = NULL;
        mymux->fundamentals[i] = -1;
    }
    mymux->mux[mydag0->dagindex] = mydag0;

    if(mydag1->type == RM) {
    for(i=0;i < MAX_DAGs;i++) {
    if(mydag1->mux[i] != NULL) {
        if(mydag1->mux[i]->type == RT) {
            myshift  = (struct _dagnode *) malloc(sizeof(struct _dagnode));
            myshift->type = RT;
            myshift->dagindex = mydag1->mux[i]->dagindex;
            myshift->fundamental = 0;
            myshift->shift = mydag1->mux[i]->shift;
            myshift->connections = 1;
            myshift->visited = False;
            myshift->rightnode = NULL;

            if(mydag1->mux[i]->leftnode->type == RI)
                myshift->leftnode = getRI(mydag0);
            else
                myshift->leftnode = op0[indexof(mydag1->mux[i]->leftnode,op_perm)];
            for(j = 0;j < MAX_DAGs;j++){
                myshift->au_type[j] = -1;
                myshift->mux[j] = NULL;
                myshift->fundamentals[j] = -1;
            }
            mymux->mux[i] = myshift;
        }
        if(mydag1->mux[i]->type == RA || mydag1->mux[i]->type == RS || mydag1->mux[i]->type == RAS) {
            mymux->mux[i] = op0[indexof(mydag1->mux[i],op_perm)];
        }
        if(mydag1->mux[i]->type == RI)
            mymux->mux[i] = getRI(mydag0);
    } // if(mydag1
    } // for(i=0
    } // if(mydag1

    if(mydag1->type == RT) {
        myshift  = (struct _dagnode *) malloc(sizeof(struct _dagnode));
        myshift->type = RT;
        myshift->dagindex = mydag1->dagindex;
        myshift->fundamental = 0;
        myshift->shift = mydag1->shift;
        myshift->connections = 1;
        myshift->visited = False;
        myshift->rightnode = NULL;

        if(mydag1->leftnode->type == RI)
        myshift->leftnode = getRI(mydag0);
        else
        myshift->leftnode = op0[indexof(mydag1->leftnode,op_perm)];
        for(i = 0;i < MAX_DAGs;i++){
            myshift->au_type[i] = -1;
            myshift->mux[i] = NULL;
            myshift->fundamentals[i] = -1;
        }
        mymux->mux[mydag1->dagindex] = myshift;
    }
    if(mydag1->type == RA || mydag1->type == RS || mydag1->type == RAS) {
        mymux->mux[mydag1->dagindex] = op0[indexof(mydag1,op_perm)];
    }
    if(mydag1->type == RI)
        mymux->mux[mydag1->dagindex] = getRI(mydag0);
    return mymux;

}



int getrelativeorder(dagnode *mydag,dagnode **op1,int *relativeorderdag1) {
    int i;
    int level = 0;
    if (mydag->leftnode != NULL)
        level = getrelativeorder(mydag->leftnode,op1,relativeorderdag1);
    if (mydag->rightnode != NULL)
        level = max(level,getrelativeorder(mydag->rightnode,op1,relativeorderdag1));
    if (mydag->type == RM)
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
                level = max(level,getrelativeorder(mydag->mux[i],op1,relativeorderdag1));
    if (mydag->type == RS || mydag->type == RA || mydag->type == RAS) {
        if(level > relativeorderdag1[indexof(mydag,op1)])
        relativeorderdag1[indexof(mydag,op1)] = level;
        return ++level;
    }
    if (mydag->type == RI)
        return 0;
    return level;
}

/* merges n DAGs */
double mergedags(dagnode **mydaglist, int daglistsize) {
    int i,j,k,tempint;
    int opcount0 = 0;
    int opcount1 = 0;
    dagnode *op0[MAX_AUs+1];
    dagnode *op1[MAX_AUs+1];

    dagnode *op_perm[MAX_AUs+1];
    dagnode *tempdag, *storeswapdag;
    int so_far[MAX_AUs+1];
    int best_so_far[MAX_AUs+1];
    int relativeorderdag1[MAX_AUs+1];
    double best = 99999.0;
    double cost = 0.0;
    double muxcount_a,muxcount_b;

    getoperators(mydaglist[0],op0,&opcount0);
    resetvisitedstate(mydaglist[0]);

    for(j=1;j<daglistsize;j++) {
      opcount1 = 0;
      getoperators(mydaglist[j],op1,&opcount1);
      resetvisitedstate(mydaglist[j]);
      storeswapdag = NULL;

      //larger DAG has to be at mydaglist[0]
      if(opcount1 > opcount0) {//swap DAGs
        storeswapdag = copydag(mydaglist[j]);
        resetvisitedstate(mydaglist[j]);

        tempdag = mydaglist[j];
        mydaglist[j] = mydaglist[0];
        mydaglist[0] = tempdag;
        opcount0 = 0;
        opcount1 = 0;
        getoperators(mydaglist[0],op0,&opcount0);
        resetvisitedstate(mydaglist[0]);
        getoperators(mydaglist[j],op1,&opcount1);
        resetvisitedstate(mydaglist[j]);
      }

      best = 99999.0;
      for(i = 0;i < MAX_AUs;i++){
        relativeorderdag1[i] = -1;
 //       so_far[i] = -1;
      }

      getrelativeorder(mydaglist[j],op1,relativeorderdag1);


      rmt(so_far,0,opcount0,opcount1,opcount0 - opcount1,op0,op1,mydaglist[0],mydaglist[j],&best,best_so_far,j,relativeorderdag1);
      cost += best;
      for(i = 0;i < opcount0;i++) {
          if(best_so_far[i] == -1)
          op_perm[i] = NULL;
      else
          op_perm[i] = op1[best_so_far[i]];
      }
      for(i = 0;i < opcount0;i++) {
          if(op_perm[i] != NULL) {
          if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RA) {
              muxcount_a = (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
          muxcount_b = (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm));
          if(muxcount_a <= muxcount_b) {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          } else {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm);
          }
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RS) {
              muxcount_a = (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
          muxcount_b = (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm));
          if(muxcount_a <= muxcount_b) {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          } else {
              tempdag = op0[i]->leftnode;
              op0[i]->leftnode = op0[i]->rightnode;
              op0[i]->rightnode = tempdag;
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          }
          op0[i]->type = RAS;
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RA && RType(op_perm[i]->type) == RAS) {
              muxcount_a = (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
          muxcount_b = (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm));
          if(muxcount_a <= muxcount_b) {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          } else {
              tempdag = op0[i]->leftnode;
              op0[i]->leftnode = op0[i]->rightnode;
              op0[i]->rightnode = tempdag;
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          }
          op0[i]->type = RAS;
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RA){
              muxcount_a = (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
          muxcount_b = (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm));
          if(muxcount_a <= muxcount_b) {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          } else {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm);
          }
          op0[i]->type = RAS;
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RA){
              muxcount_a = (rcc(op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm));
          muxcount_b = (rcc(op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm) +
                rcc(op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm));
          if(muxcount_a <= muxcount_b) {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          } else {
              op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->rightnode,op0,op_perm);
              op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->leftnode,op0,op_perm);
          }
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RS) {
          op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
          op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RAS && RType(op_perm[i]->type) == RS) {
          op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
          op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
          else if (RType(op0[i]->type) == RS && RType(op_perm[i]->type) == RAS) {
          op0[i]->leftnode = insertmux(opcount0,op0[i]->leftnode, op_perm[i]->leftnode,op0,op_perm);
          op0[i]->rightnode = insertmux(opcount0,op0[i]->rightnode, op_perm[i]->rightnode,op0,op_perm);
          op0[i]->type = RAS;
          for(k = 0;k < MAX_DAGs;k++) {
              if(op_perm[i]->au_type[k] != -1)
              op0[i]->au_type[k] = op_perm[i]->au_type[k];
              if(op_perm[i]->fundamentals[k] != -1)
              op0[i]->fundamentals[k] = op_perm[i]->fundamentals[k];
          }
          } //if (RType
      } //if(op_perm[i] != NULL)
      } //for(i

    //handle last shift
    mydaglist[0] = insertmux(opcount0,mydaglist[0], mydaglist[j],op0,op_perm);
    // restore the daglist's order before swapping
    if(storeswapdag != NULL) {
        killdag(mydaglist[j]);
        mydaglist[j] = storeswapdag;
    }
    } //for(j
    return cost;
}




int mypowertwo(int pow) {
    int i,res;
    res = 1;
    for(i=0;i<pow;i++)
        res=res*2;
    return res;
}

int logd(int value) {
    int ld = 0;
    int i=1;
    while (i < value) {
        i = i*2;
    ld++;
    }
    return ld;
}

boolean muxopt_not_in_yet(dagnode **muxstore,dagnode *mydag) {
    int i,shift;
    if(mydag->type == RT) {
        shift = mydag->shift;
        mydag=mydag->leftnode;
    } else
        shift = 0;
    for(i=0;i<MAX_DAGs;i++) {
        if(muxstore[i] != NULL) {
        if(muxstore[i]->type == RT) {
            if(muxstore[i]->shift == shift && muxstore[i]->leftnode == mydag)
            return False;
        } else
            if(shift == 0 && muxstore[i] == mydag)
            return False;
        }
    }
    return True;
}


/* calculates the finer grain cost of a DAG */
costobject *getfinalcost(dagnode *mydag,int control,double *overallcost) {
    int i;
    int maxfundamental,minshift,muxcount;
    costobject *cost_left = NULL;
    costobject *cost_right = NULL;
    costobject *cost_mux[MAX_DAGs];
    dagnode *muxstore[MAX_DAGs];


    if(mydag->visited == False) {
        mydag->visited = True;
        if (mydag->leftnode != NULL)
            cost_left = getfinalcost(mydag->leftnode,control,overallcost);
        if (mydag->rightnode != NULL)
            cost_right = getfinalcost(mydag->rightnode,control,overallcost);
        if (mydag->type == RM)
            for(i=0;i<MAX_DAGs;i++)
                if(mydag->mux[i] != NULL)
                    cost_mux[i] = getfinalcost(mydag->mux[i],i,overallcost);
                else
                    cost_mux[i] = NULL;

        mydag->nodecost  = (struct _costobject *) malloc(sizeof(struct _costobject));

        if (mydag->type == RI) {
            mydag->nodecost->fundamental = 1;
            mydag->nodecost->shift = 0;
        }
        if (mydag->type == RT) {
            mydag->nodecost->fundamental = cost_left->fundamental;
            mydag->nodecost->shift = cost_left->shift + mydag->shift;
        }
        if (mydag->type == RA) {
            *overallcost += (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) -
                            max(cost_left->shift,cost_right->shift) - 1) * ADDSIZE;
/*            cerr << "ADD: " << (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) -
                            max(cost_left->shift,cost_right->shift) - 1) * ADDSIZE << endl;
*/        }
        if (mydag->type == RS) {
            *overallcost += (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) - 2) * SUBSIZE;
/*             cerr << "SUB: " << (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) - 2) * SUBSIZE << endl;
*/        }
        if (mydag->type == RAS) {
            *overallcost += (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) - 2) * ADDSUBSIZE;
/*            cerr << "ADDSUB: " << (max((logd(cost_left->fundamental) + cost_left->shift + VL_INPUTBITWIDTH),
                    (logd(cost_right->fundamental) + cost_right->shift + VL_INPUTBITWIDTH)) - 2) * ADDSUBSIZE << ", " <<
                    cost_left->fundamental << " " << cost_left->shift << ", " << cost_right->fundamental << " " << cost_right->shift << endl;
*/        }
        if (mydag->type == RM) {
            maxfundamental = 0;
            minshift = 99999;
            muxcount = 0;
            for(i=0;i<MAX_DAGs;i++)
                muxstore[i] = NULL;
            for(i=0;i<MAX_DAGs;i++)
                if(cost_mux[i] != NULL) {
                if((cost_mux[i]->fundamental * mypowertwo(cost_mux[i]->shift)) > maxfundamental)
                    maxfundamental = cost_mux[i]->fundamental * mypowertwo(cost_mux[i]->shift);
                if(cost_mux[i]->shift < minshift)
                    minshift = cost_mux[i]->shift;
                if(muxopt_not_in_yet(muxstore,mydag->mux[i]) == True) {
                    muxstore[i] = mydag->mux[i];
                		muxcount++;
                }
            }
            if(muxcount > 1)
                *overallcost += (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * MUXSIZE +
                                (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * (muxcount - 2) * MUXPLUS;
/*                cerr << "MUX: " << (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * MUXSIZE +
                                (logd(maxfundamental) + VL_INPUTBITWIDTH - minshift) * (muxcount - 2) * MUXPLUS << " " <<
                                maxfundamental << " " << minshift << endl;
*/
            mydag->nodecost->fundamental = maxfundamental;
            mydag->nodecost->shift = minshift;
        }
    }
    if (mydag->type == RA || mydag->type == RS || mydag->type == RAS) {
        if(control == -1)
            mydag->nodecost->fundamental = mydag->fundamental;
        else
            mydag->nodecost->fundamental = mydag->fundamentals[control];
        mydag->nodecost->shift = 0;
    }
    return mydag->nodecost;
}




/*
 * recursively reorders the n DAGs
 */
dagnode *rrd(dagnode **mydaglist, dagnode **insofar,dagnode *bestsofar,int indexsofar,
     int daglistsize, double *bestcost) {
    int i;
    dagnode *tempdag;
    double cost;

    if(indexsofar == daglistsize) {

        tempdag = insofar[0];
        insofar[0] = copydag(insofar[0]);

        resetvisitedstate(tempdag);

        mergedags(insofar,daglistsize);
        cost = 0.0;
        getfinalcost(insofar[0],-1,&cost);
        resetvisitedstate(insofar[0]);

        if (cost < *bestcost) {
            *bestcost = cost;
            if(bestsofar != NULL)
                killdag(bestsofar);
            bestsofar = insofar[0];
        } else
            killdag(insofar[0]);

        insofar[0] = tempdag;

    } else {
        for(i=0;i<daglistsize;i++) {
            if(mydaglist[i] != NULL) {
                insofar[indexsofar] = mydaglist[i];
                mydaglist[i] = NULL;
                bestsofar = rrd(mydaglist, insofar,bestsofar, indexsofar+1, daglistsize,bestcost);
                mydaglist[i] = insofar[indexsofar];
            }
        }
    }
    return bestsofar;
}

/*
 * reorders the n DAGs resolving effort
 * of n! for exhaustive search and effort of
 * RANDOM_SEARCH_SIZE for random search
 */

dagnode *reorderdags(dagnode **mydaglist, int daglistsize,int *x) {
    int i,j,temp;
    int randnumber[MAX_DAGs+1];
    int newIndex[MAX_DAGs+1];
    int opcount[MAX_DAGs+1];
    double bestcost = 9999999.0;

    dagnode *tempdag;
    dagnode *insofar[MAX_DAGs+1];
    dagnode *bestsofar = NULL;

    for(i=0;i<daglistsize;i++) {
        insofar[i] = NULL;
    }

    if(USE_RANDOM_DAGORDER) {
        for(j=0;j<RANDOM_SEARCH_SIZE;j++) {
            if(j%100 == 0) {
	      //    fprintf(stderr,"*");
	      //    fflush(stderr);
            }
            for(i=0;i<daglistsize;i++) {
                insofar[i] = NULL;
            }
            for(i=0;i<daglistsize;i++) {
                do{
                    randnumber[i] = rand() % daglistsize;
                }while(insofar[randnumber[i]] != NULL);
                insofar[randnumber[i]] = mydaglist[i];
            }
            bestsofar = rrd(mydaglist, insofar,bestsofar, daglistsize, daglistsize,&bestcost);
            for(i=0;i<daglistsize;i++)
                mydaglist[i] = insofar[randnumber[i]];

        }
    }else
        bestsofar = rrd(mydaglist, insofar, bestsofar, 0, daglistsize,&bestcost);

    return bestsofar;

}


/*
 * calculates the constants from a given DAG
 * control: state of mux (if present)
 */
int testdag(dagnode *mydag,int control){
    int res_l = -1;
    int res_r = -1;
    if(mydag->visited == False) {
        mydag->visited = True;
    if(mydag->type == RM) {
      //        if(mydag->mux[control] != NULL)
            res_l = testdag(mydag->mux[control],control);
    } else {
        if (mydag->leftnode != NULL)
            res_l = testdag(mydag->leftnode,control);
        if (mydag->rightnode != NULL)
            res_r = testdag(mydag->rightnode,control);
    }
    switch (RType(mydag->type)) {
    case RI:
        mydag->fundamental = 1;
    break;

    case RT:
        mydag->fundamental = (res_l << mydag->shift);
    break;

    case RA:
        mydag->fundamental =(res_l + res_r);
    break;

    case RS:
        mydag->fundamental = (res_l - res_r);
    break;

    case RAS:
        if(mydag->au_type[control] == RA)
            mydag->fundamental =(res_l + res_r);
        else
            mydag->fundamental = (res_l - res_r);
    break;

    case RM:
        mydag->fundamental = res_l;
    }
    }
    return(mydag->fundamental);
}



void printwires(dagnode *mydag, FILE *out) {
    int i;
    if(mydag->visited == False) {
        mydag->visited = True;
    if (mydag->leftnode != NULL)
        printwires(mydag->leftnode,out);
    if (mydag->rightnode != NULL)
        printwires(mydag->rightnode,out);
    if(mydag->type == RM){
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
            printwires(mydag->mux[i],out);
        fprintf(out,"   reg [width+constwidth-1:0] temp%d;\n",mydag->dagindex);
    }
    else if(mydag->type == RAS)
        fprintf(out,"   reg [width+constwidth-1:0] temp%d /* synthesis syn_keep=1 */;\n",mydag->dagindex);
    else
        fprintf(out,"   wire [width+constwidth-1:0] temp%d;\n",mydag->dagindex);
    }
}

void rptof(dagnode *mydag,FILE * out,int daglistsize,int control) {
    int i,lastindex;
    if(mydag->visited == False) {
        mydag->visited = True;
    if (mydag->leftnode != NULL)
        rptof(mydag->leftnode,out,daglistsize,control);
    if (mydag->rightnode != NULL)
        rptof(mydag->rightnode,out,daglistsize,control);
    if (mydag->type == RM)
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
            rptof(mydag->mux[i],out,daglistsize,control);

    switch (RType(mydag->type)) {
    case RI:
      fprintf(out,"   assign temp%d = sgn_ext_INA;\n",mydag->dagindex);
    break;

    case RT:
    	if(mydag->shift == MAX_CONSTBITWIDTH+5) //replacing the 0
    			fprintf(out,"   assign temp%d = 0;\n",mydag->dagindex);
    	else
      		fprintf(out,"   assign temp%d = temp%d << %d;\n",mydag->dagindex, mydag->leftnode->dagindex, mydag->shift);
    break;

    case RA:
      fprintf(out,"   assign temp%d = temp%d + temp%d;\n",mydag->dagindex, mydag->leftnode->dagindex, mydag->rightnode->dagindex);
    break;

    case RS:
      fprintf(out,"   assign temp%d = temp%d - temp%d;\n",mydag->dagindex, mydag->leftnode->dagindex, mydag->rightnode->dagindex);
        break;

    case RM:
      fprintf(out,"   always @(control");
      for(i=0;i<daglistsize;i++)
          if(mydag->mux[i] != NULL) {
          fprintf(out," or temp%d",mydag->mux[i]->dagindex);
          lastindex = i;
          }
      fprintf(out,")\n");
      fprintf(out,"     case(control)\n");
      for(i=0;i<daglistsize;i++)
          if(mydag->mux[i] != NULL)
          if(i != lastindex)
              fprintf(out,"       %d'd%d:  temp%d <= temp%d;\n",control,i,mydag->dagindex, mydag->mux[i]->dagindex);
          else
              fprintf(out,"       default: temp%d <= temp%d;\n",mydag->dagindex, mydag->mux[i]->dagindex);
      fprintf(out,"     endcase\n");
    break;

    case RAS:
      for(i=0;i<daglistsize;i++)
          if(mydag->au_type[i] == RA)
          lastindex = i;
      fprintf(out,"   always @(control or temp%d or temp%d)\n",mydag->leftnode->dagindex,mydag->rightnode->dagindex);
      fprintf(out,"     case(control)\n");
      for(i=0;i<daglistsize;i++)
          if(i != lastindex) {
          if(mydag->au_type[i] == RA)
              fprintf(out,"       %d'd%d,\n",control,i);
          } else {
          if(mydag->au_type[i] == RA)
              fprintf(out,"       %d'd%d: temp%d <= temp%d + temp%d;\n",control,i,mydag->dagindex, mydag->leftnode->dagindex, mydag->rightnode->dagindex);
          }
      fprintf(out,"       default: temp%d <= temp%d - temp%d;\n",mydag->dagindex, mydag->leftnode->dagindex, mydag->rightnode->dagindex);
      fprintf(out,"     endcase\n");
    break;
        }
    }
}

/* converts the DAG to Verilog-code */
void printdagtoverilog(dagnode *mydag, char *filename,int daglistsize,int* x) {
    int i,control;
    FILE * out;// = stdout;

    out = fopen(filename,"w");

    control=0;
    i=1;
    while (i < daglistsize) {
        i = i*2;
    control++;
    }

    if(daglistsize == 1) //otherwise I'll get a negative range on control
        control = 1;

		fprintf(out,"/*\n");
		fprintf(out," * This source file contains a Verilog description of a MCM IP core\n");
		fprintf(out," * automatically generated by the SPIRAL Multiple Constant Multiplication\n");
		fprintf(out," * IP Generator Version 1.0:\n");
		fprintf(out," * http://www.spiral.net\n");
		fprintf(out," *\n");
		fprintf(out," * Redistributions of any form whatsoever must retain and/or include the\n");
		fprintf(out," * following acknowledgment, notices and disclaimer:\n");
		fprintf(out," *\n");
		fprintf(out," * This product includes software developed by Carnegie Mellon University.\n");
		fprintf(out," *\n");
		fprintf(out," * Copyright (c) 2003 by Peter Tummeltshammer for the SPIRAL Project,\n");
		fprintf(out," * Carnegie Mellon University\n");
		fprintf(out," *\n");
		fprintf(out," * For more information, see the SPIRAL project website at:\n");
		fprintf(out," *   http://www.spiral.net\n");
		fprintf(out," *\n");
		fprintf(out," * You may not use the name \"Carnegie Mellon University\" or derivations\n");
		fprintf(out," * thereof to endorse or promote products derived from this software.\n");
		fprintf(out," *\n");
		fprintf(out," * If you modify the software you must place a notice on or within any\n");
		fprintf(out," * modified version provided or made available to any third party stating\n");
		fprintf(out," * that you have modified the software.  The notice shall include at least\n");
		fprintf(out," * your name, address, phone number, email address and the date and purpose\n");
		fprintf(out," * of the modification.\n");
		fprintf(out," *\n");
		fprintf(out," * THE SOFTWARE IS PROVIDED \"AS-IS\" WITHOUT ANY WARRANTY OF ANY KIND, EITHER\n");
		fprintf(out," * EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO ANY WARRANTY\n");
		fprintf(out," * THAT THE SOFTWARE WILL CONFORM TO SPECIFICATIONS OR BE ERROR-FREE AND ANY\n");
		fprintf(out," * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,\n");
		fprintf(out," * TITLE, OR NON-INFRINGEMENT.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY\n");
		fprintf(out," * BE LIABLE FOR ANY DAMAGES, INCLUDING BUT NOT LIMITED TO DIRECT, INDIRECT,\n");
		fprintf(out," * SPECIAL OR CONSEQUENTIAL DAMAGES, ARISING OUT OF, RESULTING FROM, OR IN\n");
		fprintf(out," * ANY WAY CONNECTED WITH THIS SOFTWARE (WHETHER OR NOT BASED UPON WARRANTY,\n");
		fprintf(out," * CONTRACT, TORT OR OTHERWISE).\n");
		fprintf(out," *\n");
		fprintf(out," */\n\n");

    fprintf(out,"/*\n");
    time_t mytime;
    time(&mytime);
    fprintf(out," * File generated:  %s",ctime(&mytime));
    fprintf(out," * \n");
    fprintf(out," * Descr: the input INA is being multiplied by %d\n",daglistsize);
    fprintf(out," * different constants. The constants are:\n");
    for(i=0;i<daglistsize;i++){
        fprintf(out," * for control = %d: %d",i,x[i]==powertwo(MAX_CONSTBITWIDTH+5) ? 0 : x[i]);
        if(VL_FRACTIONBITWIDTH > 0)
        		fprintf(out," * 2^(-%d)",VL_FRACTIONBITWIDTH);
        fprintf(out,"\n");
    }
    fprintf(out," */\n\n\n");

    fprintf(out,"module adderchain ( INA, control,Q);\n");
    fprintf(out,"   parameter width = %d; /* input bitwidth */\n",VL_INPUTBITWIDTH);
    fprintf(out,"   parameter fractwidth = %d; /* constant fraction bitwidth */\n",VL_FRACTIONBITWIDTH);
      fprintf(out,"   parameter constwidth = %d; /* maximum constant bitwidth */\n",VL_CONSTBITWIDTH);
    fprintf(out,"   input  [width-1:0] INA;\n");
    fprintf(out,"   input  [%d:0]      control;\n",control-1);
    fprintf(out,"   output [width+constwidth-fractwidth-1:0] Q;\n");
    fprintf(out,"   wire [width+constwidth-1:0]  sgn_ext_INA;\n");
    fprintf(out,"   wire  [width+constwidth-1:0] sgn_ext_OUTA;\n\n\n");

    for(i=VL_CONSTBITWIDTH-1;i>=0;i--)
        fprintf(out,"   assign      sgn_ext_INA[width+%d] = INA[width-1];\n",i);
    fprintf(out,"   assign      sgn_ext_INA[width-1:0] = INA[width-1:0];\n\n",i);

    printwires(mydag,out);
    resetvisitedstate(mydag);
    fprintf(out,"\n\n");
    rptof(mydag,out,daglistsize,control);
    resetvisitedstate(mydag);

    fprintf(out,"   assign sgn_ext_OUTA = temp%d;\n",mydag->dagindex);
    fprintf(out,"   assign Q = sgn_ext_OUTA[width+constwidth-1:fractwidth];\n");
    fprintf(out,"endmodule\n");

		fflush(out);
    fclose(out);
}

int getNumberofAdders(dagnode *mydag){
  int i,adders = 0;
  if(mydag->visited == False) {
      mydag->visited = True;
    if (mydag->leftnode != NULL)
        adders += getNumberofAdders(mydag->leftnode);
    if (mydag->rightnode != NULL)
        adders += getNumberofAdders(mydag->rightnode);
    if(mydag->type == RM){
        for(i=0;i<MAX_DAGs;i++)
            if(mydag->mux[i] != NULL)
            	adders += getNumberofAdders(mydag->mux[i]);
    }
    if(mydag->type == RA ||mydag->type == RS ||mydag->type == RAS) {adders++;}
  }
  return adders;
}

coeff_t rand_coeff(coeff_t max) {
	coeff_t res = 0;
	/* we will generate 8 bits at a time */
	assert(RAND_MAX >= (1<<8)-1);
	for(int bytes=0; bytes < sizeof(coeff_t); ++bytes) {
		res = (res<<8) + rand() % (1<<8);
	}
	return 1 + (cfabs(res) % max);
}

/*
   Main
   ====
*/

int main(int argc, char *argv[], char *envp[]) {
    int c, errflg, tflg, constnumber, i,j, temp;
    int number_randomDAGs = 1;
    int number_iterations = 1;
    int constants[MAX_DAGs+1];
    char myBuffer[50];
    char *tablepath = "<not yet set>";
    char *outverilogpath = "./module.v";
    char *outdrawingpath = "./module.dot";
    double cost;
    double finalcost = 0.0;
    double avg_cost1 = 0.0;
    double avg_muxes = 0.0;
    double avg_adders = 0.0;
    int no_muxes;
    int no_adders;
    dagnode *mydaglist[MAX_DAGs+1];
    dagnode *resultdag = NULL;

    errflg = 0;
    tflg = 0;
    if(argc == 1)
        errflg++;
    while ((c = getopt(argc, argv, ":g:n:t:r:d:o:i:c:f:vO:")) != -1) {
        fflush(stdout);
        switch (c) {
        case 'g':
            USE_RANDOM_DAGs = True;
            sscanf(optarg, "%d", &number_iterations);
            if (number_iterations < 1)
                errflg++;
            break;
        case 'n':
            sscanf(optarg, "%d", &number_randomDAGs);
            if (number_randomDAGs < 1)
                errflg++;
            break;
        case 't':
            USE_TABLE = True;
            tablepath = optarg;
            break;
        case 'r':
            USE_RANDOM_DAGORDER = True;
            sscanf(optarg, "%d", &RANDOM_SEARCH_SIZE);
            break;
        case 'd':
            outdrawingpath = optarg;
            break;
        case 'o':
            outverilogpath = optarg;
            break;
        case 'i':
            sscanf(optarg, "%d", &VL_INPUTBITWIDTH);
            assert(VL_INPUTBITWIDTH > 0);
            break;
        case 'c':
            sscanf(optarg, "%d", &VL_CONSTBITWIDTH);
            assert(VL_CONSTBITWIDTH > 0);
            break;
        case 'f':
            sscanf(optarg, "%d", &VL_FRACTIONBITWIDTH);
            assert(VL_FRACTIONBITWIDTH >= 0);
            break;
        case 'v':
            VERBOSE = True;
            break;
        case 'O':
            sscanf(optarg, "%d", &OPTIMIZATION);
            break;
        case ':':
            fprintf(stderr,
                "Option -%c requires an operand\n", optopt);
            errflg++;
            break;
        case '?':
            fprintf(stderr, "Unrecognised option: -%c\n", optopt);
            errflg++;
        }
    }
    constnumber = argc - optind;
    if (constnumber < 1 && USE_RANDOM_DAGs == False) {
        fprintf(stderr, "ERROR: You have to specify at least one constant or use Option -g!\n");
        errflg++;
    }

    if (errflg) {
        fprintf(stderr, "usage: %s [-g NumberIterations -n NumberConstants] [-r RandomSearchSize] [-t Table-file] [-o Verilog-outputfile] ",argv[0]);
        fprintf(stderr, "[-d Graphviz-outputfile] [-i Inputbitwidth] [-c Constantbitwidth] [-f Fractionbitwidth] [-v] [-O Optimization] ");
        fprintf(stderr, "constant1 constant2 ... constantN\n");
        exit(2);
    }
    if(USE_TABLE) {init_table(tablepath); }

		srand(0);
    for(j=0;j<number_iterations;j++) {
        if(USE_RANDOM_DAGs == True) {
	          //fprintf(stderr,"Iteration = %d\n",j+1);
            constnumber = number_randomDAGs;
            for(i=0;i<constnumber;i++) {
            /*
                do {
                		constants[i] = 0;
										for(int bytes=0; bytes < sizeof(int); ++bytes) {
											//constants[i] = rand() % mypowertwo(VL_CONSTBITWIDTH);
											constants[i] = (constants[i]<<8) + rand() % (1<<8);
										}
                    constants[i] = abs(constants[i]  % mypowertwo(VL_CONSTBITWIDTH));
                } while (constants[i] == 0);
                while(constants[i] % 2 == 0){constants[i] = constants[i] / 2;}
//                if(constants[i] == 1) {
//                	i--;
//                	constnumber--;
//                }
            }
            if(constnumber == 0) {constants[0]++;constnumber++;}
            */
            	constants[i] = fundamental(rand_coeff(mypowertwo(VL_CONSTBITWIDTH)));
            	while(constants[i] % 2 == 0){constants[i] = constants[i] / 2;}
          	}
        } else {
            i = optind;
            for ( ; optind < argc; optind++)
                sscanf(argv[optind], "%d", &constants[optind-i]);
        }

        for(i=0;i<constnumber;i++) {
        		if(constants[i] == 0) { //overriding the 0
        			constants[i] = mypowertwo(MAX_CONSTBITWIDTH+5); //set it to an impossible number, to find it later
        		} else if ((logd(constants[i]) > MAX_CONSTBITWIDTH) || (constants[i] < 0)) {
                fprintf(stderr,"ERROR: All constants must lie in [1..%d]!\n",mypowertwo(MAX_CONSTBITWIDTH));
                exit(2);
            }
        }


        if(!USE_TABLE) {set_chains(constants,constnumber,OPTIMIZATION);}

        for(i=0;i<constnumber;i++) {
	          fprintf(stderr,"Building DAG: %d\n",constants[i]);
            if (logd(constants[i]+1) > VL_CONSTBITWIDTH)
                fprintf(stderr,"WARNING: constant %d is too large for constant bitwidth of %d!\n",constants[i],VL_CONSTBITWIDTH);
            mydaglist[i] = builddag(constants[i],i);
        }
        if(!USE_TABLE) {delete_chain();}

        fprintf(stderr,"Merging DAGs... ");
        fflush(stderr);
        resultdag = reorderdags(mydaglist,constnumber,constants);
//        cost = mergedags(mydaglist,constnumber);
//        resultdag = mydaglist[0];
        fprintf(stderr,"Done!\n");
        fflush(stderr);

        resultdag = removeuselessmuxes(resultdag);
        resetvisitedstate(resultdag);

        // checking results
        for(i=0;i<constnumber;i++) {
            temp = testdag(resultdag,i);
            if(constants[i] != temp) {
                fprintf(stderr,"Error at control = %d, Result = %d, should be %d\n\n",i,temp,constants[i]);
                fflush(stdout);
                exit(1);
            }
            resetvisitedstate(resultdag);
        }


        if(USE_RANDOM_DAGs == False) {
        		setindices(resultdag);
        		resetvisitedstate(resultdag);
            drawdag(resultdag,outdrawingpath);
            printdagtoverilog(resultdag,outverilogpath,constnumber,constants);
        }

        finalcost = 0.0;
        //cerr << "FINAL COST" << endl;
        getfinalcost(resultdag,-1,&finalcost);
        resetvisitedstate(resultdag);
        avg_cost1 += finalcost;
        //fprintf(stderr,"Estimated area cost in microns: %f\n",finalcost);
        //cout << "Estimated area cost in microns: " << finalcost << endl;
        no_muxes = countmuxes(resultdag);
        avg_muxes+=no_muxes;
        resetvisitedstate(resultdag);
        no_adders = getNumberofAdders(resultdag);
        avg_adders+=no_adders;
        //printf("Number of muxes: %d\n",no_muxes);
        //printf("%d, %f\n",no_muxes,finalcost);
        resetvisitedstate(resultdag);


        for(i=0;i<constnumber;i++) {
            killdag(mydaglist[i]);
            mydaglist[i] = NULL;
        }
        if(resultdag != NULL)
            killdag(resultdag);

    } //for(j=0
    if(USE_RANDOM_DAGs) fprintf(stderr,"%f\n",avg_cost1/number_iterations);//printf("\n\nOverall average area cost in microns: %f\n",avg_cost1/number_iterations);
    if(USE_RANDOM_DAGs) fprintf(stderr,"%f\n",avg_muxes/number_iterations);//printf("\n\nOverall average number of muxes: %f\n\n",avg_muxes/number_iterations);
    if(USE_RANDOM_DAGs) fprintf(stderr,"%f\n",avg_adders/number_iterations);//printf("\n\nOverall average number of muxes: %f\n\n",avg_muxes/number_iterations);
    //if(USE_RANDOM_DAGs) printf("%f\n",avg_cost1/number_iterations);
    //if(USE_RANDOM_DAGs) printf("%f\n",avg_muxes/number_iterations);
    if(USE_TABLE) {delete_table(); }
}
