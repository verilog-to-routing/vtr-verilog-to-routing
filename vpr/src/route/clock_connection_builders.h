#ifndef CLOCK_CONNECTION_BUILDERS_H
#define CLOCK_CONNECTION_BUILDERS_H

#include <string>

#include "clock_fwd.h"

#include "rr_graph_clock.h"

class ClockRRGraphBuilder;

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
    virtual ~ClockConnection() {}

    /*
     * Member functions
     */
    virtual void create_switches(const ClockRRGraphBuilder& clock_graph) = 0;
};

class RoutingToClockConnection : public ClockConnection {
  private:
    std::string clock_to_connect_to;
    std::string switch_point_name;
    Coordinates switch_location;
    int rr_switch_idx;
    float fc;

    int seed = 101;

  public:
    /*
     * Setters
     */
    void set_clock_name_to_connect_to(std::string clock_name);
    void set_clock_switch_point_name(std::string clock_switch_point_name);
    void set_switch_location(int x, int y);
    void set_switch(int rr_switch_index);
    void set_fc_val(float fc_val);

    /*
     * Member functions
     */
    /* Connects the inter-block routing to the clock source at the specified coordinates */
    void create_switches(const ClockRRGraphBuilder& clock_graph);
};

class ClockToClockConneciton : public ClockConnection {
  private:
    std::string from_clock;
    std::string from_switch;
    std::string to_clock;
    std::string to_switch;
    int rr_switch_idx;
    float fc;

  public:
    /*
     * Setters
     */
    void set_from_clock_name(std::string clock_name);
    void set_from_clock_switch_point_name(std::string switch_point_name);
    void set_to_clock_name(std::string clock_name);
    void set_to_clock_switch_point_name(std::string switch_point_name);
    void set_switch(int rr_switch_index);
    void set_fc_val(float fc_val);

    /*
     * Member functions
     */
    /* Connects a clock tap to a clock source */
    void create_switches(const ClockRRGraphBuilder& clock_graph);
};

/* This class currently only supports Clock Network to clock pin connection.
 * It cannot yet connect the clock network to any arbitrary pin. */
class ClockToPinsConnection : public ClockConnection {
  private:
    std::string clock_to_connect_from;
    std::string switch_point_name;
    int rr_switch_idx;
    float fc;

  public:
    /*
     * Setters
     */
    void set_clock_name_to_connect_from(std::string clock_name);
    void set_clock_switch_point_name(std::string connection_switch_point_name);
    void set_switch(int rr_switch_index);
    void set_fc_val(float fc_val);

    /*
     * Member functions
     */
    /* Connects the clock tap to block pins */
    void create_switches(const ClockRRGraphBuilder& clock_graph);
};

#endif
