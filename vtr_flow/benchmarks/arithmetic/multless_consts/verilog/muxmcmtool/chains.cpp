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

#include "chains.h"
#include <map>
#include <stdio.h>

int MAX_SHIFT;
int MAX_NUM;
int MAX_COST = 7;

cost_t *GEN;
coeff_t GEN_COUNT;
cfvec_t *COSTVECS;
chain *CUR_CHAIN = 0;

int COEFFNUMBER;
int CONSTNUMBER;
cfvec_t *SEARCH_COUNT;
chainvec_t **FOUND_CHAINS;

typedef map<coeff_t, pair<int,int> > addmap_t;
typedef addmap_t::const_iterator     amiter_t;

static reg_t curreg = 2;
void reset_tmpreg() { curreg=2; }
reg_t tmpreg() { return curreg++; }


void generate_initial() {
    COSTVECS[0].push_back(1);
}

inline void insert(cost_t dest_cost, coeff_t coeff) {
    assert(coeff >= 0); /* overflow check */
    bool found_duplicate = false;
    chain *tempchain;

    if(coeff != 1 && coeff <= MAX_NUM){
        for(int i=0;i<COEFFNUMBER;i++)
            for(cfviter_t coeffiter = SEARCH_COUNT[i].begin(); coeffiter != SEARCH_COUNT[i].end(); coeffiter++)
                if(CUR_CHAIN->c == (*coeffiter))
                    if(GEN[coeff] == -1 || GEN[coeff] == dest_cost) {
                        found_duplicate = false;
                        for(chainviter_t j = FOUND_CHAINS[i][dest_cost].begin(); j != FOUND_CHAINS[i][dest_cost].end(); j++)
                            if((*j)->compare(CUR_CHAIN))
                                found_duplicate = true;
                        if(GEN[coeff] == -1) {GEN_COUNT++;}
			//                        CUR_CHAIN->output(cout);
                        if(!found_duplicate) {
                            switch(CUR_CHAIN->type) {
                                case MC:  tempchain = new mult_chain(CUR_CHAIN); break;
                                case AC:  tempchain = new add_chain(CUR_CHAIN); break;
                                case L2C: tempchain = new leapfrog2_chain(CUR_CHAIN); break;
                                case L3C: tempchain = new leapfrog3_chain(CUR_CHAIN); break;
                            }
                            FOUND_CHAINS[i][dest_cost].push_back(tempchain);
                        }
                    }

        if(GEN[coeff] == -1) {
            GEN[coeff] = dest_cost;
            COSTVECS[dest_cost].push_back(coeff);
        }
    }
    delete CUR_CHAIN; CUR_CHAIN = 0;
}

void _compute_all_sumdiffs(bool is_left, addmap_t &result, coeff_t num1, coeff_t num2) {
    int shift, one=1;
    /* we store shift+1 to make possible differentiating +/- of unshifted number.
       so shift 0 is actually 1, and shift n is actually n+1 */
    #define SUM(shift)  (is_left ? pair<int,int>( shift+1, 1)  : pair<int,int>(1,  shift+1))
    #define DIFF(shift) (is_left ? pair<int,int>(-shift-1, 1)  : pair<int,int>(1, -shift-1))
    #define RDIFF(shift) (is_left ? pair<int,int>(shift+1, -1)  : pair<int,int>(-1, shift+1))

    for(shift = 0; shift <= MAX_SHIFT; ++shift) {
    coeff_t sum, diff;
    diff = num1 - (num2<<shift);
    if(diff != 0) {
        sum = num1 + (num2<<shift);
        if(sum < MAX_NUM && is_fundamental(sum))
            result[sum]   = SUM(shift);
        if(diff > 0 && diff < MAX_NUM && is_fundamental(diff))
            result[diff] = DIFF(shift);
        else if(diff < 0 && diff > -MAX_NUM && is_fundamental(-diff))
            result[-diff] = RDIFF(shift);
    }
    }
}

void compute_all_sumdiffs(addmap_t &result, coeff_t num1, coeff_t num2) {
    _compute_all_sumdiffs(false, result, num1, num2);
    _compute_all_sumdiffs(true, result, num2, num1);
}

void _insert_all_sumdiffs(bool is_left, cost_t dest_cost, coeff_t num1, coeff_t num2) {
    for(int shift = 0; shift <= MAX_SHIFT; ++shift) {
    coeff_t sum, diff;
    diff = num1 - (num2<<shift);
    if(diff != 0) {
        sum = num1 + (num2<<shift);
        CUR_CHAIN = new add_chain(dest_cost, num1, num2, SUM(shift));
        if(is_fundamental(sum))
            insert(dest_cost, sum);
        if(diff > 0) {
        CUR_CHAIN = new add_chain(dest_cost, num1, num2, DIFF(shift));
        }
        else {
        CUR_CHAIN = new add_chain(dest_cost, num1, num2, RDIFF(shift));
        diff = -diff;
        }
        if(is_fundamental(diff))
            insert(dest_cost, diff);
    }
    }
}

void insert_all_sumdiffs(cost_t dest_cost, coeff_t num1, coeff_t num2) {
    _insert_all_sumdiffs(false, dest_cost, num1, num2);
    _insert_all_sumdiffs(false, dest_cost, num2, num1);
}

void generate_add_coeffs(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
    for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
        insert_all_sumdiffs(dest_cost, *s1, *s2);
    }
    }
}

void generate_mul_coeffs(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
    for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
        coeff_t prod = (*s1) * (*s2);
        CUR_CHAIN = new mult_chain(dest_cost, *s1, *s2);
        if(is_fundamental(prod))
            insert(dest_cost, prod);
    }
    }
}

void generate_leapfrog2(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
    addmap_t leap1;
    compute_all_sumdiffs(leap1, *s1, 1);
    for(amiter_t l1 = leap1.begin(); l1 != leap1.end(); ++l1) {
        for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
        coeff_t prod = (*l1).first * (*s2);
        if(prod < MAX_NUM && is_fundamental(prod)) {
            addmap_t leap2;
            compute_all_sumdiffs(leap2, prod, *s1);
            for(amiter_t l2 = leap2.begin(); l2 != leap2.end(); ++l2) {
            CUR_CHAIN = new leapfrog2_chain(
               dest_cost, *s1, *s2, (*l1).second, (*l2).second );
            insert(dest_cost, (*l2).first);
            }
        }
        }
    }
    }
}

void generate_leapfrog3(cost_t src1_cost, cost_t src3_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src3 = COSTVECS[src3_cost];
    cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
    addmap_t leap1;
    compute_all_sumdiffs(leap1, *s1, 1);
    for(amiter_t l1 = leap1.begin(); l1 != leap1.end(); ++l1) {
        addmap_t leap2;
        compute_all_sumdiffs(leap2, (*l1).first, *s1);
        for(amiter_t l2 = leap2.begin(); l2 != leap2.end(); ++l2) {
        for(cfviter_t s3 = src3.begin(); s3 != src3.end(); ++s3) {
            coeff_t prod = (*l2).first * (*s3);
            if(prod < MAX_NUM && is_fundamental(prod)) {
            addmap_t leap3;
            compute_all_sumdiffs(leap3, prod, (*l1).first);
            for(amiter_t l3 = leap3.begin(); l3 != leap3.end(); ++l3) {
                CUR_CHAIN = new leapfrog3_chain(
                   dest_cost, *s1, *s3,
                   (*l1).second,
                   (*l2).second,
                   (*l3).second);

                insert(dest_cost, (*l3).first);
            }
            }
        }
        }
    }
    }
}

#define CK if(GEN_COUNT < CONSTNUMBER)

void set_chains(int *c, int coeffnumber, int optimization) {

    cost_t curr_costlevel;
    coeff_t MAX_SEARCH_COUNT = 0;
    bool found_duplicate;
    CONSTNUMBER = COEFFNUMBER = coeffnumber;
    SEARCH_COUNT = new cfvec_t[COEFFNUMBER+1];
    for(int i=0;i<COEFFNUMBER;i++) {
        SEARCH_COUNT[i].push_back(fundamental(c[i]));
        if(fundamental(c[i]) == 1)
            CONSTNUMBER--;
        if(fundamental(c[i]) > MAX_SEARCH_COUNT)
            MAX_SEARCH_COUNT = fundamental(c[i]);
    }

    MAX_SHIFT = logd(MAX_SEARCH_COUNT)+1;
    MAX_NUM = powertwo(MAX_SHIFT);

    GEN = new cost_t[MAX_NUM+1];
    COSTVECS = new cfvec_t[MAX_COST+1];
    FOUND_CHAINS = new chainvec_t*[COEFFNUMBER+1];
    for(int i=0;i<COEFFNUMBER;i++)
        FOUND_CHAINS[i] = new chainvec_t[MAX_COST+1];

    for(curr_costlevel = 6; curr_costlevel > 0; curr_costlevel--) {
        GEN[1] = GEN[0] = 0;
        GEN_COUNT = 0;
        for(coeff_t i = 2; i <= MAX_NUM; ++i)
            GEN[i] = -1;
        generate_initial();

       CK generate_add_coeffs(0, 0, 1);
	//CK generate_mul_coeffs(0, 1, 1); bogus - causes infinite recursive calls

        CK generate_add_coeffs(0, 1, 2);
        CK generate_mul_coeffs(1, 1, 2);

        CK generate_add_coeffs(0, 2, 3);
        CK generate_add_coeffs(1, 1, 3);
        CK generate_mul_coeffs(1, 2, 3);

        CK generate_add_coeffs(0, 3, 4);
        CK generate_add_coeffs(1, 2, 4);
        CK generate_mul_coeffs(1, 3, 4);
        CK generate_mul_coeffs(2, 2, 4);
        CK generate_leapfrog2(1, 1, 4);

        CK generate_add_coeffs(0, 4, 5);
        CK generate_add_coeffs(1, 3, 5);
        CK generate_add_coeffs(2, 2, 5);
        CK generate_mul_coeffs(1, 4, 5);
        CK generate_mul_coeffs(2, 3, 5);
        CK generate_leapfrog2(1, 2, 5);
        CK generate_leapfrog3(1, 1, 5);

        CK generate_add_coeffs(0, 5, 6);
        CK generate_add_coeffs(1, 4, 6);
        CK generate_add_coeffs(2, 3, 6);
        CK generate_mul_coeffs(1, 5, 6);
        CK generate_mul_coeffs(2, 4, 6);
        CK generate_mul_coeffs(3, 3, 6);
        CK generate_leapfrog2(1, 3, 6);
        CK generate_leapfrog2(2, 2, 6);
        CK generate_leapfrog3(1, 2, 6);

        for(int i=0;i<COEFFNUMBER;i++)
            SEARCH_COUNT[i].clear();
        CONSTNUMBER = 0;
        for(int i=0;i<MAX_COST;i++)
            COSTVECS[i].clear();
        for(int i=0;i<MAX_NUM;i++)
            GEN[i] = -1;


        for(int i=0;i<COEFFNUMBER;i++)
            for(chainviter_t chi = FOUND_CHAINS[i][curr_costlevel].begin(); chi != FOUND_CHAINS[i][curr_costlevel].end(); chi++) {
                found_duplicate = false;
		//cout << "Costlevel: " << curr_costlevel << ", Chain: " << i <<", Coeff: " << (*chi)->c << endl;
		//cout << "pushing: ";
                for(cfviter_t coi = SEARCH_COUNT[i].begin(); coi != SEARCH_COUNT[i].end(); coi++)
                    if((*chi)->c1 == (*coi))
                        found_duplicate = true;
                if(!found_duplicate && (*chi)->c1 != 1) {
                    SEARCH_COUNT[i].push_back((*chi)->c1);
                    CONSTNUMBER++;
		    //cout << (*chi)->c1 << ", ";
                }
                found_duplicate = false;
                for(cfviter_t coi = SEARCH_COUNT[i].begin(); coi != SEARCH_COUNT[i].end(); coi++)
                    if((*chi)->c2 == (*coi))
                        found_duplicate = true;
                if(!found_duplicate && (*chi)->c2 != 1) {
                    SEARCH_COUNT[i].push_back((*chi)->c2);
                    CONSTNUMBER++;
		    //cout << (*chi)->c2 << ", ";
                }
		//cout << endl;
            }
    }

    delete[] GEN; delete[] COSTVECS; delete[] SEARCH_COUNT;


    /* Optimization (weighting of the results)*/
    coeff_t c_coeff;
    cost_t c_value;
    for(int j=1;j<MAX_COST;j++)
        for(int i=0;i<COEFFNUMBER;i++)
            for(chainviter_t chi0 = FOUND_CHAINS[i][j].begin(); chi0 != FOUND_CHAINS[i][j].end(); chi0++) {
                c_value = 0;
                //get savings from previous cost level
                if(j>1)
                    for(chainviter_t chi1 = FOUND_CHAINS[i][j-1].begin(); chi1 != FOUND_CHAINS[i][j-1].end(); chi1++)
                        if(((*chi1)->c == (*chi0)->c1 || (*chi1)->c == (*chi0)->c2) && (*chi1)->value > (*chi0)->value)
                            (*chi0)->value = (*chi1)->value;
                //calculate current cost level's savings
                for(int k=0;k<COEFFNUMBER;k++)
                    for(chainviter_t chi1 = FOUND_CHAINS[k][j].begin(); chi1 != FOUND_CHAINS[k][j].end(); chi1++)
                        if(i != k && (*chi0)->c == (*chi1)->c) {
                            c_value += (*chi1)->cost;
                            break;
                        }

                //savings bigger than from previous cost level
		if (optimization == 1)
		    if(c_value > (*chi0)->value)
		        (*chi0)->value = c_value;
		//for optimization == 2 the overlapping has to include ALL constants
		if (optimization == 2)
		    if (c_value == j*(COEFFNUMBER-1))
		        (*chi0)->value = c_value;
            }
}


chain *get_chain(coeff_t c, int index) {
chain *c_chain = NULL;
cost_t c_value = -1;
  for(int i=0;i<MAX_COST;i++){
    for(chainviter_t ci = FOUND_CHAINS[index][i].begin(); ci != FOUND_CHAINS[index][i].end(); ci++){
	  //cout << "search: " << c << ", value: " << (*ci)->value << ", int: ";
	  //  (*ci)->output(cout);

      if((*ci)->c == c) {
    		if((*ci)->value > c_value){
    		    c_value = (*ci)->value;
    		    c_chain = (*ci);
    		}
	    }
    }
  }
  return c_chain;
}


void delete_chain() {
    for(int i=0;i<COEFFNUMBER;i++) {
        for(int j=0;j<MAX_COST;j++)
            FOUND_CHAINS[i][j].clear();
        delete[] FOUND_CHAINS[i];
    }
    delete[] FOUND_CHAINS;
}
