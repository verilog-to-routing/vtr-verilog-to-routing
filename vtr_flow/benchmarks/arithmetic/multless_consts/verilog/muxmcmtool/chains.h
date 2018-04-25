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

#ifndef __CHAINS_H__
#define __CHAINS_H__

#include <set>
#include <vector>
#include <cassert>
#include <iostream>
#include <map>
using namespace std;

#define COEFF_FMT "%lld"
typedef int                     cost_t;
typedef long long              	coeff_t;
typedef set<coeff_t>            cfset_t;
typedef vector<coeff_t>         cfvec_t;
typedef cfset_t::iterator       cfsiter_t;
typedef cfvec_t::iterator       cfviter_t;
typedef int                     reg_t;
typedef vector<class op*>       oplist_t;
typedef pair<int,int>           sp_t;
typedef const sp_t&             spref_t;
typedef vector<class chain*>    chainvec_t;
typedef chainvec_t::iterator    chainviter_t;
//typedef vector<chainvec_t>      chainvecvec_t;
//typedef chainvecvec_t::iterator chainvviter_t;

typedef enum { MC,AC,L2C,L3C } chain_t;

struct chain;
chain * get_chain(coeff_t c, int index);
void set_chains(int *c, int coeffnumber, int optimization);
void delete_chain();


inline coeff_t fundamental(coeff_t z) {
    assert(z>0);
    //if(z >= ((coeff_t)1<<32)) {cerr << z << endl;}
    while((z&1) == 0) z = z>>1;
    return z;
}
inline bool is_fundamental(coeff_t z) {
    assert(z>0);
    return ((z&1) != 0);
}



inline coeff_t powertwo(cost_t pow) {
    cost_t i;
    coeff_t res;
    res = 1;
    for(i=0;i<pow;i++)
        res=res*2;
    return res;
}

inline cost_t logd(coeff_t value) {
    cost_t ld = 0;
    coeff_t i=1;
    while (i < value) {
        i = i*2;
    ld++;
    }
    return ld;
}

struct op {
    /* reg_t:res <--  ADD(reg_t:r1, reg_t:r2)
       reg_t:res <--  SUB(reg_t:r1, reg_t:r2)
       reg_t:res <--  SHL(reg_t:r1, int:sh)
       reg_t:res <--  SHR(reg_t:r1, int:sh)
       reg_t:res <-- CMUL(reg_t:r1, coeff_t:c)
       reg_t:res <--  INP()                  */
    typedef enum { ADD=1, SUB=2, SHL=3, SHR=4, CMUL=5 } type_t;
    type_t type;
    coeff_t c;
    reg_t r1, r2, res;
    int sh;

    static op* add(reg_t res, reg_t r1, reg_t r2)   { return new op(ADD,  0, r1, r2, res,  0); }
    static op* sub(reg_t res, reg_t r1, reg_t r2)   { return new op(SUB,  0, r1, r2, res,  0); }
    static op* shl(reg_t res, reg_t r1, int sh)     { return new op(SHL,  0, r1,  0, res, sh); }
    static op* shr(reg_t res, reg_t r1, int sh)     { return new op(SHR,  0, r1,  0, res, sh); }
    static op* cmul(reg_t res, reg_t r1, coeff_t c) { return new op(CMUL, c, r1,  0, res,  0); }
    void output(ostream &os) const {
    os << "  t" << res << " = ";
    switch(type) {
    case ADD: os << "add(t" << r1 << ", t" << r2 << ")"; break;
    case SUB: os << "sub(t" << r1 << ", t" << r2 << ")"; break;
    case SHL: os << "shl(t" << r1 << ", " << sh << ")";  break;
    case SHR: os << "shr(t" << r1 << ", " << sh << ")";  break;
    case CMUL: os << "cmul(t" << r1 << ", " << c << ")"; break;
    default: assert(0);
    }
    os << endl;
    }

private:
    op(type_t tt, coeff_t cc, reg_t rr1, reg_t rr2, reg_t rres, int ssh) :
      type(tt), c(cc), r1(rr1), r2(rr2), res(rres), sh(ssh) {}
};

inline coeff_t cfabs(coeff_t x) { return x>=0 ? x : -x; }

inline int compute_shr(coeff_t c) {
    int shr = 0;
    while((c&1) == 0) { c = (c>>1); ++shr; }
    return shr;
}

inline coeff_t compute_sp(coeff_t c1, coeff_t c2, spref_t sp) {
    /* shifts are offset by 1, see compute_all_sumdiffs in chains.cpp */
    c1 = (sp.first < 0) ? -(c1<<(-sp.first-1)) : (c1<<(sp.first-1));
    c2 = (sp.second < 0) ? -(c2<<(-sp.second-1)) : (c2<<(sp.second-1));
    return fundamental(cfabs(c1+c2));
}

inline void seq_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)(),
         coeff_t c1, coeff_t c2) {
    reg_t t1 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    l.push_back( op::cmul(dest, t1, c2) );
}

inline void sumdiff_ops(oplist_t &l, reg_t dest, reg_t src1, reg_t src2, reg_t (tmpreg)(),
         coeff_t c1, coeff_t c2, spref_t sp) {
    int sh1, sh2, sign=+1, orient;

    /* shifts are offset by 1, see compute_all_sumdiffs in chains.cpp */
    if(sp.first < 0) { sh1 = -sp.first-1; sign = -1; orient = 1; }
    else             { sh1 =  sp.first-1;  }

    if(sp.second < 0) { sh2 = -sp.second-1; sign = -1; orient = 2; }
    else              { sh2 =  sp.second-1; }

    if(sh1 == sh2) {
    /* shifts are allowed only on one side, so sh1==sh2 means both are 0 */
    assert(sh1==0);
    assert(sign==+1 || c1!=c2);
    reg_t t1=tmpreg();
    l.push_back( op::add(t1, src1, src2) );
    l.push_back( op::shr(dest, t1, compute_shr(c1 + c2)) );
    }
    else {
    assert(sh1==0 || sh2==0);
    reg_t t1=tmpreg();

    if(sh1==0) {
        l.push_back( op::shl(t1, src2, sh2) );
        if(sign==+1)    l.push_back( op::add(dest, src1, t1) );
        else if(orient==1)  l.push_back( op::sub(dest, t1, src1) );
        else        l.push_back( op::sub(dest, src1, t1) );
    }
    else { /* sh2==0 */
        l.push_back( op::shl(t1, src1, sh1) );
        if(sign==+1)    l.push_back( op::add(dest, src2, t1) );
        else if(orient==1)  l.push_back( op::sub(dest, src2, t1) );
        else        l.push_back( op::sub(dest, t1, src2) );
    }
    }
}



struct chain {
    bool compare(chain *comp) {
        if(comp->type != type)
            return false;
        if(c != comp->c || ((c1 != comp->c1 || c2 != comp->c2) && (c1 != comp->c2 || c2 != comp->c1))) {
            return false;
        }

        return true;
    }
    virtual void output(ostream &) const = 0;
    virtual void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    assert(0 && "not implemented in subclass");
    }
    virtual ~chain() {}
    chain_t type;
    cost_t cost,value;
    coeff_t c,c1,c2;
    sp_t sp, sp1, sp2, sp3;
};


struct mult_chain : public chain {
    mult_chain(cost_t cost, coeff_t c1, coeff_t c2) {
    this->type = MC;
    this->cost = cost;
    this->value = 0;
    this->c1 = c1;
    this->c2 = c2;
    this->c = c1*c2;
    }
    mult_chain(chain *chain_c){
    this->type = MC;
    this->cost = chain_c->cost;
    this->value = chain_c->value;
    this->c1 = chain_c->c1;
    this->c2 = chain_c->c2;
    this->c = chain_c->c;
    }

    void output(ostream &os) const {
    os << c << " " << cost;
    os << " mult " << c1 << " " << c2 << endl;
    }
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    seq_ops(l, dest, src, tmpreg, c1, c2);
    }
};

struct add_chain : public chain {
    add_chain(cost_t cost, coeff_t c1, coeff_t c2, spref_t sp) {
    this->type = AC;
    this->cost = cost;
    this->value = 0;
    this->c1 = c1;
    this->c2 = c2;
    this->sp = sp;
    this->c = compute_sp(c1, c2, sp);
    }
    add_chain(chain *chain_c){
    this->type = AC;
    this->cost = chain_c->cost;
    this->value = chain_c->value;
    this->c1 = chain_c->c1;
    this->c2 = chain_c->c2;
    this->sp = chain_c->sp;
    this->c = chain_c->c;
    }
    void output(ostream &os) const {
    os << c << " " << cost;
    os << " add " << c1 << " " << c2 << " " << sp.first << " " << sp.second << endl;
    }
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t t1=tmpreg(), t2=tmpreg();
    if(c1!=1) l.push_back(op::cmul(t1, src, c1)); else t1 = src;
    if(c2!=1) l.push_back(op::cmul(t2, src, c2)); else t2 = src;
    sumdiff_ops(l, dest, t1, t2, tmpreg, c1, c2, sp);
    }
};

struct leapfrog2_chain : public chain {
    leapfrog2_chain(cost_t cost, coeff_t c1, coeff_t c2,
            spref_t sp1,
            spref_t sp2) {
    this->type = L2C;
    this->cost = cost;
    this->value = 0;
    this->c1 = c1; this->sp1 = sp1;
    this->c2 = c2; this->sp2 = sp2;
    leap1 = compute_sp(c1, 1, sp1);
    this->c = compute_sp(leap1*c2, c1, sp2);
    }
    leapfrog2_chain(chain *chain_c){
    this->type = L2C;
    this->cost = chain_c->cost;
    this->value = chain_c->value;
    this->c1 = chain_c->c1;
    this->c2 = chain_c->c2;
    this->sp1 = chain_c->sp1;
    this->sp2 = chain_c->sp2;
    leap1 = compute_sp(c1, 1, sp1);
    this->c = chain_c->c;
    }
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t leap1reg = tmpreg(), t1 = tmpreg(), t2 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    sumdiff_ops(l, leap1reg, t1, src, tmpreg, c1, 1, sp1);
    l.push_back( op::cmul(t2, leap1reg, c2) );
    sumdiff_ops(l, dest, t2, t1, tmpreg, c2*leap1, c1, sp2);
    }

    void output(ostream &os) const {
    os << c << " " << cost;
    os << " lpfr2 " << c1 << " " << c2 << " "
       << sp1.first << " " << sp1.second << " "
       << sp2.first << " " << sp2.second << " "
       << endl;
    }
    coeff_t leap1;
};

struct leapfrog3_chain : public chain {
    leapfrog3_chain(cost_t cost, coeff_t c1, coeff_t c2,
            spref_t sp1,
            spref_t sp2,
            spref_t sp3) {
    this->type = L3C;
    this->cost = cost;
    this->value = 0;
    this->c1 = c1; this->c2 = c2;
    this->sp1 = sp1; this->sp2 = sp2; this->sp3 = sp3;

    leap1 = compute_sp(c1, 1, sp1);
    leap2 = compute_sp(leap1, c1, sp2);
    this->c = compute_sp(leap2*c2, leap1, sp3);
    }
    leapfrog3_chain(chain *chain_c){
    this->type = L3C;
    this->cost = chain_c->cost;
    this->value = chain_c->value;
    this->c1 = chain_c->c1;
    this->c2 = chain_c->c2;
    this->sp1 = chain_c->sp1;
    this->sp2 = chain_c->sp2;
    this->sp3 = chain_c->sp3;
    leap1 = compute_sp(c1, 1, sp1);
    leap2 = compute_sp(leap1, c1, sp2);
    this->c = chain_c->c;
    }
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {
    reg_t leap1reg = tmpreg(), leap2reg = tmpreg(), t1 = tmpreg(), t2 = tmpreg();
    l.push_back( op::cmul(t1, src, c1) );
    sumdiff_ops(l, leap1reg, t1,       src, tmpreg, c1,    1, sp1);
    sumdiff_ops(l, leap2reg, leap1reg, t1,  tmpreg, leap1, c1, sp2);
    l.push_back( op::cmul(t2, leap2reg, c2) );
    sumdiff_ops(l, dest, t2, leap1reg, tmpreg, c2*leap2, leap1, sp3);
    }

    void output(ostream &os) const {
    os << c << " " << cost;
    os << " lpfr3 " << c1 << " " << c2 << " "
       << sp1.first << " " << sp1.second << " "
       << sp2.first << " " << sp2.second << " "
       << sp3.first << " " << sp3.second << " "
       << endl;
    }
    coeff_t leap1, leap2;
};


#endif // __CHAINS_H__
