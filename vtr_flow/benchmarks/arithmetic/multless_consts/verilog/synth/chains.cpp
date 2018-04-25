#include "chains.h"
#include <map>

#define CKPR if(PRINT_CHAINS || PRINT_INSTRS)
int MAX_NPRINT = 5;
bool PRINT_INSTRS = false;
bool PRINT_CHAINS = false;
typedef enum { CHAINS_PLAIN, CHAINS_C } t_print_chains_mode;
t_print_chains_mode PRINT_CHAINS_MODE = CHAINS_PLAIN;

bool PRINT_ALL_CHAINS = true;

bool PRINT_COSTS = false;
typedef enum { COSTS_PLAIN, COSTS_C } t_print_costs_mode;
t_print_costs_mode PRINT_COSTS_MODE = COSTS_C;
int MIN_SHIFT = 0;

coeff_t MAX_NUM;
int MAX_SHIFT;
int MAX_PRINT;
int MAX_COST;
cost_t *GEN;
coeff_t GEN_COUNT;
cfvec_t *COSTVECS;

chain *CUR_CHAIN = 0;

typedef map<coeff_t, pair<int,int> > addmap_t;
typedef addmap_t::const_iterator     amiter_t;

static reg_t curreg = 2;
void reset_tmpreg() { curreg=2; }
reg_t tmpreg() { return curreg++; }

void output_current_chain(cost_t dest_cost, coeff_t coeff) {
	(void)dest_cost;
    if(PRINT_CHAINS /*&& coeff <= MAX_PRINT*/) {
	if(PRINT_CHAINS_MODE == CHAINS_PLAIN)
	    CUR_CHAIN->output(cerr);
	else {
	    cerr << "    CHAIN(" << coeff << ", ";
	    CUR_CHAIN->output_c(cerr);
	    cerr << "); \n";
	}
    }
    if(PRINT_INSTRS && coeff <= MAX_PRINT) {
	oplist_t l;
	reset_tmpreg();
	cerr << coeff << endl;
	CUR_CHAIN->get_ops(l, 0, 1, tmpreg);
	for(oplist_t::iterator i = l.begin(); i != l.end(); ++i) {
	    (*i)->output(cerr);
	    delete (*i);
	}
    }
}

void generate_initial() {
    COSTVECS[0].push_back(1);
}

inline void insert(cost_t dest_cost, coeff_t coeff) {
    ASSERT(coeff >= 0); /* overflow check */
    if(coeff==1 || coeff >= MAX_NUM)
	;
    else if(GEN[coeff] >= 0) {
	CKPR {
	    if(PRINT_ALL_CHAINS && GEN[coeff]==dest_cost)
		output_current_chain(dest_cost, coeff);
	}
    }
    else {
        if(coeff <= MAX_PRINT)
	    ++GEN_COUNT;
	CKPR output_current_chain(dest_cost, coeff);
	GEN[coeff] = dest_cost;
	COSTVECS[dest_cost].push_back(coeff);
    }
    CKPR {
	if(CUR_CHAIN!=0) { delete CUR_CHAIN; CUR_CHAIN = 0; }
    }
}

void _insert_all_sumdiffs(bool is_left, cost_t dest_cost, coeff_t num1, coeff_t num2) {
    for(int shift = MIN_SHIFT; shift <= MAX_SHIFT; ++shift) {
	coeff_t sum, diff;
	diff = num1 - (num2<<shift);
	if(diff != 0) {
	    sum = num1 + (num2<<shift);
	    CKPR { CUR_CHAIN = new add_chain(dest_cost, num1, num2, SUM(is_left, shift)); }
	    insert(dest_cost, fundamental(sum));
	    if(diff > 0) {
		CKPR CUR_CHAIN = new add_chain(dest_cost, num1, num2, DIFF(is_left, shift));
	    }
	    else {
		CKPR CUR_CHAIN = new add_chain(dest_cost, num1, num2, RDIFF(is_left, shift));
		diff = -diff;
	    }
	    insert(dest_cost, fundamental(diff));
	}
    }
}

void insert_all_sumdiffs(cost_t dest_cost, coeff_t num1, coeff_t num2) {
    /* false/false since nums are actually remembered in chain structure */
    _insert_all_sumdiffs(false, dest_cost, num1, num2);
    _insert_all_sumdiffs(false, dest_cost, num2, num1);
}

void generate_add_coeffs(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    // cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
	for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
	    insert_all_sumdiffs(dest_cost, *s1, *s2);
	}
    }
}

void generate_mul_coeffs(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    // cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
	for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
	    coeff_t prod = (*s1) * (*s2);
	    CKPR CUR_CHAIN = new mult_chain(dest_cost, *s1, *s2);
	    insert(dest_cost, fundamental(prod));
	}
    }
}

void generate_leapfrog2(cost_t src1_cost, cost_t src2_cost, cost_t dest_cost) {
    cfvec_t &src1 = COSTVECS[src1_cost];
    cfvec_t &src2 = COSTVECS[src2_cost];
    // cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
	addmap_t leap1;
	compute_all_sumdiffs(leap1, *s1, 1);
	for(amiter_t l1 = leap1.begin(); l1 != leap1.end(); ++l1) {
	    for(cfviter_t s2 = src2.begin(); s2 != src2.end(); ++s2) {
		coeff_t prod = (*l1).first * (*s2);
		if(prod < MAX_NUM) {
		    addmap_t leap2;
		    prod = fundamental(prod);
		    compute_all_sumdiffs(leap2, prod, *s1);
		    for(amiter_t l2 = leap2.begin(); l2 != leap2.end(); ++l2) {
			CKPR CUR_CHAIN = new leapfrog2_chain(
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
    // cfvec_t &dest = COSTVECS[dest_cost];
    for(cfviter_t s1 = src1.begin(); s1 != src1.end(); ++s1) {
	addmap_t leap1;
	compute_all_sumdiffs(leap1, *s1, 1);
	for(amiter_t l1 = leap1.begin(); l1 != leap1.end(); ++l1) {
	    addmap_t leap2;
	    compute_all_sumdiffs(leap2, (*l1).first, *s1);
	    for(amiter_t l2 = leap2.begin(); l2 != leap2.end(); ++l2) {
		for(cfviter_t s3 = src3.begin(); s3 != src3.end(); ++s3) {
		    coeff_t prod = (*l2).first * (*s3);
		    if(prod < MAX_NUM) {
			addmap_t leap3;
			prod = fundamental(prod);
			compute_all_sumdiffs(leap3, prod, (*l1).first);
			for(amiter_t l3 = leap3.begin(); l3 != leap3.end(); ++l3) {
			    CKPR CUR_CHAIN = new leapfrog3_chain(
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

void output_coeffs(cost_t cost) {
    cfvec_t &coeffs = COSTVECS[cost];
    int nprinted = 0;
    printf("cost=%d: ", cost);
    for(cfviter_t i=coeffs.begin(); i != coeffs.end(); ++i) {
	if(*i <= MAX_PRINT) {
	    printf(COEFF_FMT " ", *i);
	    ++nprinted;
	}
	if(nprinted > MAX_NPRINT) {
	    printf(" ..."); break;
	}
    }
    printf("\n");
}

void output_stats() {
    coeff_t total = 0;
    coeff_t* count = new coeff_t[MAX_COST+1];
    for(cost_t cost = 0; cost <= MAX_COST; ++cost)
	count[cost] = 0;
    for(coeff_t num = 1; num <= MAX_PRINT; ++num) {
	if(GEN[num] >= 0) {
	    ++count[GEN[num]];
	    ++total;
	}
    }
    printf("Fundamentals: \n");
    for(cost_t cost = 0; cost <= MAX_COST; ++cost) {
	printf("  cost=%d: " COEFF_FMT "\n", cost, count[cost]);
    }
    printf("Total: " COEFF_FMT "\n", total);
    printf(COEFF_FMT " missing\n", total - (MAX_PRINT>>1));
    delete [] count;
}

void output_costs() {
    if(PRINT_COSTS_MODE == COSTS_C) {
	cerr << "static int CBITS = " << (MAX_SHIFT-1) << ";\n";
	cerr << "static int COSTS[] = { 0";
    }
    for(coeff_t num = 1; num <= MAX_PRINT; ++num) {
	coeff_t fund = fundamental(num);
	ASSERT(GEN[fund] >= 0);
	//cerr << num << " " << GEN[fund] << endl;
	if(PRINT_COSTS_MODE == COSTS_C) cerr << ", ";
	else cerr << " ";
	cerr << GEN[fund];
    }
    if(PRINT_COSTS_MODE == COSTS_C) cerr << " };";
    cerr << "\n";
}

// The following macro checks whether we already generated all required coefficients
#define CK if(GEN_COUNT < (MAX_PRINT>>1))

int main(int argc, char **argv) {
    if(argc==1) {
	fprintf(stderr, "Usage: chains <max_shift>\n");
	return 1;
    }

    MAX_SHIFT = atoi(argv[1]);
    MAX_NUM = 1 << MAX_SHIFT;
    MAX_PRINT = 1 << (MAX_SHIFT-1);
    MAX_COST = 6;
    GEN = new cost_t[MAX_NUM];
    COSTVECS = new cfvec_t[MAX_COST+1];

    GEN[1] = GEN[0] = 0;
    GEN_COUNT = 1;
    for(coeff_t i = 2; i <= MAX_NUM; ++i)
	GEN[i] = -1;

    generate_initial();
    output_coeffs(0);

    generate_add_coeffs(0, 0, 1);
    output_coeffs(1);

    CK generate_add_coeffs(0, 1, 2);
    CK generate_mul_coeffs(1, 1, 2);
    output_coeffs(2);

    CK generate_add_coeffs(0, 2, 3);
    CK generate_add_coeffs(1, 1, 3);
    CK generate_mul_coeffs(1, 2, 3);
    output_coeffs(3);

    CK generate_add_coeffs(0, 3, 4);
    CK generate_add_coeffs(1, 2, 4);
    CK generate_mul_coeffs(1, 3, 4);
    CK generate_mul_coeffs(2, 2, 4);
    CK generate_leapfrog2(1, 1, 4);
    output_coeffs(4);

    CK generate_add_coeffs(0, 4, 5);
    CK generate_add_coeffs(1, 3, 5);
    CK generate_add_coeffs(2, 2, 5);
    CK generate_mul_coeffs(1, 4, 5);
    CK generate_mul_coeffs(2, 3, 5);
    CK generate_leapfrog2(1, 2, 5);
    CK generate_leapfrog3(1, 1, 5);
    output_coeffs(5);

    CK generate_add_coeffs(0, 5, 6);
    CK generate_add_coeffs(1, 4, 6);
    CK generate_add_coeffs(2, 3, 6);
    CK generate_mul_coeffs(1, 5, 6);
    CK generate_mul_coeffs(2, 4, 6);
    CK generate_mul_coeffs(3, 3, 6);
    CK generate_leapfrog2(1, 3, 6);
    CK generate_leapfrog2(2, 2, 6);
    CK generate_leapfrog3(1, 2, 6);
    output_coeffs(6);

    output_stats();
    if(PRINT_COSTS)
	output_costs();
    delete GEN; delete COSTVECS;
    return 0;
}
