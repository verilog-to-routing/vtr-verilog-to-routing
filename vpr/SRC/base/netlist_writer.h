#ifndef NETLIST_WRITER_H
#define NETLIST_WRITER_H

//Writes out the post-synthesis implementation netlists in BLIF and Verilog formats,
//along with an SDF with delay annotations.
//
//All written filenames end in {basename}_post_synthesis.{fmt} where {basename} is the 
//basename argument and {fmt} is the file format (e.g. v, blif, sdf)
void netlist_writer(const std::string base_name);

#endif
