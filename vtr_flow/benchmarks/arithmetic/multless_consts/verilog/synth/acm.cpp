#undef SYNTH_MAIN
#include "synth.cpp"
#include <fstream>

void   acm_init() {}

void   acm_finish() {}

void   acm_solve(oplist_t &l, const vector<coeff_t> &coeffs,
                 const vector<reg_t> &dests, reg_t src, reg_t (*tmpreg)()) {
    ASSERT(dests.size() == coeffs.size());

    cfset_t problem;
    regmap_t * regs = new regmap_t;

    regs->clear();
    (*regs)[1] = src;

    for(int i = coeffs.size()-1; i >= 0; --i) {
        if(coeffs[i]!=1)
          (*regs)[coeffs[i]] = dests[i];
    }

    /* compute registers for all fundamentals ('reduced' constants) */
    for(size_t i = 0; i < coeffs.size(); ++i) {
        coeff_t full    = coeffs[i];
        coeff_t reduced = fundamental(cfabs(full));
        if(reduced!=1)
            ADD(problem, reduced);
        if(!CONTAINS(*regs, reduced))
            (*regs)[reduced] = tmpreg();
    }

    if_verbose(1) cerr<<"synthesizing "<<problem.size()<<" unique fundamentals\n";

    create_problem(problem);
    GEN_CODE = true;
    solve(&l, regs, tmpreg);

    for(size_t i = 0; i < coeffs.size(); ++i) {
        coeff_t full    = coeffs[i];
        reg_t   dest    = dests[i];
        coeff_t reduced = fundamental(cfabs(full));
        if(full == 1)
            l.push_back(op::shl(dest, src, 0, full)); /* move dest <- src */

        else if(full == -1)
            l.push_back(op::neg(dest, src, full)); /* move dest <- -1*src */

        /* last op */
        else if(dest == (*regs)[full]) {
            if(full != reduced) {
                int t = (*regs)[reduced];

                /* see if we already computed the negative of it */
                int found_neg = -1;
                for(size_t j=0; j<i; ++j) {
                    if(coeffs[j] == -full) {
                        found_neg = j;
                        break;
                    }
                }
                /* if not */
                if(found_neg == -1) {
                    /* this is the first instance */
                    int shr = compute_shr(cfabs(full));
                    if(shr!=0) {
                        reg_t tmp = dest;
                        if(full < 0) {
                            tmp = tmpreg();
                            for(size_t j = i+1; j < coeffs.size(); ++j) {
                                if(coeffs[j] == -full) {
                                    tmp = dests[j];
                                    break;
                                }
                            }
                        }
                        l.push_back(op::shl(tmp, (*regs)[reduced], shr, cfabs(full)));
                        t = tmp;
                    }
                    /* negate if necessary */
                    if(full<0) l.push_back(op::neg(dest,t, full));
                }
                else /* negative was already computed, just negate */
                    if(full < 0) l.push_back(op::neg(dest,dests[found_neg],full));
            }
        }
        else /* we already computed it */
            l.push_back(op::shl(dest, (*regs)[full], 0, full));
    }
    delete regs;
}

#ifdef ACM_MAIN
size_t TMP_REG_NUM = 1; /* updated in main() */
int DEST_REG_NUM = 1;

reg_t tmpreg() { return TMP_REG_NUM++; }
reg_t destreg() { return DEST_REG_NUM++; }

int main(int argc, char **argv) {
    srand(time(0));
    acm_init();
    init_synth(argc-1, argv+1);

    vector<coeff_t> targets;
    vector<reg_t> dests;
    int i=0;
    targets = SPEC_ALL_TARGETS;
    dests.resize(targets.size());
    if(targets.size() >= TMP_REG_NUM)
      TMP_REG_NUM = targets.size() + 1;

    FORALL_V(targets, t) {
        dests[i] = destreg();
        ++i;
    }

    oplist_t ops;
    acm_solve(ops, targets, dests, 0, tmpreg);

    if(ACM_OUTPUT == ACM_OUT_GAP) {
      cout << "[\n";
      for(oplist_t::iterator i = ops.begin(); i!=ops.end(); ++i)
        (*i)->output_gap(cout);
      cout << "];\n";
    }
    if(ACM_OUTPUT == ACM_OUT_DOT || ACM_OUTPUT == ACM_OUT_DOT_CODE) {
        ostream * out;
        if(ACM_OUTPUT == ACM_OUT_DOT_CODE) {
            out = new ofstream(ACM_DOT_FILE.c_str());
        }
        else { out = &cout; }

        if(! *out) {
            cerr << " Can not open '" << ACM_DOT_FILE << "' for writing. Skipping dag generation." << endl;
        }
        else {
            *out << "digraph z {\n";
            *out << " Node [peripheries=0]\n";
            *out << "X [fontsize=23]\n" ;
            for(oplist_t::iterator i = ops.begin(); i!=ops.end(); ++i)
                (*i)->output_dot(*out);
            for(size_t i=0; i < targets.size(); ++i)
                *out << "Y" << i+1 << " [fontsize=23]\n"
                     << "t" << dests[i] << " -> Y" << i+1 << "\n";
            *out << "{rank=same; ";
            for(size_t i=0; i < targets.size(); ++i) *out << "Y" << i+1 << " ";
            *out << "}\n";
            *out << "}\n";
        }
        if(ACM_OUTPUT == ACM_OUT_DOT_CODE)
            dynamic_cast<ofstream*>(out) -> close();
    }
    if(ACM_OUTPUT == ACM_OUT_CODE || ACM_OUTPUT == ACM_OUT_DOT_CODE) {
        int opcount[op::OPLAST];
        if(ACM_HEADER) {
        /* header */
        for(int i=0; i<op::OPLAST; ++i) opcount[i]=0;
        for(oplist_t::iterator i = ops.begin(); i!=ops.end(); ++i)
            ++opcount[(*i)->type];
        cout << "// -----------------------------------------------------------------------------\n";
        cout << "// This code was generated by Spiral Multiplier Block Generator, www.spiral.net\n";
        cout << "// Copyright (c) 2006, Carnegie Mellon University\n";
        cout << "// All rights reserved.\n";
        cout << "// The generated code is distributed under a BSD style license\n";
        cout << "// (see http://www.opensource.org/licenses/bsd-license.php)\n";
        cout << "// -----------------------------------------------\n" ;
        cout << "// Cost: "
             << opcount[op::ADD]+opcount[op::SUB] << " adds/subtracts "
             << opcount[op::SHL]+opcount[op::SHR] << " shifts "
             << opcount[op::NEG] << " negations\n";
        cout << "// Depth: " << maxdepth(READY) << "\n";
        cout << "// Input: \n//   int t0\n";
        cout << "// Outputs: \n";
        for(size_t i=0; i < targets.size(); ++i)
            cout << "//   int t" << dests[i] << " = " << targets[i] << " * t0\n" ;
        cout << "// -----------------------------------------------\n" ;
        }
        /* code */
        for(oplist_t::iterator i = ops.begin(); i!=ops.end(); ++i)
            (*i)->output(cout);
    }

    acm_finish();
}
#endif

