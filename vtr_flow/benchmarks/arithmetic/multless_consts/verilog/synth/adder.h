#include <list>

/*******************************************************************************
 * adag_node
 *******************************************************************************/
struct adag_node {
    adag_node(coeff_t cc, bool is_targ, reg_t rreg);
    virtual ~adag_node() { }

    void verify() { ASSERT(c == compute_coeff()); }
    void add_parent(adag_node *p) { parents.push_back(p); }
    bool is_input() const { return c==1; }
    void set_rsh(int amt);
    void update_parent_child_links(adag_node *oldchild);
    void update_child(adag_node *oldchild, adag_node *newchild);
    void update_parent(adag_node *oldpar, adag_node *newpar);
    void update_depth();
    void remove_parent(adag_node *par);
    void unlink_children();
    bool single_parent();
    virtual coeff_t compute_coeff() = 0;
    virtual void output(ostream &os) = 0;
    virtual void output_dot(ostream &os);

    int out_degree() { return parents.size(); }
    int in_degree() { return src.size(); }

    coeff_t c;
    reg_t reg;
    bool is_target;
    int depth;
    int rsh;
    vector<adag_node*> src;
    vector<int>        scales;
    list<adag_node*>   parents;
};

typedef list<adag_node*> nodelist_t;

/*******************************************************************************
 * multi_adag_node
 *******************************************************************************/
struct multi_adag_node : public adag_node {
    multi_adag_node(reg_t reg);
    multi_adag_node(coeff_t c, bool is_targ, reg_t reg);
    multi_adag_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
		    adag_node *src1, adag_node *src2);
    void add_summand(adag_node *s, int scale);
    void output(ostream &os);
    coeff_t compute_coeff();
};

/*******************************************************************************
 * binary_adag_node
 *******************************************************************************/
class binary_adag_node : public adag_node {
public:
    binary_adag_node(reg_t reg); /* input node */
    binary_adag_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
		     adag_node *src1, adag_node *src2);
    void output(ostream &os);
    coeff_t compute_coeff();
    ~binary_adag_node();
    op *o;
private:
    void set_op();
};

 /*******************************************************************************
 * adag
 *******************************************************************************/
class adag {
public:
    adag(reg_t input_reg);
    adag_node *lookup_coeff(coeff_t c);
    adag_node *lookup_input();
    adag_node *set_input_node(reg_t reg);
    adag_node *add_binary_node(coeff_t c, bool is_targ, reg_t reg, sp_t sp,
			       adag_node *src1, adag_node *src2);
    adag_node *add_node(adag_node *n);
    adag_node *add_adder(adder *a, reg_t dest, reg_t (tmpreg)(),
			 adder *(get_adder)(coeff_t));

    adag_node *to_multi(adag_node *n);
    void remove_child(adag_node *parent, adag_node *n);
    void remove_dead_code();
    void to_multi();

    void compute_max_depth();
    int  min_regs();
    int  allocate_regs();
    void compute_liveness(bool keep_input);
    void output_liveness(ostream &);
    void output_code(ostream &);
    void output_dot(ostream &os);

    typedef map<coeff_t, adag_node*> nodemap_t;
    typedef map<coeff_t, adag_node*>::const_iterator nodeiter_t;
    typedef set<reg_t> regset_t;
    typedef set<reg_t>::iterator regiter_t;
    typedef vector<regset_t> liveness_t;
    typedef map<reg_t, int> rlmap_t; /* register liveness map */
    typedef map<reg_t, int>::const_iterator rliter_t;

    cfset_t outputs;
    vector<nodelist_t> nodes_at;
    liveness_t liveness, gen_at, kill_at;
    rlmap_t gen, kill;
    nodemap_t nodes;
    int max_depth, nregs;
    map<reg_t, reg_t> assigned; /* registers */
    bool verbose;
};

/*******************************************************************************
 * adder
 *******************************************************************************/
struct adder {
    adder(coeff_t x = 0);
    adder(bool is_targ, coeff_t cc1, coeff_t cc2, spref_t ssp, int dep = -1);
    void output(ostream &os) const;
    double color(double dep) const;
    void output_dot(ostream &os) const;

    cfset_t interm;
    bool is_target;
    coeff_t c1, c2, res;
    sp_t sp;
    int depth;
};

