#include "chains.h"
#include "ac1.h"
#include <string>
#include <map>
#include <fstream>

typedef map<coeff_t, chain*> chainmap_t;
typedef chainmap_t::const_iterator chainiter_t ;
chainmap_t * CHAINS;


void   ac1_init(const char *fname) {
    CHAINS = new chainmap_t;
    ifstream fin(fname);
    ASSERT(fin);

    chain * ch = read_chain(fin);
    while(fin && ch!=NULL) {
	(*CHAINS)[ch->c] = ch;
	ch = read_chain(fin);
    }
    (*CHAINS)[1] = new empty_chain();
}

chain *ac1_get_chain(coeff_t c) {
    chainiter_t i = CHAINS->find(c);
    if(i==CHAINS->end())
	return NULL;
    else return (*i).second;
}

void   ac1_finish() {
    delete CHAINS;
}

#ifdef AC1_MAIN
int MAX_SHIFT, MIN_SHIFT;
coeff_t MAX_NUM;

static reg_t curreg = 100;
void reset_tmpreg() { curreg=100; }
reg_t tmpreg() { return curreg++; }

int main(int argc, char **argv) {
    ASSERT(argc >= 2 && "file name required");
    ASSERT(argc <= 3);
    ac1_init(argv[1]);

    coeff_t num=-1;
    if(argc==3)
        num = atoi(argv[2]);

    /* output all chains (internal format) */
    if(argc == 2)
	for(chainiter_t i = CHAINS->begin(); i != CHAINS->end(); ++i)
	    (*i).second->output(cout);
    /* output *code* for one chain */
    else {
	chain * c = ac1_get_chain(num);
	if(c==NULL) ASSERT(0 && "chain not found");
	oplist_t l;
	reset_tmpreg();
	cout << num << endl;
	c->get_ops(l, 1, 0, tmpreg);
	for(oplist_t::iterator i = l.begin(); i != l.end(); ++i)
	    (*i)->output(cout);

	cfset_t inter;
	c->get_intermediates(inter, ac1_get_chain);
	FORALL(inter, i) cout << (*i) << " ";
	cout << endl;
    }

    ac1_finish();
}
#endif
