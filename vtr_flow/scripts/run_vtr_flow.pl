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
# 	-starting_stage <stage>: Start the VTR flow at the specified stage.
#								Acceptable values: odin, abc, script, vpr.
#								Default value is odin.
#   -ending_stage <stage>: End the VTR flow at the specified stage. Acceptable
#								values: odin, abc, script, vpr. Default value is
#								vpr.
# 	-keep_intermediate_files: Do not delete the intermediate files.
#
#   -temp_dir <dir>: Directory used for all temporary files
###################################################################################

use strict;
use Cwd;
use File::Spec;
use POSIX;
use File::Copy;
use FindBin;

use lib "$FindBin::Bin/perl_libs/XML-TreePP-0.41/lib";
use XML::TreePP;

# check the parametes.  Note PERL does not consider the script itself a parameter.
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

my $temp_dir = "./temp";

my $stage_idx_odin   = 1;
my $stage_idx_abc    = 2;
my $stage_idx_ace    = 3;
my $stage_idx_prevpr = 4;
my $stage_idx_vpr    = 5;

my $circuit_file_path      = expand_user_path( shift(@ARGV) );
my $architecture_file_path = expand_user_path( shift(@ARGV) );
my $sdc_file_path;

my $token;
my $ext;
my $starting_stage          = stage_index("odin");
my $ending_stage            = stage_index("vpr");
my $keep_intermediate_files = 0;
my $has_memory              = 1;
my $timing_driven           = "on";
my $min_chan_width          = -1; 
my $lut_size                = -1;
my $vpr_cluster_seed_type   = "";
my $tech_file               = "";
my $do_power                = 0;
my $check_equivalent		= "off";
my $gen_postsynthesis_netlist 	= "off";
my $seed					= 1;
my $min_hard_adder_size		= 1;

while ( $token = shift(@ARGV) ) {
	if ( $token eq "-sdc_file" ) {
		$sdc_file_path = expand_user_path( shift(@ARGV) );
	}
	elsif ( $token eq "-starting_stage" ) {
		$starting_stage = stage_index( shift(@ARGV) );
	}
	elsif ( $token eq "-ending_stage" ) {
		$ending_stage = stage_index( shift(@ARGV) );
	}
	elsif ( $token eq "-keep_intermediate_files" ) {
		$keep_intermediate_files = 1;
	}
	elsif ( $token eq "-no_mem" ) {
		$has_memory = 0;
	}
	elsif ( $token eq "-no_timing" ) {
		$timing_driven = "off";
	}
	elsif ( $token eq "-vpr_route_chan_width" ) {
		$min_chan_width = shift(@ARGV);
	}
	elsif ( $token eq "-lut_size" ) {
		$lut_size = shift(@ARGV);
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
	}
	elsif ( $token eq "-gen_postsynthesis_netlist" ) {
		$gen_postsynthesis_netlist = "on";
	}
	elsif ( $token eq "-seed" ) {
		$seed = shift(@ARGV);
	}
	elsif ( $token eq "-min_hard_adder_size" ) {
		$min_hard_adder_size = shift(@ARGV);
	}
	else {
		die "Error: Invalid argument ($token)\n";
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
		$vpr_cluster_seed_type = "timing";
	}
}

if ( !-d $temp_dir ) {
	system "mkdir $temp_dir";
}
-d $temp_dir or die "Could not make temporary directory ($temp_dir)\n";
if ( !( $temp_dir =~ /.*\/$/ ) ) {
	$temp_dir = $temp_dir . "/";
}

my $timeout      = 10 * 24 * 60 * 60;         # 10 day execution timeout
my $results_path = "${temp_dir}output.txt";

my $error;
my $error_code = 0;

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

my $vpr_path;
if ( $stage_idx_vpr >= $starting_stage and $stage_idx_vpr <= $ending_stage ) {
	$vpr_path = "$vtr_flow_path/../vpr/vpr";
	( -r $vpr_path or -r "${vpr_path}.exe" )
	  or die "Cannot find vpr exectuable ($vpr_path)";
}

my $odin2_path;
my $odin_config_file_name;
my $odin_config_file_path;
if (    $stage_idx_odin >= $starting_stage
	and $stage_idx_odin <= $ending_stage )
{
	$odin2_path = "$vtr_flow_path/../ODIN_II/odin_II.exe";
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
if ( $stage_idx_abc >= $starting_stage and $stage_idx_abc <= $ending_stage ) {
	$abc_path = "$vtr_flow_path/../abc_with_bb_support/abc";
	( -e $abc_path or -e "${abc_path}.exe" )
	  or die "Cannot find ABC executable ($abc_path)";

	$abc_rc_path = "$vtr_flow_path/../abc_with_bb_support/abc.rc";
	( -e $abc_rc_path ) or die "Cannot find ABC RC file ($abc_rc_path)";

	copy( $abc_rc_path, $temp_dir );
}

my $ace_path;
if ( $stage_idx_ace >= $starting_stage and $stage_idx_ace <= $ending_stage and $do_power) {
	$ace_path = "$vtr_flow_path/../ace2/ace";
	( -e $ace_path or -e "${ace_path}.exe" )
	  or die "Cannot find ACE executable ($ace_path)";
}

# Get circuit name (everything up to the first '.' in the circuit file)
my ( $vol, $path, $circuit_file_name ) =
  File::Spec->splitpath($circuit_file_path);
$circuit_file_name =~ m/(.*)[.].*?/;
my $benchmark_name = $1;

# Get architecture name
$architecture_file_path =~ m/.*\/(.*?.xml)/;
my $architecture_file_name = $1;

$architecture_file_name =~ m/(.*).xml$/;
my $architecture_name = $1;
print "$architecture_name/$benchmark_name...";

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
	print "failed: cannot determine arch LUT k-value";
	$error_code = 1;
}

# Get memory size
$mem_size = xml_find_mem_size($xml_tree);

my $odin_output_file_name =
  "$benchmark_name" . file_ext_for_stage($stage_idx_odin);
my $odin_output_file_path = "$temp_dir$odin_output_file_name";

my $abc_output_file_name =
  "$benchmark_name" . file_ext_for_stage($stage_idx_abc);
my $abc_output_file_path = "$temp_dir$abc_output_file_name";

my $ace_output_blif_name =
  "$benchmark_name" . file_ext_for_stage($stage_idx_ace);
my $ace_output_blif_path = "$temp_dir$ace_output_blif_name";

my $ace_output_act_name = "$benchmark_name" . ".act";
my $ace_output_act_path = "$temp_dir$ace_output_act_name";

my $prevpr_output_file_name =
  "$benchmark_name" . file_ext_for_stage($stage_idx_prevpr);
my $prevpr_output_file_path = "$temp_dir$prevpr_output_file_name";

my $vpr_route_output_file_name = "$benchmark_name.route";
my $vpr_route_output_file_path = "$temp_dir$vpr_route_output_file_name";

#system "cp $abc_rc_path $temp_dir";
#system "cp $architecture_path $temp_dir";
#system "cp $circuit_path $temp_dir/$benchmark_name" . file_ext_for_stage($starting_stage - 1);
#system "cp $odin2_base_config"

my $architecture_file_path_new = "$temp_dir$architecture_file_name";
copy( $architecture_file_path, $architecture_file_path_new );
$architecture_file_path = $architecture_file_path_new;

my $circuit_file_path_new =
  "$temp_dir$benchmark_name" . file_ext_for_stage( $starting_stage - 1 );
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
	file_find_and_replace( $odin_config_file_path, "YYY",
		$architecture_file_name );
	file_find_and_replace( $odin_config_file_path, "ZZZ",
		$odin_output_file_name );
	file_find_and_replace( $odin_config_file_path, "PPP", $mem_size );
	file_find_and_replace( $odin_config_file_path, "AAA", $min_hard_adder_size );

	if ( !$error_code ) {
		$q =
		  &system_with_timeout( "$odin2_path", "odin.out", $timeout, $temp_dir,
			"-c", $odin_config_file_name );

		if ( -e $odin_output_file_path ) {
			if ( !$keep_intermediate_files ) {
				system "rm -f ${temp_dir}*.dot";
				system "rm -f ${temp_dir}*.v";
				system "rm -f $odin_config_file_path";
			}
		}
		else {
			print "failed: odin";
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
	$q = &system_with_timeout( $abc_path, "abc.out", $timeout, $temp_dir, "-c",
		"read $odin_output_file_name; time; resyn; resyn2; if -K $lut_size; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; write_hie $odin_output_file_name $abc_output_file_name; print_stats"
	);

	if ( -e $abc_output_file_path ) {

		#system "rm -f abc.out";
		if ( !$keep_intermediate_files ) {
			system "rm -f $odin_output_file_path";
			system "rm -f ${temp_dir}*.rc";
		}
	}
	else {
		print "failed: abc";
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
	$q = &system_with_timeout(
		$ace_path, "ace.out",             $timeout, $temp_dir,
		"-b",      $abc_output_file_name, "-n",     $ace_output_blif_name,
		"-o",      $ace_output_act_name
	);

	if ( -e $ace_output_blif_path ) {
		if ( !$keep_intermediate_files ) {
			system "rm -f $abc_output_file_path";
			#system "rm -f ${temp_dir}*.rc";
		}
	}
	else {
		print "failed: ace";
		$error_code = 1;
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
		print "failed: prevpr";
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
	if ( $min_chan_width < 0 ) {
		$q = &system_with_timeout(
			$vpr_path,                    "vpr.out",
			$timeout,                     $temp_dir,
			$architecture_file_name,      "$benchmark_name",
			"--blif_file",				  "$prevpr_output_file_name",
			"--timing_analysis",          "$timing_driven",
			"--timing_driven_clustering", "$timing_driven",
			"--cluster_seed_type",        "$vpr_cluster_seed_type",
			"--sdc_file", 				  "$sdc_file_path",
			"--seed",			 		  "$seed",
			"--nodisp"
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

				if ( $content =~ m/(.*Error.*)/i ) {
					$error = $1;
				}

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
				$q = &system_with_timeout(
					$vpr_path,               "vpr.crit_path.out",
					$timeout,                $temp_dir,
					$architecture_file_name, "$benchmark_name",
					"--route",
					"--blif_file",           "$prevpr_output_file_name",
					"--route_chan_width",    "$min_chan_width",
					"--cluster_seed_type",   "$vpr_cluster_seed_type",
					"--max_router_iterations", "100",
					"--nodisp",              @vpr_power_args,
					"--gen_postsynthesis_netlist", "$gen_postsynthesis_netlist",
					"--sdc_file",			 "$sdc_file_path"
				);
			}
		}
	}
	else {
		$q = &system_with_timeout(
			$vpr_path,                    "vpr.out",
			$timeout,                     $temp_dir,
			$architecture_file_name,      "$benchmark_name",
			"--blif_file",                "$prevpr_output_file_name",
			"--timing_analysis",          "$timing_driven",
			"--timing_driven_clustering", "$timing_driven",
			"--route_chan_width",         "$min_chan_width",
			"--nodisp",                   "--cluster_seed_type",
			"$vpr_cluster_seed_type",     @vpr_power_args,
			"--gen_postsynthesis_netlist", "$gen_postsynthesis_netlist",
			"--sdc_file",				  "$sdc_file_path"
		);
	}
	  					
	if (-e $vpr_route_output_file_path and $q eq "success")
	{
		if($check_equivalent eq "on") {
			if($abc_path eq "") {
				$abc_path = "$vtr_flow_path/../abc_with_bb_support/abc";
			}
			$q = &system_with_timeout($abc_path, 
							"equiv.out",
							$timeout,
							$temp_dir,
							"-c", 
							"cec $prevpr_output_file_name post_pack_netlist.blif;sec $prevpr_output_file_name post_pack_netlist.blif"
			);
		}
		if (! $keep_intermediate_files)
		{
			system "rm -f $prevpr_output_file_name";
			system "rm -f ${temp_dir}*.xml";
			system "rm -f ${temp_dir}*.net";
			system "rm -f ${temp_dir}*.place";
			system "rm -f ${temp_dir}*.route";
			system "rm -f ${temp_dir}*.sdf";
			system "rm -f ${temp_dir}*.v";
			if ($do_power) {
				system "rm -f $ace_output_act_path";
			}
		}
	}
	else {
		print("failed: vpr");
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

	if ( $content =~ m/(.*Error.*)/i ) {
		$error = $1;
	}
}
print RESULTS "error=$error\n";

close(RESULTS);

# Clean up files not used that take up a lot of space

#system "rm -f *.blif";
#system "rm -f *.xml";
#system "rm -f core.*";
#system "rm -f gc.txt";

if ( !$error_code ) {
	system "rm -f *.echo";
	print "OK";
}
print "\n";

################################################################################
# Subroutine to execute a system call with a timeout
# system_with_timeout(<program>, <stdout file>, <timeout>, <dir>, <arg1>, <arg2>, etc)
#    make sure args is an array
# Returns: "timeout", "exited", "success", "crashed"
################################################################################
sub system_with_timeout {

	# Check args
	( $#_ > 2 )   or die "system_with_timeout: not enough args\n";
	( -f $_[0] )  or die "system_with_timeout: can't find executable $_[0]\n";
	( $_[2] > 0 ) or die "system_with_timeout: invalid timeout\n";

	# Save the pid of child process
	my $pid = fork;

	if ( $pid == 0 ) {

		# Redirect STDOUT for vpr
		chdir $_[3];

		
		open( STDOUT, "> $_[1]" );
		open( STDERR, ">&STDOUT" );
		

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
				return "exited";
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
		return ".pre-vpr.blif";
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
