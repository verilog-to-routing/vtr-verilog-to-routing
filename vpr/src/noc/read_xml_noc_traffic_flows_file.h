#ifndef READ_XML_NOC_TRAFFIC_FLOWS_FILE_H
#define READ_XML_NOC_TRAFFIC_FLOWS_FILE_H

#include"noc_traffic_flows.h"

#include <iostream>
#include<vector>
#include<string>

// identifier when an integer conversion failed while reading an attribute value in an xml file
constexpr int NUMERICAL_ATTRIBUTE_CONVERSION_FAILURE = -1;

/**
 * @brief Parse an xml '.flows' file that contains a number of traffic
 *        in the NoC. A traffic flow is a communication between one router
 *        in the NoC to another. The XML file contains a number of these traffic
 *        flows and provides additional information about them, such as the
 *        size of data being tranferred and constraints on the latency of the
 *        data transmission. Once the traffic flows are parsed, they are stored
 *        inside the NocTrafficFlows class.
 * 
 * @param noc_flows_file Name of the noc '.flows' file
 */
void read_xml_noc_traffic_flows_file(char* noc_flows_file);















#endif