#ifndef CLOCK_MODELING_H
#define CLOCK_MODELING_H

enum e_clock_modeling {
    IDEAL_CLOCK,      //Treat the clock pins as ideal (i.e. not routed)
    ROUTED_CLOCK,     //Treat the clock pins as normal nets (i.e. routed)
    DEDICATED_NETWORK //Connect clock pins to a dedicated clock network
};


namespace ClockModeling
{
    /* Removes global pin flag from clock pins and adds routable pin flag for clock pins
       This causes clock nets to also be treated as non-global;
       therefore, they will be routed using inter-block routing */
    void treat_clock_pins_as_non_globals();

    /* Sets the is_routable_pin flag for clock pins. This allows for clock nets to be routed
       Note: this does not remove the global pin flag from clocks (therefore rr_graph generation
       in rr_graph.cpp is not effected by this change) */
    void assign_clock_pins_as_routable();
}

#endif

