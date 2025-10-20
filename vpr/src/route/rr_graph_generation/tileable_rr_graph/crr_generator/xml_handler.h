#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>

#include <pugixml.hpp>

#include "crr_common.h"
#include "custom_rr_graph.h"

namespace crrgenerator {

/**
 * @brief Handles XML file I/O operations for RR graphs
 *
 * This class provides functionality to read and write RR graph XML files,
 * with support for attribute mapping and pretty printing.
 */
class XMLHandler {
public:
    XMLHandler();

    /**
     * @brief Read RR graph from XML file
     * @param filename Path to the XML file
     * @return Parsed RR graph
     * @throws FileException if file cannot be read
     * @throws ParseException if XML is malformed
     */
    std::unique_ptr<RRGraph> read_rr_graph(const std::string& filename);

    /**
     * @brief Write RR graph to XML file
     * @param graph RR graph to write
     * @param filename Output file path
     * @throws FileException if file cannot be written
     */
    void write_rr_graph(const std::string& filename, const RRGraph& graph);

    /**
     * @brief Update VPR XML file with new switches
     * @param switches Vector of switches to add
     * @param input_file Input VPR XML file
     * @param output_file Output VPR XML file
     * @param switch_delay_min Minimum switch delay ID
     * @param switch_delay_max Maximum switch delay ID
     */
    void update_vpr_xml_with_switches(const std::vector<Switch>& switches,
                                    const std::string& input_file,
                                    const std::string& output_file,
                                    SwitchId switch_delay_min,
                                    SwitchId switch_delay_max);

 private:
    // Attribute mapping for XML conversion
    std::unordered_map<std::string, std::string> attribute_map_;

    // XML parsing methods
    void parse_tool_info(const pugi::xml_node& root, RRGraph& graph);
    void parse_channels(const pugi::xml_node& channels_node, RRGraph& graph);
    void parse_switches(const pugi::xml_node& switches_node, RRGraph& graph);
    void parse_segments(const pugi::xml_node& segments_node, RRGraph& graph);
    void parse_block_types(const pugi::xml_node& block_types_node,
                            RRGraph& graph);
    void parse_grid(const pugi::xml_node& grids_node, RRGraph& graph);
    void parse_nodes(const pugi::xml_node& nodes_node, RRGraph& graph);
    void parse_edges(const pugi::xml_node& edges_node, RRGraph& graph);

    // Node parsing helpers
    RRNode parse_node(const pugi::xml_node& node_xml);
    Location parse_location(const pugi::xml_node& loc_xml);
    NodeTiming parse_node_timing(const pugi::xml_node& timing_xml);
    NodeSegmentId parse_node_segment(const pugi::xml_node& segment_xml);

    // Channel parsing helpers
    Channel parse_channel(const pugi::xml_node& channel_xml);
    XYList parse_xy_list(const pugi::xml_node& xy_list_xml);

    // Switch parsing helpers
    Switch parse_switch(const pugi::xml_node& switch_xml);
    Timing parse_timing(const pugi::xml_node& timing_xml);
    Sizing parse_sizing(const pugi::xml_node& sizing_xml);

    // Segment parsing helpers
    Segment parse_segment(const pugi::xml_node& segment_xml);

    // Block type parsing helpers
    BlockType parse_block_type(const pugi::xml_node& block_type_xml);
    PinClass parse_pin_class(const pugi::xml_node& pin_class_xml);
    Pin parse_pin(const pugi::xml_node& pin_xml);

    // Grid parsing helpers
    GridLoc parse_grid_loc(const pugi::xml_node& grid_xml);

    // Edge parsing helpers
    RREdge parse_edge(const pugi::xml_node& edge_xml);

    // XML writing methods (streaming)
    void write_tool_info(std::ostream& out, const RRGraph& graph);
    void write_channels(std::ostream& out, const RRGraph& graph);
    void write_switches(std::ostream& out, const RRGraph& graph);
    void write_segments(std::ostream& out, const RRGraph& graph);
    void write_block_types(std::ostream& out, const RRGraph& graph);
    void write_grid(std::ostream& out, const RRGraph& graph);
    void write_nodes(std::ostream& out, const RRGraph& graph);
    void write_edges(std::ostream& out, const RRGraph& graph);

    // Writing helpers (streaming)
    void write_channel(std::ostream& out, const Channel& channel);
    void write_xy_list(std::ostream& out, const XYList& xy_list);
    void write_switch(std::ostream& out, const Switch& switch_obj);
    void write_timing(std::ostream& out, const Timing& timing);
    void write_sizing(std::ostream& out, const Sizing& sizing);
    void write_block_type(std::ostream& out, const BlockType& block_type);
    void write_pin_class(std::ostream& out, const PinClass& pin_class);
    void write_pin(std::ostream& out, const Pin& pin);
    void write_grid_loc(std::ostream& out, const GridLoc& grid_loc);
    void write_node(std::ostream& out, const RRNode& node);
    void write_location(std::ostream& out, const Location& location);
    void write_node_timing(std::ostream& out, const NodeTiming& timing);
    void write_node_segment(std::ostream& out, const NodeSegmentId& segment_id);
    void write_edge(std::ostream& out, const RREdge& edge);

    // Utility methods
    void setup_attribute_mapping();
    std::string map_attribute_name(const std::string& internal_name);
    std::string format_float_value(double value);
    void update_element_attributes(
        pugi::xml_node& element,
        const std::map<std::string, std::string>& attributes);

    // Error handling
    void validate_xml_structure(const pugi::xml_document& doc);
    void check_required_elements(
        const pugi::xml_node& parent,
        const std::vector<std::string>& required_children);
};

}  // namespace crrgenerator
