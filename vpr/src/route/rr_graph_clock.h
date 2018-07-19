#ifndef RR_GRAPH_CLOCK_H
#define RR_GRAPH_CLOCK_H

class ClockRRGraph {
    public:
        /* Creates the routing resourse (rr) graph of the clock network and appends it to the
           existing rr graph created in build_rr_graph for inter-block and intra-block routing. */
        static void create_and_append_clock_rr_graph();
    
    private:
        /* Dummy clock network that connects every I/O input to every clock pin. */
        static void create_star_model_network();    
};

#endif

