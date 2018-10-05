#ifndef CLOCK_CONNECTION_TYPES
#define CLOCK_CONNECTION_TYPES

#include <string>

#include "clock_fwd.h"

#include "rr_graph_clock.h"

class ClockRRGraph;

enum class ClockConnectionType {
    ROUTING_TO_CLOCK,
    CLOCK_TO_CLOCK,
    CLOCK_TO_PINS,
    ROUTING_TO_PINS
};


class ClockConnection {
    public:
        /*
         * Destructor
         */
        virtual ~ClockConnection() {};

        /*
         * Getters
         */
        virtual ClockConnectionType get_connection_type() const = 0;

        /*
         * Member functions
         */
        virtual void create_switches(const ClockRRGraph& clock_graph) = 0;
};


class RoutingToClockConnection : public ClockConnection {
    private:
        std::string clock_to_connect_to;
        std::string switch_name;
        Coordinates switch_location;
        int switch_idx;
        float fc;

        int seed = 101;
    public:
        /*
         * Getters
         */
        ClockConnectionType get_connection_type() const;

        /*
         * Setters
         */
        void set_clock_name_to_connect_to(std::string clock_name);
        void set_clock_switch_name(std::string clock_switch_name);
        void set_switch_location(int x, int y);
        void set_switch(int switch_index);
        void set_fc_val(float fc_val);

        /*
         * Member functions
         */
        /* Connects the inter-block routing to the clock source at the specified coordinates */
        void create_switches(const ClockRRGraph& clock_graph);

    private:

        std::vector<int> get_chan_wire_indices_at_switch_location(t_rr_type rr_type);

        std::vector<int> get_clock_indices_at_switch_location(const ClockRRGraph& clock_graph);
};


class ClockToClockConneciton : public ClockConnection {
    private:
        std::string from_clock;
        std::string from_switch;
        std::string to_clock;
        std::string to_switch;
        int switch_idx;
        float fc;
    
    public:
        /*
         * Getters
         */
        ClockConnectionType get_connection_type() const;
        
        /*
         * Setters
         */
        void set_from_clock_name(std::string clock_name);
        void set_from_clock_switch_name(std::string switch_name);
        void set_to_clock_name(std::string clock_name);
        void set_to_clock_switch_name(std::string switch_name);
        void set_switch(int switch_index);
        void set_fc_val(float fc_val);

        /*
         * Member functions
         */
        /* Connects a clock tap to a clock source */
        void create_switches(const ClockRRGraph& clock_graph);
};


class ClockToPinsConnection : public ClockConnection {
    private:
        std::string clock_to_connect_from;
        std::string switch_name;
//        pin_type; //To
        int switch_idx;
        float fc;

    public:
        /*
         * Getters
         */
        ClockConnectionType get_connection_type() const;
        
        /*
         * Setters
         */
        void set_clock_name_to_connect_from(std::string clock_name);
        void set_clock_switch_name(std::string connection_switch_name);
        void set_switch(int switch_index);
        void set_fc_val(float fc_val);

        /*
         * Member functions
         */
        /* Connects the clock tap to block pins */
        void create_switches(const ClockRRGraph& clock_graph);

};

#endif

