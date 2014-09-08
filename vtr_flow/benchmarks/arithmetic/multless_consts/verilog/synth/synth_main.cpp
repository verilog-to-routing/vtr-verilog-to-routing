#include "synth.cpp"

#include <stdio.h>

reg_t tmpreg() { static int c=100; return c++; }
reg_t destreg() { static int c=1; return c++; }
regmap_t regs;

int main(int argc, char **argv) {
    srand(time(0));
    init_synth(argc-1, argv+1);

    if(MODE == MODE_SINGLE) {
        create_problem(SPEC_TARGETS);
        if_verbose(1) {
            print_set("targets:", TARGETS, 0); cerr << endl;
            cerr << "maxcost: " << maxcost(TARGETS) << endl;
        }
        solve(0,&regs,tmpreg);
        if_verbose(1) {
            cerr << "solved: " << READY.size()-GIVEN.size() << endl;
            cerr << "depth: " << maxdepth(READY) << endl;
        }
        //print_set("ready:", READY, 0); cerr << endl;
    }
    else if(MODE == MODE_RANDOM) {
        int max_depth = MAX_DEPTH;
        FORALL_V(SIZES, size) {
            if_verbose(1) cerr << "size=" << *size << ": ";
            double totalmax = 0.0, totalsolv = 0.0, totaldepth = 0.0, totalsolv_sq = 0.0;
            double totalregs = 0.0;
            for(int n = 0; n < NTESTS; ++n) {
                /* MAX_DEPTH is auto adjusted if unfeasible, restore here to original user setting */
                MAX_DEPTH = max_depth;
                create_random_problem(*size);
                if(PRINT_TARGETS) {
                    cout << "  /* ";
                    print_set("targets:", TARGETS, 0, cout);
                    cout << " */ " << endl;
                }
                coeff_t max = maxcost(TARGETS);
                solve(0,&regs,tmpreg);
                coeff_t solv = READY.size()-GIVEN.size();
                totalmax += max;
                totalsolv += solv;
                totalsolv_sq += ((double)solv) * ((double)solv);
                totaldepth += maxdepth(READY);
                if(ALLOCATE_REGS && GEN_DAG) {
                    totalregs += ADAG->nregs;
                    if_verbose(1) cerr << ADAG->nregs << " ";
                }
                else if_verbose(1) cerr << solv << " ";
            }
            if_verbose(1) cerr << endl;
            double mean_solv_sq = totalsolv_sq / NTESTS;
            double mean_solv = totalsolv / NTESTS;
            double mean_dep = totaldepth / NTESTS;
            double mean_max = totalmax / NTESTS;
            double mean_regs = totalregs / NTESTS;
            double sd = sqrt(mean_solv_sq - mean_solv*mean_solv);
            /*<< mean_solv/mean_max * 100.0 << "%" */
            if(!GEN_DOT)
                printf("%3d %6.2f %6.2f  dep %4.2f sd %3.2f ci95 %6.2f %6.2f ", ((int) *size) ,
                   mean_max, mean_solv, mean_dep, sd, mean_solv - 2*sd, mean_solv + 2*sd);
            if(ALLOCATE_REGS && GEN_DAG) printf("regs %6.2f", mean_regs);
            printf("\n");
        }
    }
    else if(MODE == MODE_EXHAUST) {
        double totalmax = 0.0, totalsolv = 0.0, totaldepth = 0.0;
        cfset_t s;
        for(int n = 1; n <= MAX_GEN; ++n) {
            s.clear(); ADD(s, n);
            create_problem(s);
            solve(0,&regs,tmpreg);
            totalmax += maxcost(s);
            totalsolv += READY.size()-GIVEN.size();
            totaldepth += maxdepth(READY);
            if_verbose(1) if(n % 100 == 0) {
                fprintf(stderr, "%g ", totalsolv/n); fflush(stderr);
            }
        }
        if_verbose(1) cerr << endl;
        cerr << totalmax / MAX_GEN << "  " << totalsolv / MAX_GEN << "  "
             << totalsolv/totalmax * 100.0 << "%"
             << "  dep " << totaldepth/NTESTS << endl;
    }
    return 0;
}
