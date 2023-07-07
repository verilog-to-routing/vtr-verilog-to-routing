#ifndef NOC_LINK_H
#define NOC_LINK_H

/**
 * @file
 * @brief This file defines the NocLink class.
 *
 * Overview
 * ========
 * The NocLink represents a connection between two routers in the NoC.
 * The NocLink acts as edges in the NoC and can be used to traverse between routers.The
 * NocLinks are created based on the user provided topology information in the arch file.
 * The NocLink contains the following information:
 *
 * - The source router and destination router the link connects
 * - The bandwidth usage of the link. When a link is used within a
 * traffic flow (communication between two routers),
 * each link in the communication path needs to
 * support a predefined bandwidth of the flow. Every time a link
 * is used in a flow, its bandwidth usage increases based on the
 * bandwidth needed by this link. This is useful to track as it
 * can indicate when a link is being overused (the bandwidth usage
 * exceeds the links supported capability).
 * 
 * Example:
 * ```
 *  ----------                       ----------
 *  /        /        link           /        /
 *  / router / --------------------->/ router /
 *  /   a    /                       /   b    /
 *  /        /                       /        /
 *  /--------/                       /--------/
 * ```
 * In the example above the links source router would be router a and
 * the sink router would be router b. 
 * 
 * 
 */

#include <iostream>
#include "noc_router.h"
#include "noc_data_types.h"

class NocLink {
  private:
    // the two routers that are connected by this link
    NocRouterId source_router; /*!< The router which uses this link as an outgoing edge*/
    NocRouterId sink_router;   /*!< The router which uses this link as an incoming edge*/

    double bandwidth_usage; /*!< Represents the bandwidth of the data being transmitted on the link. Units in bits-per-second(bps)*/

  public:
    NocLink(NocRouterId source_router, NocRouterId sink_router);

    // getters

    /**
     * @brief Provides the id of the router that has this link as an outgoing
     * edge
     * @return A unique id (NocRouterId) that identifies the source router of the link
     */
    NocRouterId get_source_router(void) const;

    /**
     * @brief Provides the id of the router that has this link as an incoming
     * edge
     * @return A unique id (NocRouterId) that identifies the sink router of the link
     */
    NocRouterId get_sink_router(void) const;

    /**
     * @brief Provides the size of the data (bandwidth) being currently transmitted using the link.
     * @return A numeric value of the bandwidth of the link
     */
    double get_bandwidth_usage(void) const;

    // setters
    /**
     * @brief Can be used to set the source router of the link to a different router. 
     * @param source An identifier representing the router that should be the source of
     * this link
     */
    void set_source_router(NocRouterId source);

    /**
     * @brief Can be used to set the sink router of the link to a different router. 
     * @param sink An identifier representing the router that should be the sink of
     * this link
     */
    void set_sink_router(NocRouterId sink);

    /**
     * @brief Can modify the bandwidth of the link. It is expected that when the NoC is being placed
     * the traffic flows will be re-routed multiple times. So the links will end up being used and un-used
     * by different traffic flows and the bandwidths of the links will correspondingly change. This function
     * can be used to make those changes
     * @param new_bandwidth_usage The new value of the bandwidth of the link
     */
    void set_bandwidth_usage(double new_bandwidth_usage);
};

#endif