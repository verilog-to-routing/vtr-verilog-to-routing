#ifndef NOC_ROUTING_ALGORITHM_CREATOR
#define NOC_ROUTING_ALGORITHM_CREATOR

/**
 * @file
 * @brief  This file defines the NocRoutingAlgorithmCreator class, which creates
 * the routing algorithm that will be used to route packets within the NoC.
 * 
 * Overview
 * ========
 * There are a number of different available NoC routing algorithms. This class is a factory object for the NocRouting abstract class. This class constructs 
 * the appropriate routing algorithm based on the user specification in the
 * command line. The user identifies a 
 * specific routing algorithm in the command line by providing a string
 * (which is the name of routing algorithm).
 * Then the corresponding routing algorithm is created here based on the 
 * provided string.
 */

#include <string>

#include "noc_routing.h"
#include "xy_routing.h"
#include "bfs_routing.h"

class NocRoutingAlgorithmCreator {
  public:
    // nothing to do in the constructor and destructor
    NocRoutingAlgorithmCreator() {}
    ~NocRoutingAlgorithmCreator() {}

    /**
     * @brief Given a string that identifies a NoC routing algorithm, this 
     * function creates the corresponding routing algorithm and returns a
     * reference to it. If the provided string does not match any
     * available routing algorithms then an error is thrown.
     * 
     * @param routing_algorithm_name A user provided string that identifies a 
     * NoC routing algorithm
     * @return NocRouting* A reference to the created NoC routing algorithm
     */
    NocRouting* create_routing_algorithm(std::string routing_algorithm_name);
};

#endif
