#include <sstream> // for std::ostringstream
#include <iomanip> // for std::setprecision and std::scientific

#include "xml_handler.h"

#include "vtr_log.h"
#include "vtr_assert.h"

namespace crrgenerator {

inline void xml_escape_and_write(std::ostream& os, const std::string& s) {
    for (char c : s) {
        switch (c) {
            case '&':
                os << "&amp;";
                break;
            case '<':
                os << "&lt;";
                break;
            case '>':
                os << "&gt;";
                break;
            case '"':
                os << "&quot;";
                break;
            case '\'':
                os << "&apos;";
                break;
            default:
                os << c;
                break;
        }
    }
}
inline void write_attr(std::ostream& os, const char* name, const std::string& v) {
    os << ' ' << name << "=\"";
    xml_escape_and_write(os, v);
    os << '"';
}
template<typename T>
inline void write_attr(std::ostream& os, const char* name, T v) {
    os << ' ' << name << "=\"" << v << '"';
}

XMLHandler::XMLHandler() { setup_attribute_mapping(); }

void XMLHandler::setup_attribute_mapping() {
    attribute_map_ = {{"x_max", "xMax"},
                      {"x_min", "xMin"},
                      {"y_max", "yMax"},
                      {"y_min", "yMin"},
                      {"res_type", "resType"},
                      {"chan_width_max", "chanWidthMax"},
                      {"buf_size", "bufSize"},
                      {"mux_trans_size", "muxTransSize"},
                      {"C_per_meter", "cPerMeter"},
                      {"R_per_meter", "rPerMeter"},
                      {"block_type_id", "blockTypeId"},
                      {"height_offset", "heightOffset"},
                      {"width_offset", "widthOffset"},
                      {"C", "c"},
                      {"R", "r"},
                      {"Cinternal", "cinternal"},
                      {"Cin", "cin"},
                      {"Cout", "cout"},
                      {"Tdel", "tdel"},
                      {"src_node", "srcNode"},
                      {"sink_node", "sinkNode"},
                      {"switch_id", "switchId"},
                      {"segment_id", "segmentId"}};
}

std::unique_ptr<RRGraph> XMLHandler::read_rr_graph(
    const std::string& filename) {
    VTR_LOG("Reading RR graph from: %s\n", filename.c_str());

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str(),
                                                  pugi::parse_default);

    if (!result) {
        std::ifstream in(filename);
        std::string line;
        size_t pos = 0;
        int lineno = 1;
        while (std::getline(in, line)) {
            if (pos + line.size() + 1 > static_cast<size_t>(result.offset)) {
                VTR_LOG("Parse error at line %d near: %s\n", lineno, line.c_str());
                break;
            }
            pos += line.size() + 1;
            lineno++;
        }

        VTR_LOG_ERROR("Failed to parse XML file %s: %s (at byte offset %zu)\n", filename.c_str(),
                      result.description(), result.offset);
    }

    validate_xml_structure(doc);

    auto graph = std::make_unique<RRGraph>();
    pugi::xml_node root = doc.child("rr_graph");

    // Parse tool information
    parse_tool_info(root, *graph);

    // Parse main sections
    if (auto channels = root.child("channels")) {
        parse_channels(channels, *graph);
    }

    if (auto switches = root.child("switches")) {
        parse_switches(switches, *graph);
    }

    if (auto segments = root.child("segments")) {
        parse_segments(segments, *graph);
    }

    if (auto block_types = root.child("block_types")) {
        parse_block_types(block_types, *graph);
    }

    if (auto grids = root.child("grid")) {
        parse_grid(grids, *graph);
    }

    if (auto rr_nodes = root.child("rr_nodes")) {
        parse_nodes(rr_nodes, *graph);
    }

    if (auto rr_edges = root.child("rr_edges")) {
        parse_edges(rr_edges, *graph);
    }

    graph->print_statistics();
    VTR_LOG("Successfully loaded RR graph from %s\n", filename.c_str());

    return graph;
}

void XMLHandler::write_rr_graph(const std::string& filename,
                                const RRGraph& graph) {
    VTR_LOG("Writing RR graph to: %s\n", filename.c_str());

    std::string temp_filename = filename + ".tmp";
    std::vector<char> out_buf(100 * 1024 * 1024);
    std::ofstream out;
    out.rdbuf()->pubsetbuf(out_buf.data(), static_cast<std::streamsize>(out_buf.size()));
    out.open(temp_filename, std::ios::out | std::ios::binary);
    if (!out) {
        VTR_LOG_ERROR("Failed to open output file: %s\n", temp_filename.c_str());
    }

    out << "<?xml version=\"1.0\"?>\n";
    out << "<rr_graph";
    write_tool_info(out, graph);
    out << ">\n";

    write_channels(out, graph);
    write_switches(out, graph);
    write_segments(out, graph);
    write_block_types(out, graph);
    write_grid(out, graph);
    write_nodes(out, graph);
    write_edges(out, graph);

    out << "</rr_graph>\n";
    out.close();

    if (std::rename(temp_filename.c_str(), filename.c_str()) != 0) {
        std::remove(temp_filename.c_str());
        VTR_LOG_ERROR("Failed to atomically replace XML file: %s\n", filename.c_str());
    }

    VTR_LOG("Successfully wrote RR graph to %s\n", filename.c_str());
}

void XMLHandler::parse_tool_info(const pugi::xml_node& root, RRGraph& graph) {
    std::string tool_name = root.attribute("tool_name").as_string();
    std::string tool_version = root.attribute("tool_version").as_string();
    std::string tool_comment = root.attribute("tool_comment").as_string();

    graph.set_tool_info(tool_name, tool_version, tool_comment);
}

void XMLHandler::parse_channels(const pugi::xml_node& channels_node,
                                RRGraph& graph) {
    for (pugi::xml_node channel_node : channels_node.children("channel")) {
        Channel channel = parse_channel(channel_node);
        graph.add_channel(channel);
    }
    for (pugi::xml_node channel_node : channels_node.children("x_list")) {
        XYList xy_list = parse_xy_list(channel_node);
        graph.add_xy_list(xy_list);
    }
    for (pugi::xml_node channel_node : channels_node.children("y_list")) {
        XYList xy_list = parse_xy_list(channel_node);
        graph.add_xy_list(xy_list);
    }
    VTR_LOG_DEBUG("Parsed %zu channels\n", graph.get_channel_count());
}

Channel XMLHandler::parse_channel(const pugi::xml_node& channel_node) {
    int max_width = channel_node.attribute("chan_width_max").as_int();
    int x_max = channel_node.attribute("x_max").as_int();
    int x_min = channel_node.attribute("x_min").as_int();
    int y_max = channel_node.attribute("y_max").as_int();
    int y_min = channel_node.attribute("y_min").as_int();

    return Channel(max_width, x_max, x_min, y_max, y_min);
}

XYList XMLHandler::parse_xy_list(const pugi::xml_node& xy_list_node) {
    XYList::Type type = XYList::Type::INVALID_LIST;
    if (std::string(xy_list_node.name()) == "x_list") {
        type = XYList::Type::X_LIST;
    } else {
        VTR_ASSERT(std::string(xy_list_node.name()) == "y_list");
        type = XYList::Type::Y_LIST;
    }
    int index = xy_list_node.attribute("index").as_int();
    int info = xy_list_node.attribute("info").as_int();
    return XYList(type, index, info);
}

void XMLHandler::parse_switches(const pugi::xml_node& switches_node,
                                RRGraph& graph) {
    for (pugi::xml_node switch_node : switches_node.children("switch")) {
        Switch switch_obj = parse_switch(switch_node);
        graph.add_switch(switch_obj);
    }
    VTR_LOG_DEBUG("Parsed %zu switches\n", graph.get_switch_count());
}

Switch XMLHandler::parse_switch(const pugi::xml_node& switch_xml) {
    SwitchId id = switch_xml.attribute("id").as_int();
    std::string name = switch_xml.attribute("name").as_string();
    std::string type = switch_xml.attribute("type").as_string();

    Timing timing;
    if (auto timing_node = switch_xml.child("timing")) {
        timing = parse_timing(timing_node);
    }

    Sizing sizing;
    if (auto sizing_node = switch_xml.child("sizing")) {
        sizing = parse_sizing(sizing_node);
    }

    return Switch(id, name, type, timing, sizing);
}

Timing XMLHandler::parse_timing(const pugi::xml_node& timing_xml) {
    return Timing(timing_xml.attribute("Cin").as_float(),
                  timing_xml.attribute("Tdel").as_float());
}

Sizing XMLHandler::parse_sizing(const pugi::xml_node& sizing_xml) {
    return Sizing(sizing_xml.attribute("mux_trans_size").as_float(),
                  sizing_xml.attribute("buf_size").as_float());
}

void XMLHandler::parse_segments(const pugi::xml_node& segments_node,
                                RRGraph& graph) {
    for (pugi::xml_node segment_node : segments_node.children("segment")) {
        Segment segment = parse_segment(segment_node);
        graph.add_segment(segment);
    }
    VTR_LOG_DEBUG("Parsed %zu segments\n", graph.get_segment_count());
}

Segment XMLHandler::parse_segment(const pugi::xml_node& segment_xml) {
    SegmentId id = segment_xml.attribute("id").as_int();
    std::string name = segment_xml.attribute("name").as_string();
    int length = -1;
    if (segment_xml.attribute("length")) {
        length = segment_xml.attribute("length").as_int();
    }
    std::string res_type = "";
    if (segment_xml.attribute("res_type")) {
        res_type = segment_xml.attribute("res_type").as_string();
    }
    return Segment(id, name, length, res_type);
}

void XMLHandler::parse_block_types(const pugi::xml_node& block_types_node,
                                   RRGraph& graph) {
    for (pugi::xml_node block_type_node :
         block_types_node.children("block_type")) {
        BlockType block_type = parse_block_type(block_type_node);
        graph.add_block_type(block_type);
    }
    VTR_LOG_DEBUG("Parsed %zu block types\n", graph.get_block_type_count());
}

BlockType XMLHandler::parse_block_type(const pugi::xml_node& block_type_node) {
    int id = block_type_node.attribute("id").as_int();
    std::string name = block_type_node.attribute("name").as_string();
    int height = block_type_node.attribute("height").as_int();
    int width = block_type_node.attribute("width").as_int();
    std::vector<PinClass> pin_classes;
    for (pugi::xml_node pin_class_node : block_type_node.children("pin_class")) {
        PinClass pin_class = parse_pin_class(pin_class_node);
        pin_classes.push_back(pin_class);
    }
    return BlockType(id, name, height, width, pin_classes);
}

PinClass XMLHandler::parse_pin_class(const pugi::xml_node& pin_class_node) {
    std::string type = pin_class_node.attribute("type").as_string();
    std::vector<Pin> pins;
    for (pugi::xml_node pin_node : pin_class_node.children("pin")) {
        Pin pin = parse_pin(pin_node);
        pins.push_back(pin);
    }
    return PinClass(type, pins);
}

Pin XMLHandler::parse_pin(const pugi::xml_node& pin_node) {
    int ptc = pin_node.attribute("ptc").as_int();
    std::string value = pin_node.text().get();
    return Pin(ptc, value);
}

void XMLHandler::parse_grid(const pugi::xml_node& grids_node, RRGraph& graph) {
    for (pugi::xml_node grid_node : grids_node.children("grid_loc")) {
        GridLoc grid_loc = parse_grid_loc(grid_node);
        graph.add_grid_loc(grid_loc);
    }
    VTR_LOG_DEBUG("Parsed %zu grid locations\n", graph.get_grid_loc_count());
}

GridLoc XMLHandler::parse_grid_loc(const pugi::xml_node& grid_node) {
    int type_id = grid_node.attribute("block_type_id").as_int();
    int height_offset = grid_node.attribute("height_offset").as_int();
    int width_offset = grid_node.attribute("width_offset").as_int();
    int x = grid_node.attribute("x").as_int();
    int y = grid_node.attribute("y").as_int();
    int layer = grid_node.attribute("layer").as_int();
    return GridLoc(type_id, height_offset, width_offset, x, y, layer);
}

void XMLHandler::parse_nodes(const pugi::xml_node& nodes_node, RRGraph& graph) {
    for (pugi::xml_node node_xml : nodes_node.children("node")) {
        RRNode node = parse_node(node_xml);
        graph.add_node(node);
    }
    VTR_LOG_DEBUG("Parsed %zu nodes\n", graph.get_node_count());
}

RRNode XMLHandler::parse_node(const pugi::xml_node& node_xml) {
    NodeId id = node_xml.attribute("id").as_int();
    NodeType type = string_to_node_type(node_xml.attribute("type").as_string());
    int capacity = node_xml.attribute("capacity").as_int();
    std::string direction = "";
    if (node_xml.attribute("direction")) {
        direction = node_xml.attribute("direction").as_string();
    }

    Location location;
    if (auto loc_node = node_xml.child("loc")) {
        location = parse_location(loc_node);
    }

    NodeTiming timing;
    if (auto timing_node = node_xml.child("timing")) {
        timing = parse_node_timing(timing_node);
    }

    NodeSegmentId segment_id;
    if (auto segment_node = node_xml.child("segment")) {
        segment_id = parse_node_segment(segment_node);
    }

    return RRNode(id, type, capacity, direction, location, timing, segment_id);
}

Location XMLHandler::parse_location(const pugi::xml_node& loc_xml) {
    int layer = 0;
    if (loc_xml.attribute("layer")) {
        layer = loc_xml.attribute("layer").as_int();
    }
    std::string side = "";
    if (loc_xml.attribute("side")) {
        side = loc_xml.attribute("side").as_string();
    }
    return Location(
        loc_xml.attribute("xlow").as_int(), loc_xml.attribute("xhigh").as_int(),
        loc_xml.attribute("ylow").as_int(), loc_xml.attribute("yhigh").as_int(),
        layer, loc_xml.attribute("ptc").as_string(), side);
}

NodeTiming XMLHandler::parse_node_timing(const pugi::xml_node& timing_xml) {
    return NodeTiming(timing_xml.attribute("R").as_float(),
                      timing_xml.attribute("C").as_float());
}

NodeSegmentId XMLHandler::parse_node_segment(const pugi::xml_node& segment_xml) {
    return NodeSegmentId(segment_xml.attribute("segment_id").as_int());
}

void XMLHandler::parse_edges(const pugi::xml_node& edges_node, RRGraph& graph) {
    for (pugi::xml_node edge_xml : edges_node.children("edge")) {
        RREdge edge = parse_edge(edge_xml);
        graph.add_edge(edge);
    }
    VTR_LOG_DEBUG("Parsed %zu edges\n", graph.get_edge_count());
}

RREdge XMLHandler::parse_edge(const pugi::xml_node& edge_xml) {
    NodeId src_node = edge_xml.attribute("src_node").as_int();
    NodeId sink_node = edge_xml.attribute("sink_node").as_int();
    SwitchId switch_id = edge_xml.attribute("switch_id").as_int();

    return RREdge(src_node, sink_node, switch_id);
}

void XMLHandler::write_tool_info(std::ostream& out, const RRGraph& graph) {
    if (!graph.get_tool_name().empty()) write_attr(out, "tool_name", graph.get_tool_name());
    if (!graph.get_tool_version().empty()) write_attr(out, "tool_version", graph.get_tool_version());
    if (!graph.get_tool_comment().empty()) write_attr(out, "tool_comment", graph.get_tool_comment());
}

void XMLHandler::write_channels(std::ostream& out, const RRGraph& graph) {
    out << "  <channels>\n";
    for (const auto& channel : graph.get_channels())
        write_channel(out, channel);
    for (const auto& xy_list : graph.get_xy_lists())
        write_xy_list(out, xy_list);
    out << "  </channels>\n";
}

void XMLHandler::write_channel(std::ostream& out, const Channel& channel) {
    out << "    <channel";
    write_attr(out, "chan_width_max", channel.get_max_width());
    write_attr(out, "x_max", channel.get_x_max());
    write_attr(out, "x_min", channel.get_x_min());
    write_attr(out, "y_max", channel.get_y_max());
    write_attr(out, "y_min", channel.get_y_min());
    out << "/>\n";
}

void XMLHandler::write_xy_list(std::ostream& out, const XYList& xy_list) {
    out << "    <" << (xy_list.get_type() == XYList::Type::X_LIST ? "x_list" : "y_list");
    write_attr(out, "index", xy_list.get_index());
    write_attr(out, "info", xy_list.get_info());
    out << "/>\n";
}

void XMLHandler::write_switches(std::ostream& out, const RRGraph& graph) {
    out << "  <switches>\n";
    for (const auto& s : graph.get_switches())
        write_switch(out, s);
    out << "  </switches>\n";
}

void XMLHandler::write_switch(std::ostream& out, const Switch& s) {
    out << "    <switch";
    write_attr(out, "id", s.get_id());
    write_attr(out, "name", s.get_name());
    write_attr(out, "type", s.get_type());
    out << ">\n";
    out << "      <timing";
    write_attr(out, "Cin", format_float_value(s.get_timing().Cin));
    write_attr(out, "Tdel", format_float_value(s.get_timing().Tdel));
    out << "/>\n";
    out << "      <sizing";
    write_attr(out, "mux_trans_size", format_float_value(s.get_sizing().mux_trans_size));
    write_attr(out, "buf_size", format_float_value(s.get_sizing().buf_size));
    out << "/>\n";
    out << "    </switch>\n";
}

void XMLHandler::write_segments(std::ostream& out, const RRGraph& graph) {
    if (graph.get_segment_count() == 0) return;
    out << "  <segments>\n";
    for (const auto& seg : graph.get_segments()) {
        out << "    <segment";
        write_attr(out, "id", seg.get_id());
        write_attr(out, "name", seg.get_name());
        if (seg.get_length() != -1) write_attr(out, "length", seg.get_length());
        if (!seg.get_res_type().empty()) write_attr(out, "res_type", seg.get_res_type());
        out << "/>\n";
    }
    out << "  </segments>\n";
}

void XMLHandler::write_block_types(std::ostream& out, const RRGraph& graph) {
    out << "  <block_types>\n";
    for (const auto& bt : graph.get_block_types())
        write_block_type(out, bt);
    out << "  </block_types>\n";
}

void XMLHandler::write_block_type(std::ostream& out, const BlockType& bt) {
    out << "    <block_type";
    write_attr(out, "id", bt.get_id());
    write_attr(out, "name", bt.get_name());
    write_attr(out, "height", bt.get_height());
    write_attr(out, "width", bt.get_width());
    out << ">\n";
    for (const auto& pc : bt.get_pin_classes())
        write_pin_class(out, pc);
    out << "    </block_type>\n";
}

void XMLHandler::write_pin_class(std::ostream& out, const PinClass& pc) {
    out << "      <pin_class";
    write_attr(out, "type", pc.get_type());
    out << ">\n";
    for (const auto& p : pc.get_pins())
        write_pin(out, p);
    out << "      </pin_class>\n";
}

void XMLHandler::write_pin(std::ostream& out, const Pin& pin) {
    out << "        <pin";
    write_attr(out, "ptc", pin.get_ptc());
    out << '>';
    xml_escape_and_write(out, pin.get_value());
    out << "</pin>\n";
}

void XMLHandler::write_grid(std::ostream& out, const RRGraph& graph) {
    out << "  <grid>\n";
    for (const auto& gl : graph.get_grid_locs())
        write_grid_loc(out, gl);
    out << "  </grid>\n";
}

void XMLHandler::write_grid_loc(std::ostream& out, const GridLoc& gl) {
    out << "    <grid_loc";
    write_attr(out, "block_type_id", gl.get_type_id());
    write_attr(out, "height_offset", gl.get_height_offset());
    write_attr(out, "width_offset", gl.get_width_offset());
    write_attr(out, "x", gl.get_x());
    write_attr(out, "y", gl.get_y());
    write_attr(out, "layer", gl.get_layer());
    out << "/>\n";
}

void XMLHandler::write_nodes(std::ostream& out, const RRGraph& graph) {
    out << "  <rr_nodes>\n";
    for (const auto& n : graph.get_nodes())
        write_node(out, n);
    out << "  </rr_nodes>\n";
}

void XMLHandler::write_node(std::ostream& out, const RRNode& n) {
    out << "    <node";
    write_attr(out, "id", n.get_id());
    write_attr(out, "type", to_string(n.get_type()));
    write_attr(out, "capacity", n.get_capacity());
    if (!n.get_direction().empty()) write_attr(out, "direction", n.get_direction());
    out << ">\n";
    write_location(out, n.get_location());
    write_node_timing(out, n.get_timing());
    if (!n.get_segment().empty()) write_node_segment(out, n.get_segment());
    out << "    </node>\n";
}

void XMLHandler::write_location(std::ostream& out, const Location& loc) {
    out << "      <loc";
    write_attr(out, "xlow", loc.x_low);
    write_attr(out, "xhigh", loc.x_high);
    write_attr(out, "ylow", loc.y_low);
    write_attr(out, "yhigh", loc.y_high);
    write_attr(out, "layer", loc.layer);
    write_attr(out, "ptc", loc.ptc);
    if (!loc.side.empty()) write_attr(out, "side", loc.side);
    out << "/>\n";
}

void XMLHandler::write_node_timing(std::ostream& out, const NodeTiming& t) {
    out << "      <timing";
    write_attr(out, "R", format_float_value(t.r));
    write_attr(out, "C", format_float_value(t.c));
    out << "/>\n";
}

void XMLHandler::write_node_segment(std::ostream& out, const NodeSegmentId& seg) {
    out << "      <segment";
    write_attr(out, "segment_id", seg.segment_id);
    out << "/>\n";
}

void XMLHandler::write_edges(std::ostream& out, const RRGraph& graph) {
    out << "  <rr_edges>\n";
    for (const auto& e : graph.get_edges())
        write_edge(out, e);
    out << "  </rr_edges>\n";
}

void XMLHandler::write_edge(std::ostream& out, const RREdge& e) {
    out << "    <edge";
    write_attr(out, "src_node", e.get_src_node());
    write_attr(out, "sink_node", e.get_sink_node());
    write_attr(out, "switch_id", e.get_switch_id());
    out << "/>\n";
}

std::string XMLHandler::format_float_value(double value) {
    std::ostringstream oss;
    oss << value;
    std::string result = oss.str();

    // Remove trailing .0 for integer values
    if (result.length() >= 2 && result.ends_with(".0")) {
        result = result.substr(0, result.length() - 2);
    }

    // Format scientific notation
    if (result.find("e-") != std::string::npos) {
        oss.str("");
        oss.clear();
        oss << std::scientific << std::setprecision(8) << value;
        result = oss.str();
    }

    return result;
}

void XMLHandler::validate_xml_structure(const pugi::xml_document& doc) {
    pugi::xml_node root = doc.child("rr_graph");
    if (!root) {
        VTR_LOG_ERROR("Root element 'rr_graph' not found\n");
    }
}

std::string XMLHandler::map_attribute_name(const std::string& internal_name) {
    auto it = attribute_map_.find(internal_name);
    return (it != attribute_map_.end()) ? it->second : internal_name;
}

} // namespace crrgenerator
