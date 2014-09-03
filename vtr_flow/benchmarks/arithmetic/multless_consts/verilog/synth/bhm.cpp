#include <algorithm>

/****************************************************************************/
void print_set(const char *nam, cfset_t &s, int max_nprint, ostream &ostr) {
    int nprinted = 0;
    ostr << nam << " ";
    FORALL(s, i) {
        ostr << *i << " ";
        ++nprinted;
        if(nprinted == max_nprint) break;
    }
}

void print_set(const char *nam, full_addmap_t &s, int max_nprint, ostream &ostr) {
    int nprinted = 0;
    ostr << nam << " ";
    FORALL_FAM(s, i) {
        ostr << (*i).first << " ";
        ++nprinted;
        if(nprinted == max_nprint) break;
    }
}

void _print_set(cfset_t &s) { print_set("", s, INT_MAX); }
void _print_set(full_addmap_t &s) { print_set("", s, INT_MAX); }
void _print_set_set(cfset_t &s) { print_set("", s, INT_MAX); }
void _print_set_map(full_addmap_t &s) { print_set("", s, INT_MAX); }

void state() {
    print_set("TARGETS", TARGETS); cerr << endl;
    print_set("SUCCS  ", SUCCS  ); cerr << endl;
    print_set("READY  ", READY  ); cerr << endl;
}

void set_bits(int b) {
    MAX_SHIFT = 1 + b;
    MAX_GEN = ((coeff_t)1) << b;
    MAX_NUM = (((coeff_t)1) << (1+b)) ;
}
void set_algo_bits(int b) {
    MAX_SHIFT = 1 + b;
    MAX_NUM = (((coeff_t)1) << (1+b)) ;
}

void parse_cmdline(cmditer_t *ci) {
    char * st;
    while((st = cmditer_pop(ci)) != NULL) {
        string s = st;
        if(s == "-r") {
            MODE = MODE_RANDOM;
            NTESTS = cmditer_pop_int(ci, "-r");
            SIZES.push_back(cmditer_pop_int(ci, "-r"));
            int s;
            while((s = cmditer_maybe_pop_posint(ci)) > 0) SIZES.push_back(s);
        }

        else if(s == "-maxdepth") {
            MAX_DEPTH = cmditer_pop_int(ci, "-maxdepth");
            USE_EXPENSIVE_ESTIMATION = true;
            USE_AUX_TARGETS = true;
        }

        else if(s == "-maxb") MAX_BENEFIT = true;
        else if(s == "-d2") DIST_TEST = 2;
        else if(s == "-d3") DIST_TEST = 3;
        else if(s == "-ac1") {
            USE_AC1 = true;
            ac1_init(cmditer_pop_string(ci, "-ac1 (.chains file)"));
        }
        else if(s == "-nw")    NO_WEIGHT = true;
        else if(s == "-estv")   EST_MODE = EST_VALUE;
        else if(s == "-esth")   EST_MODE = EST_HYBRID;
        else if(s == "-estopt")   EST_MODE = EST_OPT_TABLE;
        else if(s == "-hc")   HYB_CUT = cmditer_pop_int(ci, "-hc");

        else if(s == "-nofs")    OPTIMIZE_NOFS = true;
        else if(s == "-aux")     USE_AUX_TARGETS = true;
        else if(s == "-expensive")     USE_EXPENSIVE_ESTIMATION = true;
        else if(s == "-iac1")    IMPROVE_AC1 = true;
        else if(s == "-ex")      MODE = MODE_EXHAUST;
        else if(s == "-v")       VERBOSE = cmditer_pop_int(ci, "-v");
        else if(s == "-succs2")  USE_SUCCS2 = true;
        else if(s == "-c1algo")  USE_C1_ALGO = true;
        else if(s == "-seed")    srand(cmditer_pop_int(ci, "-seed (rand() seed)"));
        else if(s == "-b")       set_bits(cmditer_pop_int(ci, "-b (bits of maximum generated coeff)"));
        else if(s == "-gu")      GEN_UNIQUE = true;
        else if(s == "-gc")      GEN_CODE = true;
        else if(s == "-ga")      GEN_ADDERS = true;
        else if(s == "-gd")      { GEN_DAG = true; ALLOCATE_REGS = true; }
        else if(s == "-gdot")    { GEN_DAG = true; GEN_DOT = true; }
        else if(s == "-pt")      PRINT_TARGETS = true;
        else if(s == "-mg")      MAX_GEN = cmditer_pop_int(ci, "-mg (maximum generated coeff)");
        else if(s == "-ms")      MAX_SHIFT = cmditer_pop_int(ci, "-ms (maximum shift)");
        else if(s == "-g")       ADD(GIVEN, cmditer_pop_int(ci, "-g (given coefficient)"));
        else if(s == "-bhm")     BHM = true;

        else if(s == "-split2")  {
            SPLIT_2 = true;
            SPLIT_THRESH = cmditer_pop_int(ci, "-split2");
        }

        /* these options are interpreted by acm only, and ignored by synth */
        else if(s == "-gap")     ACM_OUTPUT = ACM_OUT_GAP;
        else if(s == "-dot")     ACM_OUTPUT = ACM_OUT_DOT;
        else if(s == "-dotcode") { ACM_OUTPUT = ACM_OUT_DOT_CODE;
                                   ACM_DOT_FILE = cmditer_pop_string(ci, "-dotcode (output .dot file)"); }
        else if(s == "-code")     ACM_OUTPUT = ACM_OUT_CODE;
        else if(s == "-noheader") ACM_HEADER = false;

        else { /* not a recognized option, must be a coefficient */
            cmditer_unpop(ci);
            coeff_t c = cmditer_pop_int(ci, "targets");
            ADD(SPEC_TARGETS, fundamental(cfabs(c)));
            SPEC_ALL_TARGETS.push_back(c);
        }
    }
}

int nonzero_bits(coeff_t c) {
    int res = 0, bits = 0;
    while(c) { if(c&1) ++res; c = (c>>1); ++bits; }
    return res; //min(res,bits-res);
}
int min_bits(coeff_t c) {
    int res = 0, bits = 0;
    while(c) { if(c&1) ++res; c = (c>>1); ++bits; }
    return min(res,bits-res);
}

int cost_estimate(coeff_t c) {
  switch(EST_MODE) {
  case EST_OPT_TABLE:
    return COSTS[c];
  case EST_VALUE:
    return clog2(c);
  case EST_HYBRID: /* CSD for smaller values, log2 for larger values */
    return 2*csd_bits(c) + clog2(c);
  case EST_CSD:
    return csd_bits(c);
  default:
    assert(0 && "invalid value of EST_MODE in cost_estimate()");
  }
}

/* Upper bound (cost of computing each multiplication separately) */
coeff_t maxcost(cfset_t &coeffs) {
    coeff_t res = 0;
    FORALL(coeffs,i) res += cost_estimate(*i);
    return res;
}

coeff_t maxdepth(const full_addmap_t &coeffs) {
    int max = -1;
    FORALL_FAM(coeffs,i) {
        int d = (*i).second.depth;
        if(d > max) max = d;
    }
    return max;
}

void gen_code(coeff_t c, const adder &ad) {
    coeff_t c1 = ad.c1, c2 = ad.c2;
    spref_t sp = ad.sp;
    reg_t dest; bool is_target = false;
    if(!CONTAINS(*REGS, c)) {
        if(CONTAINS(SPEC_TARGETS, c)) is_target = true;
        dest = (*REGS)[c] = TMPREG();
    }
    else dest = (*REGS)[c];
    oplist_t l;
    sumdiff_ops(l, (*REGS)[c], (*REGS)[c1], (*REGS)[c2], TMPREG, c1, c2, sp);
    if(OPS==NULL) for(oplist_t::iterator i = l.begin(); i != l.end(); ++i) {
        cout << "#ifdef USE_CHAINS" << endl;
        if((*i)->res == dest && is_target)
            cout << "target_decl(" << dest << ") ";
        else
            cout << "decl(" << (*i)->res << ") ";
        (*i)->output(cout);
        cout << "#else" << endl;
        if((*i)->res == dest && is_target)
            cout << "target_decl(" << dest << ") t" << dest << " = "  << c << " * t0;" << endl;
        cout << "#endif" << endl;
    }
    else for(oplist_t::iterator i = l.begin(); i != l.end(); ++i)
        OPS->push_back(*i);
}

adder *get_adder(coeff_t c) {
  return & READY[c];
}

void ac1_decompose_cheapest() {
    coeff_t mincost = INT_MAX, mindist = INT_MAX, cc;
    coeff_t cheapest  = 0, closest = 0;
    FORALL(TARGETS, t) {
        /* we select cheapest coeff of minimum value */
        if((cc=cost_estimate(*t)) < mincost) { mincost=cc; cheapest=*t; }
        if(cc == mincost && *t < cheapest) cheapest = *t;
        if((cc=lookup_dist(*t)) < mindist) { mindist = cc; closest = *t; }
    }
    assert(cheapest!=0 && closest!=0 && mindist > 1);

    if(mindist==2) {
        if(IMPROVE_AC1) {
            FORALL_FAM(SUCCS, ss) {
                coeff_t s = (*ss).first;
                cfset_t tmp;
                compute_fast_sumdiffs(tmp, s, closest);
                FORALL(tmp, t) {
                    if(*t == s || CONTAINS(READY, *t)) {
                        synthesize(s, SUCCS[s]); return;
                    }
                }
            }
            assert(0);
        }
        else {
            FORALL(TARGETS, t) {
                FORALL_FAM(READY, r1) {
                    coeff_t cand1 = fundamental(cfabs(*t - (*r1).first));
                    if(CONTAINS(COST_1, cand1)) {
                        synthesize(cand1, SUCCS[cand1]); return;
                    }
                    FORALL_FAM(READY, r2) {
                        coeff_t cand2 = (*r1).first + (*r2).first;
                        if(fundamental(cfabs(*t - cand2)) == 1) {
                            cand2 = fundamental(cand2);
                            synthesize(cand2, SUCCS[cand2]); return;
                        }
                    }
                }
            }
        }
    }

    cfset_t interm;
    ac1_get_chain(cheapest)->get_intermediates(interm, ac1_get_chain);
    FORALL(interm, i)
        if(!CONTAINS(READY, *i)) ADD(TARGETS, *i);
    SUCCS.clear();
    FORALL_FAM(READY, r) ADD(WORKLIST, (*r).first);

}

adder bhm_plus(bool is_target, coeff_t c1, coeff_t c2) {
    (void) is_target;
    sp_t sp = sp_t(1+compute_shr(c1),1+compute_shr(c2));
    c1 = fundamental(c1);
    c2 = fundamental(c2);
    return adder(false, c1, c2, sp.normalize(c1, c2));
}

adder bhm_minus(bool is_target, coeff_t c1, coeff_t c2) {
    (void) is_target;
    sp_t sp;
    if(c1 > c2) sp = sp_t(1+compute_shr(c1), -1-compute_shr(c2));
    else        sp = sp_t(-1-compute_shr(c1), 1+compute_shr(c2));
    c1 = fundamental(c1);
    c2 = fundamental(c2);
    return adder(false, c1, c2, sp.normalize(c1, c2));
}

void bhm_fill_ready(coeff_t fund, adder a, coeff_t maxtarget) {
    coeff_t c = fund;
    while(c <= 2*maxtarget) {
        READY[c] = a;
        c = 2*c;
    }
}

void bhm_iter_status(coeff_t target, coeff_t error, coeff_t best, coeff_t closest) {
    if_verbose(2) { cerr << " T=" << target << " error=" << error
                         << " best=" << best << " (" << fundamental(best) << ")"
                         << " closest=" << closest << " (" << fundamental(closest) << ")"
                         << endl; }
}

void bhm_reduce_ready() {
    full_addmap_t new_r;
    FORALL_FAM(READY, rr) {
        coeff_t r = (*rr).first;
        if(r % 2 == 1)
            new_r[r] = (*rr).second;
    }
    READY=new_r;
}

coeff_t bhm_update_closest(coeff_t old_closest, coeff_t target, coeff_t best) {
    coeff_t closest;
    if(old_closest < target) {
        closest = old_closest + best;
        if(closest!=best)
            synthesize(fundamental(closest),  bhm_plus(false, old_closest, best));
    }
    else {
        closest = old_closest - best;
        if(closest!=best)
            synthesize(fundamental(closest),  bhm_minus(false, old_closest, best));
    }
    return closest;
}

bool cheaper(coeff_t c1, coeff_t c2) {
    return cost_estimate(c1) < cost_estimate(c2);
}

void bhm_solve() {
    cfvec_t stargets;
    coeff_t maxtarget = 0;
    FORALL(TARGETS, t) {
        stargets.push_back(*t);
        if(*t > maxtarget) maxtarget = *t;
    }
    sort(stargets.begin(), stargets.end(), cheaper);
    bhm_fill_ready(1, adder(1), maxtarget);

    ITER = 0;
    FORALL_V(stargets, t) {
        if(CONTAINS(READY, *t)) continue;

        coeff_t error = *t;
        coeff_t closest = 0;
        while(error != 0) {
            if(CONTAINS(READY, error)) {
                closest = bhm_update_closest(closest, *t, error);
                error = 0;
            }
            else {
                coeff_t best = 0;
                adder   best_adder;

                FORALL_FAM(READY, rr1) {
                    FORALL_FAM(READY, rr2) {
                        coeff_t r1   = (*rr1).first, r2 = (*rr2).first;
                        coeff_t sum  = r1 + r2;
                        coeff_t diff = cfabs(r1 - r2);

                        if(cfabs(error - sum)  <  cfabs(error - best)) {
                            best = sum;
                            best_adder = bhm_plus(false, r1, r2);
                        }
                        if(cfabs(error - diff)  <  cfabs(error - best)) {
                            best = diff;
                            best_adder = bhm_minus(false, r1, r2);
                        }
                    }
                }
                error = cfabs(error - best);
                synthesize(fundamental(best), best_adder);
                closest = bhm_update_closest(closest, *t, best);
                bhm_iter_status(*t, error, best, closest);
                bhm_fill_ready(fundamental(best), best_adder, maxtarget);
            }
            ++ITER;
        }
        assert(closest == *t);
    }

    bhm_reduce_ready();
}


void dempster_c1_solve() { /* C1 depth minimization algorithm due to A.Dempster */
    cfset_t targets = TARGETS;
    FORALL(COST_1, c) ADD(targets, *c);
    create_problem(targets);
    _solve();
    /* remove unnecessary cost-1 coefficients */
    int target_depth = maxdepth(READY);
    FORALL(COST_1, c) {
        REMOVE(targets, *c);
        create_problem(targets);
        _solve();
        if(maxdepth(READY) != target_depth) ADD(targets, *c);
    }
    /* final solution */
    create_problem(targets);
    _solve();
}
