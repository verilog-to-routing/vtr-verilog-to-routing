# Helper file for synthesis.tcl
# 
# Includes a function that builds a filelist of
# HDL files provided by the circuit list to be
# read by read_slang. This function also identifies
# the respective top modules of HDL files in the filelist.


namespace eval ::slang {


	variable top_mods_slang {}
	variable top_mods_verilog {}

# Function - build_filelist:
#
#	Validates file extensions of input files and writes the names
#	of input files to the file list to be read by yosys-slang. Also appends
#	the respective top modules of HDL files being read by slang to the read_slang command.
#
# Parameters:
#
#	circuit_list - list of circuits passed into the flow
#	file_list - text file being written to that will contain
#	     		the names of circuits from circuit list.
#
	proc build_filelist { circuit_list file_list tops_path} {
		variable top_mods_slang
		variable top_mods_verilog
		variable top_map
		variable includes_map
		source [file join [pwd] "$tops_path"]
		set top_mods_slang {}
		set top_mods_verilog {}
		set fh [open $file_list "w"]
		foreach f $circuit_list {
			set ext [string tolower [file extension $f]]
			if {$ext == ".sv" || $ext == ".svh" || $ext == ".v" || $ext == ".vh"} {
				if {$f != "vtr_primitives.v" && $f != "vtr_blackboxes.v"} {
					#Includes file
					if {[info exists includes_map($f)]} {
						puts $fh $f	
					#HDL file or top module isn't in top_map
					} elseif {![info exists top_map($f)]} {
						error "No top module set for $f"
					#HDL file and respective top module in top_map
					} else {
						puts $fh $f
						set top_name $top_map($f)
						lappend top_mods_slang --top $top_name
						lappend top_mods_verilog -top $top_name
					}
				}
			} else {
				close $fh
				error "Unsupported file type. Yosys-Slang accepts .sv .svh .v .vh. File {$f}"
			}
		}
		close $fh
		return [list $top_mods_slang $top_mods_verilog]
	}
}
