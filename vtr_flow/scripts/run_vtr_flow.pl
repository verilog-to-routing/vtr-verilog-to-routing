#!/usr/bin/perl
###################################################################################
# This script runs the VTR flow for a single benchmark circuit and architecture
# file.
#
# Usage:
#	run_vtr_flow.pl <circuit_file> <architecture_file> [OPTIONS]
#
# Parameters:
# 	circuit_file: Path to the input circuit file (verilog, blif, etc)
#   architecture_file: Path to the architecture file (.xml)
#
# Options:
#   Note that all unrecognized parameters are forwarded to VPR.
# 	-starting_stage <stage>: Start the VTR flow at the specified stage.
#								Acceptable values: odin, abc, script, vpr.
#								Default value is odin.
#   -ending_stage <stage>: End the VTR flow at the specified stage. Acceptable
#								values: odin, abc, script, vpr. Default value is
#								vpr.
#   -specific_vpr_stage <stage>: Perform only this stage of VPR. Acceptable
#                               values: pack, place, route. Default is empty,
#                               which means to perform all. Note that specifying
#                               the routing stage requires a channel width
#                               to also be specified. To have any time saving
#                               effect, previous result files must be kept as the
#                               most recent necessary ones will be moved to the
#                               current run directory. (use inside tasks only)
# 	-keep_intermediate_files: Do not delete the intermediate files.
#   -keep_result_files: Do not delete the result files (.net, .place, .route)
#   -track_memory_usage: Print out memory usage for each stage (NOT fully portable)
#   -limit_memory_usage: Kill benchmark if it is taking up too much memory to avoid
#                       slow disk swaps.
#   -timeout: Maximum amount of time to spend on a single stage of a task in seconds;
#               default is 14 days.
#
#   -temp_dir <dir>: Directory used for all temporary files
###################################################################################

use strict;
use Cwd;
use File::Spec;
use POSIX;
use File::Copy;
use FindBin;
use File::Find;
use File::Basename;
use Time::HiRes;

use lib "$FindBin::Bin/perl_libs/XML-TreePP-0.41/lib";
use XML::TreePP;

my $vtr_flow_start_time = Time::HiRes::gettimeofday();

# Check the parameters.  Note PERL does not consider the script itself a parameter.
my $number_arguments = @ARGV;
if ( $number_arguments < 2 ) {
	print(
		"usage: run_vtr_flow.pl <circuit_file> <architecture_file> [OPTIONS]\n"
	);
	exit(-1);
}

# Get Absoluate Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*\/vtr_flow)\//;
my $vtr_flow_path = $1;
# my $vtr_flow_path = "./vtr_flow";

sub stage_index;
sub file_ext_for_stage;
sub expand_user_path;
sub file_find_and_replace;
sub xml_find_LUT_Kvalue;
sub xml_find_mem_size;
sub find_and_move_newest;
sub exe_for_platform;

my $temp_dir = "./temp";
my $diff_exec = "diff";

my $stage_idx_odin   = 1;
my $stage_idx_abc    = 2;
my $stage_idx_ace    = 3;
my $stage_idx_prevpr = 4;
my $stage_idx_vpr    = 5;

my $circuit_file_path      = expand_user_path( shift(@ARGV) );
my $architecture_file_path = expand_user_path( shift(@ARGV) );
my $sdc_file_path;
my $pad_file_path;

my $token;
my $ext;
my $starting_stage          = stage_index("odin");
my $ending_stage            = stage_index("vpr");
my $specific_vpr_stage      = "";
my $keep_intermediate_files = 1;
my $keep_result_files       = 1;
my $has_memory              = 1;
my $timing_driven           = "on";
my $min_chan_width          = -1;
my $max_router_iterations   = 50;
my $lut_size                = -1;
my $vpr_cluster_seed_type   = "";
my $routing_failure_predictor = "safe";
my $tech_file               = "";
my $do_power                = 0;
my $check_equivalent		= "off";
my $gen_post_synthesis_netlist 	= "off";
my $seed					= 1;
my $min_hard_mult_size		= 3;
my $min_hard_adder_size		= 1;
my $congestion_analysis		= "";
my $switch_usage_analysis   = "";

my $track_memory_usage      = 1;
my $memory_tracker          = "/usr/bin/time";
my @memory_tracker_args     = ("-v");
my $limit_memory_usage      = -1;
my $timeout                 = 14 * 24 * 60 * 60;         # 14 day execution timeout
my $valgrind 		    = 0;
my @valgrind_args	    = ("--leak-check=full", "--errors-for-leak-kinds=none", "--error-exitcode=1", "--track-origins=yes");
my $abc_quote_addition      = 0;
my @forwarded_vpr_args;   # VPR arguments that pass through the script
my $verify_rr_graph         = 0;
my $rr_graph_error_check    = 0;
my $check_route             = 0;
my $check_place             = 0;
my $use_old_abc             = 0;
my $enable_gui		    = 0;
my $routing_budgets_algorithm = "disable";
my $run_name = "";
my $expect_fail = 0;
my $verbosity = 0;
my $odin_adder_config_path = "default";
my $use_odin_xml_config = 1;

while ( $token = shift(@ARGV) ) {
	if ( $token eq "-sdc_file" ) {
		$sdc_file_path = expand_user_path( shift(@ARGV) );
	}
	elsif ( $token eq "-fix_pins" and $ARGV[0] ne "random") {
		$pad_file_path = $vtr_flow_path . shift(@ARGV);
	}
	elsif ( $token eq "-starting_stage" ) {
		$starting_stage = stage_index( shift(@ARGV) );
	}
	elsif ( $token eq "-ending_stage" ) {
		$ending_stage = stage_index( shift(@ARGV) );
	}
    elsif ( $token eq "-specific_vpr_stage" ) {
        $specific_vpr_stage = shift(@ARGV);
        if ($specific_vpr_stage eq "pack" or $specific_vpr_stage eq "place" or $specific_vpr_stage eq "route") {
            $specific_vpr_stage = "--" . $specific_vpr_stage;
        }
        else {
            $specific_vpr_stage = "";
        }
    }
	elsif ( $token eq "-delete_intermediate_files" ) {
		$keep_intermediate_files = 0;
	}
    elsif ( $token eq "-delete_result_files" ) {
        $keep_result_files = 0;
    }
    elsif ( $token eq "-track_memory_usage" ) {
        $track_memory_usage = 1;
    }
    elsif ( $token eq "-enable_gui" ) {
	$enable_gui = 1;
    }
    elsif ( $token eq "-limit_memory_usage" ) {
        $limit_memory_usage = shift(@ARGV);
        $abc_quote_addition = 1;
    }
    elsif ( $token eq "-timeout" ) {
        $timeout = shift(@ARGV);
    }
    elsif ( $token eq "-valgrind" ) {
        $valgrind = 1;
    }
	elsif ( $token eq "-no_mem" ) {
		$has_memory = 0;
	}
	elsif ( $token eq "-no_timing" ) {
		$timing_driven = "off";
	}
	elsif ( $token eq "-congestion_analysis") {
		$congestion_analysis = $token;
	}
    elsif ( $token eq "-switch_stats") {
        $switch_usage_analysis = $token;
    }
	elsif ( $token eq "-vpr_route_chan_width" ) {
		$min_chan_width = shift(@ARGV);
	}
    elsif ( $token eq "-vpr_max_router_iterations" ) {
        $max_router_iterations = shift(@ARGV);
    }
	elsif ( $token eq "-lut_size" ) {
		$lut_size = shift(@ARGV);
	}
	elsif ( $token eq "-routing_failure_predictor" ) {
		$routing_failure_predictor = shift(@ARGV);
	}
	elsif ( $token eq "-vpr_cluster_seed_type" ) {
		$vpr_cluster_seed_type = shift(@ARGV);
	}
	elsif ( $token eq "-temp_dir" ) {
		$temp_dir = shift(@ARGV);
	}
	elsif ( $token eq "-cmos_tech" ) {
		$tech_file = shift(@ARGV);
	}
	elsif ( $token eq "-power" ) {
		$do_power = 1;
	}
	elsif ( $token eq "-check_equivalent" ) {
		$check_equivalent = "on";
		$keep_intermediate_files = 1;
	}
	elsif ( $token eq "-gen_post_synthesis_netlist" ) {
		$gen_post_synthesis_netlist = "on";
	}
	elsif ( $token eq "-seed" ) {
		$seed = shift(@ARGV);
	}
	elsif ( $token eq "-min_hard_mult_size" ) {
		$min_hard_mult_size = shift(@ARGV);
	}
	elsif ( $token eq "-min_hard_adder_size" ) {
		$min_hard_adder_size = shift(@ARGV);
	}
    elsif ( $token eq "-verify_rr_graph" ){
            $verify_rr_graph = 1;
    }
    elsif ( $token eq "-rr_graph_error_check" ) {
            $rr_graph_error_check = 1;
    }
    elsif ( $token eq "-check_route" ){
            $check_route = 1;
    }
    elsif ( $token eq "-check_place" ){
            $check_place = 1;
    }
    elsif ( $token eq "-routing_budgets_algorithm"){
            $routing_budgets_algorithm = shift(@ARGV);
    }
    elsif ( $token eq "-use_old_abc"){
            $use_old_abc = 1;
    }
    elsif ( $token eq "-name"){
            $run_name = shift(@ARGV);
    }
    elsif ( $token eq "-expect_fail"){
            $expect_fail = 1;
    }
    elsif ( $token eq "-verbose"){
            $expect_fail = shift(@ARGV);
    }
		elsif ( $token eq "-adder_type"){
			$odin_adder_config_path = shift(@ARGV);
			if ( ($odin_adder_config_path ne "default") && ($odin_adder_config_path ne "optimized") ) {
					$odin_adder_config_path = $vtr_flow_path . $odin_adder_config_path;
			}
		}
		elsif ( $token eq "-disable_odin_xml" ){
						$use_odin_xml_config = 0;
		}
    # else forward the argument
	else {
        push @forwarded_vpr_args, $token;
        # next element could be the number 0, which needs to be forwarded
        if ( $ARGV[0] == '0' ) {
            $token = shift(@ARGV);
            push @forwarded_vpr_args, $token;
        }
	}

	if ( $starting_stage == -1 or $ending_stage == -1 ) {
		die
		  "Error: Invalid starting/ending stage name (start $starting_stage end $ending_stage).\n";
	}
}

if ( $ending_stage < $starting_stage ) {
	die "Error: Ending stage is before starting stage.";
}
if ($do_power) {
	if ( $tech_file eq "" ) {
		die "A CMOS technology behavior file must be provided.";
	}
	elsif ( not -r $tech_file ) {
		die "The CMOS technology behavior file ($tech_file) cannot be opened.";
	}
	$tech_file = Cwd::abs_path($tech_file);
}

if ( $vpr_cluster_seed_type eq "" ) {
	if ( $timing_driven eq "off" ) {
		$vpr_cluster_seed_type = "max_inputs";
	}
	else {
		$vpr_cluster_seed_type = "blend";
	}
}

if ( !-d $temp_dir ) {
	system "mkdir $temp_dir";
}
-d $temp_dir or die "Could not make temporary directory ($temp_dir)\n";
if ( !( $temp_dir =~ /.*\/$/ ) ) {
	$temp_dir = $temp_dir . "/";
}

my $results_path = "${temp_dir}output.txt";

my $error;
my $error_code = 0;
my $error_status = "OK";

my $arch_param;
my $cluster_size;
my $inputs_per_cluster = -1;

# Test for file existance
( -f $circuit_file_path )
  or die "Circuit file not found ($circuit_file_path)";
( -f $architecture_file_path )
  or die "Architecture file not found ($architecture_file_path)";

if ( !-e $sdc_file_path ) {
	# open( OUTPUT_FILE, ">$sdc_file_path" );
	# close ( OUTPUT_FILE );
	my $sdc_file_path;
}

if ( !-e $pad_file_path ) {
	# open( OUTPUT_FILE, ">$sdc_file_path" );
	# close ( OUTPUT_FILE );
	my $pad_file_path;
}

my $vpr_path;
if ( $stage_idx_vpr >= $starting_stage and $stage_idx_vpr <= $ending_stage ) {
	$vpr_path = exe_for_platform("$vtr_flow_path/../vpr/vpr");
	( -r $vpr_path or -r "${vpr_path}.exe" ) or die "Cannot find vpr exectuable ($vpr_path)";
}

my $odin2_path; my $odin_config_file_name; my $odin_config_file_path;
if (    $stage_idx_odin >= $starting_stage
	and $stage_idx_odin <= $ending_stage )
{
	$odin2_path = exe_for_platform("$vtr_flow_path/../ODIN_II/odin_II");
	( -e $odin2_path )
	  or die "Cannot find ODIN_II executable ($odin2_path)";

	$odin_config_file_name = "basic_odin_config_split.xml";

	$odin_config_file_path = "$vtr_flow_path/misc/$odin_config_file_name";
	( -e $odin_config_file_path )
	  or die "Cannot find ODIN config template ($odin_config_file_path)";

	$odin_config_file_name = "odin_config.xml";
	my $odin_config_file_path_new = "$temp_dir" . "odin_config.xml";
	copy( $odin_config_file_path, $odin_config_file_path_new );
	$odin_config_file_path = $odin_config_file_path_new;
}

my $abc_path;
my $abc_rc_path;
if ( $stage_idx_abc >= $starting_stage or $stage_idx_vpr <= $ending_stage ) {
    #Need ABC for either synthesis or post-VPR verification
    my $abc_dir_path = "$vtr_flow_path/../abc";

    if ($use_old_abc) {
        my $abc_dir_path = "$vtr_flow_path/../abc_with_bb_support";
    }

	$abc_path = "$abc_dir_path/abc";
	( -e $abc_path or -e "${abc_path}.exe" )
	  or die "Cannot find ABC executable ($abc_path)";

	$abc_rc_path = "$abc_dir_path/abc.rc";
	( -e $abc_rc_path ) or die "Cannot find ABC RC file ($abc_rc_path)";

	copy( $abc_rc_path, $temp_dir );
}

my $restore_multiclock_info_script = "$vtr_flow_path/scripts/restore_multiclock_latch_information.pl";

my $ace_path;
if ( $stage_idx_ace >= $starting_stage and $stage_idx_ace <= $ending_stage and $do_power) {
	$ace_path = "$vtr_flow_path/../ace2/ace";
	( -e $ace_path or -e "${ace_path}.exe" )
	  or die "Cannot find ACE executable ($ace_path)";
}
my $ace_clk_extraction_path = "$vtr_flow_path/../ace2/scripts/extract_clk_from_blif.py";

#Extract the circuit/architecture name and filename
my ($benchmark_name, $tmp_path, $circuit_suffix) = fileparse($circuit_file_path, '\.[^\.]*');
my $circuit_file_name = $benchmark_name . $circuit_suffix;

my ($architecture_name, $tmp_path, $arch_suffix) = fileparse($architecture_file_path, '\.[^\.]*');
my ($architecture_name_error, $tmp_path, $arch_suffix) = fileparse($architecture_file_path, '\.[^\.]*');

my $architecture_file_name = $architecture_name . $arch_suffix;
my $error_architecture_file_name = join "", $architecture_name, "_error", $arch_suffix;


if ($run_name eq "") {
    $run_name = "$architecture_name/$benchmark_name";
}
printf("%-80s", $run_name);

# Get Memory Size
my $mem_size = -1;
my $line;
my $in_memory_block;
my $in_mode;

# Read arch XML
my $tpp      = XML::TreePP->new();
my $xml_tree = $tpp->parsefile($architecture_file_path);

# Get lut size
my $lut_size = xml_find_LUT_Kvalue($xml_tree);
if ( $lut_size < 1 ) {
	$error_status = "failed: cannot determine arch LUT k-value";
	$error_code = 1;
}

# Get memory size
$mem_size = xml_find_mem_size($xml_tree);

my $odin_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_odin, $circuit_suffix);
my $odin_output_file_path = "$temp_dir$odin_output_file_name";

#The raw unprocessed ABC output
my $abc_raw_output_file_name = "$benchmark_name" . ".raw" . file_ext_for_stage($stage_idx_abc, $circuit_suffix);
my $abc_raw_output_file_path = "$temp_dir$abc_raw_output_file_name";

#The processed ABC output useable by downstream tools
my $abc_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_abc, $circuit_suffix);
my $abc_output_file_path = "$temp_dir$abc_output_file_name";

#Clock information for ACE
my $ace_clk_file_name = "ace_clk.txt";
my $ace_clk_file_path = "$temp_dir$ace_clk_file_name";

#The raw unprocessed ACE output
my $ace_raw_output_blif_name = "$benchmark_name" . ".raw" . file_ext_for_stage($stage_idx_ace, $circuit_suffix);
my $ace_raw_output_blif_path = "$temp_dir$ace_raw_output_blif_name";

#The processed ACE output useable by downstream tools
my $ace_output_blif_name = "$benchmark_name" . file_ext_for_stage($stage_idx_ace, $circuit_suffix);
my $ace_output_blif_path = "$temp_dir$ace_output_blif_name";

my $ace_output_act_name = "$benchmark_name" . ".act";
my $ace_output_act_path = "$temp_dir$ace_output_act_name";

my $prevpr_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_prevpr, $circuit_suffix);
my $prevpr_output_file_path = "$temp_dir$prevpr_output_file_name";

my $vpr_route_output_file_name = "$benchmark_name.route";
my $vpr_route_output_file_path = "$temp_dir$vpr_route_output_file_name";
my $vpr_postsynthesis_netlist = "";

#system"cp $abc_rc_path $temp_dir";
#system "cp $architecture_path $temp_dir";
#system "cp $circuit_path $temp_dir/$benchmark_name" . file_ext_for_stage($starting_stage - 1);
#system "cp $odin2_base_config"

my $architecture_file_path_new = "$temp_dir$architecture_file_name";
copy( $architecture_file_path, $architecture_file_path_new );
$architecture_file_path = $architecture_file_path_new;

my $circuit_file_path_new = "$temp_dir$benchmark_name" . file_ext_for_stage($starting_stage - 1, $circuit_suffix);
copy( $circuit_file_path, $circuit_file_path_new );
$circuit_file_path = $circuit_file_path_new;

# Call executable and time it
my $StartTime = time;
my $q         = "not_run";

#################################################################################
################################## ODIN #########################################
#################################################################################

if ( $starting_stage <= $stage_idx_odin and !$error_code ) {

	#system "sed 's/XXX/$benchmark_name.v/g' < $odin2_base_config > temp1.xml";
	#system "sed 's/YYY/$arch_name/g' < temp1.xml > temp2.xml";
	#system "sed 's/ZZZ/$odin_output_file_path/g' < temp2.xml > temp3.xml";
	#system "sed 's/PPP/$mem_size/g' < temp3.xml > circuit_config.xml";

	file_find_and_replace( $odin_config_file_path, "XXX", $circuit_file_name );
	file_find_and_replace( $odin_config_file_path, "YYY", $architecture_file_name );
	file_find_and_replace( $odin_config_file_path, "ZZZ", $odin_output_file_name );
	file_find_and_replace( $odin_config_file_path, "PPP", $mem_size );
	file_find_and_replace( $odin_config_file_path, "MMM", $min_hard_mult_size );
	file_find_and_replace( $odin_config_file_path, "AAA", $min_hard_adder_size );

	if ( !$error_code ) {
		if ( $use_odin_xml_config ) {
			$q = &system_with_timeout( "$odin2_path", "odin.out", $timeout, $temp_dir,
				"-c", $odin_config_file_name, "--adder_type", $odin_adder_config_path);
		} else {
			$q = &system_with_timeout( "$odin2_path", "odin.out", $timeout, $temp_dir,
				"--adder_type", $odin_adder_config_path,
				"-a", $temp_dir . $architecture_file_name,
				"-V", $temp_dir . $circuit_file_name,
				"-o", $temp_dir . $odin_output_file_name);
		}

		if ( -e $odin_output_file_path and $q eq "success") {
			if ( !$keep_intermediate_files ) {
				system "rm -f ${temp_dir}*.dot";
				system "rm -f ${temp_dir}*.v";
				system "rm -f $odin_config_file_path";
			}
		}
		else {
			$error_status = "failed: odin";
			$error_code = 1;
		}
	}
}

#################################################################################
################################## ABC ##########################################
#################################################################################
if (    $starting_stage <= $stage_idx_abc
	and $ending_stage >= $stage_idx_abc
	and !$error_code )
{
	#For ABC’s documentation see: https://people.eecs.berkeley.edu/~alanmi/abc/abc.htm
    #
    #Some key points on the script used:
    #
	#  strash : The strash command (which build's ABC's internal AIG) is needed before clean-up
    #           related commands (e.g. ifraig) otherwise they will fail with “Only works for
    #           structurally hashed networks”.
    #
	#  if –K #: This command techmaps the logic to LUTS. It should appear as the (near) final step
    #           before writing the optimized netlist. In recent versions, ABC does not remember
    #           that LUT size you want to techmap to. As a result, specifying if -K # early in
    #           the script causes ABC techmap to 2-LUTs, greatly increasing the amount of logic required (CLB’s, blocks, nets, etc.).
    #
    # The current script is based off the one used by YOSYS and on discussions with Alan Mishchenko (ABC author).
    # On 2018/04/28 Alan suggested the following:
    #   (1) run synthesis commands such as "dc2" after "ifraig" and "scorr" (this way more equivalences are typically found - improves quality)
    #   (2) run "ifraig" before "scorr" (this way comb equivalences are removed before seq equivalences are computed - improves runtime)
    #   (3) run "dch -f" immediately before mapping "if" (this alone greatly improves both area and delay of mapping)
    #   (4) no need to run "scleanup" if "scorr" is used ("scorr" internally performs "scleanup" - improves runtime)
    #   (5) no need to run"dc2" if "dch -f" is used, alternatively run "dc2; dch -f" (this will take more runtime but may not improve quality)
    #   (6) the only place to run "strash" is after technology mapping (if the script is run more than once - can improve quality)
    my $abc_commands="
echo '';
echo 'Load Netlist';
echo '============';
read $odin_output_file_name;
time;

echo '';
echo 'Circuit Info';
echo '==========';
print_stats;
print_latch;
time;

echo '';
echo 'LUT Costs';
echo '=========';
print_lut;
time;

echo '';
echo 'Logic Opt + Techmap';
echo '===================';
strash;
ifraig -v;
scorr -v;
dc2 -v;
dch -f;
if -K $lut_size -v;
mfs2 -v;
print_stats;
time;

echo '';
echo 'Output Netlist';
echo '==============';
write_hie $odin_output_file_name $abc_raw_output_file_name;
time;
";

    if ($use_old_abc) {
        #Legacy ABC script
        $abc_commands="read $odin_output_file_name; time; resyn; resyn2; if -K $lut_size; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; write_hie $odin_output_file_name $abc_raw_output_file_name; print_stats";
    }

    $abc_commands =~ s/\R/ /g; #Convert new-lines to spaces

    if ($abc_quote_addition) {$abc_commands = "'" . $abc_commands . "'";}

    #added so that valgrind will not run on abc because of existing memory errors
    if ($valgrind) {
            $valgrind = 0;
	    $q = &system_with_timeout( $abc_path, "abc.out", $timeout, $temp_dir, "-c",
		$abc_commands);
            $valgrind = 1;
    }
    else {
        $q = &system_with_timeout( $abc_path, "abc.out", $timeout, $temp_dir, "-c",
                $abc_commands);
    }

	if ( -e $abc_raw_output_file_path and $q eq "success") {

		# Restore Multi-Clock Latch Information from ODIN II that was striped out by ABC
        $q = &system_with_timeout($restore_multiclock_info_script, "restore_multiclock_latch_information.abc.out", $timeout, $temp_dir,
                $odin_output_file_name, $abc_raw_output_file_name, $abc_output_file_name);

        if ($q ne "success") {
            $error_status = "failed: to restore multi-clock latch info";
            $error_code = 1;

        }

		#system "rm -f abc.out";
		if ( !$keep_intermediate_files ) {
			if (! $do_power) {
				system "rm -f $odin_output_file_path";
			}
			system "rm -f ${temp_dir}*.rc";
		}
	}
	else {
		$error_status = "failed: abc";
		$error_code = 1;
	}
}

#################################################################################
################################## ACE ##########################################
#################################################################################
if (    $starting_stage <= $stage_idx_ace
	and $ending_stage >= $stage_idx_ace
	and $do_power
	and !$error_code )
{
	my $abc_clk_name;

	$q = &system_with_timeout($ace_clk_extraction_path, "ace_clk_extraction.out", $timeout, $temp_dir,
            $ace_clk_file_name, $abc_output_file_name);

	if ($q ne "success") {
		$error_status = "failed: ace clock extraction (only single clock activiy estimation is supported)";
        $error_code = 1;
    }

	{
	  local $/ = undef;
	  open(FILE, $ace_clk_file_path) or die "Can't read file '$ace_clk_file_path' ($!)\n";
	  $abc_clk_name = <FILE>;
	  close (FILE);
	}

	if (!$error_code) {
		$q = &system_with_timeout(
			$ace_path, "ace.out", $timeout, $temp_dir,
			"-b", $abc_output_file_name,
			"-c", $abc_clk_name,
            "-n", $ace_raw_output_blif_name,
			"-o", $ace_output_act_name,
			"-s", $seed
		);

		if ( -e $ace_raw_output_blif_path and $q eq "success") {

            # Restore Multi-Clock Latch Information from ODIN II that was striped out by ACE
            $q = &system_with_timeout($restore_multiclock_info_script, "restore_multiclock_latch_information.ace.out", $timeout, $temp_dir,
                    $odin_output_file_name, $ace_raw_output_blif_name, $ace_output_blif_name);

            if ($q ne "success") {
                $error_status = "failed: to restore multi-clock latch info";
                $error_code = 1;

            }

			if ( !$keep_intermediate_files ) {
				system "rm -f $abc_output_file_path";
				system "rm -f $odin_output_file_path";
			}
		}
		else {
			$error_status = "failed: ace";
			$error_code = 1;
		}
	}
}

#################################################################################
################################## PRE-VPR ######################################
#################################################################################
if (    $starting_stage <= $stage_idx_prevpr
	and $ending_stage >= $stage_idx_prevpr
	and !$error_code )
{
	my $prevpr_success   = 1;
	my $prevpr_input_blif_path;
	if ($do_power) {
		$prevpr_input_blif_path = $ace_output_blif_path;
	} else {
		$prevpr_input_blif_path = $abc_output_file_path;
	}

	copy($prevpr_input_blif_path, $prevpr_output_file_path);

	if ($prevpr_success) {
		if ( !$keep_intermediate_files ) {
			system "rm -f $prevpr_input_blif_path";
		}
	}
	else {
		$error_status = "failed: prevpr";
		$error_code = 1;
	}
}

#################################################################################
################################## VPR ##########################################
#################################################################################

if ( $ending_stage >= $stage_idx_vpr and !$error_code ) {
	my @vpr_power_args;

	if ($do_power) {
		push( @vpr_power_args, "--power" );
		push( @vpr_power_args, "--tech_properties" );
		push( @vpr_power_args, "$tech_file" );
	}

        #set a min chan width if it is not set to ensure equal results
        if ( ($check_route or $check_place) and $min_chan_width < 0){
            $min_chan_width = 300;
        }

	if ( $min_chan_width < 0 ) {

		my @vpr_args;
		push( @vpr_args, $architecture_file_name );
		push( @vpr_args, "$benchmark_name" );
		push( @vpr_args, "--circuit_file"	);
		push( @vpr_args, "$prevpr_output_file_name");
		push( @vpr_args, "--timing_analysis" );
		push( @vpr_args, "$timing_driven");
		push( @vpr_args, "--timing_driven_clustering" );
		push( @vpr_args, "$timing_driven");
		push( @vpr_args, "--cluster_seed_type" );
		push( @vpr_args, "$vpr_cluster_seed_type");
		push( @vpr_args, "--routing_failure_predictor" );
		push( @vpr_args, "$routing_failure_predictor");
		if (-e $sdc_file_path){
			push( @vpr_args, "--sdc_file" );
			push( @vpr_args, "$sdc_file_path");
		}
		if (-e $pad_file_path){
			push( @vpr_args, "--fix_pins" );
			push( @vpr_args, "$pad_file_path");
		}
                push( @vpr_args, "--routing_budgets_algorithm" );
                push( @vpr_args, "$routing_budgets_algorithm");
		push( @vpr_args, "--seed");
		push( @vpr_args, "$seed");
		push( @vpr_args, "$congestion_analysis");
		push( @vpr_args, "$switch_usage_analysis");
		push( @vpr_args, @forwarded_vpr_args);
		#run VPR with GUI
		if ($enable_gui) {
			push( @vpr_args, "--disp");
			push( @vpr_args, "on");

	 	}


		$q = &system_with_timeout(
            $vpr_path, "vpr.out",
            $timeout, $temp_dir, @vpr_args
		);

		if ( $timing_driven eq "on" ) {
			# Critical path delay is nonsensical at minimum channel width because congestion constraints completely dominate the cost function.
			# Additional channel width needs to be added so that there is a reasonable trade-off between delay and area
			# Commercial FPGAs are also desiged to have more channels than minimum for this reason

			# Parse out min_chan_width
			if ( open( VPROUT, "<${temp_dir}vpr.out" ) ) {
				undef $/;
				my $content = <VPROUT>;
				close(VPROUT);
				$/ = "\n";    # Restore for normal behaviour later in script

				if ( $content =~
					/Best routing used a channel width factor of (\d+)/m )
				{
					$min_chan_width = $1;
				}
			}

			$min_chan_width = ( $min_chan_width * 1.3 );
			$min_chan_width = floor($min_chan_width);
			if ( $min_chan_width % 2 ) {
				$min_chan_width = $min_chan_width + 1;
			}

			if ( -e $vpr_route_output_file_path ) {
				system "rm -f $vpr_route_output_file_path";

				my @vpr_args;
				push( @vpr_args, $architecture_file_name );
				push( @vpr_args, "$benchmark_name" );
				push( @vpr_args, "--route" );
                push( @vpr_args, "--analysis" );
				push( @vpr_args, "--circuit_file"	);
				push( @vpr_args, "$prevpr_output_file_name");
				push( @vpr_args, "--route_chan_width" );
				push( @vpr_args, "$min_chan_width" );
				push( @vpr_args, "--max_router_iterations" );
				push( @vpr_args, "$max_router_iterations");
				push( @vpr_args, "--cluster_seed_type"   );
				push( @vpr_args, "$vpr_cluster_seed_type");
				push( @vpr_args, @vpr_power_args);
				push( @vpr_args, "--gen_post_synthesis_netlist" );
				push( @vpr_args, "$gen_post_synthesis_netlist");
				if (-e $sdc_file_path){
					push( @vpr_args, "--sdc_file");
					push( @vpr_args, "$sdc_file_path");
				}
                                push( @vpr_args, "--routing_budgets_algorithm" );
                                push( @vpr_args, "$routing_budgets_algorithm");
				push( @vpr_args, @forwarded_vpr_args);
				push( @vpr_args, "$congestion_analysis");
				push( @vpr_args, "$switch_usage_analysis");
				#run VPR with GUI
				if ($enable_gui) {
					push( @vpr_args, "--disp");
					push( @vpr_args, "on");
	 			}

				$q = &system_with_timeout(
					$vpr_path, "vpr.crit_path.out",
					$timeout, $temp_dir, @vpr_args
				);
			}
		}
	} else { # specified channel width
        # move the most recent necessary result files to temp directory for specific vpr stage
        if ($specific_vpr_stage eq "--place" or $specific_vpr_stage eq "--route") {
            my $found_prev = &find_and_move_newest("$benchmark_name", "net");
            if ($found_prev and $specific_vpr_stage eq "--route") {
                &find_and_move_newest("$benchmark_name", "place");
            }
        }
        my $rr_graph_out_file = "rr_graph.xml";
        my $rr_graph_out_file2 = "rr_graph.2.xml";
		my @vpr_args;
		push( @vpr_args, $architecture_file_name );
		push( @vpr_args, "$benchmark_name" );
		push( @vpr_args, "--circuit_file"	);
		push( @vpr_args, "$prevpr_output_file_name");
		push( @vpr_args, "--timing_analysis" );
		push( @vpr_args, "$timing_driven");
		push( @vpr_args, "--timing_driven_clustering" );
		push( @vpr_args, "$timing_driven");
		push( @vpr_args, "--route_chan_width" );
		push( @vpr_args, "$min_chan_width" );
		push( @vpr_args, "--max_router_iterations" );
		push( @vpr_args, "$max_router_iterations");
		push( @vpr_args, "--cluster_seed_type" );
		push( @vpr_args, "$vpr_cluster_seed_type");
		push( @vpr_args, @vpr_power_args);
		push( @vpr_args, "--gen_post_synthesis_netlist" );
		push( @vpr_args, "$gen_post_synthesis_netlist");
		if (-e $sdc_file_path){
			push( @vpr_args, "--sdc_file" );
			push( @vpr_args, "$sdc_file_path");
		}
		if (-e $pad_file_path){
			push( @vpr_args, "--fix_pins" );
			push( @vpr_args, "$pad_file_path");
		}
		push( @vpr_args, "--seed");
		push( @vpr_args, "$seed");
                push( @vpr_args, "--routing_budgets_algorithm" );
                push( @vpr_args, "$routing_budgets_algorithm");
		if ($verify_rr_graph || $rr_graph_error_check){
			push( @vpr_args, "--write_rr_graph" );
			push( @vpr_args, $rr_graph_out_file);
		}
		push( @vpr_args, "$switch_usage_analysis");
		push( @vpr_args, @forwarded_vpr_args);
		push( @vpr_args, $specific_vpr_stage);
		#run VPR with GUI
		if ($enable_gui) {
			push( @vpr_args, "--disp");
			push( @vpr_args, "on");
	 	}

        $q = &system_with_timeout(
			   $vpr_path, "vpr.out",
			   $timeout, $temp_dir,
			   @vpr_args);

        #run vpr again with additional parameters. This is for running a certain stage only or checking the rr graph
        if ($verify_rr_graph or $check_route or $check_place or $rr_graph_error_check){
            # move the most recent necessary result files to temp directory for specific vpr stage
            if ($specific_vpr_stage eq "--place" or $specific_vpr_stage eq "--route") {
                my $found_prev = &find_and_move_newest("$benchmark_name", "net");
                if ($found_prev and $specific_vpr_stage eq "--route") {
                 &find_and_move_newest("$benchmark_name", "place");
                }
            }

            #load the architecture file with errors if we're checking for it
			if ($rr_graph_error_check){
				my $architecture_file_path_new_error = "$temp_dir$error_architecture_file_name";
				copy( $architecture_file_path, $architecture_file_path_new_error);
				$architecture_file_path = $architecture_file_path_new_error;
			}

            my @vpr_args;
            if ($rr_graph_error_check){
				push( @vpr_args, $error_architecture_file_name );
			}else{
				push( @vpr_args, $architecture_file_name );
			}

            push( @vpr_args, "$benchmark_name" );

			#only perform routing for error check. Special care was taken prevent netlist check warnings
            if ($rr_graph_error_check){
				push( @vpr_args, "--verify_file_digests" );
                push( @vpr_args, "off" );
			}
            push( @vpr_args, "--circuit_file"	);
            push( @vpr_args, "$prevpr_output_file_name");
            push( @vpr_args, "--timing_analysis" );
            push( @vpr_args, "$timing_driven");
            push( @vpr_args, "--timing_driven_clustering" );
            push( @vpr_args, "$timing_driven");
            push( @vpr_args, "--route_chan_width" );
            push( @vpr_args, "$min_chan_width" );
            push( @vpr_args, "--max_router_iterations" );
            push( @vpr_args, "$max_router_iterations");
            push( @vpr_args, "--cluster_seed_type" );
            push( @vpr_args, "$vpr_cluster_seed_type");
            push( @vpr_args, @vpr_power_args);
            push( @vpr_args, "--gen_post_synthesis_netlist" );
            push( @vpr_args, "$gen_post_synthesis_netlist");
            if (-e $sdc_file_path){
                push( @vpr_args, "--sdc_file" );
                push( @vpr_args, "$sdc_file_path");
            }
            if (-e $pad_file_path){
                push( @vpr_args, "-fix_pins" );
                push( @vpr_args, "$pad_file_path");
            }
            if ($verify_rr_graph){
                if (! -e $rr_graph_out_file || -z $rr_graph_out_file) {
                    $error_status = "failed: vpr (no RR graph file produced)";
                    $error_code = 1;
                }
                push( @vpr_args, "--read_rr_graph" );
                push( @vpr_args, $rr_graph_out_file);
                push( @vpr_args, "--write_rr_graph" );
                push( @vpr_args, $rr_graph_out_file2);
            }
            push( @vpr_args, "--routing_budgets_algorithm" );
            push( @vpr_args, "$routing_budgets_algorithm");
            if ($check_route){
				push( @vpr_args, "--analysis");
            } elsif ($check_place or $rr_graph_error_check){
				push( @vpr_args, "--route");
            }

            push( @vpr_args, "$switch_usage_analysis");
            push( @vpr_args, @forwarded_vpr_args);
            push( @vpr_args, $specific_vpr_stage);
	    #run VPR with GUI
	    if ($enable_gui) {
			push( @vpr_args, "--disp");
			push( @vpr_args, "on");
	    }

			#run vpr again with a different name and additional parameters

            $q = &system_with_timeout(
                    $vpr_path, "vpr_second_run.out",
                    $timeout,  $temp_dir,
                    @vpr_args
            );

            if ($verify_rr_graph) {
                #Sanity check that the RR graph produced after reading the
                #previously dumped RR graph is identical

                my @diff_args;
                push(@diff_args, $rr_graph_out_file);
                push(@diff_args, $rr_graph_out_file2);

                my $diff_result = &system_with_timeout(
                                    $diff_exec, "diff.rr_graph.out",
                                    $timeout,  $temp_dir,
                                    @diff_args
                            );

                if ($diff_result ne "success") {
                    $error_status = "failed: vpr (RR Graph XML output not consistent when reloaded)";
                    $error_code = 1;
                }
            }
        }
    }


	#Removed check for existing vpr_route_output_path in order to pass when routing is turned off (only pack/place)
	if ($q eq "success") {
		if($check_equivalent eq "on") {

			find(\&find_postsynthesis_netlist, ".");

            #Pick the netlist to verify against
            #
            #We pick the 'earliest' netlist of the stages that where run
            my $reference_netlist = "";
            if($starting_stage <= $stage_idx_odin) {
                $reference_netlist = $odin_output_file_name;
            } elsif ($starting_stage <= $stage_idx_abc) {
                $reference_netlist = $abc_output_file_name;
            } else {
                #VPR's input
                $reference_netlist = $prevpr_output_file_name;
            }


            #First try ABC's Unbounded Sequential Equivalence Check (DSEC)
			$q = &system_with_timeout($abc_path,
							"abc.sec.out",
							$timeout,
							$temp_dir,
							"-c",
							"dsec $reference_netlist $vpr_postsynthesis_netlist"
			);

            # Parse ABC verification output
            if ( open( SECOUT, "< $temp_dir/abc.sec.out" ) ) {
                undef $/;
                my $sec_content = <SECOUT>;
                close(SECOUT);
                $/ = "\n";    # Restore for normal behaviour later in script

                if ( $sec_content =~ m/(.*The network has no latches. Used combinational command "cec".*)/i ) {
                    # This circuit has no latches, ABC's 'sec' command only supports circuits with latches.
                    # Re-try using ABC's Combinational Equivalence Check (CEC)
                    $q = &system_with_timeout($abc_path,
                                    "abc.cec.out",
                                    $timeout,
                                    $temp_dir,
                                    "-c",
                                    "cec $reference_netlist $vpr_postsynthesis_netlist;"
                    );

                    if ( open( CECOUT, "< $temp_dir/abc.cec.out" ) ) {
                        undef $/;
                        my $cec_content = <CECOUT>;
                        close(CECOUT);
                        $/ = "\n";    # Restore for normal behaviour later in script

                        if ( $cec_content !~ m/(.*Networks are equivalent.*)/i ) {
                            $error_status = "failed: formal verification";
                            $error_code = 1;
                        }
                    } else {
                        $error_status = "failed: no CEC output";
                        $error_code = 1;
                    }
                } elsif ( $sec_content !~ m/(.*Networks are equivalent.*)/i ) {
                    $error_status = "failed: formal verification";
                    $error_code = 1;
                }
            } else {
                $error_status = "failed: no DSEC output";
                $error_code = 1;
            }
        }

		if (! $keep_intermediate_files)
		{
			system "rm -f $prevpr_output_file_name";
			system "rm -f ${temp_dir}*.xml";
			system "rm -f ${temp_dir}*.sdf";
			system "rm -f ${temp_dir}*.v";
            if (! $keep_result_files) {
                system "rm -f ${temp_dir}*.net";
                system "rm -f ${temp_dir}*.place";
                system "rm -f ${temp_dir}*.route";
            }
			if ($do_power) {
				system "rm -f $ace_output_act_path";
			}
		}
	} else {
		$error_status = "failed: vpr ($q)";
		$error_code = 1;
	}
}

my $EndTime = time;

# Determine running time
my $seconds    = ( $EndTime - $StartTime );
my $runseconds = $seconds % 60;

# Start collecting results to output.txt
open( RESULTS, "> $results_path" );

# Output vpr status and runtime
print RESULTS "vpr_status=$q\n";
print RESULTS "vpr_seconds=$seconds\n";

# Parse VPR output
if ( open( VPROUT, "< vpr.out" ) ) {
	undef $/;
	my $content = <VPROUT>;
	close(VPROUT);
	$/ = "\n";    # Restore for normal behaviour later in script
}
print RESULTS "error=$error\n";

close(RESULTS);

if ($expect_fail) {
    if ($error_code != 0) {
        #Failed as expected, invert message
        $error_code = 0;
        $error_status = "OK";
        if ($verbosity > 0) {
            $error_status .= "(as expected " . $error_status . ")";
        } else {
            $error_status .= "*";
        }
    } else {
        #Passed when expected failure
        $error_status = "failed: expected to fail but was " . $error_status;
    }
}

my $elapsed_time_str = sprintf("(took %.2f seconds)", Time::HiRes::gettimeofday() - $vtr_flow_start_time);

printf(" %-15s %19s", $error_status, $elapsed_time_str);

print "\n";

exit $error_code;

################################################################################
# Subroutine to execute a system call with a timeout
# system_with_timeout(<program>, <stdout file>, <timeout>, <dir>, <arg1>, <arg2>, etc)
#    make sure args is an array
# Returns: "timeout", "exited", "success", "crashed"
################################################################################
sub system_with_timeout {
	# Check for existence of /usr/bin/time module
	unless (-f $memory_tracker) {
		die "system_with_timeout: /usr/bin/time does not exist"
	}

	# Check args
	( $#_ > 2 )   or die "system_with_timeout: not enough args\n";
    if ($valgrind) {
        my $program = shift @_;
	unshift @_, "valgrind";
        splice @_, 4, 0, , @valgrind_args, $program;
    }
    # Use a memory tracker to call executable usr/bin/time
    my $program = shift @_;
    unshift @_, $memory_tracker;
    splice @_, 4, 0, @memory_tracker_args, $program;

    if ($limit_memory_usage > 0) {
        my $program = shift @_;
        unshift @_, "bash";
        # flatten array
        my $params = join(" ", @_[4 .. ($#_)]);
        splice @_, 4, 1000, "-c", "ulimit -Sv $limit_memory_usage;" . $program . " " . $params;
    }
	# ( -f $_[0] )  or die "system_with_timeout: can't find executable $_[0]\n";
	( $_[2] > 0 ) or die "system_with_timeout: invalid timeout\n";

	#start valgrind output on new line
	if ($valgrind) {
		print "\n";
	}

	# Save the pid of child process
	my $pid = fork;

	if ( $pid == 0 ) {

		# Redirect STDOUT for vpr
		chdir $_[3];


		open( STDOUT, "> $_[1]" );
		if (!$valgrind) {
			open( STDERR, ">&STDOUT" );
		}

		# Copy the args and cut out first four
		my @VPRARGS = @_;
		shift @VPRARGS;
		shift @VPRARGS;
		shift @VPRARGS;
		shift @VPRARGS;

		# Run command
		# This must be an exec call and there most be no special shell characters
		# like redirects so that perl will use execvp and $pid will actually be
		# that of vpr so we can kill it later.

        # first strip out empty
        @VPRARGS = grep { $_ ne ''} @VPRARGS;

		print "\n$_[0] @VPRARGS\n";
		exec $_[0], @VPRARGS;
	}
	else {
		my $timed_out = "false";

		# Register signal handler, to kill child process (SIGABRT)
		$SIG{ALRM} = sub { kill 6, $pid; $timed_out = "true"; };

		# Register handlers to take down child if we are killed (SIGHUP)
		$SIG{INTR} = sub { print "SIGINTR\n"; kill 1, $pid; exit; };
		$SIG{HUP}  = sub { print "SIGHUP\n";  kill 1, $pid; exit; };

		# Set SIGALRM timeout
		alarm $_[2];

		# Wait for child process to end OR timeout to expire
		wait;

		# Unset the alarm in case we didn't timeout
		alarm 0;

		# Check if timed out or not
		if ( $timed_out eq "true" ) {
			return "timeout";
		}
		else {
			my $did_crash = "false";
			if ( $? & 127 ) { $did_crash = "true"; }

			my $return_code = $? >> 8;

			if ( $did_crash eq "true" ) {
				return "crashed";
			}
			elsif ( $return_code != 0 ) {
				return "exited with return code $return_code";
			}
			else {
				return "success";
			}
		}
	}
}

sub stage_index {
	my $stage_name = $_[0];

	if ( lc($stage_name) eq "odin" ) {
		return $stage_idx_odin;
	}
	if ( lc($stage_name) eq "abc" ) {
		return $stage_idx_abc;
	}
	if ( lc($stage_name) eq "ace" ) {
		return $stage_idx_ace;
	}
	if ( lc($stage_name) eq "prevpr" ) {
		return $stage_idx_prevpr;
	}
	if ( lc($stage_name) eq "vpr" ) {
		return $stage_idx_vpr;
	}
	return -1;
}

sub file_ext_for_stage {
	my $stage_idx = $_[0];
	my $circuit_suffix = $_[1];

    my $vpr_netlist_suffix = ".blif";
    if ($circuit_suffix eq ".eblif") {
        $vpr_netlist_suffix = $circuit_suffix;
    }

	if ( $stage_idx == 0 ) {
		return ".v";
	}
	elsif ( $stage_idx == $stage_idx_odin ) {
		return ".odin.blif";
	}
	elsif ( $stage_idx == $stage_idx_abc ) {
		return ".abc.blif";
	}
	elsif ( $stage_idx == $stage_idx_ace ) {
		return ".ace.blif";
	}
	elsif ( $stage_idx == $stage_idx_prevpr ) {
		return ".pre-vpr" . $vpr_netlist_suffix;
	}
}

sub expand_user_path {
	my $str = shift;
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}

sub file_find_and_replace {
	my $file_path      = shift();
	my $search_string  = shift();
	my $replace_string = shift();

	open( FILE_IN, "$file_path" );
	my $file_contents = do { local $/; <FILE_IN> };
	close(FILE_IN);

	$file_contents =~ s/$search_string/$replace_string/mg;

	open( FILE_OUT, ">$file_path" );
	print FILE_OUT $file_contents;
	close(FILE_OUT);
}

sub xml_find_key {
	my $tree = shift();
	my $key  = shift();

	foreach my $subtree ( keys %{$tree} ) {
		if ( $subtree eq $key ) {
			return $tree->{$subtree};
		}
	}
	return "";
}

sub xml_find_child_by_key_value {
	my $tree = shift();
	my $key  = shift();
	my $val  = shift();

	if ( ref($tree) eq "HASH" ) {

		# Only a single item in the child array
		if ( $tree->{$key} eq $val ) {
			return $tree;
		}
	}
	elsif ( ref($tree) eq "ARRAY" ) {

		# Child Array
		foreach my $child (@$tree) {
			if ( $child->{$key} eq $val ) {
				return $child;
			}
		}
	}

	return "";
}

sub xml_find_LUT_Kvalue {
	my $tree = shift();

	#Check if this is a LUT
	if ( xml_find_key( $tree, "-blif_model" ) eq ".names" ) {
		return $tree->{input}->{"-num_pins"};
	}

	my $max = 0;
	my $val = 0;

	foreach my $subtree ( keys %{$tree} ) {
		my $child = $tree->{$subtree};

		if ( ref($child) eq "ARRAY" ) {
			foreach my $item (@$child) {
				$val = xml_find_LUT_Kvalue($item);
				if ( $val > $max ) {
					$max = $val;
				}
			}
		}
		elsif ( ref($child) eq "HASH" ) {
			$val = xml_find_LUT_Kvalue($child);
			if ( $val > $max ) {
				$max = $val;
			}
		}
		else {

			# Leaf - do nothing
		}
	}

	return $max;
}

sub xml_find_mem_size_recursive {
	my $tree = shift();

	#Check if this is a Memory
	if ( xml_find_key( $tree, "-blif_model" ) =~ "port_ram" ) {
		my $input_pins = $tree->{input};
		foreach my $input_pin (@$input_pins) {
			if ( xml_find_key( $input_pin, "-name" ) =~ "addr" ) {
				return $input_pin->{"-num_pins"};
			}
		}
		return 0;
	}

	# Otherwise iterate down
	my $max = 0;
	my $val = 0;

	foreach my $subtree ( keys %{$tree} ) {
		my $child = $tree->{$subtree};

		if ( ref($child) eq "ARRAY" ) {
			foreach my $item (@$child) {
				$val = xml_find_mem_size_recursive($item);
				if ( $val > $max ) {
					$max = $val;
				}
			}
		}
		elsif ( ref($child) eq "HASH" ) {
			$val = xml_find_mem_size_recursive($child);
			if ( $val > $max ) {
				$max = $val;
			}
		}
		else {

			# Leaf - do nothing
		}
	}

	return $max;
}

sub xml_find_mem_size {
	my $tree = shift();

	my $pb_tree = $tree->{architecture}->{complexblocklist}->{pb_type};
	if ( $pb_tree eq "" ) {
		return "";
	}

	my $memory_pb = xml_find_child_by_key_value ($pb_tree, "-name", "memory");
	if ( $memory_pb eq "" ) {
		return "";
	}

	return xml_find_mem_size_recursive($memory_pb);
}

sub find_and_move_newest {
    my $benchmark_name = shift();
    my $file_type = shift();

    my $found_prev = system("$vtr_flow_path/scripts/mover.sh \"*$benchmark_name*/*.$file_type\" ../../../ $temp_dir");
    # cannot find previous version, disregard specific vpr stage argument
    if ($found_prev ne 0) {
        print "$file_type file not found, disregarding specific vpr stage\n";
        $specific_vpr_stage = "";
        return 0;
    }

    # negate bash exit truth for perl
    return 1;
}

sub find_postsynthesis_netlist {
    my $file_name = $_;
    if ($file_name =~ /_post_synthesis\.blif/) {
        $vpr_postsynthesis_netlist = $file_name;
        return;
    }
}

# Returns the executable extension based on the platform
sub exe_for_platform {
	my $file_name = shift();
	if ($^O eq "cygwin" or $^O eq "MSWIN32") {
		$file_name = $file_name . ".exe";
	}
	return $file_name;
}

sub get_clocks {
    my $filename = shift();

    open my $fh, $filename or die "Cannot open $filename: $!\n";

    my @clocks = <$fh>;

    return @clocks;
}
