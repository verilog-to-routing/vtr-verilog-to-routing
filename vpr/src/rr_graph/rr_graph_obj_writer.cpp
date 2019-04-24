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
#include "rr_graph_obj.h"
#include "rr_graph_obj_writer.h"

using namespace std;

/* All values are printed with this precision value. The higher the
 * value, the more accurate the read in rr graph is. Using numeric_limits
 * max_digits10 guarentees that no values change during a sequence of
 * float -> string -> float conversions */
constexpr int FLOAT_PRECISION = std::numeric_limits<float>::max_digits10;

/*********************** External Subroutines to this module *******************/
void write_rr_channel(fstream &fp);
void write_rr_grid(fstream &fp);
void write_rr_block_types(fstream &fp);

/*********************** Subroutines local to this module *******************/
void write_rr_graph_node(fstream &fp, const RRGraph* rr_graph);
void write_rr_graph_switches(fstream &fp, const RRGraph* rr_graph);
void write_rr_graph_edges(fstream &fp, const RRGraph* rr_graph);
void write_rr_graph_segments(fstream &fp, const RRGraph* rr_graph);

/************************ Subroutine definitions ****************************/

/* This function is used to write the rr_graph into xml format into a a file with name: file_name */
void write_rr_graph_obj_to_xml(const char *file_name, const RRGraph* rr_graph) {
    fstream fp;
    fp.open(file_name, fstream::out | fstream::trunc);

    /* Prints out general info for easy error checking*/
    if (!fp.is_open() || !fp.good()) {
        vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
                "couldn't open file \"%s\" for generating RR graph file\n", file_name);
    }
    cout << "Writing RR graph" << endl;
    fp << "<rr_graph tool_name=\"vpr\" tool_version=\"" << vtr::VERSION <<
            "\" tool_comment=\"Generated from arch file "
            << get_arch_file_name() << "\">" << endl;

    /* Write out each individual component
     * Use existing write_rr_* functions as much as possible
     * 1. For those using external data structures outside RRGraph, 
     *    leave as it is.
     * 2. For those using RRGraph internal data, 
     *    write new functions 
     */
    write_rr_channel(fp);
    write_rr_graph_switches(fp, rr_graph);
    write_rr_graph_segments(fp, rr_graph);
    write_rr_block_types(fp);
    write_rr_grid(fp);
    write_rr_graph_node(fp, rr_graph);
    write_rr_graph_edges(fp, rr_graph);
    fp << "</rr_graph>";

    fp.close();

    cout << "Finished generating RR graph file named " << file_name << endl << endl;
}

static void add_metadata_to_xml(fstream &fp, const char *tab_prefix, const t_metadata_dict & meta) {
    fp << tab_prefix << "<metadata>" << endl;

    for(const auto &meta_elem : meta) {
        const std::string & key = meta_elem.first;
        const std::vector<t_metadata_value> & values = meta_elem.second;
        for(const auto &value : values) {
          fp << tab_prefix << "\t<meta name=\"" << key << "\"";
          fp << ">" << value.as_string() << "</meta>" << endl;
        }
    }
    fp << tab_prefix << "</metadata>" << endl;
}


/* All relevant rr node info is written out to the graph.
 * This includes location, timing, and segment info*/
void write_rr_graph_node(fstream &fp, const RRGraph* rr_graph) {
  /* TODO: we should make it function full independent from device_ctx !!! */
  auto& device_ctx = g_vpr_ctx.device();

  fp << "\t<rr_nodes>" << endl;

  for (auto node : rr_graph->nodes()) {
    fp << "\t\t<node";
    fp << " id=\"" << rr_graph->node_index(node);
    fp << "\" type=\"" <<  rr_graph->node_type_string(node);
    if (CHANX == rr_graph->node_type(node) || CHANY == rr_graph->node_type(node)) {
      fp << "\" direction=\"" << rr_graph->node_direction_string(node);
    }
    fp << "\" capacity=\"" << rr_graph->node_capacity(node);
    fp << "\">" << endl;
    fp << "\t\t\t<loc";
    fp << " xlow=\"" << rr_graph->node_xlow(node);
    fp << "\" ylow=\"" << rr_graph->node_ylow(node);
    fp << "\" xhigh=\"" << rr_graph->node_xhigh(node);
    fp << "\" yhigh=\"" << rr_graph->node_yhigh(node);
    if (IPIN == rr_graph->node_type(node) || OPIN == rr_graph->node_type(node)) {
      fp << "\" side=\"" << rr_graph->node_side_string(node) ;
    }
    fp << "\" ptc=\"" << rr_graph->node_ptc_num(node) ;
    fp << "\"/>" << endl;
    fp << "\t\t\t<timing R=\"" << setprecision(FLOAT_PRECISION) << rr_graph->node_R(node)
            << "\" C=\"" << setprecision(FLOAT_PRECISION) << rr_graph->node_C(node) << "\"/>" << endl;

    if (-1 != rr_graph->node_segment_id(node)) {
      fp << "\t\t\t<segment segment_id=\"" << rr_graph->node_segment_id(node) << "\"/>" << endl;
    }

    const auto iter = device_ctx.rr_node_metadata.find(rr_graph->node_index(node));
    if(iter != device_ctx.rr_node_metadata.end()) {
      const t_metadata_dict & meta = iter->second;
      add_metadata_to_xml(fp, "\t\t\t", meta);
    }

    fp << "\t\t</node>" << endl;
  }

  fp << "\t</rr_nodes>" << endl << endl;

  return;
}

/* Segment information in the t_segment_inf data structure is written out.
 * Information includes segment id, name, and optional timing parameters*/
void write_rr_graph_segments(fstream &fp, const RRGraph* rr_graph) {
  fp << "\t<segments>" << endl;

  for (auto seg : rr_graph->segments()) {
    fp << "\t\t<segment id=\"" << rr_graph->segment_index(seg) <<
          "\" name=\"" << rr_graph->get_segment(seg).name << "\">" << endl;
    fp << "\t\t\t<timing R_per_meter=\"" << setprecision(FLOAT_PRECISION) << rr_graph->get_segment(seg).Rmetal <<
          "\" C_per_meter=\"" <<setprecision(FLOAT_PRECISION) << rr_graph->get_segment(seg).Cmetal << "\"/>" << endl;
    fp << "\t\t</segment>" << endl;
  }
  fp << "\t</segments>" << endl << endl;

  return;
}

/* Switch info is written out into xml format. This includes
 * general, sizing, and optional timing information*/
void write_rr_graph_switches(fstream &fp, const RRGraph* rr_graph) {
  fp << "\t<switches>" << endl;

  for (auto rr_switch : rr_graph->switches()) {
    fp << "\t\t<switch id=\"" << rr_graph->switch_index(rr_switch);
    t_rr_switch_inf cur_switch = rr_graph->get_switch(rr_switch);

    if (cur_switch.type() == SwitchType::TRISTATE) {
      fp << "\" type=\"tristate";
    } else if (cur_switch.type() == SwitchType::MUX) {
      fp << "\" type=\"mux";
    } else if (cur_switch.type() == SwitchType::PASS_GATE) {
      fp << "\" type=\"pass_gate";
    } else if (cur_switch.type() == SwitchType::SHORT) {
      fp << "\" type=\"short";
    } else if (cur_switch.type() == SwitchType::BUFFER) {
      fp << "\" type=\"buffer";
    } else {
      VPR_THROW(VPR_ERROR_ROUTE, "Invalid switch type %d\n", cur_switch.type());
    }
    fp << "\"";

    if (cur_switch.name) {
      fp << " name=\"" << cur_switch.name << "\"";
    }
    fp << ">" << endl;

    fp << "\t\t\t<timing R=\"" << setprecision(FLOAT_PRECISION) << cur_switch.R <<
          "\" Cin=\"" << setprecision(FLOAT_PRECISION) << cur_switch.Cin <<
          "\" Cout=\"" << setprecision(FLOAT_PRECISION) << cur_switch.Cout <<
          "\" Tdel=\"" << setprecision(FLOAT_PRECISION) << cur_switch.Tdel << "\"/>" << endl;
    fp << "\t\t\t<sizing mux_trans_size=\"" << setprecision(FLOAT_PRECISION) << cur_switch.mux_trans_size <<
          "\" buf_size=\"" << setprecision(FLOAT_PRECISION) << cur_switch.buf_size << "\"/>" << endl;
    fp << "\t\t</switch>" << endl;
  }
  fp << "\t</switches>" << endl << endl;

  return;
}

/* Edges connecting to each rr node is printed out. The two nodes
 * it connects to are also printed*/
void write_rr_graph_edges(fstream &fp, const RRGraph* rr_graph) {
  auto& device_ctx = g_vpr_ctx.device();
  fp << "\t<rr_edges>" << endl;

  for (auto node : rr_graph->nodes()) {
    for (auto edge: rr_graph->node_out_edges(node)) {
      fp << "\t\t<edge src_node=\"" << rr_graph->node_index(node) <<
            "\" sink_node=\"" << rr_graph->node_index(rr_graph->edge_sink_node(edge)) <<
            "\" switch_id=\"" << rr_graph->switch_index(rr_graph->edge_switch(edge)) << "\"";

      bool wrote_edge_metadata = false;
      const auto iter = device_ctx.rr_edge_metadata.find( std::make_tuple(rr_graph->node_index(node), 
                                                          rr_graph->node_index(rr_graph->edge_sink_node(edge)),
                                                          rr_graph->switch_index(rr_graph->edge_switch(edge))) );
      if(iter != device_ctx.rr_edge_metadata.end()) {
        fp << ">" << endl;

        const t_metadata_dict & meta = iter->second;
        add_metadata_to_xml(fp, "\t\t\t", meta);
        wrote_edge_metadata = true;
      }

      if(wrote_edge_metadata == false) {
        fp << "/>" << endl;
      } else {
        fp << "\t\t</edge>" << endl;
      }
    }
  }
  fp << "\t</rr_edges>" << endl << endl;
}
