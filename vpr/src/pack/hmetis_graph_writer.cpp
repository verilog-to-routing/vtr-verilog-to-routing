/* This file contains the functions to write the AtomNetlist into a hypergraph
* (format: http://glaros.dtc.umn.edu/gkhome/fetch/sw/hmetis/manual.pdf) for hMetis to partition.
*
* Brief summary of formatting
* ===========================
* Hypergraph H = (V, Eh) with V vertices and Eh hyperedges stored in plain text file.
* Contains |Eh|+1 lines if vertices weightless, and |Eh|+|V|+1 lines if weighted.
* Lines starting with '%' are comments.
*
* First line contains 2 or 3 integers:
*	(1) |Eh| = # of hyperedges
*	(2) |V| = # of vertices
*	(3) fmt = format (can be omitted, 1, 10, or 11, further explained below)
*
* The next |Eh| lines represent vertices contained in each hyper edge.
* Weights for hyperedge Eh_i are represented with an extra # at the end of each line.
*
* There are another |V| lines if vertices are weighted.
* Weights are a single #, for each vertex V_j.
*
* Example
* =======
*		|Eh| = 4, |V| = 7, fmt = omitted
*
*		Eh	| V - Connected cliques
*		----------------------------
*		1	| 1, 2
*		2	| 1, 7, 5, 6
*		3	| 5, 6, 4
*		4	| 2, 3, 4
*
* This example translates to the following input hypergraph file:
*		4 7
*		1 2
*		1 7 5 6
*		5 6 4
*		2 3 4
*
* IMPORTANT NOTE
* ==============
* Hyperedges and vertices for input to hMetis start from 1 -> num_edges/vertices, not from 0
*
*/

#include <fstream>
#include <iostream>
#include <string>
#include "vpr_error.h"
#include "globals.h"
#include "hmetis_graph_writer.h"

using namespace std;


/* Local subroutines */
void write_unweighted_edges(fstream &fp);
void write_weighted_edges(fstream &fp);
void write_weighted_vertices(fstream &fp);


/* Main function in writing the hMetis hypergraph
*	file_name: name of the file to write out to
*/
void write_hmetis_graph(std::string &file_name) {
	fstream fp;
	fp.open(file_name, fstream::out | fstream::trunc);

	/* Prints out general info for easy error checking*/
	if (!fp.is_open() || !fp.good()) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
			"couldn't open file \"%s\" for generating hypergraph file\n", file_name.c_str());
	}
	cout << "Writing hMetis hypergraph to " << file_name << endl;

	auto& atom_ctx = g_vpr_ctx.atom();

	/* Possible fmt values
	*		omitted - unweighted graph
	*		1		- weighted Eh
	*		10		- weighted V
	*		11		- weighted Eh & V
	*/
	string fmt; //TODO: set fmt accordingly (may be passed in as an argument)

	//First line - write # of hyperedges, then # of vertices, (optional) then fmt
	fp << size_t(atom_ctx.nlist.nets().size()) << " " << size_t(atom_ctx.nlist.blocks().size()) << " " << fmt;

	// Unweighted
	if (fmt.empty()) {
		write_unweighted_edges(fp);
	}

	// Weighted Eh
	else if (fmt == "1") {
		write_weighted_edges(fp);
	}

	// Weighted V
	else if (fmt == "10") {
		write_weighted_vertices(fp);
	}

	// Weighted Eh & V
	else {
		if (fmt != "11") {
			vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"fmt %s is not (empty, 1, 10, 11)", fmt.c_str());
		}
		write_weighted_edges(fp);
		write_weighted_vertices(fp);
	}

	fp.close();

	cout << "Finished writing hMetis hypergraph to " << file_name << endl << endl;

}

/* Function to write if the hypergraph has unweighted edges
*  The ith line written has the vertices connected by Eh_i
*/
void write_unweighted_edges(fstream &fp) {
	auto& atom_ctx = g_vpr_ctx.atom();

	// For each net, write the blocks connected to that net
	for (auto net_id : atom_ctx.nlist.nets()) {
		fp << endl;
		for (auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
			//TODO: Put this into a map to check for duplicated blocks
			//i.e. Net connects to more than one pin of the same block
			fp << size_t(atom_ctx.nlist.pin_block(pin_id)) + 1 << " ";
		}
	}
}


/* Function to write if the hypergraph has weighted edges */
void write_weighted_edges(fstream &fp) {
	auto& atom_ctx = g_vpr_ctx.atom();

	// For each net, write all the blocks connected to that net
	for (auto net_id : atom_ctx.nlist.nets()) {
		int net_weight = 0;	//TODO: Find a good metric for weight of the net

		fp << endl;
		for (auto pin_id : atom_ctx.nlist.net_pins(net_id)) {
			fp << size_t(atom_ctx.nlist.pin_block(pin_id)) + 1 << " ";
		}
		// After all blocks are written, write the weight of the net
		fp << net_weight;;
	}
}


/* Function to write if the hypergraph has weighted vertices */
void write_weighted_vertices(fstream &fp) {
	auto& atom_ctx = g_vpr_ctx.atom();

	// For each block (vertex), write the weight on a separate line
//	for (auto block_id : atom_ctx.nlist.blocks()) {
	for (unsigned int i = 0; i < atom_ctx.nlist.blocks().size(); i++) {
		int block_weight = 0; //TODO: Find a good metric for weight of the pin
		fp << endl << block_weight;
	}
}
