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
    NocLinkId id;

    // the two routers that are connected by this link
    NocRouterId source_router; /*!< The router which uses this link as an outgoing edge*/
    NocRouterId sink_router;   /*!< The router which uses this link as an incoming edge*/

    double bandwidth; /*!< Represents the maximum bits per second that can be transmitted over the link without causing congestion*/
    double latency; /*!< The zero-load latency of this link in seconds.*/

  public:
    NocLink(NocLinkId link_id, NocRouterId source_router, NocRouterId sink_router,
            double bw, double lat);

    // getters

    /**
     * @brief Provides the id of the router that has this link as an outgoing
     * edge
     * @return A unique id (NocRouterId) that identifies the source router of the link
     */
    NocRouterId get_source_router() const;

    /**
     * @brief Provides the id of the router that has this link as an incoming
     * edge
     * @return A unique id (NocRouterId) that identifies the sink router of the link
     */
    NocRouterId get_sink_router() const;

    /**
     * @brief Returns the maximum bandwidth that the link can carry without congestion.
     * @return A numeric value of the bandwidth capacity of the link
     */
    double get_bandwidth() const;

    /**
     * @brief Returns the zero-load latency of the link.
     * @return double Zero-load latency of the link.
     */
    double get_latency() const;

    /**
     * @brief Returns the unique link ID. The ID can be used to index
     * vtr::vector<NoCLinkId, ...> instances.
     * @return The unique ID for the link
     */
    NocLinkId get_link_id() const;

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
     * @brief Sets the bandwidth capacity of the link. This function should be used when
     * global NoC data structures are created and populated. The bandwidth capacity is used
     * along with bandwidth_usage to measure congestion.
     * @param new_bandwidth The new value of the bandwidth of the link
     */
    void set_bandwidth(double new_bandwidth);

    /**
     * @brief Returns the unique link ID. The ID can be used to index
     * vtr::vector<NoCLinkId, ...> instances.
     * @return The unique ID for the link
     */
    operator NocLinkId() const;
};

#endif