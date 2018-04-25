#ifndef ADJ_H
#define ADJ_H
#include <vector>

class adj_t {
public:
    /* cost of 1 adjacency matrix */
    adj_t();
    /* cost of (pred->cost + 1) adjacency matrix */
    adj_t(const adj_t *pred, int in1=0, int in2=0);

    bool is_valid() { return num_outputs()==1; }
    int  num_outputs();
    void output() const;
    void output_dot(int base) const;

    /* find predecessor responsible for first input */
    const adj_t *in_pred1() const { return in_pred(in1); }
    /* find predecessor responsible for second input */
    const adj_t *in_pred2() const { return in_pred(in2); }
    /* find predecessor responsible for second input */
    const adj_t *in_pred(int n) const;

    int cost, in1, in2;
    const adj_t *pred;

protected:
    void output_in(int c) const;
};

template<class X, class Y, class Z>
struct triple {
    triple(const Z &x, const Y &y, const Z &z) : fst(x), snd(y), trd(z) {}
    X fst;
    Y snd;
    Z trd;
};

typedef std::vector<adj_t*>    adjvec_t;
typedef adjvec_t::iterator     adjiter_t;

#endif
