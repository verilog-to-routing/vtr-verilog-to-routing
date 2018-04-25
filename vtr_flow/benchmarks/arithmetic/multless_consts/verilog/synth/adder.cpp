#include "chains.h"
#include <iomanip>

/*******************************************************************************
 * adag_node
 *******************************************************************************/
adag_node::adag_node(coeff_t cc, bool is_targ, reg_t rreg)
    : c(cc), reg(rreg), is_target(is_targ) {
    depth=0; rsh=0;
}
void adag_node::unlink_children() {
    for(list<adag_node*>::iterator p = parents.begin(); p != parents.end(); ++p)
	(*p)->remove_parent(this);
}

void adag_node::update_parent_child_links(adag_node *oldchild) {
    for(list<adag_node*>::iterator p = parents.begin(); p != parents.end(); ++p)
	(*p)->update_child(oldchild, this);
}
void adag_node::update_child(adag_node *oldchild, adag_node *newchild) {
    for(size_t i=0; i<src.size(); ++i)
	if(src[i]==oldchild) src[i] = newchild;
}
void adag_node::remove_parent(adag_node *par) {
    for(list<adag_node*>::iterator p = parents.begin(); p != parents.end(); ++p) {
	if(*p == par) { parents.erase(p); return; }
    }
    assert(0 && "remove_parent() called with non-existing parent");
}
void adag_node::update_parent(adag_node *oldpar, adag_node *newpar) {
    for(list<adag_node*>::iterator p = parents.begin(); p != parents.end(); ++p) {
	if(*p == oldpar) { *p = newpar; }
    }
}
void adag_node::set_rsh(int amt) {
    for(size_t i=0; i < scales.size(); ++i)
	scales[i] = compose_scales(scales[i], amt+1);
    rsh = amt;
}
void adag_node::update_depth() {
    depth = 0;
    for(size_t i=0; i < src.size(); ++i)
	depth = max(depth, src[i]->depth);
    depth = 1+depth;
}
bool adag_node::single_parent() {
    assert(parents.size() >= 1);
    if(parents.size()==1) return true;
    adag_node *p = parents.front();
    list<adag_node*>::iterator i = parents.begin();
    for(++i; i != parents.end(); ++i)
	if(*i!=p) return false;
    return true;
}

void adag_node::output_dot(ostream &os) {
    if(is_input()) os << c << " [peripheries=2]\n";
    else {
	os << c << "[style=filled fillcolor=\"#"
	   << (char)('a'+rand()%6) << (char)('a'+rand()%6) << (char)('a'+rand()%6)
	   << (char)('a'+rand()%6) << (char)('a'+rand()%6) << (char)('a'+rand()%6)
	   << "\"" << (is_target ? " peripheries=2]" : " peripheries=1]") << endl;
	for(size_t i=0; i<src.size(); ++i) {
          os << src[i]->c << "->" << c << " [weight=1 label=\"";
          if(scales[i] < 0) { os << "-"; if(scales[i]!=-1) os << " <<" << -scales[i]-1; }
          else              { os << "+"; if(scales[i]!=1)  os << " <<" << scales[i]-1; }
          os << "\"]" << endl;
	}
    }
}

/*******************************************************************************
 * multi_adag_node
 *******************************************************************************/
multi_adag_node::multi_adag_node(reg_t reg) : adag_node(1, false, reg) { /* input_node */
}

multi_adag_node::multi_adag_node(coeff_t c, bool is_targ, reg_t reg) : adag_node(c, is_targ, reg) {
}

multi_adag_node::multi_adag_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
				 adag_node *src1, adag_node *src2)
    : adag_node(c, is_targ, reg) {
    src.resize(2); scales.resize(2);
    sp.normalize(src1->c, src2->c);
    src[0] = src1; scales[0] = sp.first;
    src[1] = src2; scales[1] = sp.second;
    depth = 1 + max(src1->depth, src2->depth);
    src[0]->add_parent(this); src[1]->add_parent(this);
    rsh = compute_shr(compute_coeff());
    verify();
}
void multi_adag_node::add_summand(adag_node *s, int scale) {
    { scales.push_back(scale); src.push_back(s); }
    s->add_parent(this);
    depth = max(depth, 1 + s->depth);
}

void multi_adag_node::output(ostream &os) {
    if(is_input()) os << "t" << reg << " = " << c << "\n";
    else {
	os << (is_target?"T":"t") << reg << " = multi_add(";
	for(size_t i=0; i<src.size(); ++i) {
	    os << "t" << src[i]->reg << "/" << scales[i];
	    if(i!=src.size()-1) os << ", ";
	}
	os << ") >>" << rsh << "\n";
    }
}

coeff_t multi_adag_node::compute_coeff() {
    if(is_input()) return c;
    ASSERT(scales.size() == src.size());
    ASSERT(src.size() >= 2);
    coeff_t res = compute_scale(src[0]->c, scales[0]);
    for(size_t i = 1; i < src.size(); ++i) {
	res = res + compute_scale(src[i]->c, scales[i]);
    }
    return res >> rsh;
}

 /*******************************************************************************
 * binary_adag_node
 *******************************************************************************/

binary_adag_node::binary_adag_node(reg_t reg): adag_node(1, false, reg) { /* input node */
    o = NULL;
    output(cout);
}

binary_adag_node::binary_adag_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
				   adag_node *src1, adag_node *src2)
    : adag_node(c, is_targ, reg) {
    src.resize(2); scales.resize(2);
    src[0] = src1; scales[0] = sp.first;
    src[1] = src2; scales[1] = sp.second;
    depth = 1 + max(src1->depth, src2->depth);
    src[0]->add_parent(this); src[1]->add_parent(this);
    rsh = compute_shr(compute_sp(src1->c, src2->c, sp));
    verify(); set_op();
    output(cout);
}
void binary_adag_node::output(ostream &os) {
    os << "dep=" << depth << " c=" << c << ":\t";
    if(o==NULL) os << "t" << reg << " = 1\n";
    else  { if(is_target) os <<"T <<"<<rsh; o->output(os); }
}

coeff_t binary_adag_node::compute_coeff() {
    if(is_input()) return c;
    else return
     (compute_scale(src[0]->c, scales[0]) + compute_scale(src[1]->c, scales[1]))
	 >> rsh;
}

binary_adag_node::~binary_adag_node() { if(o!=NULL) delete o; }

void binary_adag_node::set_op() {
    if(scales[0] > 0 && scales[1] > 0) {
	ASSERT(scales[0]==1 || scales[1]==1);
	if(scales[0]==1)  o = op::addshl(reg, src[0]->reg, src[1]->reg, scales[1]-1);
	else if(scales[1]==1)  o = op::addshl(reg, src[1]->reg, src[0]->reg, scales[0]-1);
    }
    else {
	ASSERT(scales[0] > 0 || scales[1] > 0);
	if(scales[0]==-1) o = op::shlsub(reg, src[1]->reg, src[0]->reg, scales[1]-1);
	else if(scales[1]==-1) o = op::shlsub(reg, src[0]->reg, src[1]->reg, scales[0]-1);
	else if(scales[1] < 0) o = op::subshl(reg, src[0]->reg, src[1]->reg, -(scales[1]+1));
	else if(scales[0] < 0)  o = op::subshl(reg, src[1]->reg, src[0]->reg, -(scales[0]+1));
    }
}

 /*******************************************************************************
 * adag
 *******************************************************************************/

adag::adag(reg_t input_reg) {
    set_input_node(input_reg);
    max_depth = 0;
    nregs = -1;
    verbose = false;
}

adag_node *adag::lookup_coeff(coeff_t c) {
    nodeiter_t it = nodes.find(c);
    if(it==nodes.end()) return NULL;
    else return it->second;
}

adag_node *adag::lookup_input() {
    adag_node * inp = lookup_coeff(1);
    ASSERT(inp!=NULL);
    return inp;
}

adag_node *adag::set_input_node(reg_t reg) {
    return (nodes[1] = new multi_adag_node(reg));
}

adag_node *adag::add_binary_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
				 adag_node *src1, adag_node *src2) {
    if(is_targ)	outputs.insert(c);
    return (nodes[c] = new multi_adag_node(c, is_targ, reg, sp, src1, src2));
}

adag_node *adag::add_node(adag_node *n) {
    return (nodes[n->c] = n);
}

adag_node *adag::to_multi(adag_node *nn) {
    multi_adag_node *n = dynamic_cast<multi_adag_node*>(nn);

    if(n->is_input())
	return n;

    for(size_t i = 0; i < n->src.size(); ++i) {
	adag_node *ch = n->src[i];
	int scale     = n->scales[i];
	int numparents = ch->parents.size();
	ASSERT(numparents >= 1);

	/* outdegree == 1 and not input or output */
	if(numparents == 1 && !ch->is_input() && !ch->is_target) {
	    nodes.erase(ch->c);
	    n->src.erase(n->src.begin()+i);
	    n->scales.erase(n->scales.begin()+i);
	    --i; /* take into account deletion */

	    if(ch->rsh > 0)
		n->set_rsh(ch->rsh);
	    for(size_t j = 0; j < ch->src.size(); ++j) {
		n->add_summand(ch->src[j],
			       compose_scales(scale, ch->scales[j]));
		ch->src[j]->update_parent(ch, n);
	    }
	    delete ch;
	}
    }
    n->update_depth();
    return n;
}

void adag::remove_child(adag_node *parent, adag_node *n) {
    if(n->parents.size() <= 1) {
	for(size_t i=0; i < n->src.size(); ++i)
	    remove_child(n, n->src[i]);
	nodes.erase(n->c);
	delete n;
    }
    else n->remove_parent(parent);
}

void adag::remove_dead_code() {
    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	if(n->parents.size() == 0 && !n->is_target)
	    remove_child(NULL, n);
    }
}

void adag::output_code(ostream &os) {
    compute_max_depth();
    nodes_at.clear(); nodes_at.resize(max_depth + 1);
    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	nodes_at[n->depth].push_back(n);
    }

    for(int i=0; i <= max_depth; ++i) {
	for(nodelist_t::iterator ni = nodes_at[i].begin(); ni != nodes_at[i].end(); ++ni) {
	    adag_node *n = (*ni);
	    os << n->depth << " " << setw(10) << n->compute_coeff() << " ";
	    n->output(os);
	}
    }
}

void adag::output_dot(ostream &os) {
    compute_max_depth();
    nodes_at.clear(); nodes_at.resize(max_depth + 1);
    os << "{rank=same; ";
    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	nodes_at[n->depth].push_back(n);
	if(n->is_target && n->parents.size()==0) os << n->c << " ";
    }
    os << " }" << endl;

    for(int i=0; i <= max_depth; ++i) {
	for(nodelist_t::iterator ni = nodes_at[i].begin(); ni != nodes_at[i].end(); ++ni) {
	    adag_node *n = (*ni);
	    n->output_dot(os);
	}
    }
}

void adag::to_multi() {
    remove_dead_code();
    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	to_multi(n)->verify();
    }
}

adag_node *adag::add_adder(adder *ad, reg_t dest, reg_t (tmpreg)(),
			   adder *(get_adder)(coeff_t)) {
    adag_node *n = lookup_coeff(ad->res);
    if(n!=NULL) 	 return n;
    else if(ad->res==1)  return lookup_input();
    else {
	return add_binary_node(ad->res, ad->is_target, dest, ad->sp,
			    add_adder(get_adder(ad->c1), tmpreg(), tmpreg, get_adder),
			    add_adder(get_adder(ad->c2), tmpreg(), tmpreg, get_adder));
    }
}

void adag::compute_max_depth() {
    max_depth = 0;
    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	max_depth = max(max_depth, n->depth);
    }
}

int adag::min_regs() {
    int numregs = 0;
    for(int i=0; i <= max_depth; ++i)
	numregs = max(numregs, liveness[i].size());
    return numregs;
}

int adag::allocate_regs() {
    if(verbose) output_liveness(cerr);

    nregs = nodes_at[0].size();
    assigned.clear();
    assigned[lookup_input()->reg] = lookup_input()->reg;
    map<reg_t, adag_node*> regnodes;   /* regs -> nodes */
    map<reg_t, set<adag_node*> >  refs; /* out degree for registers */

    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	regnodes[n->reg] = n;
	for(nodelist_t::iterator pi = n->parents.begin(); pi!=n->parents.end(); ++pi) {
	    refs[n->reg].insert(*pi);
	}
	//cerr << "numrefs(" << n->reg << ")=" << refs[n->reg].size() << endl;
    }

    /* start at depth=1, bypassing input */
    for(int i=1; i <= max_depth; ++i) {
	regset_t &kills_here = kill_at[i];
	regset_t &gens_here = gen_at[i];
	int to_assign = gens_here.size();

	while(to_assign > 0) {
	    int changed=0;

	    do {
		changed = 0;
		for(regiter_t r = kills_here.begin(); r != kills_here.end(); ++r) {
		    if(refs[*r].size() == 1) {
			adag_node *gen_node = *( refs[*r].begin() );

			/* check whether we assigned different register for *r before */
			reg_t assigned_reg = *r;
			if(assigned.find(*r) != assigned.end())
			    assigned_reg = assigned[*r];

			assigned[gen_node->reg] = assigned_reg;
			--to_assign;
			if(verbose) cerr << "assigned " << assigned_reg << " -> " << gen_node->reg << endl;

			for(size_t i=0; i < gen_node->src.size(); ++i)
			    refs[gen_node->src[i]->reg].erase(gen_node);
			++changed;
		    }
		}
	    } while(changed > 0 && to_assign > 0);

	    /* check if we are done */
	    if(to_assign==0) break;

	    for(regiter_t r = gens_here.begin(); r != gens_here.end(); ++r) {
		/* find first unassigned gen */
		if(assigned.find(*r) == assigned.end()) {
		    assigned[*r] = *r;
		    if(verbose) cerr << "assigned " << *r << " -> " << *r << endl;

		    adag_node *gen_node = regnodes[*r];
		    for(size_t i=0; i < gen_node->src.size(); ++i)
			refs[gen_node->src[i]->reg].erase(gen_node);
		    --to_assign;
		    ++nregs;
		    break;
		}
	    }
	}
    }
    if(verbose) cerr << "--- adjusted ---" << endl;
    map<reg_t,reg_t> adjustment;
    FORALL(outputs, o) {
	reg_t wanted_reg = nodes[*o]->reg;
	reg_t actual_reg = assigned[wanted_reg];
	adjustment[actual_reg] = wanted_reg;
    }
    for(map<reg_t,reg_t>::const_iterator i = assigned.begin(); i != assigned.end(); ++i) {
	reg_t from = i->first;
	reg_t assig = i->second;
	if(adjustment.find(assig) != adjustment.end())
	    assigned[from] = adjustment[assig];
    }
    if(verbose) {
	for(map<reg_t,reg_t>::const_iterator i = assigned.begin(); i != assigned.end(); ++i)
	    cerr << "assigned " << i->second << " -> " << i->first << endl;
    }

    return nregs;
}

void adag::output_liveness(ostream &os) {
    for(int i=0; i <= max_depth; ++i) {
	os << "// " << i << " : ";
	for(regiter_t r = liveness[i].begin(); r != liveness[i].end(); ++r)
	    os << *r << " ";
	os << endl;
    }
}

void adag::compute_liveness(bool keep_input) {
    compute_max_depth();
    gen.clear(); kill.clear();
    liveness.clear(); nodes_at.clear(); gen_at.clear(); kill_at.clear();
    liveness.resize(max_depth + 1);
      gen_at.resize(max_depth + 1);
     kill_at.resize(max_depth + 2);
    nodes_at.resize(max_depth + 1);

    if(keep_input)
	kill[lookup_input()->reg] = max_depth + 1;

    for(nodeiter_t ni = nodes.begin(); ni!=nodes.end(); ++ni) {
	adag_node *n = (*ni).second;
	nodes_at[n->depth].push_back(n);

	if(gen.find(n->reg) != gen.end()) {
	    int oldg = gen[n->reg];
	    if(n->depth < oldg)
		gen[n->reg] = n->depth;
	}
	else gen[n->reg] = n->depth;

	if(n->is_target)
	    kill[n->reg] = max_depth+1;

	for(size_t i=0; i<n->src.size(); ++i) {
	    reg_t src_reg = n->src[i]->reg;

	    if(kill.find(src_reg) != kill.end()) {
		int oldk = kill[src_reg];
		if(n->depth > oldk)
		    kill[src_reg] = n->depth;
	    }
	    else kill[src_reg] = n->depth;
	}
    }
    for(rliter_t g = gen.begin(); g != gen.end(); ++g) {
	reg_t reg = g->first;
	int reg_gen_at = g->second;
	int reg_kill_at = kill[reg];
	gen_at[reg_gen_at].insert(reg);
	kill_at[reg_kill_at].insert(reg);
	for(int i = reg_gen_at; i < reg_kill_at; ++i)
	    liveness[i].insert(reg);
    }
}

/*******************************************************************************
 * adder
 *******************************************************************************/

adder::adder(coeff_t x) {
    res = x; c1 = c2 = 0; depth = 0; is_target = false;
}

adder::adder(bool is_targ, coeff_t cc1, coeff_t cc2, spref_t ssp, int dep)
  : is_target(is_targ), c1(cc1), c2(cc2), sp(ssp), depth(dep) {
    ASSERT(c1!=c2 || (sp.first != -sp.second));
    sp.normalize(cc1, cc2);
    res = fundamental(compute_scale(c1, sp.first) + compute_scale(c2, sp.second));
}

void adder::output(ostream &os) const {
    os << res << " = adder(" << c1 << ", " << c2 << ", "
       << sp.first << "/" << sp.second << ", "
       << depth << ") ";
}

double adder::color(double dep) const {
    const double ceil = 20.0;
    if(dep > ceil) dep = ceil;
    return 0 + dep/ceil;
}

void adder::output_dot(ostream &os) const {
    os << res << "[style=filled, fillcolor=\"" << color(depth) << ",128,128\"]\n";
    os << c1 << "->" << res << "[label=" << sp.first << "]\n";
    os << c2 << "->" << res << "[label=" << sp.second << "]\n";
}
