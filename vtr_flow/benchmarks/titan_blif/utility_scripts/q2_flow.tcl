# This script should be run using the quartus_sh command-line tool:
#   $ quartus_sh -t q2_flow.tcl [options]
#
# This script can perform three actions on the specified quartus 2 project:
#    (1) Synthesize (map) the design
#    (2) Write out a VQM file for use by the 'vqm2blif' conversion tool
#    (3) Fit (pack, place, route) the design onto an FPGA
#
# Options (2) and (3) can only occur after option (1), But do not
# neccessarily have to occur in the same run of quartus; provided
# the 'db' and 'incremental_db' directories quartus generates during
# (1) exist and are valid.
#
# NOTE: VQM output must be explicitly enabled before mapping
#       the design.  The setting used depends on the device
#       family. This script handles these settings.

#Handle command line arguments
package require cmdline
set options {\
    { "project.arg" "" "Project name" }     \
    { "family.arg" "" "Device family, currently only 'stratixiv' and 'stratixiii' supported" }     \
    { "synth" "Runs quartus_map on the design" }  \
    { "vqm_out_file.arg" "" "Generates a VQM file from a synthesized design" } \
    { "vqm_post_fit_out_file.arg" "" "Generates a VQM file from a post-fit design" } \
    { "fit" "Runs quartus_fit on the synthesized design" } \
    { "disable_timing" "Disable timing optimization during quartus_fit" }  \
    { "auto_size" "Let Quartus pick the target FPGA device" } \
    { "fit_ini_vars.arg" "" "Quartus INI_VARS for use during fitting" } \
    { "fit_assignment_vars.arg" "" "Quartus assgnment vars for use during fitting, expects a list of the form 'VAR1=VAL1; VAR2=VAL2' [translated to 'set_global_assignment -name VAR1 VAL1' etc.]" } \
    { "sta_fit" "Runs quartus_sta on the fitted design" } \
    { "sta_map" "Runs quartus_sta on the mapped design" } \
    { "sta_report_script.arg" "" "The script to perform reporting in quartus_sta." } \
    { "sdc_file.arg" "" "Over-ride the default SDC file." } \
    { "ncpu.arg" "1" "Number of CPUs quartus can use" } \
    { "report_dir.arg" "q2_out" "The directory to write report files" } \
    { "auto_partition_map" "Run the design partition to auto-partition the design after synthesis" } \
    { "auto_partition_fit" "Run the design partition to auto-partition the design after synthesis" } \
    { "auto_partition_logic_percentage.arg" "" "Percentage of the design logic to place in partitions" }
    { "auto_merge_plls" "Enables/Disables automatic PLL merging" }
	{ "partition_hard_blocks" "Empty Partitions design modules that represent hard blocks. This helps preserve hard block information in the generated netlist. PLEASE NOTE that none of the auto partitioning options can be used with this option." }
	{ "hard_block_names.arg" "" "Module names within the design that should be partitioned, expectes a list of module names of the form 'module_name_1;module_name_2;module_name_3'" }
}
set usage ": quartus_sh -t q2_flow.tcl -project QUARTUS_PROJECT_FILE -family FPGA_FAMILY_NAME \[-synth] \[-vqm_out_file VQM_FILE] \[-sta_map] \[-fit] \[-sta_fit] \[-sta_report_script] \[other options] \noptions:"
if { [catch {array set opts [::cmdline::getoptions quartus(args) $options $usage] } error] } {
    puts $error
    qexit -error
}

#############################################
#Procedure definitions
#############################################
proc run_exit_error {args} {
    #Handle args
    array set "" {-cmd "" -op_name ""}
    foreach {key value} $args {
        if {![info exists ($key)]} {error "bad option '$key'"}
        set ($key) $value
    }

    puts "INFO: Running $(-cmd) ..."
    if {[catch {eval $(-cmd)} result]} {
        puts "\nERROR: $(-op_name) failed. See the report file. (returned: $result)\n"
        qexit -error
    } else {
        puts "\nINFO: $(-op_name) was successful.\n"
    }
}

#For q2 execute_module tcl command (runs quartus_map, quartus_cdb 
# command line tools)
load_package flow

#For partition manipulation commands (auto_partition_design, delete_all_partitions)
load_package incremental_compilation

#Save the old pwd, since we need it to define any project output files
set script_run_dir [pwd]
#The directory where the project file exists
set project_dir [file dirname $opts(project)]
#The project file
set project_file [file tail $opts(project)]

#The source directory containing this script
set script_src_dir [file dirname $argv0]

puts "\nINFO: Starting q2_flow.tcl"

###############################################################################
# Open Quartus Project
###############################################################################
#Quartus can only open the project file from its own directory
puts "INFO: Moving to quartus project directory '$project_dir'"
cd $project_dir
#Open the project file
# Note: -force ensures the project opens even if it was last run with
#       a different version of quartus
# Note: -current_revision ensures the project opens the revision specified
#       in the qpf, instead of using the project name.
project_open -force -current_revision $project_file


###############################################################################
# General settings
###############################################################################
# Automatically choose the device size, may cause fit issues with some designs
#set_global_assignment -name DEVICE AUTO

# Write all report files to a special directory
if {[file pathtype $opts(report_dir)] == "relative"} {
    set report_output_dir [file join $script_run_dir $opts(report_dir)]
} else {
    #Ablsolute path
    set report_output_dir $opts(report_dir)
}
#Make the report dir if it doesn't already exist
file mkdir $report_output_dir

puts "INFO: Writing output files to: $report_output_dir"
set_global_assignment -name PROJECT_OUTPUT_DIRECTORY $report_output_dir

# The maximum number of CPUs quartus can use
puts "INFO: Limiting to at most $opts(ncpu) processor(s)"
set_global_assignment -name NUM_PARALLEL_PROCESSORS $opts(ncpu)

# Allow Q2 to optimize for timing
if {$opts(disable_timing)} {
    puts "INFO: Disabling Timing Optimization"
    set_global_assignment -name OPTIMIZE_TIMING "OFF"
} else {
    puts "INFO: Enabling Timing Optimization"
    set_global_assignment -name OPTIMIZE_TIMING "NORMAL COMPILATION"
}

if {$opts(auto_merge_plls)} {
    puts "INFO: PLL Merging Enabled (note: SDC clocks may not match!)"
    set_global_assignment -name AUTO_MERGE_PLLS "ON"
} else {
    puts "INFO: PLL Merging Disabled"
    set_global_assignment -name AUTO_MERGE_PLLS "OFF"
}

puts "INFO: Forcing Standard Fit"
set_global_assignment -name FITTER_EFFORT "STANDARD FIT"

if {$opts(auto_size)} {
    #Set device to auto, but filter for fastest speedgrade
    puts "INFO: Forcing device to AUTO"
    set_global_assignment -name DEVICE "AUTO"

    puts "INFO: Forcing device speed grade filter to FASTEST"
    set_global_assignment -name DEVICE_FILTER_SPEED_GRADE "FASTEST"

    puts "INFO: Forcing device voltage filter to 900mV"
    set_global_assignment -name DEVICE_FILTER_VOLTAGE 900
}

#Force junction temp
puts "INFO: Forcing MIN junction temp to 0 C"
set_global_assignment -name MIN_CORE_JUNCTION_TEMP 0

puts "INFO: Forcing MAX junction temp to 85 C"
set_global_assignment -name MAX_CORE_JUNCTION_TEMP 85

###############################################################################
# Enable VQM output
###############################################################################
#The options to enable VQM output are family dependant.  Only the options for
# stratixiv have been extensively tested.

# Stratix Family
if {$opts(family) == "stratixiv"} {
	puts "\nINFO: Enabling VQM output for stratixiv.\n"

    #Fixes issue where despite running map with '--family=stratixiv'
    # cdb would complain about map having been run with a different family
    # and refuse to output vqm
    set_global_assignment -name FAMILY "Stratix IV"
    
    #Enables vqm output
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;vqmo_gen_sivgx_vqm=on"

} elseif {$opts(family) == "stratixiii"} {
	puts "\nINFO: Enabling VQM output for stratixiii.\n"
    set_global_assignment -name FAMILY "Stratix III"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;vqmo_gen_siii_vqm=on"

} elseif {$opts(family) == "stratixv"} {
	puts "\nINFO: Enabling VQM output for stratixv.\n"
    set_global_assignment -name FAMILY "Stratix V"
    #Initial tests show that providing vqmo_gen_sv_vqm=on, or vqmo_gen_svgx_vqm=on
    # makes no difference to vqm output
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "stratixii"} {
	puts "\nINFO: Enabling VQM output for stratixii.\n"
    set_global_assignment -name FAMILY "Stratix II"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "stratix" || $opts(family) == "stratixi"} {
    set opts(family) "stratix"
	puts "\nINFO: Enabling VQM output for stratix.\n"
    set_global_assignment -name FAMILY "Stratix"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

# Cyclone Family
} elseif {$opts(family) == "cyclone" || $opts(family) == "cyclonei"} {
    set opts(family) "cyclone"
	puts "\nINFO: Enabling VQM output for cyclone.\n"
    set_global_assignment -name FAMILY "Cyclone"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "cycloneii"} {
	puts "\nINFO: Enabling VQM output for cycloneii.\n"
    set_global_assignment -name FAMILY "Cyclone II"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "cycloneiii"} {
	puts "\nINFO: Enabling VQM output for cycloneiii.\n"
    set_global_assignment -name FAMILY "Cyclone III"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "cycloneiv"} {
	puts "\nINFO: Enabling VQM output for cycloneiv.\n"
    set_global_assignment -name FAMILY "Cyclone IV"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "cyclonev"} {
	puts "\nINFO: Enabling VQM output for cyclonev.\n"
    set_global_assignment -name FAMILY "Cyclone V"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

# Arria Family
#   NOTE: There is no Aria III, or Arria IV.  Altera skipped directly from II to V.
} elseif {$opts(family) == "arria" || $opts(family) == "arriai" || $opts(family) == "arriagx"} {
    #Family name is actually arriagx, not arria
    set opts(family) "arriagx"
	puts "\nINFO: Enabling VQM output for arriagx.\n"
    set_global_assignment -name FAMILY "Arria GX"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "arriaii"} {
	puts "\nINFO: Enabling VQM output for arriaii.\n"
    set_global_assignment -name FAMILY "Arria II"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} elseif {$opts(family) == "arriav"} {
	puts "\nINFO: Enabling VQM output for arriav.\n"
    set_global_assignment -name FAMILY "Arria V"
    set_global_assignment -name INI_VARS "qatm_force_vqm=on;"

} else {
    puts "ERROR: Unkown device family '$opts(family)'\n"
    puts "       If this is a valid device family please"
    puts "       add the appropriate command to enable vqm"
    puts "       output to this script ($argv0)"
    qexit -error
}


###############################################################################
# Partition hard blocks
###############################################################################
if {$opts(partition_hard_blocks)} {

	# need to remove all partitions, so that there aren't inconsistencies any 
	# when we create them 
	run_exit_error -cmd "delete_all_partitions" -op_name "Deleting Partitions" 
	
	#get an updated list of all the nodes in the design
	set map_opts "--analysis_and_elaboration"
	run_exit_error -cmd "execute_module -tool map -args $map_opts" -op_name "Analysis & Elaboration (quartus_map)"
	 
	# if the user provided names of hard blocks in the design, then we partition all of them before synthesis 
	if {$opts(hard_block_names) != ""} {
    	 
		# store all hard block names
		set hard_blocks_to_partition [split $opts(hard_block_names) ";"]
		
		# this is equivalent to storing the components seen in the hierarchical view
		# in quartus.
		set hierarchical_node_ids [get_names -filter * -node_type hierarchy -observable_type pre_synthesis]
		
		# create all the empty partitions for the design modules that represent hard blocks
		foreach_in_collection node_id $hierarchical_node_ids {      
			
			# get the full path name of the node 
			set node_name [get_name_info -info full_path -observable_type pre_synthesis $node_id]
			
			# divide the node name into its different hierarchical components
			set node_name_hierarchial_levels [split $node_name "|"]
			
			# The last element in the list above is the lowest level
			# modules within the design, so we store it.
			set module_name [lindex $node_name_hierarchial_levels end]
			
			# we now compare the lowest level module name with the list
			# of custom hard block names. If there is a match, then we
			# know that we have to partition it.
			foreach hard_block_name $hard_blocks_to_partition {
				
				# check to see if the module is a custom hard block.
				# compare the moule name to the list of hard block names.
				if {[regexp "^$hard_block_name:.*" $module_name]} {
					
					# the module was a hard block, so we partition it
					# using its node name. Set it to a empty partition.
					create_partition -contents $node_name -partition $node_name
					set_partition -partition $node_name -netlist_type "Empty"
								
				}
			}	
		}
    }	
}

###############################################################################
# Synthesize the design
###############################################################################
if {$opts(synth)} {

    #Synthesize the design
    set map_opts "--family=$opts(family)"
    run_exit_error -cmd "execute_module -tool map -args $map_opts" -op_name "Analysis & Synthesis (quartus_map)"
    #if {[catch {execute_module -tool map -args $map_opts} result]} {
        #puts "\nERROR: Analysis & Synthesis (quartus_map) failed. See the report file. (returned: $result)\n\n"
        #qexit -error
    #} else {
        #puts "\nINFO: Analysis & Synthesis (quartus_map) was successful.\n"
    #}

    #Tell cdb to run merge on the design 
    # This merges the design if it is partitioned
    set cdb_opts "--merge"
    run_exit_error -cmd "execute_module -tool cdb -args $cdb_opts" -op_name "CDB Merge (quartus_cdb)"
    #if {[catch {execute_module -tool cdb -args $cdb_opts} result]} {
        #puts "\nERROR: CDB Merge (quartus_cdb) failed. See the report file. (returned: $result)\n"
        #qexit -error
    #} else {
        #puts "\nINFO: CDB Merge (quartus_cdb) was successful.\n"
    #}
}


###############################################################################
# Generate the VQM file
###############################################################################
if {$opts(vqm_out_file) != ""} {

    if {[file pathtype $opts(vqm_out_file)] == "relative"} {
        #Convert the relative path to an absolute path
        # This is required since we moved directory to open the quartus project
        set absolute_vqm_file_path [file join $script_run_dir [file tail $opts(vqm_out_file)]]
    } else {
        # Ablsolute paths or volume specific paths should be ok
        set absolute_vqm_file_path $opts(vqm_out_file)
    }

    set cdb_opts "--vqm=$absolute_vqm_file_path" 
    run_exit_error -cmd "execute_module -tool cdb -args $cdb_opts" -op_name "VQM Dump (quartus_cdb)"
    #if {[catch {execute_module -tool cdb -args $cdb_opts} result]} {
        #puts "\nERROR: VQM Dump (quartus_cdb) failed. See the report file. (returned: $result)\n"
        #qexit -error
    #} else {
        #puts "\nINFO: VQM Dump (quartus_cdb) was successful.\n"
    #}
}

###############################################################################
# Attempt to partition the design after synthesis
###############################################################################
if {$opts(auto_partition_map) && !$opts(partition_hard_blocks)} {

    #First delete any pre-existing partitions
    run_exit_error -cmd "delete_all_partitions" -op_name "Deleting Partitions" 

    #Generate new partitions
    set auto_partition_args ""
    if {$opts(auto_partition_logic_percentage) != ""} {
        set auto_partition_args "$auto_partition_args -percent_to_partition $opts(auto_partition_logic_percentage)" 
    }
    run_exit_error -cmd "auto_partition_design $auto_partition_args" -op_name "Post-Map Auto-Partitioning" 
}

###############################################################################
# Run STA on the mapped design to determine critical path delay
###############################################################################
if {$opts(sta_map)} {
    if {$opts(sta_report_script) == ""} {
        puts "\nError: must provide reporting script (may be blank) to run STA.\n"
        qexit -error
    } else {

        set sdc_file_override ""
        if {$opts(sdc_file) != ""} {
            set sdc_file_override "--sdc $opts(sdc_file)"
        }

        set sta_opts "--post_map --report_script $opts(sta_report_script) $sdc_file_override"
        #run_exit_error -cmd "execute_module -tool sta -args $sta_opts" -op_name "Post-map STA (quartus_sta)"
        if {[catch {execute_module -tool sta -args $sta_opts} result]} {
            puts "\nERROR: Fitting (quartus_sta) failed. See the report file. (returned: $result)\n"
            qexit -error
        } else {
            puts "\nINFO: Fitting (quartus_sta) was successful.\n"
        }
    }
}

###############################################################################
# Fit the design
###############################################################################
#Call the quartus fitter
if {$opts(fit)} {

    if {$opts(fit_ini_vars) != ""} {
        #User supplied ini_vars to modify quartus fit behaviour
        set_global_assignment -name INI_VARS "$opts(fit_ini_vars)"
    }
    if {$opts(fit_assignment_vars) != ""} {
        #A string with each assignment seperated by a semicolon
        set assignments [split $opts(fit_assignment_vars) ";"]

        foreach assignment $assignments {
            #Each assignment has a variable and a value seperated by an equals
            set values [split $assignment "="]

            set var [lindex $values 0]
            set val [lindex $values 1]

            #Set the assignment
            set cmd "set_global_assignment -name $var \"$val\""
            puts $cmd
            eval $cmd

        }
    }

    #Do not allow quartus to try multiple fits
    set fit_opts "--one_fit_attempt=on"
    run_exit_error -cmd "execute_module -tool fit -args $fit_opts" -op_name "Fitting (quartus_fit)"
    #if {[catch {execute_module -tool fit -args $fit_opts} result]} {
        #puts "\nERROR: Fitting (quartus_fit) failed. See the report file. (returned: $result)\n"
        #qexit -error
    #} else {
        #puts "\nINFO: Fitting (quartus_fit) was successful.\n"
    #}
}

###############################################################################
# Run STA on the fitted design to determine critical path delay
###############################################################################
if {$opts(sta_fit)} {
    if {$opts(sta_report_script) == ""} {
        puts "\nError: must provide reporting script (may be blank) to run STA.\n"
        qexit -error
    } else {
        set sta_opts "--report_script $opts(sta_report_script)"

        #run_exit_error -cmd "execute_module -tool sta -args $sta_opts" -op_name "Post-Fit STA (quartus_sta)"
        if {[catch {execute_module -tool sta -args $sta_opts} result]} {
            puts "\nERROR: Fitting (quartus_sta) failed. See the report file. (returned: $result)\n"
            qexit -error
        } else {
            puts "\nINFO: Fitting (quartus_sta) was successful.\n"
        }
    }
}

###############################################################################
# Generate post-fit VQM
###############################################################################
if {$opts(vqm_post_fit_out_file) != ""} {

    if {[file pathtype $opts(vqm_post_fit_out_file)] == "relative"} {
        #Convert the relative path to an absolute path
        # This is required since we moved directory to open the quartus project
        set absolute_vqm_file_path [file join $script_run_dir [file tail $opts(vqm_post_fit_out_file)]]
    } else {
        # Ablsolute paths or volume specific paths should be ok
        set absolute_vqm_file_path $opts(vqm_post_fit_out_file)
    }

    set cdb_opts "--vqm=$absolute_vqm_file_path" 
    run_exit_error -cmd "execute_module -tool cdb -args $cdb_opts" -op_name "VQM Dump (quartus_cdb)"
}

###############################################################################
# Attempt to partition the design after fit
###############################################################################
if {$opts(auto_partition_fit) && !$opts(partition_hard_blocks)} {

    #First delete any pre-existing partitions
    run_exit_error -cmd "delete_all_partitions" -op_name "Deleting Partitions" 

    #Generate new partitions
    set auto_partition_args ""
    if {$opts(auto_partition_logic_percentage) != ""} {
        set auto_partition_args "$auto_partition_args -percent_to_partition $opts(auto_partition_logic_percentage)" 
    }
    run_exit_error -cmd "auto_partition_design $auto_partition_args" -op_name "Post-Map Auto-Partitioning" 
}


puts "\nINFO: Completed q2_flow.tcl\n"
