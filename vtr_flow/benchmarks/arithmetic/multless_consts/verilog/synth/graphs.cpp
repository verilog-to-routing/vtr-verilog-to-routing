#include <map>
#include <algorithm>
#include "chains.h"

typedef pair<int,int> edge_t;

struct graph_t : public vector<edge_t> {
    int length;
    graph_t() { length=0; }
    void add_edge(const edge_t &e) {
	assert(e.first < e.second);
	push_back(e);
	if(e.second > length) length = e.second;
    }
    void move_edges_by_1(int pos) {
	if(pos >= length)
	    return;
	for(iterator i=begin(); i!=end(); ++i) {
	    if((*i).first >=pos)
		++((*i).first);
	    else if((*i).second > pos)
		++((*i).second);
	}
	++length;
    }
    bool is_valid() {
	vector<bool> has_in(length+1);
	has_in.resize(length+1);
	for(int i=0; i<=length; i++)
	    has_in[i]=false;
	for(iterator i=begin(); i!=end(); ++i)
	    has_in[(*i).second] = true;
	for(int i=1; i<=length; i++)
	    if(!has_in[i]) return false;
	return true;

    }
    void output() const {
	for(const_iterator i=begin(); i!=end(); ++i)
	    printf("%d-%d ", (*i).first, (*i).second);
	printf("\n");
    }
    graph_t &reduce() {
	vector<bool> has_out(length+1);
	has_out.resize(length+1); /* has_out[length] will always be false */
	for(int i=0; i <= length; i++)
	    has_out[i]=false;
	for(iterator i=begin(); i!=end(); ++i)
	    has_out[(*i).first] = true;

	map<int,int> shifts;
	int sh=0;
	for(int i=0; i <= length; i++) {
	    shifts[i] = sh;
	    if(!has_out[i])
		++sh;
	}

	for(iterator i=begin(); i!=end(); ++i) {
	    int &f = (*i).first, &s = (*i).second;
	    f = f - shifts[f];
	    s = s - shifts[s];
	}
	return *this;
    }
};

typedef set<graph_t>         graphset_t;
typedef graphset_t::iterator gsiter_t;
vector<graphset_t> GRAPHS;

void generate_graphs(int cost) {
    graphset_t &src = GRAPHS[cost-1];
    graphset_t &dest = GRAPHS[cost];
    for(gsiter_t gi = src.begin(); gi!=src.end(); ++gi) {
	const graph_t &g = *gi;
	/* new graph by increasing the length (critical path) of a lower cost graph */
	/* splitp = which subexpression we reuse */
	for(int splitp = 0; splitp <= g.length; ++splitp) {
	    graph_t gnew;
	    /* endp = where we plug it in */
	    //	    for(int endp = splitp; endp <= g.length; ++endp) {
		gnew = graph_t(g);
		//gnew.move_edges_by_1(endp);
		gnew.add_edge(edge_t(splitp, g.length+1));//endp+1));
		if(gnew.is_valid()) {
		    sort(gnew.begin(), gnew.end());
		    dest.insert(gnew);
		}
		//}
	}
	graph_t gnew = graph_t(g);
	gnew.move_edges_by_1(0);
	gnew.add_edge(edge_t(0,1));
	if(gnew.is_valid()) {
	    sort(gnew.begin(), gnew.end());
	    dest.insert(gnew);
	}

    }
}

void output_graphs(int cost) {
    graphset_t &gset = GRAPHS[cost];
    for(gsiter_t i=gset.begin(); i!=gset.end(); ++i) {
	(*i).output();
    }
    printf("==> ngraphs=%d\n", gset.size());
}

void output_reduced_graphs(int cost) {
    graphset_t &gset = GRAPHS[cost];
    graphset_t greduc;
    for(gsiter_t i=gset.begin(); i!=gset.end(); ++i) {
	graph_t gnew = graph_t(*i).reduce();
	sort(gnew.begin(), gnew.end());
	greduc.insert(gnew);
    }
    for(gsiter_t i=greduc.begin(); i!=greduc.end(); ++i)
	(*i).output();
    printf("==> nreduced=%d\n", greduc.size());
}


int main(int argc, char **argv) {
    if(argc==1) {
	fprintf(stderr, "Usage: graphs <cost>\n");
	return 1;
    }
    int maxcost = atoi(argv[1]);
    assert(maxcost > 0);
    printf("cost=%d\n", maxcost);
    int narcs = (maxcost*(maxcost+1)) / 2;
    printf("arcs=%d\n", narcs);

    GRAPHS = vector<graphset_t>(maxcost);
    GRAPHS.push_back(graphset_t());
    GRAPHS[0].insert(graph_t());
    for(int cost=1; cost<=maxcost; ++cost) {
	GRAPHS.push_back(graphset_t());
	generate_graphs(cost);
	output_reduced_graphs(cost);
    }

}
