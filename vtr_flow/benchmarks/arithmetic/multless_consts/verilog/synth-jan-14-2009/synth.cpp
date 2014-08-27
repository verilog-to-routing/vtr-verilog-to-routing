#include "synth.h"
#include "ac1.h"
#include <string>
#include <vector>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <list>
#include <map>
#include <iomanip>
using namespace std;
#include "cmdline.h"

extern int CBITS;
extern int COSTS[];

/****************************************************************************/
int MIN_SHIFT = 0;
int MAX_SHIFT = 20;
int VERBOSE = 0;
coeff_t MAX_NUM    = 1 << MAX_SHIFT;
coeff_t MAX_PRINT  = MAX_NUM >> 1;
coeff_t MAX_GEN    = MAX_PRINT;
coeff_t MAX_NPRINT = 5;

int MAX_DEPTH = -1; /* if >= 0, only searches solutions with limited depth */

typedef enum { MODE_SINGLE, MODE_RANDOM, MODE_EXHAUST } synth_mode_t;
synth_mode_t MODE = MODE_SINGLE;

int NTESTS;    /* for MODE_RANDOM */
int TEST_SIZE; /* for MODE_RANDOM */

int HYB_CUT = 27;

typedef enum { EST_OPT_TABLE, /* Est(c) = optimal cost of c, i.e. complexity */
       EST_VALUE,     /* Est(c) = ceil(log_2(c)) */
       EST_HYBRID,    /* Est(c) = ceil(log_2(c)) if c > 2^27, csd_bits(c) else */
       EST_CSD        /* Est(c) = # non-zero CSD bits of c */
} est_mode_t;
est_mode_t EST_MODE = EST_CSD;

typedef enum { ACM_OUT_CODE, ACM_OUT_GAP, ACM_OUT_DOT, ACM_OUT_DOT_CODE } acm_out_t;
acm_out_t ACM_OUTPUT = ACM_OUT_CODE;
bool ACM_HEADER = true;
string ACM_DOT_FILE = "./acm.dot";

int DIST_TEST = 3; /* either 2 or 3 */
bool NO_WEIGHT     = false; /* NO_WEIGHT sets the weight function W(dist)=1 */
bool GEN_UNIQUE    = false;
bool GEN_CODE      = false;
bool GEN_ADDERS    = false;
bool GEN_DAG       = false;
bool GEN_DOT       = false;
bool ALLOCATE_REGS = false;
bool PRINT_TARGETS = false;
bool USE_BOUND     = true;
bool USE_SUCCS2    = false;
bool USE_AC1       = false;
bool IMPROVE_AC1   = false;
bool USE_C1_ALGO   = false;
bool MAX_BENEFIT = false;
bool BHM = false;
bool OUTPUT_GAP = false;
bool OPTIMIZE_NOFS = false;
bool USE_AUX_TARGETS = false;
bool USE_EXPENSIVE_ESTIMATION = false;

bool SPLIT_2 = false;
int SPLIT_THRESH = 6;
#define if_verbose(x) if(VERBOSE >=x)

/************** Solver data ****************/
cfvec_t SIZES;
cfvec_t SPEC_ALL_TARGETS; /* all specified targets (including duplicates) */
cfset_t SPEC_TARGETS;     /* duplicates/evens/negatives are omitted here */
cfset_t TARGETS;          /* not yet synthesized coefficients */
cfset_t WORKLIST; cfset_t GIVEN, SUCCS2;
full_addmap_t READY; /* synthesized coefficients */
full_addmap_t SUCCS; /* successors of READY */

map<coeff_t, int> DEPTH_LIMITS;
map<coeff_t, cfset_t> AUX_TARGETS;
distmap_t DISTS, ESTDISTS;     /* distance cache */
cfset_t COST_0, COST_1, COST_2;
int CURCOST, ITER, HEUR;

coeff_t BEST_BENEFIT        = COEFF_MAX;

/**************************** Code generation *******************************/
static oplist_t *OPS;
static regmap_t *REGS;
static reg_t   (*TMPREG)();
adag *ADAG = NULL;

/**************************** **** *******************************/
/**************************** ALGO *******************************/
/**************************** **** *******************************/
inline int cost_estimate(coeff_t c);
void gen_code(coeff_t c, const adder &ad);
void synthesize(coeff_t c, adder ad);
adder *get_adder(coeff_t c);
void ac1_decompose_cheapest();
void print_set(const char *nam, full_addmap_t &s, int max_nprint = MAX_NPRINT, ostream &ostr = cerr);
void print_set(const char *nam, cfset_t &s, int max_nprint = MAX_NPRINT, ostream &ostr = cerr);

int MAX_TARGET_DEPTH(coeff_t t) {
    if(DEPTH_LIMITS.find(t)==DEPTH_LIMITS.end()) return MAX_DEPTH;
    else return DEPTH_LIMITS[t];
}

void clear_dists() {
    DEPTH_LIMITS.clear();
    DISTS.clear();
    ESTDISTS.clear();
}

void clear_est_dists() {
    ESTDISTS.clear();
}

long SAVE_DIST(coeff_t succ, coeff_t t, long dist, bool print = false) {
    if_verbose(4) { print = true; }
    DISTS[cfpair_t(succ,t)] = dist_t(dist, 0, -1);
    if(print) { cerr << "dist " << succ << " " << t << " " << dist << endl; }
    return dist;
}

long EST_SAVE_DIST(coeff_t succ, coeff_t t, long dist, coeff_t aux, int max_depth = -1, bool print = false) {
    if_verbose(4) { print = true; }
    ESTDISTS[cfpair_t(succ,t)] = dist_t(dist, aux, max_depth);
    if(print && dist < INT_MAX) { cerr << "estdist " << succ << " -> " << t << " " << dist
                                       << " aux=" << aux << " md=" << max_depth << endl; }
    return dist;
}

inline int lookup_succ_dist(coeff_t succ, coeff_t t) {
    int d1=-1, d2=-1;
    distmap_t::const_iterator d;

    d = DISTS.find(cfpair_t(succ,t));
    if(d!=DISTS.end()) d1 = (*d).second.dist;

    d = ESTDISTS.find(cfpair_t(succ,t));
    if(d!=ESTDISTS.end()) d2 = (*d).second.dist;

    if(d1==-1) return d2;
    else if(d2==-1) return d1;
    else return min(d1, d2);
}

inline coeff_t lookup_succ_aux(coeff_t succ, coeff_t t) {
    distmap_t::const_iterator d;

    d = DISTS.find(cfpair_t(succ,t));
    if(d!=DISTS.end()) return 0;

    d = ESTDISTS.find(cfpair_t(succ,t));
    if(d!=ESTDISTS.end()) return (*d).second.aux;
    else return 0;
}

inline coeff_t lookup_succ_aux_max_depth(coeff_t succ, coeff_t t) {
    distmap_t::const_iterator d;

    d = DISTS.find(cfpair_t(succ,t));
    if(d!=DISTS.end()) return -1;

    d = ESTDISTS.find(cfpair_t(succ,t));
    if(d!=ESTDISTS.end()) return (*d).second.max_depth;
    else return 0;
}

inline int lookup_dist(coeff_t t) {
    distmap_t::const_iterator d = DISTS.find(cfpair_t(1,t));
    if(d!=DISTS.end()) return (*d).second.dist;
    else return -1;
}

inline int depth_estimate(coeff_t n) {
    return clog2(1+csd_bits(n));
}

#define Astar compute_fast_sumdiffs

coeff_t _add_dist(coeff_t succ, coeff_t t, int sdepth, int max_depth) {
    cfset_t s;
    int mincost = INT_MAX;
    if(max_depth >= 0 && (max_depth <= 1 || (sdepth > max_depth - 1))) return mincost;

    Astar(s, succ, t);
    FORALL(s, i) {
        int gap = (sdepth <= max_depth-2) ? 1 : 0; /* XXX */
        if(CONTAINS(SUCCS, *i)) {
            int gap_allowed = (READY[SUCCS[*i].c1].depth != READY[SUCCS[*i].c2].depth);
            if(max_depth < 0 || SUCCS[*i].depth <= max_depth-1+gap*gap_allowed) return 2;
        }

        /* check if there is a way to obtain *i without exceeding remaining depth  */
        gap = max_depth-1-sdepth;  /* XXX */
        if(max_depth >= 0 && depth_estimate(*i) > max_depth-1+gap) continue;
        coeff_t c = 1+cost_estimate(*i);
        if(c < mincost) {
            if(c==2) return 2;
            else mincost = c;
        }
    }
    return mincost;
}

#define LINE_NUM if_verbose(5) { cerr << __FILE__ << ":" << __LINE__ << " "; }
#define LINE_NUM1(p1) if_verbose(5) { cerr << __FILE__ << ":" << __LINE__ << "  sdepth=" << sdepth << " p=" << (p1) << " "; }
#define LINE_NUM2(p1,p2) if_verbose(5) { cerr << __FILE__ << ":" << __LINE__ << "  sdepth=" << sdepth << " p=" << (p1) << " p2=" << p2; }
#define LINE_NUM_CUST(lnum, par) if_verbose(5) { cerr << __FILE__ << ":" << lnum << " F  sdepth=" << sdepth << " p=" << par << " "; }

int estimate_target_dist(coeff_t succ, coeff_t t, int sdepth, int max_depth) {
    cfset_t s,ss;
    coeff_t mincost = INT_MAX, aux = 0;
    int dbg_line = 0; coeff_t dbg_par = 0;

    if(max_depth > 0 && sdepth >= max_depth) {
        /*LINE_NUM;*/ return EST_SAVE_DIST(succ, t, INT_MAX, 0);
    };

    Astar(s, succ, t);
    FORALL(s, z) {
        int gap = 0; //(sdepth < max_depth-1) ? 1 : 0;

        /* check if there is a way to obtain *z without exceeding remaining depth  */
        if(max_depth >= 0 && depth_estimate(*z) > max_depth-1+gap) continue;

        coeff_t c = 1 + cost_estimate(*z); /* (*z) should not be a cost-0 number (=1), */
        if(c < mincost) {                  /* it is caught by exact distance tests */
            mincost = c;
            if(mincost==DIST_TEST) { LINE_NUM1(*z); return EST_SAVE_DIST(succ, t, DIST_TEST, 0);}
            dbg_line = __LINE__;  dbg_par = *z;
        }
        // XXX
        /*if(mincost > 3 && CONTAINS(SUCCS2, *z)) {
                    mincost = 3;
                    if(mincost==DIST_TEST) { LINE_NUM; return EST_SAVE_DIST(succ, t, DIST_TEST, *z);}
                }
        */
    }

    /** mincost >= 3 */
    if(DIST_TEST==3 && mincost > 3) {
        cfset_t prods, _succ; ADD(_succ, succ);
        gen_mul_coeffs(COST_1, _succ, prods); // subtract_set(prods, COST_1); REMOVE(prods, succ);
        FORALL(prods, p) {
            coeff_t cc = 1 + _add_dist(*p, t, sdepth+1, max_depth); /* forward */
            if(cc<mincost) {
                mincost = cc;
                if(mincost==3) { LINE_NUM1(*p); return EST_SAVE_DIST(succ, t, 3, 0); }
                dbg_line = __LINE__;  dbg_par = (*p);
            }
        }

        //      if(max_depth < 0) /* use this test only if depth is unbounded, because with bounded depth
        //                           this distance test makes distance function inadmissible */
        FORALL(COST_1, j) {
            if(t % (*j) != 0) continue; // backward
            coeff_t cc = 1 + _add_dist(succ, t / (*j), sdepth, max_depth-1);
            if(cc<mincost) {
                mincost = cc; aux = t/(*j);
                if(mincost==3) { LINE_NUM1(t/(*j)); EST_SAVE_DIST(succ, aux, 2, 0, max_depth-1); return EST_SAVE_DIST(succ, t, 3, aux); }
                dbg_line = __LINE__;  dbg_par = aux;
            }
        }

        if(USE_EXPENSIVE_ESTIMATION) {
            /* variant 1 : t = A(succ, A(R, z)) */
            if(max_depth < 0 || sdepth<=max_depth-1) {
                FORALL(s, i) { /* s = Astar(succ, t) */
                    FORALL_FAM(READY, rr) {
                        coeff_t r = (*rr).first;
                        int rdepth = (*rr).second.depth;
                        if(max_depth >= 0 && (rdepth > max_depth-2)) continue;
                        int gap = 2*max_depth - 3 - sdepth - rdepth; /* gap can NOT be negative */
                        cfset_t Z;
                        Astar(Z, r, (*i));  /* pred = A_star(R, t) */
                        FORALL(Z, z) {
                            /* check if there is a way to obtain *z without exceeding remaining depth  */
                            int zdepth = depth_estimate(*z);
                            if(max_depth >= 0 && (zdepth > max_depth-2+gap)) continue;
                            coeff_t cc = 2+cost_estimate(*z);
                            if(cc < mincost) {
                                mincost = cc; aux = *i;
                                if(mincost==3) { LINE_NUM2(*z,r); EST_SAVE_DIST(succ, aux, 2, 0, max_depth-1); return EST_SAVE_DIST(succ, t, 3, aux); }
                                dbg_line = __LINE__; dbg_par = *z;
                            }
                        }
                    }
                }
            }
            /* variant 2 : t = A(R, A(succ, z)) */
            if(max_depth < 0 || sdepth<=max_depth-2) {

                s.clear();
                FORALL_FAM(READY, rr) {
                    coeff_t r = (*rr).first;
                    int rdepth = (*rr).second.depth;
                    if(rdepth > max_depth-1) continue;
                    Astar(s, r, t);
                }

                int gap = max_depth - 2 - sdepth; /* XXX should take rdepth into account */
                FORALL(s, i) { /* s = Astar(R, t) */
                    cfset_t Z;
                    Astar(Z, succ, (*i));  /* pred = A_star(R, t) */
                    FORALL(Z, z) {
                        /* check if there is a way to obtain *z without exceeding remaining depth  */
                        int zdepth = depth_estimate(*z);
                        if(max_depth >= 0 && (zdepth > max_depth-2+gap)) continue;
                        coeff_t cc = 2+cost_estimate(*z);
                        if(cc < mincost) {
                            mincost = cc; aux = *i;
                            if(mincost==3) { LINE_NUM1(*z); EST_SAVE_DIST(succ, aux, 2, 0, max_depth-1); return EST_SAVE_DIST(succ, t, 3, aux); }
                            dbg_line = __LINE__; dbg_par = *z;
                        }
                    }
                }
            }
        }
    }

    if(mincost < INT_MAX) {
        if(aux>1) { LINE_NUM_CUST(dbg_line, 0); EST_SAVE_DIST(succ, aux, mincost-1, 0, max_depth-1); }
        LINE_NUM_CUST(dbg_line, dbg_par);
    }
    return EST_SAVE_DIST(succ, t, mincost, aux);
}

coeff_t maxdepth(const full_addmap_t &coeffs);

inline coeff_t W(coeff_t dist) {
    //if(dist<3) dist=3;
    if(NO_WEIGHT || dist > 11) return 1;
    else return (coeff_t)(pow(13.0, 11.0-dist));
}

cfpair_t cumulative_benefit(coeff_t succ, int sdepth) {
    coeff_t total_benefit = 0;
    coeff_t max_benefit = 0;
    coeff_t b; //, bonus;
    coeff_t closest = 0;

    if(CONTAINS(TARGETS, succ))  return cfpair_t(INT_MAX,0); /* XXX check depth bound here */

    FORALL(TARGETS, t) {
        coeff_t R_t_dist = lookup_dist(*t);
        coeff_t Rs_t_dist = lookup_succ_dist(succ, *t);
        int max_depth = MAX_TARGET_DEPTH(*t);
        assert(R_t_dist!=-1);

        if ( Rs_t_dist == -1 ) {
            /* do not estimate if target is within exact distance test range */
            if ( R_t_dist <= DIST_TEST ) Rs_t_dist = R_t_dist;
            /* estimate */
            else Rs_t_dist = estimate_target_dist(succ, *t, sdepth, max_depth);
        }

        if(Rs_t_dist < R_t_dist) b = (R_t_dist - Rs_t_dist) * W(Rs_t_dist);
        else b = 0;

        total_benefit += b;
        if(b > max_benefit) { max_benefit = b; closest = *t; }
        if(MAX_BENEFIT && b > 0 && Rs_t_dist == 1) return cfpair_t(BEST_BENEFIT, closest);
    }

    return cfpair_t(MAX_BENEFIT ? max_benefit : total_benefit, closest);
}

/* Computes intersection of A and S and or all elements (*a) in the intersection
   that satisfy max_depth bound, saves distance (*a) -> t to dist.
   Returns true if intersection is non-null */
bool intersect_save_dist(cfset_t &A, full_addmap_t &S, coeff_t t, long dist, int max_depth) {
    bool found_intersection = false;
    FORALL(A, a) {
        if(CONTAINS(S, *a)) {
          if(max_depth >= 0 && S[*a].depth > max_depth) continue;
            SAVE_DIST( 1, t, dist);
            SAVE_DIST(*a, t, dist - 1);
            found_intersection = true;
        }
    }
    return found_intersection;
}

void init_target_dists() {
    if_verbose(4) { cerr << "\n-- init_target_dists --" << endl; }
    FORALL(TARGETS, tt)
        SAVE_DIST(1,*tt,1000);
}

void compute_target_dists_exact() {
    if_verbose(4) { cerr << "\n-- compute_target_dists_exact --" << endl; }
    FORALL(TARGETS, tt) {
        coeff_t t = (*tt);
        cfset_t t_div_c1, t_div_c2;
        // coeff_t succ;
        long dist = lookup_dist(t);
        int max_depth = MAX_TARGET_DEPTH(t);

        /* Distance 2 Exact Tests -------------------------------*/
        if(dist == 1) continue;

        if_verbose(4) { cerr << " ** (t / c1) ^ S  // dist 2, case 2 " << endl; }
        set_ratio(t_div_c1, t, COST_1);
        if(intersect_save_dist(t_div_c1, SUCCS, t, 2, max_depth-1)) dist=2;

        if_verbose(4) { cerr << " ** Astar(r,t) ^ S  // dist 2 case 1 " << endl; }
        FORALL_FAM(READY, rr) {
            coeff_t r = (*rr).first;
            cfset_t rt;
            if(max_depth >= 0 && (*rr).second.depth > max_depth-1) continue;

            Astar(rt, r, t);
            if(intersect_save_dist(rt, SUCCS, t, 2, max_depth-1)) dist=2;
        }

        /* Distance 3 Exact Tests -------------------------------*/
        if(dist < 3 || DIST_TEST < 3) continue;

        if_verbose(4) { cerr << " ** (t / c2) ^ S  // dist 3, cases 1,2 " << endl; }
        set_ratio(t_div_c2, t, COST_2);
        if(intersect_save_dist(t_div_c2, SUCCS, t, 3, max_depth-2)) dist=3;

        if_verbose(4) { cerr << " ** (Astar(r, t) / c1) ^ S  // dist 3 case 4" << endl; }
        FORALL_FAM(READY, rr) {
            coeff_t r = (*rr).first;
            cfset_t rt, rt_div_c1;

            if(max_depth >= 0 && (*rr).second.depth > max_depth-1) continue;
            Astar(rt, r, t); /* rt = Astar(r, t) */
            set_ratio(rt_div_c1, rt, COST_1);
            //if(CONTAINS(rt_div_c1, 513)) {cerr << "r=" << r; READY[r].output(cerr); print_set("rt=", rt, INT_MAX); cerr << endl; }
            if(intersect_save_dist(rt_div_c1, SUCCS, t, 3, max_depth-2)) dist=3;
        }

        if_verbose(4) { cerr << " ** Astar(r, t/c1) ^ S  // dist 3 case 3" << endl; }
        FORALL_FAM(READY, rr) {
            coeff_t r = (*rr).first;
            cfset_t rt2;
            if(max_depth >= 0 && (*rr).second.depth > max_depth-2) continue;
            FORALL(t_div_c1, z) Astar(rt2, r, *z); /* rt2 = Astar(r, t/c1) */
            if(intersect_save_dist(rt2, SUCCS, t, 3, max_depth-2)) dist=3;
        }

        if_verbose(4) { cerr << " ** Astar(S, t) ^ S  // dist 3 case 5" << endl; }
        FORALL_FAM(SUCCS, ss) {
            coeff_t s = (*ss).first;
            adder aa = (*ss).second;

            if(max_depth >= 0 && aa.depth > max_depth) continue;

            cfset_t st;
            Astar(st, s, t);
            int second_succ_depth_bound = max_depth - 1;
            /* the graph has two isomorphic equivalents, based on the equivalent, we
               have different balance of depths of the nodes */
            if(max_depth >= 0 && aa.depth == max_depth) {
                if (READY[aa.c1].depth < max_depth-1 || READY[aa.c2].depth < max_depth-1) {
                    if(READY[aa.c1].depth < max_depth - 1) REMOVE(st, aa.c1);
                    else REMOVE(st, aa.c2);
                    second_succ_depth_bound = max_depth-2;
                }
                else continue;
            }

            if(intersect_save_dist(st, SUCCS, t, 3, second_succ_depth_bound)) dist=3;
        }
    }

}

/* update dist to all targets once a successor is selected to be synthesized
   in the heuristic part */
void update_target_dists(coeff_t succ) {
    if_verbose(4) { cerr << "-- update_target_dists --" << endl; }
    FORALL(TARGETS, tt) {
        coeff_t t = *tt;
        long dist = lookup_succ_dist(succ, t);
        if ( dist > 0 ) {
          long orig_dist = lookup_dist(t);
          if(orig_dist > dist || orig_dist==-1)
              SAVE_DIST(1,t,dist);
        }
    }
}

void heur_synth_best() {
    coeff_t best_benefit = 0;
    coeff_t best_coeff = 0;
    coeff_t closest = 0;
    adder   best_adder;

    if_verbose(4) { cerr << "\n-- heur_synth_best --" << endl; }
    FORALL_FAM(SUCCS, s) {
        coeff_t c = (*s).first;
        adder   aa = (*s).second;
        cfpair_t ret = cumulative_benefit(c, aa.depth);
        coeff_t benefit = ret.first;
        coeff_t target = ret.second;

        if(benefit == BEST_BENEFIT) {
            best_coeff = c;
            best_adder = aa;
            closest = target;
            break;
        }
        else if(benefit > best_benefit) {
            best_benefit = benefit;
            best_coeff = c;
            best_adder = aa;
            closest = target;
        }
        /* Use some randomness here, this has potential to find better solutions
           over several runs of the tool. Earlier I just optimized for depth */
        else if(benefit == best_benefit &&
                (/*(aa.depth < best_adder.depth) ||*/
                 (/*aa.depth == best_adder.depth && */(OPTIMIZE_NOFS ? (c < best_coeff) : (rand() % 3 == 2)))
                )) {
            best_coeff = c;
            best_adder = aa;
            closest = target;
        }
    }

    if(best_coeff == 0) {
      /*cerr << endl; print_set("S = ", SUCCS, INT_MAX);
        cerr << endl; print_set("R = ", READY, INT_MAX); */
        ASSERT(best_coeff != 0);
    }
    ASSERT(closest!=0);

    if(USE_AUX_TARGETS) {
        // coeff_t mindist = INT_MAX;
        //coeff_t closest = 0;
        //      FORALL(TARGETS, tt) {
        //          coeff_t dist = lookup_succ_dist(best_coeff, *tt);
        //          if(dist < mindist) { mindist = dist; closest = *tt; }
        //      }
        coeff_t aux = lookup_succ_aux(best_coeff, closest);
        if(aux>1) {
            int max_target_depth = lookup_succ_aux_max_depth(best_coeff, aux);
            if(max_target_depth!=-1)
                DEPTH_LIMITS[aux] = max_target_depth;
            if_verbose(2) { cerr << "aux targ. " << aux << " md=" << max_target_depth
                                 << " (based on " << closest << ") "; }
            ADD(AUX_TARGETS[closest], aux);
            ADD(TARGETS, aux);
        }
    }
    update_target_dists(best_coeff);
    clear_est_dists(); /* XXX */
    if_verbose(2) cerr << "heur.syn. ";
    synthesize(best_coeff, best_adder);
}

void synthesize(coeff_t c, adder ad) {
    if(! (CONTAINS(READY, ad.c1) && CONTAINS(READY, ad.c2))) {
        cerr << "offending adder:";
        ad.output(cerr); cerr << " actual=" << ad.res << endl;
        assert(CONTAINS(READY, ad.c1) && CONTAINS(READY, ad.c2));
    }
    if(ad.depth==-1)
        ad.depth = 1 + max(READY[ad.c1].depth, READY[ad.c2].depth);

    if(CONTAINS(TARGETS, c)) {
        REMOVE(TARGETS, c);
        //if(AUX_TARGETS.find(c) != AUX_TARGETS.end()) {
        cfset_t aux = AUX_TARGETS[c];
        while(aux.size()!=0) {
            cfset_t newaux;
            FORALL(aux, a) {
                if(CONTAINS(TARGETS, *a)) {
                    if_verbose(2) { cerr << "remove aux. " << *a << " (based on " << c << ") "; }
                    REMOVE(TARGETS, *a);
                }
                unite_set(newaux, AUX_TARGETS[*a]);
            }
            aux = newaux;
        }
    }

    REMOVE(SUCCS, c);
    REMOVE(SUCCS2, c);
    READY[c] = ad;
    ADD(WORKLIST, c);

    if(GEN_CODE)   gen_code(c, ad);
    if(GEN_ADDERS) { ad.output(cout); cout << endl; }
    if(GEN_DAG) {
        reg_t dest;
        if(REGS->find(c) != REGS->end()) dest = (*REGS)[c];
        else dest = TMPREG();
        adag_node * n = ADAG->add_adder(&ad, dest, TMPREG, get_adder);
        if_verbose(1) {
            cerr << n->depth << " " << setw(10) << n->compute_coeff() << " ";
            n->output(cerr);
        }
    }
    if_verbose(2) cerr << c << "/" << ad.depth << " ";
    ++CURCOST;
}

void compute_succs2(coeff_t newsucc) {
    REMOVE(SUCCS2, newsucc);
    set_product(SUCCS2, newsucc, READY);
    compute_fast_sumdiffs(SUCCS2, newsucc, newsucc);
}

void compute_succs() {
    addmap_t newsuccs;
    cfset_t work = WORKLIST;
    full_addmap_t targets;
    if_verbose(5) { cerr << "\n-- compute_succs -- \n"; }
    WORKLIST.clear();

    if(USE_SUCCS2)
        set_product(SUCCS2, SUCCS, work);

    FORALL_FAM(READY, rr) {
        coeff_t r = (*rr).first;
        int rdepth = (*rr).second.depth;

        if(MAX_DEPTH >= 0 && rdepth > MAX_DEPTH-1) continue; /* skip if depth bound is violated */

        FORALL(work, w) {
            int wdepth = READY[*w].depth;
            int depth = 1 + max(rdepth, wdepth);
            if((MAX_DEPTH >= 0) && (wdepth > MAX_DEPTH-1)) continue; /* skip if depth bound is violated */

            newsuccs.clear();
            compute_all_sumdiffs(newsuccs, r, *w);
            FORALL_AM(newsuccs, s) {
                coeff_t c = (*s).first;
                sp_t sp = (*s).second;
                int max_depth = MAX_TARGET_DEPTH(c);
                if(CONTAINS(TARGETS, c) && (max_depth < 0 || depth <= max_depth)) {
                    if(!CONTAINS(targets, c) || depth < targets[c].depth)
                        targets[c] = adder(true, r, *w, sp, depth);
                }
                else if(! CONTAINS(READY, c)) {
                    bool is_new = ! CONTAINS(SUCCS, c);
                    if(USE_SUCCS2 && is_new)
                        compute_succs2(c);
                    if(is_new || depth < SUCCS[c].depth) {
                        SUCCS[c] = adder(false, r, *w, sp, depth);
                    }
                }
            }
        }
    }
    if(!targets.empty()) { if_verbose(2) cerr << "exact syn. "; }
    FORALL_FAM(targets, c) synthesize((*c).first, (*c).second);
}

void dump_state(ostream &os) {
    os << ITER << " | "
       << "curcost " << CURCOST << "/" << maxdepth(READY) << " | "
       << "succs " << SUCCS.size() << " " << SUCCS2.size() << " | ";
    FORALL(TARGETS, t)
        os << *t << "(" << lookup_dist(*t) << ") ";
    os << "|| ";
    //if_verbose(3) print_set("targets:", TARGETS, 0, os);
}

void dump_sets() {
    cerr << endl; print_set("R = ", READY, INT_MAX);
    cerr << endl; print_set("S = ", SUCCS, INT_MAX);
    cerr << endl; print_set("T = ", TARGETS, INT_MAX);
}

void _solve() {
    // coeff_t best;
    FORALL_FAM(READY, r) ADD(WORKLIST, (*r).first);
    CURCOST = ITER = HEUR = 0;
    init_target_dists();

    while(!TARGETS.empty()) {
        if_verbose(4) cerr << "\n************************ ITERATION " << ITER << "***********************\n";
        while(!WORKLIST.empty()) compute_succs();

        compute_target_dists_exact();
        if_verbose(2) { cerr << endl; dump_state(cerr); }

        if(!TARGETS.empty()) {
            if(!USE_AC1) {
                heur_synth_best();
            } else {
                ac1_decompose_cheapest();
            }
        }
        ++ITER;
    }
}

/*****************************************************************************/
#include "bhm.cpp"

void create_problem(cfset_t &coeffs, reg_t);

void solve(oplist_t *l, regmap_t *regs, reg_t (*tmpreg)()) {
    OPS = l; REGS = regs; TMPREG = tmpreg;
    if(GEN_CODE || GEN_DAG) {
        if(REGS==NULL) REGS = new regmap_t;
        ASSERT(TMPREG!=NULL);
    }

    if(USE_C1_ALGO) dempster_c1_solve();
    else if(BHM) bhm_solve();
    else _solve();

    if(GEN_DAG) {
        ADAG->to_multi();
        if(GEN_DOT) { cout << "digraph z{\n"; ADAG->output_dot(cout); cout << "}\n"; }
        if_verbose(1) {
            cerr << "--- reduced graph ---\n";
            ADAG->output_code(cerr);
        }
        if(ALLOCATE_REGS) {
            ADAG->compute_liveness(true);
            ADAG->allocate_regs();

            if_verbose(1) {
                cerr << " --- liveness ---\n";
                ADAG->output_liveness(cerr);
                cerr << "// minregs = " << ADAG->min_regs() << endl;
                cerr << "// allocated = " << ADAG->nregs << endl;
            }
        }
    }
    if(regs==NULL && REGS!=NULL) delete REGS;
}

void create_problem(cfset_t &coeffs, reg_t input_reg) {
    SUCCS.clear();   SUCCS2.clear();  TARGETS.clear();
    READY.clear();   WORKLIST.clear(); AUX_TARGETS.clear();
    clear_dists();
    if(GEN_DAG) { if(ADAG!=NULL) delete ADAG; ADAG = new adag(input_reg); }

    coeff_t max = 1, q, max_csd_bits = 0;
    FORALL(coeffs, c) {
        coeff_t tt = fundamental(cfabs(*c));
        if(1+csd_bits(tt) > max_csd_bits) max_csd_bits = 1+csd_bits(tt);

        if(SPLIT_2 && csd_bits(tt) >= SPLIT_THRESH) {
            long csd = csd_bits(tt);
            long ofs=0;
            while(csd_bits(tt>>ofs) > 3*csd/4) ++ofs;
            ADD(TARGETS, (q=fundamental(tt>>ofs))); if(q>max) max=q;
            ADD(TARGETS, (q=fundamental(tt & ((1<<ofs)-1)))); if(q>max) max=q;
        }
        else {
            ADD(TARGETS, tt); if(tt>max) max=tt;
        }
    }
    if_verbose(3) cerr << "Max csd bits = " << max_csd_bits
                       << "   min depth = " << clog2(max_csd_bits) << endl;
    if(MAX_DEPTH >= 0 && clog2(max_csd_bits) > MAX_DEPTH) {
        cerr << "Solution infeasible with MAX_DEPTH="
             << MAX_DEPTH << ". "
             << "Increasing MAX_DEPTH=" << clog2(max_csd_bits) << ".\n";
        MAX_DEPTH = clog2(max_csd_bits);
    }

    set_algo_bits(clog2(max));
    //if_verbose(1) { cerr << "Effective bits = " << clog2(max) << endl; }
    FORALL(GIVEN, g) {
        coeff_t gg = fundamental(cfabs(*g));
        READY[gg] = adder(gg);
        REMOVE(TARGETS, gg);
    }
    REMOVE(TARGETS, 1);
    SPEC_TARGETS = TARGETS;
}

coeff_t rand_coeff(coeff_t max) {
    coeff_t res = 0;
    /* we will generate 8 bits at a time */
    assert(RAND_MAX >= (1<<8)-1);
    for(size_t bytes=0; bytes < sizeof(coeff_t); ++bytes) {
        res = (res<<8) + rand() % (1<<8);
        //printf("%lld \n", res);
    }
    return 1 + (cfabs(res) % max);
}

void create_random_problem(int size) {
    cfset_t problem;
    for(int i = 0; i < size; ++i) {
        coeff_t c;
        if(GEN_UNIQUE) {
            do c = rand_coeff(MAX_GEN) | 1; /* odd numbers only */
            while ( c==1 || CONTAINS(problem, c) );
            ASSERT(fundamental(c)==c && c!=1);
        }
        else c = fundamental(rand_coeff(MAX_GEN));

        ADD(problem, c);
    }
    create_problem(problem);
}

void init_synth(int argc, char **argv) {
    ADD(GIVEN, 1);
    parse_cmdline(cmditer_new(argc, argv));
    ADD(COST_0, 1);
    gen_add_coeffs(COST_0, COST_0, COST_1);
    gen_add_coeffs(COST_0, COST_1, COST_2);
    gen_mul_coeffs(COST_1, COST_1, COST_2);

    subtract_set(COST_1, COST_0);
    subtract_set(COST_2, COST_0);
    subtract_set(COST_2, COST_1);
}
