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
#include <memory>

#include "noc_routing.h"

class NocRoutingAlgorithmCreator {
  public:
    // nothing to do in the constructor and destructor
    NocRoutingAlgorithmCreator() = default;
    ~NocRoutingAlgorithmCreator() = default;

    /**
     * @brief Given a string that identifies a NoC routing algorithm, this 
     * function creates the corresponding routing algorithm and returns a
     * reference to it. If the provided string does not match any
     * available routing algorithms then an error is thrown.
     * 
     * @param routing_algorithm_name A user provided string that identifies a 
     * NoC routing algorithm
     * @return std::unique_ptr<NocRouting> A reference to the created NoC routing algorithm
     */
    static std::unique_ptr<NocRouting> create_routing_algorithm(const std::string& routing_algorithm_name);
};

#endif
