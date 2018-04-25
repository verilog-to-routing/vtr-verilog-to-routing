#include "synth.cpp"

bool USE_TABLE = false;

coeff_t optcost(cfset_t &coeffs) {
    coeff_t res = 0;
    FORALL(coeffs,i) res += COSTS[*i];
    return res;
}

coeff_t csdcost(cfset_t &coeffs) {
    coeff_t res = 0;
    FORALL(coeffs,i) res += csd_bits(*i);
    return res;
}

coeff_t bitscost(cfset_t &coeffs) {
    coeff_t res = 0;
    FORALL(coeffs,i) res += nonzero_bits(*i) - 1;
    return res;
}

int main(int argc, char **argv) {
    srand(time(0));
    init_synth(argc-1, argv+1);

    double totalcsd = 0.0, totalbits = 0.0,
	   totalopt = 0.0;

    for(int n = 0; n < NTESTS; ++n) {
	create_random_problem(1);
	if(PRINT_TARGETS) {
	    cout << "  /* ";
	    print_set("targets:", TARGETS, 0, cout);
	    cout << " */ " << endl;
	}
	totalcsd += csdcost(TARGETS);
	totalbits += bitscost(TARGETS);
	if(USE_TABLE) totalopt += optcost(TARGETS);
    }

    if_verbose(1) cerr << endl;
    double mean_csd = totalcsd / NTESTS;
    double mean_bits = totalbits / NTESTS;
    double mean_opt = 0;
    if(USE_TABLE) mean_opt = totalopt / NTESTS;

    printf("%.2d  %6.2f %6.2f %4.2f%%", MAX_SHIFT-1,
	mean_bits, mean_csd, (mean_bits-mean_csd)/mean_bits*100);
    if(USE_TABLE)
        printf(" %6.2f %4.2f%%",
	   mean_opt, (mean_bits-mean_opt)/mean_bits*100);
    printf("\n");
    return 0;
}
