bool COMPUTE_GEN_SETS = false;
typedef set<cfvec_t> outset_t;
typedef set<cfvec_t>::iterator outiter_t;
map<const adj_t*, outset_t> GENSETS;

void insert(coeff_t c, cost_t cost, cfset_t &where) {
    if(c > MAX_NUM) return;
    c = fundamental(c);
    /* was this coefficient previously generated with lesser cost? */
    /*if(GEN[c] >= 0 && GEN[c] < cost)
	return;
	else*/
    {
	GEN[c] = cost;
	where.insert(c);
    }
}

void compute_all_sumdiffs(cfset_t &result, cost_t cost, coeff_t num1, coeff_t num2) {
    for(int shift = 0; shift <= MAX_SHIFT; ++shift) {
	coeff_t sum, diff;
	diff = num1 - (num2<<shift);
	if(diff != 0) {
	    sum = num1 + (num2<<shift);
	    diff = diff > 0 ? diff : -diff;
	    insert(sum, cost, result);
	    insert(diff, cost, result);
	}
	diff = num2 - (num1<<shift);
	if(diff != 0) {
	    sum = num2 + (num1<<shift);
	    diff = diff > 0 ? diff : -diff;
	    insert(sum, cost, result);
	    insert(diff, cost, result);
	}
    }
}

void compute_gen_set(const adj_t *adj) {
    outset_t &predset = GENSETS[adj->pred];
    outset_t out;

    for(outiter_t pred = predset.begin(); pred != predset.end(); ++pred) {
	coeff_t c1 = (*pred)[adj->in1];
	coeff_t c2 = (*pred)[adj->in2];
	cfset_t result;
	compute_all_sumdiffs(result, adj->cost, c1, c2);
	for(cfsiter_t r = result.begin(); r!=result.end(); ++r) {
	    cfvec_t curout(*pred);
	    curout.push_back(*r);
	    out.insert(curout);
	}
    }
    GENSETS[adj] = out;
}

void output_gen_set(outset_t &coeffs) {
    int nprinted = 0;
    cfset_t res;
    for(outiter_t i = coeffs.begin(); i != coeffs.end(); ++i) {
	coeff_t c = *((*i).end()-1);
	res.insert(c);
    }
    for(cfsiter_t r = res.begin(); r != res.end(); ++r) {
	if(*r <= MAX_PRINT) {
	    printf(COEFF_FMT " ", *r);
	    ++nprinted;
	}
	if(nprinted > MAX_NPRINTED) {
	    printf(" ..."); break;
	}
    }
    printf("\n");
}

/* init */
    cfvec_t outset1;
    outset1.push_back(1);
    GENSETS[NULL] = outset_t();
    GENSETS[NULL].insert(outset1);
    compute_gen_set(ADJS[1][0]);
