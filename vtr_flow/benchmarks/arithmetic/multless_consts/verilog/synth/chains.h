#ifndef __CHAINS_H__
#define __CHAINS_H__

#include <set>
#include <vector>
#include <map>
#include <iostream>
#include <cassert>
#include <climits>
#define myassert(cond) if(!(cond)) myabort(); else;
void    myabort();

#ifdef __CYGWIN32__
#define ASSERT assert
#else
#define ASSERT assert
#endif

using namespace std;

#define COEFF_FMT "%lld"
#define COEFF_MAX LONG_LONG_MAX
class adder;

typedef int                    cost_t;
typedef long long              coeff_t;
typedef pair<coeff_t, coeff_t> cfpair_t;
typedef set<coeff_t>           cfset_t;
typedef vector<coeff_t>        cfvec_t;
typedef cfset_t::iterator      cfsiter_t;
typedef cfvec_t::iterator      cfviter_t;
typedef int                    reg_t;
typedef map<coeff_t,reg_t>     regmap_t;
typedef vector<class op*>      oplist_t;
typedef map<coeff_t, pair<int,int> >  addmap_t;
typedef addmap_t::const_iterator      amiter_t;
typedef map<coeff_t, adder>           full_addmap_t;
typedef full_addmap_t::const_iterator full_amiter_t;

struct dist_t {
  dist_t(int dist, coeff_t aux, int max_depth) : dist(dist), aux(aux), max_depth(max_depth) {}
  dist_t() : dist(-1), aux(-1), max_depth(-1) {}
  int dist;
  coeff_t aux;
  int max_depth;
};

typedef map<cfpair_t, dist_t>            distmap_t;

#include "arith.h"
#include "codegen.h"

#define FORALL(set, it) for(cfsiter_t it = (set).begin(); it != (set).end(); ++it)
#define FORALL_V(vec, it) for(cfviter_t it = (vec).begin(); it != (vec).end(); ++it)
#define FORALL_AM(set, it) for(amiter_t it = (set).begin(); it != (set).end(); ++it)
#define FORALL_FAM(set, it) for(full_amiter_t it = set.begin(); it != set.end(); ++it)

#define ADD(set,el) ((set).insert(el))
#define CONTAINS(set,el) ((set).find(el) != (set).end())
#define REMOVE(set, el) ((set).erase(el))

#define SUM(is_left, shift)   (is_left ? pair<int,int>( shift+1, 1)  : pair<int,int>(1,  shift+1))
#define DIFF(is_left, shift)  (is_left ? pair<int,int>(-shift-1, 1)  : pair<int,int>(1, -shift-1))
#define RDIFF(is_left, shift) (is_left ? pair<int,int>(shift+1, -1)  : pair<int,int>(-1, shift+1))

extern int MIN_SHIFT;
extern int MAX_SHIFT;
extern coeff_t MAX_NUM;

struct chain {
    chain() { _depth = -1; _regs = -1; }
    virtual void output(ostream &) const = 0;
    virtual void output_c(ostream &os) const = 0;
    virtual void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) = 0;
    virtual ~chain() {}
    virtual coeff_t get_intermediates_sum(chain *(get_ch)(coeff_t));
    virtual void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) = 0;
    virtual void compute_depth(chain *(get_ch)(coeff_t)) = 0;
  //    virtual void compute_regs(chain *(get_ch)(coeff_t)) = 0;
    int depth(chain *(get_ch)(coeff_t)) { if(_depth==-1) compute_depth(get_ch); return _depth; }
  //    int regs(chain *(get_ch)(coeff_t)) { if(_regs==-1) compute_regs(get_ch); return _regs; }
    coeff_t c;
    cost_t cost;
protected:
    int _depth, _regs;
};

chain * read_chain(istream &fin);
void gen_add_coeffs(cfset_t &src1, cfset_t &src2, cfset_t &dest);
void gen_mul_coeffs(cfset_t &src1, cfset_t &src2, cfset_t &dest);
void compute_all_sumdiffs(addmap_t &result, coeff_t num1, coeff_t num2);
void compute_fast_sumdiffs(cfset_t &result, coeff_t num1, coeff_t num2);
void sumdiff_ops(oplist_t &l,
		 reg_t dest, reg_t src1, reg_t src2, reg_t (tmpreg)(),
		 coeff_t c1, coeff_t c2, spref_t sp);

/* result of set_product is all possible outputs of an adder with
   inputs from 2 different sets */
void set_product(cfset_t &dest, cfset_t &src1, cfset_t &src2);
void set_product(cfset_t &dest, coeff_t c, cfset_t &src);
void set_product(cfset_t &dest, coeff_t c, full_addmap_t &src);
void set_product(cfset_t &dest, full_addmap_t &src1, cfset_t &src2);
void set_product(cfset_t &dest, full_addmap_t &src1, full_addmap_t &src2);

void set_ratio(cfset_t &dest, coeff_t c, cfset_t &denom);
void set_ratio(cfset_t &dest, cfset_t &nom, cfset_t &denom);

void subtract_set(cfset_t &dest, cfset_t &sub);
void unite_set(cfset_t &dest, cfset_t &add);

#include "adder.h"

struct empty_chain : public chain {
    empty_chain() { c = 1; cost = 0; _depth=0; _regs=1; }
    void output(ostream &os) const { os << "empty" << endl; }
    void output_c(ostream &os) const { os << "new empty_chain()"; }
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)()) {(void)l;(void)dest;(void)src;(void)tmpreg;}
    void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t)) {(void)dest; (void)get_ch;}
    void compute_depth(chain *(get_ch)(coeff_t)) {(void)get_ch;}
  //    void compute_regs(chain *(get_ch)(coeff_t)) {}
};

struct mult_chain : public chain {
    mult_chain(cost_t cost, coeff_t c1, coeff_t c2);
    void output(ostream &os) const;
    void output_c(ostream &os) const;
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)());
    void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t));
    void compute_depth(chain *(get_ch)(coeff_t));
  //    void compute_regs(chain *(get_ch)(coeff_t));
    coeff_t c1, c2;
};

struct add_chain : public chain {
    add_chain(cost_t cost, coeff_t c1, coeff_t c2, spref_t sp);
    void output(ostream &os) const;
    void output_c(ostream &os) const;
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)());
    void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t));
    void compute_depth(chain *(get_ch)(coeff_t));
  //    void compute_regs(chain *(get_ch)(coeff_t));
    coeff_t c1, c2;
    sp_t sp;
};

struct leapfrog2_chain : public chain {
    leapfrog2_chain(cost_t cost, coeff_t c1, coeff_t c2, spref_t sp1, spref_t sp2);
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)());
    void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t));
    void output(ostream &os) const;
    void output_c(ostream &os) const;
    void compute_depth(chain *(get_ch)(coeff_t));
  //    void compute_regs(chain *(get_ch)(coeff_t));
    coeff_t c1, c2, leap1;
    sp_t sp1, sp2;
};

struct leapfrog3_chain : public chain {
    leapfrog3_chain(cost_t cost, coeff_t c1, coeff_t c2,
		    spref_t sp1, spref_t sp2, spref_t sp3);
    void get_ops(oplist_t &l, reg_t dest, reg_t src, reg_t (tmpreg)());
    void output(ostream &os) const;
    void output_c(ostream &os) const;
    void get_intermediates(cfset_t &dest, chain *(get_ch)(coeff_t));
    void compute_depth(chain *(get_ch)(coeff_t));
  //    void compute_regs(chain *(get_ch)(coeff_t));
    coeff_t c1, c2, leap1, leap2;
    sp_t sp1, sp2, sp3;
};

#endif // __CHAINS_H__
