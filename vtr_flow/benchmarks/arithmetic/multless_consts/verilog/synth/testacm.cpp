#include <vector>
#include "acm.h"

reg_t tmpreg() { static int c=100; return c++; }

int main(int argc, char **argv) {
    acm_init();

    vector<coeff_t> targets;
    vector<reg_t> dests;

    targets.resize(2); dests.resize(2);
    targets[0] = 4;  dests[0] = 0;
    targets[1] = 5;  dests[1] = 2;

    oplist_t ops;
    acm_solve(ops, targets, dests, 99, tmpreg);
    for(oplist_t::iterator i = ops.begin(); i!=ops.end(); ++i)
	(*i)->output(cerr);

    acm_finish();
}
