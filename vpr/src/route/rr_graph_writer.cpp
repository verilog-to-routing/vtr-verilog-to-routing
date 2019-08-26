/*
 * This file defines the writing rr graph function in XML format.
 * The rr graph is separated into channels, nodes, switches,
 * grids, edges, block types, and segments. Each tag has several
 * children tags such as timing, location, or some general
 * details. Each tag has attributes to describe them */

#include <fstream>
#include <iostream>
#include <string.h>
#include <iomanip>
#include <limits>
#include "vpr_error.h"
#include "globals.h"
#include "read_xml_arch_file.h"
#include "vtr_version.h"
#include "rr_graph_writer.h"

using namespace std;

/* All values are printed with this precision value. The higher the
 * value, the more accurate the read in rr graph is. Using numeric_limits
 * max_digits10 guarentees that no values change during a sequence of
 * float -> string -> float conversions */
constexpr int FLOAT_PRECISION = std::numeric_limits<float>::max_digits10;
/*********************** Subroutines local to this module *******************/
void write_rr_channel(fstream& fp);
void write_rr_node(fstream& fp);
void write_rr_switches(fstream& fp);
void write_rr_grid(fstream& fp);
void write_rr_edges(fstream& fp);
void write_rr_block_types(fstream& fp);
void write_rr_segments(fstream& fp, const std::vector<t_segment_inf>& segment_inf);

/************************ Subroutine definitions ****************************/

/* This function is used to write the rr_graph into xml format into a a file with name: file_name */
void write_rr_graph(const char* file_name, const std::vector<t_segment_inf>& segment_inf) {
    fstream fp;
    fp.open(file_name, fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        VPR_FATAL_ERROR(VPR_ERROR_OTHER,
                        "couldn't open file \"%s\" for generating RR graph file\n", file_name);
    }
    cout << "Writing RR graph" << endl;
    fp << "<rr_graph tool_name=\"vpr\" tool_version=\"" << vtr::VERSION << "\" tool_comment=\"Generated from arch file "
       << get_arch_file_name() << "\">" << endl;

    /* Write out each individual component*/
    write_rr_channel(fp);
    write_rr_switches(fp);
    write_rr_segments(fp, segment_inf);
    write_rr_block_types(fp);
    write_rr_grid(fp);
    write_rr_node(fp);
    write_rr_edges(fp);
    fp << "</rr_graph>";

    fp.close();

    cout << "Finished generating RR graph file named " << file_name << endl
         << endl;
}

static void add_metadata_to_xml(fstream& fp, const char* tab_prefix, const t_metadata_dict& meta) {
    fp << tab_prefix << "<metadata>" << endl;

    for (const auto& meta_elem : meta) {
        const std::string& key = meta_elem.first;
        const std::vector<t_metadata_value>& values = meta_elem.second;
        for (const auto& value : values) {
            fp << tab_prefix << "\t<meta name=\"" << key << "\"";
            fp << ">" << value.as_string() << "</meta>" << endl;
        }
    }
    fp << tab_prefix << "</metadata>" << endl;
}

/* Channel info in device_ctx.chan_width is written in xml format.
 * A general summary of the min and max values of the channels are first printed. Every
 * x and y channel list is printed out in its own attribute*/
void write_rr_channel(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<channels>" << endl;
    fp << "\t\t<channel chan_width_max =\"" << device_ctx.chan_width.max << "\" x_min=\"" << device_ctx.chan_width.x_min << "\" y_min=\"" << device_ctx.chan_width.y_min << "\" x_max=\"" << device_ctx.chan_width.x_max << "\" y_max=\"" << device_ctx.chan_width.y_max << "\"/>" << endl;

    auto& list = device_ctx.chan_width.x_list;
    for (size_t i = 0; i < device_ctx.grid.height() - 1; i++) {
        fp << "\t\t<x_list index =\"" << i << "\" info=\"" << list[i] << "\"/>" << endl;
    }
    auto& list2 = device_ctx.chan_width.y_list;
    for (size_t i = 0; i < device_ctx.grid.width() - 1; i++) {
        fp << "\t\t<y_list index =\"" << i << "\" info=\"" << list2[i] << "\"/>" << endl;
    }
    fp << "\t</channels>" << endl;
}

/* All relevant rr node info is written out to the graph.
 * This includes location, timing, and segment info*/
void write_rr_node(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();

    fp << "\t<rr_nodes>" << endl;

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        fp << "\t\t<node";
        fp << " id=\"" << inode;
        fp << "\" type=\"" << node.type_string();
        if (node.type() == CHANX || node.type() == CHANY) {
            fp << "\" direction=\"" << node.direction_string();
        }
        fp << "\" capacity=\"" << node.capacity();
        fp << "\">" << endl;
        fp << "\t\t\t<loc";
        fp << " xlow=\"" << node.xlow();
        fp << "\" ylow=\"" << node.ylow();
        fp << "\" xhigh=\"" << node.xhigh();
        fp << "\" yhigh=\"" << node.yhigh();
        if (node.type() == IPIN || node.type() == OPIN) {
            fp << "\" side=\"" << node.side_string();
        }
        fp << "\" ptc=\"" << node.ptc_num();
        fp << "\"/>" << endl;
        fp << "\t\t\t<timing R=\"" << setprecision(FLOAT_PRECISION) << node.R()
           << "\" C=\"" << setprecision(FLOAT_PRECISION) << node.C() << "\"/>" << endl;

        if (device_ctx.rr_indexed_data[node.cost_index()].seg_index != -1) {
            fp << "\t\t\t<segment segment_id=\"" << device_ctx.rr_indexed_data[node.cost_index()].seg_index << "\"/>" << endl;
        }

        const auto iter = device_ctx.rr_node_metadata.find(inode);
        if (iter != device_ctx.rr_node_metadata.end()) {
            const t_metadata_dict& meta = iter->second;
            add_metadata_to_xml(fp, "\t\t\t", meta);
        }

        fp << "\t\t</node>" << endl;
    }

    fp << "\t</rr_nodes>" << endl
       << endl;
}

/* Segment information in the t_segment_inf data structure is written out.
 * Information includes segment id, name, and optional timing parameters*/
void write_rr_segments(fstream& fp, const std::vector<t_segment_inf>& segment_inf) {
    fp << "\t<segments>" << endl;

    for (size_t iseg = 0; iseg < segment_inf.size(); iseg++) {
        fp << "\t\t<segment id=\"" << iseg << "\" name=\"" << segment_inf[iseg].name << "\">" << endl;
        fp << "\t\t\t<timing R_per_meter=\"" << setprecision(FLOAT_PRECISION) << segment_inf[iseg].Rmetal << "\" C_per_meter=\"" << setprecision(FLOAT_PRECISION) << segment_inf[iseg].Cmetal << "\"/>" << endl;
        fp << "\t\t</segment>" << endl;
    }
    fp << "\t</segments>" << endl
       << endl;
}

/* Switch info is written out into xml format. This includes
 * general, sizing, and optional timing information*/
void write_rr_switches(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<switches>" << endl;

    for (size_t iSwitch = 0; iSwitch < device_ctx.rr_switch_inf.size(); iSwitch++) {
        t_rr_switch_inf rr_switch = device_ctx.rr_switch_inf[iSwitch];

        fp << "\t\t<switch id=\"" << iSwitch;

        if (rr_switch.type() == SwitchType::TRISTATE) {
            fp << "\" type=\"tristate";
        } else if (rr_switch.type() == SwitchType::MUX) {
            fp << "\" type=\"mux";
        } else if (rr_switch.type() == SwitchType::PASS_GATE) {
            fp << "\" type=\"pass_gate";
        } else if (rr_switch.type() == SwitchType::SHORT) {
            fp << "\" type=\"short";
        } else if (rr_switch.type() == SwitchType::BUFFER) {
            fp << "\" type=\"buffer";
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_ROUTE, "Invalid switch type %d\n", rr_switch.type());
        }
        fp << "\"";

        if (rr_switch.name) {
            fp << " name=\"" << rr_switch.name << "\"";
        }
        fp << ">" << endl;

        fp << "\t\t\t<timing R=\"" << setprecision(FLOAT_PRECISION) << rr_switch.R << "\" Cin=\"" << setprecision(FLOAT_PRECISION) << rr_switch.Cin << "\" Cout=\"" << setprecision(FLOAT_PRECISION) << rr_switch.Cout << "\" Cinternal=\"" << setprecision(FLOAT_PRECISION) << rr_switch.Cinternal << "\" Tdel=\"" << setprecision(FLOAT_PRECISION) << rr_switch.Tdel << "\"/>" << endl;
        fp << "\t\t\t<sizing mux_trans_size=\"" << setprecision(FLOAT_PRECISION) << rr_switch.mux_trans_size << "\" buf_size=\"" << setprecision(FLOAT_PRECISION) << rr_switch.buf_size << "\"/>" << endl;
        fp << "\t\t</switch>" << endl;
    }
    fp << "\t</switches>" << endl
       << endl;
}

/* Block information is printed out in xml format. This includes general,
 * pin class, and pins */
void write_rr_block_types(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<block_types>" << endl;

    for (const auto& btype : device_ctx.physical_tile_types) {
        fp << "\t\t<block_type id=\"" << btype.index;

        VTR_ASSERT(btype.name);
        fp << "\" name=\"" << btype.name;

        fp << "\" width=\"" << btype.width << "\" height=\"" << btype.height << "\">" << endl;

        for (int iClass = 0; iClass < btype.num_class; iClass++) {
            auto& class_inf = btype.class_inf[iClass];

            char const* pin_type;
            switch (class_inf.type) {
                case -1:
                    pin_type = "OPEN";
                    break;
                case 0:
                    pin_type = "OUTPUT"; //driver
                    break;
                case 1:
                    pin_type = "INPUT"; //receiver
                    break;
                default:
                    pin_type = "NONE";
                    break;
            }

            //the pin list is printed out as the child values
            fp << "\t\t\t<pin_class type=\"" << pin_type << "\">\n";
            for (int iPin = 0; iPin < class_inf.num_pins; iPin++) {
                auto pin = class_inf.pinlist[iPin];
                fp << vtr::string_fmt("\t\t\t\t<pin ptc=\"%d\">%s</pin>\n",
                                      pin, block_type_pin_index_to_name(&btype, pin).c_str());
            }
            fp << "\t\t\t</pin_class>" << endl;
        }
        fp << "\t\t</block_type>" << endl;
    }
    fp << "\t</block_types>" << endl
       << endl;
}

/* Grid information is printed out in xml format. Each grid location
 * and its relevant information is included*/
void write_rr_grid(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();

    fp << "\t<grid>" << endl;

    for (size_t x = 0; x < device_ctx.grid.width(); x++) {
        for (size_t y = 0; y < device_ctx.grid.height(); y++) {
            t_grid_tile grid_tile = device_ctx.grid[x][y];

            fp << "\t\t<grid_loc x=\"" << x << "\" y=\"" << y << "\" block_type_id=\"" << grid_tile.type->index << "\" width_offset=\"" << grid_tile.width_offset << "\" height_offset=\"" << grid_tile.height_offset << "\"/>" << endl;
        }
    }
    fp << "\t</grid>" << endl
       << endl;
}

/* Edges connecting to each rr node is printed out. The two nodes
 * it connects to are also printed*/
void write_rr_edges(fstream& fp) {
    auto& device_ctx = g_vpr_ctx.device();
    fp << "\t<rr_edges>" << endl;

    for (size_t inode = 0; inode < device_ctx.rr_nodes.size(); inode++) {
        auto& node = device_ctx.rr_nodes[inode];
        for (t_edge_size iedge = 0; iedge < node.num_edges(); iedge++) {
            fp << "\t\t<edge src_node=\"" << inode << "\" sink_node=\"" << node.edge_sink_node(iedge) << "\" switch_id=\"" << node.edge_switch(iedge) << "\"";

            bool wrote_edge_metadata = false;
            const auto iter = device_ctx.rr_edge_metadata.find(std::make_tuple(inode, node.edge_sink_node(iedge), node.edge_switch(iedge)));
            if (iter != device_ctx.rr_edge_metadata.end()) {
                fp << ">" << endl;

                const t_metadata_dict& meta = iter->second;
                add_metadata_to_xml(fp, "\t\t\t", meta);
                wrote_edge_metadata = true;
            }

            if (wrote_edge_metadata == false) {
                fp << "/>" << endl;
            } else {
                fp << "\t\t</edge>" << endl;
            }
        }
    }
    fp << "\t</rr_edges>" << endl
       << endl;
}
