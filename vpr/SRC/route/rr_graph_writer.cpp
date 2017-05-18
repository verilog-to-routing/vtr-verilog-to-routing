/* This file defines the writing rr graph function in XML format */


#include <fstream>
#include <iostream>
#include <string.h>

#include "vpr_error.h"
#include "globals.h"
#include "read_xml_arch_file.h"
#include "vtr_version.h"
#include "rr_graph_writer.h"

using namespace std;


void write_rr_node(fstream &fp);
void write_rr_switches(fstream &fp);
void write_rr_grid(fstream &fp);
void write_rr_edges(fstream &fp);
void write_rr_block_types(fstream &fp);
void write_rr_segments(fstream &fp, const t_segment_inf *segment_inf, const int num_seg_types);

void write_rr_graph(const char *file_name, const t_segment_inf *segment_inf, const int num_seg_types){
	fstream fp;
	fp.open(file_name, fstream::out | fstream::trunc);
	
	if (!fp.is_open() || !fp.good()){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, 
		"couldn't open file \"%s\" for generating RR graph file\n", file_name);
	}
	
	fp << "<rr_graph tool_name=\"vpr\" tool_version=\"" << vtr::VERSION << 
		"\" tool_comment=\"Generated from arch file " 
		<< get_arch_file_name() <<"\">"<<endl;

	write_rr_switches(fp);
	write_rr_segments(fp, segment_inf, num_seg_types);
	write_rr_block_types(fp);
	write_rr_grid(fp);
	write_rr_node(fp);
	write_rr_edges(fp);
	fp<< "</rr_graph>";
	
	fp.close();
	
	cout << "Finished generating RR graph file named RR_GRAPH.xml" <<endl<<endl;

}

void write_rr_node(fstream &fp){	
	cout << "Writing RR nodes" <<endl;
	auto& device_ctx = g_vpr_ctx.device();
	
	fp << "\t<rr_nodes>"<< endl;

	for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++){
		t_rr_node& node = device_ctx.rr_nodes[inode];
		fp << "\t\t<node id=\""<< inode << "\" type=\"" << node.type_string()<<
			"\" direction=\"" << node.direction_string()<<
			"\" capacity=\""<< node.capacity()<< "\">" << endl;
		fp << "\t\t\t<loc xlow=\""<< node.xlow() << "\" ylow=\"" << node.ylow()<<
			"\" xhigh=\"" << node.xhigh()<<"\" yhigh=\""<< node.yhigh()<< 
			"\" ptc=\"" << node.ptc_num()<<"\"/>" << endl;
		fp << "\t\t\t<timing R=\""<< node.R() << "\" C=\"" << node.C()<< "\"/>" << endl;
		
		if (device_ctx.rr_indexed_data[node.cost_index()].seg_index!= -1){
			
		fp << "\t\t\t<segment segment_id=\""<< 
			device_ctx.rr_indexed_data[node.cost_index()].seg_index << "\"/>" << endl;
		}
		
		fp << "\t\t</node>" <<endl;
	}

	fp << "\t</rr_nodes>"<< endl<<endl;

}

void write_rr_segments(fstream &fp, const t_segment_inf *segment_inf, const int num_seg_types){
	
	cout << "Writing RR segments" <<endl;
	fp << "\t<segments>"<< endl;

	for (int segNum = 0; segNum < num_seg_types; segNum++){
		fp << "\t\t<segment id=\""<< segNum << 
			"\" name=\"" << segment_inf[segNum].name<<"\"/>" << endl;
		fp << "\t\t\t<timing R_per_meter=\""<< segment_inf[segNum].Rmetal << 
			"\" C_per_meter=\"" << segment_inf[segNum].Cmetal <<"\"/>" << endl;
	}


	fp << "\t</segments>"<< endl<<endl;
}

void write_rr_switches(fstream &fp){
	auto& device_ctx = g_vpr_ctx.device();
	cout << "Writing RR switches" <<endl;
	fp << "\t<switches>"<< endl;

	for (int switchNum=0; switchNum<device_ctx.num_rr_switches; switchNum++){
		s_rr_switch_inf rr_switch = device_ctx.rr_switch_inf[switchNum];

		fp << "\t\t<switch id=\""<< switchNum;
		if (rr_switch.name){
			fp << "\" name=\"" << rr_switch.name;
		}
			fp << "\" buffered=\"" << (int)rr_switch.buffered<<"\">" << endl;
		fp << "\t\t\t<timing R=\""<< rr_switch.R << "\" Cin=\"" << rr_switch.Cin<<
			"\" Cout=\"" << rr_switch.Cout<<
			"\" Tdel=\""<< rr_switch.Tdel<<"\"/>" << endl;
		fp << "\t\t\t<sizing mux_trans_size=\""<<rr_switch.mux_trans_size << 
			"\" buf_size=\"" << rr_switch.buf_size<<"\"/>" << endl;
		fp << "\t\t</switch>" <<endl;
	}


	fp << "\t</switches>"<< endl<<endl;

}

void write_rr_block_types(fstream &fp){
	auto& device_ctx = g_vpr_ctx.device();

	cout << "Writing RR block types" <<endl;
	fp << "\t<block_types>"<< endl;

	for (int blockType = 0; blockType < device_ctx.num_block_types; blockType++){
		s_type_descriptor btype = device_ctx.block_types[blockType];

		fp << "\t\t<block_type id=\""<< btype.index;

		if (btype.name && strcmp(btype.name, "<EMPTY>")!=0){
			fp << "\" name=\"" << btype.name;
		}

		fp <<"\" width=\"" << btype.width<<"\" height=\"" << btype.height<< "\">" << endl;

		for (int classNum = 0; classNum< btype.num_class; classNum++){
			s_class class_inf = btype.class_inf[classNum];
			
			char const* pin_type;
			switch (class_inf.type){
				case -1:
					pin_type = "OPEN";
					break;
				case 0:
					pin_type = "OUTPUT";//driver
					break;
				case 1:
					pin_type = "INPUT"; //receiver
					break;
				default:
					break;
			}
			
			fp << "\t\t\t<pin_class type=\""<<pin_type << "\">" << endl;
			fp << "\t\t\t\t";
			for (int pinNum = 0; pinNum < class_inf.num_pins; pinNum++){
				fp << class_inf.pinlist[pinNum]<< " ";
			}
			fp << endl << "\t\t\t</pin_class>"<<endl;
		}
		fp<<"\t\t</block_type>"<<endl;
		
	}
	fp << "\t</block_types>"<< endl<<endl;
}

void write_rr_grid(fstream &fp){
	auto& device_ctx = g_vpr_ctx.device();
	cout << "Writing RR grids" <<endl;

	fp << "\t<grid>"<< endl;

	for (int x = 0; x < device_ctx.nx+1; x++){
		for (int y = 0; y < device_ctx.ny+1;y++){
			t_grid_tile grid_tile = device_ctx.grid[x][y];

			fp << "\t\t<grid_loc x=\""<< x << "\" y=\"" << y<<
				"\" block_type_id=\"" << grid_tile.type->index<<
				"\" width_offset=\""<< grid_tile.width_offset<<
				"\" height_offset=\""<< grid_tile.height_offset<< "\"/>" << endl;
		}
	}
	fp << "\t</grid>"<< endl<<endl;
}

void write_rr_edges(fstream &fp){
	auto& device_ctx = g_vpr_ctx.device();

	cout << "Writing RR edges" <<endl;
	fp << "\t<rr_edges>"<< endl;

	for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++){
		t_rr_node& node = device_ctx.rr_nodes[inode];
		for (int iedge = 0; iedge < node.num_edges(); iedge++){
			fp << "\t\t<edge src_node=\""<< iedge << 
				"\" sink_node=\"" << node.edge_sink_node(iedge)<<
				"\" switch_id=\"" << node.edge_switch(iedge)<<"\"/>" << endl;
		}
	}
	fp << "\t</rr_edges>"<< endl<<endl;
}






