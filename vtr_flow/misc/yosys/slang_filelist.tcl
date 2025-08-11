# Helper file for synthesis.tcl
# 
# Includes a function that builds a filelist of
# HDL files provided by the circuit list to be
# read by read_slang. This function also identifies
# the respective top modules of HDL files in the filelist.


namespace eval ::slang {
	# INFO: Maps HDL files to respective top module(s) 
	# top_map insertion syntax:
	# <file> <top module>
	array set top_map {
		and_latch.v and_latch
		multiclock_output_and_latch.v multiclock_output_and_latch
		multiclock_reader_writer.v multiclock_reader_writer
		multiclock_separate_and_latch.v multiclock_separate_and_latch
		arm_core.v arm_core
		bgm.v bgm
		blob_merge.v RLE_BlobMerging
		boundtop.v paj_boundtop_hierarchy_no_mem
		ch_intrinsics.v memset
		diffeq1.v diffeq_paj_convert
		diffeq2.v diffeq_f_systemC
		LU8PEEng.v LU8PEEng
		LU32PEEng.v LU32PEEng
		LU64PEEng.v LU64PEEng
		mcml.v mcml
		mkDelayWorker32B.v mkDelayWorker32B
		mkPktMerge.v mkPktMerge
		mkSMAdapter4B.v mkSMAdapter4B
		or1200.v or1200_flat
		raygentop.v paj_raygentop_hierarchy_no_mem
		sha.v sha1
		stereovision0.v sv_chip0_hierarchy_no_mem
		stereovision1.v sv_chip1_hierarchy_no_mem
		stereovision2.v sv_chip2_hierarchy_no_mem
		stereovision3.v sv_chip3_hierarchy_no_mem
		button_controller.sv top
		display_control.sv top
		debounce.sv top
		timer.sv top
		deepfreeze.style1.sv top
		pulse_led.v top
		clock.sv top
		single_ff.v top
		single_wire.v top
		PWM.v top
		flattened_pulse_width_led.sv top
		modify_count.sv top
		time_counter.sv top
		spree.v system
		attention_layer.v attention_layer
		bnn.v bnn
		tpu_like.small.os.v top
		tpu_like.small.ws.v top
		dla_like.small.v DLA
		conv_layer_hls.v top
		conv_layer.v conv_layer
		eltwise_layer.v eltwise_layer
		robot_rl.v robot_maze
		reduction_layer.v reduction_layer
		spmv.v spmv
		softmax.v softmax
	}
	# INFO: List of HDL includes files 
	# includes_map insertion syntax:
	# <file> include
	array set includes_map {
		hard_block_include.v include
	}


	variable top_args {}

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
	proc build_filelist { circuit_list file_list } {
		variable top_args
		variable top_map
		variable includes_map
		set top_args {}
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
						lappend top_args --top $top_name
					}
				}

			} else {
				close $fh
				error "Unsupported file type. Yosys-Slang accepts .sv .svh .v .vh. File {$f}"
			}
		}
		close $fh
		return $top_args
	}
}
