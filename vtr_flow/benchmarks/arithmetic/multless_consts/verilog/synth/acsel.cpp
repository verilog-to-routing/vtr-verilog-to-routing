#define USE_COMPILED_CHAINS

#include "chains.h"
#include "ac1.h"
#include <string>
#include <map>
#include <fstream>
#include <list>
#include <vector>
using namespace std;

typedef map<coeff_t, vector<chain*> >chain_multimap_t;
typedef chain_multimap_t::iterator   chain_mmiter_t;
typedef vector<chain*>::iterator     chain_listiter_t;

typedef map<coeff_t, chain*>        chainmap_t;
typedef chainmap_t::const_iterator  chainiter_t ;

int MAX_SHIFT, MIN_SHIFT;
coeff_t MAX_NUM;

chainmap_t * CHAINS;

#define COST(chain) (chain)->depth(acsel_get_chain)

chain *acsel_get_chain(coeff_t c) {
    chainiter_t i = CHAINS->find(c);
    if(i==CHAINS->end()) return NULL;
    else return (*i).second;
}

void   acsel_init(const char *fname) {
    CHAINS = new chainmap_t;
    /*#ifdef USE_COMPILED_CHAINS
#define CHAIN(coeff, c) (*CHAINS)[coeff] = c
#include "17.chains.c"
#else*/
    ifstream fin(fname);
    ASSERT(fin);
    chain * ch = read_chain(fin);
    coeff_t n = 0;
    cerr << "read ";
    (*CHAINS)[1] = new empty_chain();
    while(fin && ch!=NULL) {
	if((++n) % 100000 == 0) cerr << n << " ";
	chain *old = acsel_get_chain(ch->c);
	if(old==NULL || COST(old) < COST(ch))
	    (*CHAINS)[ch->c] = ch;

	ch = read_chain(fin);
    }
    cerr << endl;
    //#endif
}

void   acsel_finish() {
    delete CHAINS;
}

static reg_t curreg = 100;
void reset_tmpreg() { curreg=100; }
reg_t tmpreg() { return curreg++; }

int main(int argc, char **argv) {
    ASSERT(argc >= 2 && "file name required");
    ASSERT(argc <= 3);
    acsel_init(argv[1]);

    coeff_t num=-1;
    if(argc==3)
        num = atoi(argv[2]);

    /* output all chains (internal format) */
    if(argc == 2) {
	int n = 0;
	double total_cost = 0;

	cerr << "output ";
	for(chainiter_t i = CHAINS->begin(); i != CHAINS->end(); ++i) {
	    if((++n) % 10000 == 0) cerr << n << " ";
	    chain * ch = (*i).second;
	    if(ch->c != 1) ch->output(cout);
	    total_cost += (double) COST(ch);
	}
	cerr << endl;
	cerr << "avg. cost = " << (total_cost / (double)n) << endl;
    }
    /* output *code* for one chain */
    else {
	chain * c = acsel_get_chain(num);
	if(c==NULL) ASSERT(0 && "chain not found");
	oplist_t l;
	reset_tmpreg();
	cout << num << endl;
	c->get_ops(l, 1, 0, tmpreg);
	for(oplist_t::iterator i = l.begin(); i != l.end(); ++i)
	    (*i)->output(cout);
	cout << "cost = " << COST(c) << endl;
    }

    acsel_finish();
}
