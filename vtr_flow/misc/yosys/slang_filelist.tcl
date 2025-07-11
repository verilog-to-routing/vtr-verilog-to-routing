#Parameters:
#
#	circuit_list - list of circuits passed into the flow
#	file_list - text file being written to that will contain
#	     		the names of circuits from circuit list.
#
#Function:
#
#	Validates file extensions of input files and writes the names
#	of input files to the file list to be read by yosys-slang.
 
proc build_filelist { circuit_list file_list } {
	set fh [open $file_list "w"]
	foreach f $circuit_list {
		set ext [string tolower [file extension $f]]
		if {$ext == ".sv" || $ext == ".svh" || $ext == ".v" || $ext == ".vh"} {
			puts $fh $f
		} else {
			close $fh
			error "Unsupported file type. Yosys-Slang accepts .sv .svh .v .vh. File {$f}"
		}
	}
	close $fh
}
