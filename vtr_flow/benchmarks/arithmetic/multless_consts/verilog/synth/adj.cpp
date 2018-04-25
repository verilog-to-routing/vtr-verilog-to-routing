#include "chains.h"
#include "adj.h"
#include <ext/hash_map>

int MAX_SHIFT = 21;
coeff_t MAX_NUM = 1 << (MAX_SHIFT);
coeff_t MAX_PRINT = 1 << (MAX_SHIFT-1);
coeff_t NCALLS=0;

bool OUTPUT_INVALID = false;
int  COMPUTE_GEN_SETS = 0;//10000;
int  MAX_NPRINTED = 1 << 30;

int MAX_COST;
vector<adjvec_t> ADJS;
cost_t *GEN;
coeff_t GEN_COUNT;

typedef cfset_t   genset_t;
typedef cfsiter_t gsiter_t;
namespace __gnu_cxx {
    struct hash<const adj_t*> {
	size_t operator()(const adj_t* pt) const { return (size_t) pt; }
    };
};
__gnu_cxx::hash_map<const adj_t*, genset_t> GENSETS;

void cfinsert(coeff_t c, cost_t cost, genset_t &where) {
    //if(c > MAX_NUM) return;
    //c = fundamental(c);
    /* was this coefficient previously generated with lesser cost? */
    if(GEN[c] >= 0 && GEN[c] <= cost)
	return;
    else {
	/*if(GEN[c] != cost)*/ /* if above we have GEN[c] < cost, this is needed */
	{
	    if(c <= MAX_PRINT)
		++GEN_COUNT;
	    GEN[c] = cost;
	}
	where.insert(c);
    }
}

struct adder_t {
    adder_t() { c=1; in1=in2=0; }
    adder_t(coeff_t coef, int i1, int i2) : c(coef), in1(i1), in2(i2) {}
    coeff_t c; int in1; int in2;
};

void compute_gen_set(genset_t &result, int cost, vector<adder_t> &adders, int start) {
    ++NCALLS;
    if(start >= adders.size())
	return;
    bool last = start == (adders.size()-1);
    coeff_t c1 = adders[ adders[start].in1 ].c;
    coeff_t c2 = adders[ adders[start].in2 ].c;

    for(int sign = 0; sign <= 1; ++sign) {
	/* no shift by 0 for sign=1(minus) */
	int minshift;
	if(c1==c2 && sign==1) minshift = 1;
	else minshift = 0;

	for(int shift = minshift; shift <= MAX_SHIFT; ++shift) {
	    {
		coeff_t sum1 = (sign==0) ? (c1 + (c2 << shift)) : (c1 - (c2 << shift));
		sum1 = sum1 < 0 ? -sum1 : sum1;
		if(sum1 < MAX_NUM) {
		    sum1 = fundamental(sum1);
		    if(last) cfinsert(sum1, cost, result);
		    else {  adders[start].c = sum1;
		            compute_gen_set(result, cost, adders, start+1);  }
		}
	    }
	    if(c1!=c2) {
		coeff_t sum2 = (sign==0) ? (c2 + (c1 << shift)) : (c2 - (c1 << shift));
		sum2 = sum2 < 0 ? -sum2 : sum2;
		if(sum2 < MAX_NUM) {
		    sum2 = fundamental(sum2);
		    if(last) cfinsert(sum2, cost, result);
		    else {  adders[start].c = sum2;
		            compute_gen_set(result, cost, adders, start+1);  }
		}
	    }
	}
    }
}

void output_gen_set(genset_t &coeffs) {
    int nprinted = 0;
    for(gsiter_t c = coeffs.begin(); c != coeffs.end(); ++c) {
	if(*c <= MAX_PRINT) {
	    printf(COEFF_FMT " ", *c);
	    ++nprinted;
	}
	if(nprinted > MAX_NPRINTED) {
	    printf(" ..."); break;
	}
    }
    printf("\n");
}

void compute_gen_set(const adj_t *adj) {
    /* init */
    vector<adder_t> adders(adj->cost + 1);
    adders.resize(adj->cost + 1);
    adders[0] = adder_t(1,0,0); /* input */
    const adj_t *p = adj;
    while(p!=NULL) {
	adders[p->cost] = adder_t(0, p->in1, p->in2);
	p = p->pred;
    }
    genset_t result;
    compute_gen_set(result, adj->cost, adders, 1);
    output_gen_set(result);
}


/* cost of 1 adjacency matrix */
adj_t::adj_t() {
    cost = 1;
    in1 = in2 = 0;
    pred = NULL;
}

/* cost of (pred->cost + 1) adjacency matrix */
adj_t::adj_t(const adj_t *pred, int in1, int in2) {
    cost = pred->cost + 1;
    this->in1 = in1;
    this->in2 = in2;
    this->pred = pred;
}

const adj_t* adj_t::in_pred(int n) const {
    const adj_t *p = this;
    while(p!=NULL && p->cost != n)
	p = p->pred;
    return p;
}

int adj_t::num_outputs() {
    set<int> has_out;
    const adj_t *p = this;
    while(p!=NULL) {
	has_out.insert(p->in1);
	has_out.insert(p->in2);
	p = p->pred;
    }
    /* total of cost+1 (all adders and input vertex) vertices, outputs
       are nodes which have no outgoing edges, that do not serve as
       input to some other vertex */
    return (cost+1) - has_out.size();
}

//_|i|1|2|3
//i|- 2 2 1
//1|  -   1
//2|    -
//3|      -
void adj_t::output() const {
    printf("_|i");
    for(int c=1; c<=cost; ++c)
	printf("|%d", c);
    printf("\n");
    for(int c=0; c<=cost; ++c) {
	if(c==0) printf("i|");
	else     printf("%d|", c);
	for(int i=0; i<c; ++i)
	    printf("  ");
	printf("- ");
	output_in(c);
	printf("\n");
    }
}

void adj_t::output_in(int c) const {
    if(c >= cost)
	return;
    else {
	if(pred!=NULL)
	    pred->output_in(c);
	if(in1==c && in2==c)  printf("2 ");
	else if(in1==c)       printf("1 ");
	else if(in2==c)       printf("1 ");
	else                  printf("  ");
    }
}

void adj_t::output_dot(int base) const {
    const adj_t *p = this;
    while(p!=NULL) {
	printf("\t n%d_%d -> n%d_%d\n", base, p->in1, base, p->cost);
	printf("\t n%d_%d -> n%d_%d\n", base, p->in2, base, p->cost);
	p = p->pred;
    }
}

void generate_adjs(int cost) {
    adjvec_t &src = ADJS[cost-1];
    adjvec_t &dest = ADJS[cost];
    int compute_gen_sets = COMPUTE_GEN_SETS;
    int n = 1;
    printf("=============== cost=%d ==================\n", cost);
    for(adjiter_t i = src.begin(); i!=src.end(); ++i) {
	const adj_t *pred = (*i);
	for(int in1=0; in1<=cost-1; ++in1) {
	    for(int in2=in1; in2<=cost-1; ++in2) {
		adj_t * adjnew = new adj_t(pred, in1, in2);
		/* check whether this will lead to a valid adjacency
		   matrix with cost <= MAX_COST */
		if(adjnew->num_outputs()-1 + cost <= MAX_COST) {
		    dest.push_back(adjnew);
		    if(adjnew->is_valid()) {
		        adjnew->output();
			printf("n=%d\n",n++);
		    }
		    if(compute_gen_sets && adjnew->is_valid()) {
			printf("> GEN: ", dest.size()); fflush(stdout);
			compute_gen_set(adjnew);
			--compute_gen_sets;
		    }
		}
		else delete adjnew;
	    }
	}
    }
    //    printf("\n");
}

void output_adjs(int cost) {
    return;
    adjvec_t &adjvec = ADJS[cost];
    int nvalid=0;
    printf("=============== cost=%d ==================\n", cost);
    for(adjiter_t i=adjvec.begin(); i!=adjvec.end(); ++i) {
	if((*i)->is_valid()) {
	    nvalid++;
	    (*i)->output();
	    if(COMPUTE_GEN_SETS) {
		printf("> GEN ");
		output_gen_set(GENSETS[*i]);
	    }
	}
	else if(OUTPUT_INVALID)
 	    (*i)->output();
    }
    printf("==> nmatrices=%d <==\n", adjvec.size());
    printf("==> valid=%d <==\n", nvalid);
}

void output_dot(int cost) {
    adjvec_t &adjvec = ADJS[cost];
    int base=0;

    printf("digraph cost_%d {\n", cost);
    for(adjiter_t i=adjvec.begin(); i!=adjvec.end(); ++i) {
	if((*i)->is_valid() || OUTPUT_INVALID) {
	    ++base;
	    printf("  subgraph cluster_%d {\n", base);
	    (*i)->output_dot(base);
	    printf("  }\n");
	}
    }
    printf("}\n");
}

void output_stats() {
    coeff_t total = 0;
    coeff_t count[MAX_COST+1];
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
}

int main(int argc, char **argv) {
    if(argc==1) {
	fprintf(stderr, "Usage: adj <cost>\n");
	return 1;
    }
    MAX_COST = atoi(argv[1]);
    ASSERT(MAX_COST > 0);

    GEN_COUNT = 1;
    GEN = new cost_t[MAX_NUM];
    GEN[0] = GEN[1] = 0;
    for(coeff_t i = 2; i <= MAX_NUM; ++i)
	GEN[i] = -1;

    GENSETS[NULL] = genset_t();

    ADJS = vector<adjvec_t>(MAX_COST);
    ADJS.push_back(adjvec_t());
    ADJS.push_back(adjvec_t());
    ADJS[1].push_back(new adj_t());
    compute_gen_set(ADJS[1][0]);

    output_adjs(1);
    for(int cost=2; cost<=MAX_COST; ++cost) {
	ADJS.push_back(adjvec_t());
	generate_adjs(cost);
	output_adjs(cost);
	if(GEN_COUNT == MAX_PRINT >> 1) break;
    }
    printf("\nGEN_COUNT = %d\n", GEN_COUNT);
    printf("expected = %d\n", MAX_PRINT >> 1);
    printf("NCALLS = %ld\n", NCALLS);

    output_stats();
    //output_dot(maxcost);
}
