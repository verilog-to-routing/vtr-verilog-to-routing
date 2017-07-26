/*
The dump_rr_structs function can be used to write some of VPR's rr graph structures
to a file. These dumped structures can then be read into another program.

See comment of "dump_rr_structs" function for formate
*/


#include <fstream>
#include <iostream>
#include <string>

#include "vpr_error.h"

#include "dump_rr_structs.h"
#include "globals.h"

using namespace std;


/**** Function Declarations ****/
/* dumps all rr_node's to specified file */
static void dump_rr_nodes( fstream &file );
/* dumps all rr switches to specified file */
static void dump_rr_switches( fstream &file );
/* dumps all physical block types (CLB, memory, mult, etc) to the specified file */
static void dump_block_types( fstream &file );
/* dumps the grid structure which specifies which physical block type is at what coordinate */
static void dump_grid( fstream &file );
/* dumps the rr node indices which help look up which rr node is at which physical location */
static void dump_rr_node_indices( fstream &file );

/**** Function Definitions ****/

/* The main function for dumping rr structs to a specified file. The structures dumped are:
	- rr nodes (device_ctx.rr_nodes)
	- rr switches (device_ctx.rr_switch_inf)
	- the grid (device_ctx.grid)
	- physical block types (device_ctx.block_types)
	- node index lookups (rr_node_indices)
	
TODO: document the format for each section
	 */
void dump_rr_structs( const char *filename ){
	
	/* open the file for writing and discrad any current contents */
	fstream fid;
	fid.open( filename, ios::out | ios::trunc );
	if (!fid.is_open() || !fid.good()){
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__, "couldn't open file \"%s\" for dumping rr structs\n", filename);
	}

	/* dump rr nodes */
	cout << "Dumping rr nodes" << endl;
	dump_rr_nodes(fid);

	/* dump rr switches */
	cout << "Dumping rr switches" << endl;
	dump_rr_switches(fid);

	/* dump physical block types */
	cout << "Dumping physical block types" << endl;
	dump_block_types(fid);

	/* dump the grid structure */
	cout << "Dummping device_ctx.grid" << endl;
	dump_grid(fid);

	/* dump the rr node indices */
	cout << "Dumping rr node indices" << endl;
	dump_rr_node_indices(fid);

	fid.close(); 
}

/* dumps all routing resource nodes to specified file */
static void dump_rr_nodes( fstream &file ){
    auto& device_ctx = g_vpr_ctx.device();

	/* specify that we're in the rr node section and how many nodes there are */
	file << endl;
	file << ".rr_nodes(" << device_ctx.num_rr_nodes << ")" << endl;

	for (int inode = 0; inode < device_ctx.num_rr_nodes; inode++){
		const t_rr_node& node = device_ctx.rr_nodes[inode];

		/* a node's info is printed entirely on one line */
		file << " node_" << inode << ": ";
		file << "rr_type(" << node.type_string() << ") ";
		file << "xlow(" << node.xlow() << ") ";
		file << "xhigh(" << node.xhigh() << ") ";
		file << "ylow(" << node.ylow() << ") ";
		file << "yhigh(" << node.yhigh() << ") ";
		file << "ptc_num(" << node.ptc_num() << ") ";
		file << "fan_in(" << node.fan_in() << ") ";
		file << "direction(" << (int)node.direction() << ") ";
		file << "R(" << node.R() << ") ";
		file << "C(" << node.C() << ") ";

		/* print edges and switches */
		file << endl << "  .edges(" << node.num_edges() << ")" << endl;	//specify how many edges/switches there are
		for (int iedge = 0; iedge < node.num_edges(); iedge++){
			file << "   " << iedge << ": "; 
			file << "edge(" << node.edge_sink_node(iedge) << ") ";
			file << "switch(" << node.edge_switch(iedge) << ")" << endl;
		}
		file << "  .end edges" << endl;
	}
	file << ".end rr_nodes" << endl;
}

/* dumps all rr switches to specified file */
static void dump_rr_switches( fstream &file ){
    auto& device_ctx = g_vpr_ctx.device();

	/* specify that we're in the rr switch section and how many switches there are */
	file << endl;
	file << ".rr_switch(" << device_ctx.num_rr_switches << ")" << endl;

	for (int iswitch = 0; iswitch < device_ctx.num_rr_switches; iswitch++){
		t_rr_switch_inf rr_switch = device_ctx.rr_switch_inf[iswitch];
		
		file << " switch_" << iswitch << ": ";
		file << "buffered(" << (int)rr_switch.buffered << ") ";
		file << "R(" << rr_switch.R << ") ";
		file << "Cin(" << rr_switch.Cin << ") ";
		file << "Cout(" << rr_switch.Cout << ") ";
		file << "Tdel(" << rr_switch.Tdel << ") ";
		file << "mux_trans_size(" << rr_switch.mux_trans_size << ") ";
		file << "buf_size(" << rr_switch.buf_size << ") ";

		file << endl;
	}
	file << ".end rr_switch" << endl;
}

/* dumps all physical block types (CLB, memory, mult, etc) to the specified file */
static void dump_block_types( fstream &file ){
    auto& device_ctx = g_vpr_ctx.device();

	/* specify that we're in the physical block type section, and how many types there are */
	file << endl;
	file << ".block_type(" << device_ctx.num_block_types << ")" << endl;

	for (int itype = 0; itype < device_ctx.num_block_types; itype++){
		t_type_descriptor btype = device_ctx.block_types[itype];

		file << " type_" << itype << ": ";
		file << "name(" << btype.name << ") ";
		file << "num_pins(" << btype.num_pins << ") ";
		file << "width(" << btype.width << ") ";
		file << "height(" << btype.height << ") ";

		/* skipping pinloc and all that stuff */
	
		int num_class = btype.num_class;
		file << "num_class(" << num_class << ") ";

		/* skipping Fc information -- this stuff is part of the rr graph already */

		/* skipping clustering information */

		/* skipping grid location information */

		file << "num_drivers(" << btype.num_drivers << ") ";
		file << "num_receivers(" << btype.num_receivers << ") ";
		file << "index(" << btype.index << ") ";


		/**** Now print all arrays ****/

		/* print class info */
		file << endl;
		file << "  .classes(" << num_class << ")" << endl;
		for (int iclass = 0; iclass < num_class; iclass++){
			t_class class_inf = btype.class_inf[iclass];
			file << "   " << iclass << ": " << "pin_type(" << class_inf.type << ") ";
			file << "num_pins(" << class_inf.num_pins << ") ";
			
			/* print class pin list */
			file << endl << "    .pinlist(" << class_inf.num_pins << ")" << endl;
			for (int ipin = 0; ipin < class_inf.num_pins; ipin++){
				file << "     " << ipin << ": " << class_inf.pinlist[ipin] << endl;
			}
			file << "    .end pinlist" << endl;
		}
		file << "  .end classes" << endl;
		
		/* print which pins belong to which class */
		file << "  .pin_class(" << btype.num_pins << ")" << endl;
		for (int ipin = 0; ipin < btype.num_pins; ipin++){
			file << "   " << ipin << ": " << btype.pin_class[ipin] << endl;
		}
		file << "  .end pin_class" << endl;

		/* print which pins are global */
		file << "  .is_global_pin(" << btype.num_pins << ")" << endl;
		for (int ipin = 0; ipin < btype.num_pins; ipin++){
			file << "   " << ipin << ": " << btype.is_global_pin[ipin] << endl;
		}
		file << "  .end is_global_pin" << endl;		
	}
	file << ".end block_type" << endl;
}


/* dumps the grid structure which specifies which physical block type is at what coordinate */
static void dump_grid( fstream &file ){
    auto& device_ctx = g_vpr_ctx.device();

	/* specify that we're in the grid section and how many grid elements there are */
	file << endl;
	file << ".grid(" << (device_ctx.nx+2) << ", " << (device_ctx.ny+2) << ")" << endl;

	for (int ix = 0; ix <= device_ctx.nx+1; ix++){
		for (int iy = 0; iy <= device_ctx.ny+1; iy++){
			t_grid_tile grid_tile = device_ctx.grid[ix][iy];
	
			file << " grid_x" << ix << "_y" << iy << ": ";
			file << "block_type_index(" << grid_tile.type->index << ") ";
			file << "width_offset(" << grid_tile.width_offset << ") ";
			file << "height_offset(" << grid_tile.height_offset << ") ";

			/* skipping usage and atom blocks */

			file << endl;
		}
	}

	file << ".end grid" << endl;
}

/* dumps the rr node indices which help look up which rr node is at which physical location */
static void dump_rr_node_indices( fstream &file ){
    auto& device_ctx = g_vpr_ctx.device();

	file << endl;
	/* rr_node_indices are [0..NUM_RR_TYPES-1][0..device_ctx.nx+2][0..device_ctx.ny+2]. each entry then contains a vtr::t_ivec with nelem entries */
	file << ".rr_node_indices(" << NUM_RR_TYPES-1 << ", " << device_ctx.nx+2 << ", " << device_ctx.ny+2 << ")" << endl;

	/* note that the rr_node_indices structure uses the chan/seg convention. in terms of coordinates, this affects CHANX nodes
	   in which case chan=y and seg=x, whereas for all other nodes chan=x and seg=y. i'm not sure how non-square FPGAs are handled in that case... */
	
	for (int itype = 0; itype < NUM_RR_TYPES; itype++){
		if (device_ctx.rr_node_indices.empty() || device_ctx.rr_node_indices[itype].empty()) {
			/* skip if not allocated */
			continue;
		}
		for (int ix = 0; ix < device_ctx.nx+2; ix++){
			for (int iy = 0; iy < device_ctx.ny+2; iy++){
				t_rr_type rr_type = (t_rr_type)itype;

				/* because indexing into this structure uses the chan/seg convention, we need to swap the x and y values
				   in the case of CHANX nodes */
				int x = ix;
				int y = iy;
				if (rr_type == CHANX){
					x = iy;
					y = ix;
				}

				if (device_ctx.rr_node_indices[rr_type][ix].empty()){
					/* skip if not allocated */
					continue;
				}

				std::vector<int> vec = device_ctx.rr_node_indices[rr_type][ix][iy];

				if (vec.empty()){
					/* skip if vector not allocated */
					continue;
				}

				/* Note: there's a peculiarity with the ptc_num of ipins/opins in relation to the rr_node_indices.
				   The ptc num of the ipin/opin is the index of the pin within its block type, regardless of whether its 
				   t_rr_type is OPIN or IPIN. So in rr_node_indices the info contained in the IPIN and OPIN sections
				   is duplicate of each other (IPIN section has same info as OPIN section) -- in rr_node_indices the 
				   the lookup of a pin's corresponding node can be done through either IPIN or OPIN sections. */
				file << " rr_node_index_type" << itype << "_x" << x << "_y" << y;

				/* print the node corresponding to each ptc index at this type/location */
				file << endl;
				file << "  .nodes(" << vec.size() << ")" << endl;
				for (unsigned inode = 0; inode < vec.size(); inode++){
					file << "   " << inode << ": " << vec[inode] << endl;
				}
				file << "  .end nodes" << endl;
			}
		}
	}

	file << ".end rr_node_indices" << endl;
}

