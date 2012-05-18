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
use Cwd 'abs_path';
use File::Spec;
use POSIX;
use File::Copy;

# check the parametes.  Note PERL does not consider the script itself a parameter.
my $number_arguments = @ARGV;
if ($number_arguments < 2)
{
	print("usage: run_vtr_flow.pl <circuit_file> <architecture_file> [OPTIONS]\n");
	exit(-1);
}


# Get Absoluate Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*\/vtr_flow)\//;
my $vtr_flow_path = $1;

sub stage_index;
sub file_ext_for_stage;
sub expand_user_path;
sub file_find_and_replace;

my $temp_dir = "./temp";

my $stage_idx_odin = 1;
my $stage_idx_abc = 2;
my $stage_idx_script = 3;
my $stage_idx_vpr = 4;

my $circuit_file_path = expand_user_path(shift(@ARGV));
my $architecture_file_path = expand_user_path(shift(@ARGV));

my $token;
my $starting_stage = stage_index("odin");
my $ending_stage = stage_index("vpr");
my $keep_intermediate_files = 0;
my $has_memory = 1;
my $timing_driven = "on";
my $min_chan_width = -1;
my $lut_size = -1;
my $vpr_cluster_seed_type = "";


while ($token = shift(@ARGV))
{
	if ($token eq "-starting_stage")
	{
		$starting_stage = stage_index(shift(@ARGV));
	}
	elsif ($token eq "-ending_stage")
	{
		$ending_stage = stage_index(shift(@ARGV));
	}
	elsif ($token eq "-keep_intermediate_files")
	{
		$keep_intermediate_files = 1;
	}
	elsif ($token eq "-no_mem")
	{
		$has_memory = 0;
	}
	elsif ($token eq "-no_timing")
	{
		$timing_driven = "off";
	}	
	elsif ($token eq "-vpr_route_chan_width")
	{
		$min_chan_width = shift(@ARGV);
	}
	elsif ($token eq "-lut_size")
	{
		$lut_size = shift(@ARGV);
	}
	elsif ($token eq "-vpr_cluster_seed_type")
	{
		$vpr_cluster_seed_type = shift(@ARGV);
	}
	elsif ($token eq "-temp_dir")
	{
		$temp_dir = shift(@ARGV);
	}
	else
	{
		die "Error: Invalid argument ($token)\n";
	}
	
	if ($starting_stage == -1 or $ending_stage == -1)
	{
		die "Error: Invalid starting/ending stage name (start $starting_stage end $ending_stage).\n";
	}
}

if ($ending_stage < $starting_stage)
{
	die "Error: Ending stage is before starting stage.\n";
}

if($vpr_cluster_seed_type eq "") {
	if($timing_driven eq "off") {
		$vpr_cluster_seed_type = "max_inputs";
	} else {
		$vpr_cluster_seed_type = "timing";
	}
}

if (! -d $temp_dir)
{
	system "mkdir $temp_dir";
}
-d $temp_dir or die "Could not make temporary directory ($temp_dir)\n";
if (! ($temp_dir =~ /.*\/$/))
{
	$temp_dir = $temp_dir . "/";
}

my $timeout = 10*24*60*60; # 10 day execution timeout
my $results_path = "${temp_dir}output.txt";

my $error;
my $error_code = 0;

my $arch_param;
my $cluster_size;
my $inputs_per_cluster = -1;

# Test for file existance
(-f $circuit_file_path) or die "Circuit file not found ($circuit_file_path)";
(-f $architecture_file_path) or die "Architecture file not found ($architecture_file_path)";

# Select executables
my $vpr_path = "$vtr_flow_path/../vpr/vpr";
(-e $vpr_path) or die "Cannot find vpr exectuable ($vpr_path)";

my $odin2_path = "$vtr_flow_path/../ODIN_II/odin_II.exe";
(-e $odin2_path) or die "Cannot find ODIN_II executable ($odin2_path)";

my $abc_path = "$vtr_flow_path/../abc_with_bb_support/abc";
(-e $abc_path) or die "Cannot find ABC executable ($abc_path)";

my $hack_fix_lines_n_latches = "$vtr_flow_path/scripts/hack_fix_lines_and_latches.pl";
(-e $hack_fix_lines_n_latches) or die "Cannot find line/latch fixing script ($hack_fix_lines_n_latches)";

# Select files
my $abc_rc_path = "$vtr_flow_path/../abc_with_bb_support/abc.rc";
(-e $abc_rc_path) or die "Cannot find ABC RC file ($abc_rc_path)";

my $clock_path = "$vtr_flow_path/benchmarks/misc/benchmark_clocks.clock";
(-e $clock_path) or die "Cannot find benchmark clock file ($clock_path)";

my $clock_path_user = "$vtr_flow_path/benchmarks/misc/user_clocks.clock";

my $odin_config_file_name = "basic_odin_config_split.xml";
my $odin_config_file_path = "$vtr_flow_path/misc/$odin_config_file_name";
(-e $odin_config_file_path) or die "Cannot find ODIN config template ($odin_config_file_path)";

# Get circuit name (everything up to the first '.' in the circuit file)
my ($vol, $path, $circuit_file_name) = File::Spec->splitpath( $circuit_file_path);
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

open(ARCH_IN_FILE, 	$architecture_file_path);
my $arch_contents = do { local $/; <ARCH_IN_FILE> };
close(ARCH_IN_FILE);

if($has_memory == 1)
{
	open(ARCH_IN_FILE, 	$architecture_file_path);
	while (<ARCH_IN_FILE>)
	{
		$line = $_;
		chomp($line);
		if ($line =~ m/pb_type name="memory"/)
		{
			$in_memory_block = 1;
		}
		elsif ($in_memory_block && !$in_mode && $line =~ m/<\/pb_type>/)
		{
			$in_memory_block = 0;
		}
		
		if ($in_memory_block && $line =~ m/<mode.*?>/)
		{
			$in_mode = 1;
		}
		elsif ($in_memory_block && $in_mode && $line =~ m/<\/mode>/)
		{
			$in_mode = 0;
		}
		
		if ($in_memory_block && $in_mode)
		{
			if ($line =~ m/<input name="addr" num_pins="(\d+)"/)
			{
				if (int($1) > $mem_size)
				{
					$mem_size = int($1)
				}
			}
		}	
	}
	if ($mem_size < 0)
	{
		print "failed: cannot determine arch mem size";
		$error_code = 1;
	}
	close(ARCH_IN_FILE);
} else {
	$mem_size = 0;
}

# Get lut size
if (! $error_code)
{
	if($lut_size < 0) {
		if ($arch_contents =~ m/pb_type name="clb".*?pb_type name="ble".*?input.*?num_pins="(\d+)"/s)
		{
			$lut_size = $1;
		}
		else
		{
			print "failed: cannot determine arch LUT k-value";
			$error_code = 1;
		}			
	}
}



my $odin_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_odin);
my $odin_output_file_path = "$temp_dir$odin_output_file_name";

my $abc_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_abc);
my $abc_output_file_path = "$temp_dir$abc_output_file_name";

my $scripts_output_file_name;
if($starting_stage == stage_index("vpr")) 
{
	$scripts_output_file_name = "$benchmark_name" . ".blif";
} else {
	$scripts_output_file_name = "$benchmark_name" . file_ext_for_stage($stage_idx_script);
}
my $scripts_output_file_path = "$temp_dir$scripts_output_file_name";

my $vpr_route_output_file_name = "$benchmark_name.route";
my $vpr_route_output_file_path = "$temp_dir$vpr_route_output_file_name";


#system "cp $abc_rc_path $temp_dir";
#system "cp $architecture_path $temp_dir";
#system "cp $circuit_path $temp_dir/$benchmark_name" . file_ext_for_stage($starting_stage - 1);
#system "cp $odin2_base_config"

my $odin_config_file_name = "odin_config.xml";
my $odin_config_file_path_new = "$temp_dir" . "odin_config.xml";
copy ($odin_config_file_path, $odin_config_file_path_new);
$odin_config_file_path = $odin_config_file_path_new;

my $architecture_file_path_new = "$temp_dir$architecture_file_name";
copy ($architecture_file_path, $architecture_file_path_new);
$architecture_file_path = $architecture_file_path_new;

my $circuit_file_path_new = "$temp_dir$circuit_file_name";
copy ($circuit_file_path, $circuit_file_path_new);
$circuit_file_path = $circuit_file_path_new; 

copy ($abc_rc_path, $temp_dir);

# Call executable and time it
my $StartTime = time;
my $q = "not_run";


#################################################################################
################################## ODIN #########################################
#################################################################################

if ($starting_stage <= $stage_idx_odin and ! $error_code)
{
	#system "sed 's/XXX/$benchmark_name.v/g' < $odin2_base_config > temp1.xml";
	#system "sed 's/YYY/$arch_name/g' < temp1.xml > temp2.xml";
	#system "sed 's/ZZZ/$odin_output_file_path/g' < temp2.xml > temp3.xml";
	#system "sed 's/PPP/$mem_size/g' < temp3.xml > circuit_config.xml";
	
	file_find_and_replace($odin_config_file_path, "XXX", $circuit_file_name);
	file_find_and_replace($odin_config_file_path, "YYY", $architecture_file_name);
	file_find_and_replace($odin_config_file_path, "ZZZ", $odin_output_file_name);
	file_find_and_replace($odin_config_file_path, "PPP", $mem_size);

	if (! $error_code)
	{
		$q = &system_with_timeout("$odin2_path", 
								   "odin.out",
								   $timeout,
								   $temp_dir,
								   "-c", $odin_config_file_name
		);
		
		if (-e $odin_output_file_path)
		{
			if (! $keep_intermediate_files)
			{
				system "rm -f ${temp_dir}*.dot";
				system "rm -f ${temp_dir}*.v";
				system "rm -f $odin_config_file_path";
			}
		}
		else
		{
			print "failed: odin";
			$error_code = 1;
		}
	}
}

#################################################################################
################################## ABC ##########################################
#################################################################################
if ($starting_stage <= $stage_idx_abc and $ending_stage >= $stage_idx_abc and ! $error_code)
{
	$q = &system_with_timeout($abc_path, 
                          		"abc.out",
                          		$timeout,
                          		$temp_dir,
								"-c", 
								"read $odin_output_file_name; time; resyn; resyn2; if -K $lut_size -a; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; scleanup; time; write_hie $odin_output_file_name $abc_output_file_name; print_stats"
	);
	
	if (-e $abc_output_file_path)
	{
		#system "rm -f abc.out";
		if (! $keep_intermediate_files)
		{
			system "rm -f $odin_output_file_path";
			system "rm -f ${temp_dir}*.rc";
		}
	}
	else
	{
		print "failed: abc";
		$error_code = 1;
	}	
}

#################################################################################
################################## FIX SCRIPTS ##################################
#################################################################################
if ($starting_stage <= $stage_idx_script and $ending_stage >= $stage_idx_script and ! $error_code)
{
	my $scripts_success = 0;
	my $stage_output_file = "${temp_dir}fix_scripts.out";
	system "$hack_fix_lines_n_latches $abc_output_file_path $scripts_output_file_path $clock_path $clock_path_user > $stage_output_file";
	
	if (-r $scripts_output_file_path)
	{
		open(OUTPUT_FILE, $stage_output_file);
		my $file_contents = do { local $/; <OUTPUT_FILE> };
		close(OUTPUT_FILE); 
		
		if ($file_contents =~ /^Success$/m)
		{
			$scripts_success = 1;
		}
	}
	
	if ($scripts_success)	
	{
		if (! $keep_intermediate_files)
		{
			system "rm -f $abc_output_file_path";
		}
	}
	else
	{
		print "failed: scripts";
		$error_code = 1;
	}	
}


#################################################################################
################################## VPR ##########################################
#################################################################################

if ($ending_stage >= $stage_idx_vpr and ! $error_code)
{
	if($min_chan_width < 0) {
		$q = &system_with_timeout($vpr_path, 
									"vpr.out",
									$timeout,
									$temp_dir,
									$architecture_file_name,
									"$benchmark_name",
									"--blif_file", "$scripts_output_file_name",									
									"--timing_analysis", "$timing_driven",
									"--timing_driven_clustering", "$timing_driven",
									"--cluster_seed_type", "$vpr_cluster_seed_type",
									"--nodisp"									
		);
		if($timing_driven eq "on") {
			# Critical path delay is nonsensical at minimum channel width because congestion constraints completely dominate the cost function.
			# Additional channel width needs to be added so that there is a reasonable trade-off between delay and area
			# Commercial FPGAs are also desiged to have more channels than minimum for this reason

			# Parse out min_chan_width
			if(open(VPROUT, "<${temp_dir}vpr.out"))
			{
				undef $/;
				my $content = <VPROUT>;
				close (VPROUT);
				$/ = "\n";     # Restore for normal behaviour later in script
				
				if ($content =~ m/(.*Error.*)/i) {
					$error = $1;
				}
				
				if ($content =~ /Best routing used a channel width factor of (\d+)/m) {
					$min_chan_width = $1 ;
				}		
			}			

			$min_chan_width = ($min_chan_width * 1.3);
			$min_chan_width = floor($min_chan_width);
			if($min_chan_width % 2) {
				$min_chan_width = $min_chan_width + 1;
			}

			if (-e $vpr_route_output_file_path)
			{
				system "rm -f $vpr_route_output_file_path";
				
				$q = &system_with_timeout($vpr_path, 
										"vpr.crit_path.out",
										$timeout,
										$temp_dir,
										$architecture_file_name,
										"$benchmark_name",									
										"--place_file", "$benchmark_name.place",									
										"--route",
										"--route_chan_width", "$min_chan_width",
										"--cluster_seed_type", "$vpr_cluster_seed_type",
										"--nodisp",
				);
			}
		}
	} else {
		$q = &system_with_timeout($vpr_path, 
									"vpr.out",
									$timeout,
									$temp_dir,
									$architecture_file_path,
									"$benchmark_name",
									"--blif_file", "$scripts_output_file_name",								
									"--timing_analysis", "$timing_driven",
									"--timing_driven_clustering", "$timing_driven",
									"--route_chan_width", "$min_chan_width",
									"--nodisp",
									"--cluster_seed_type", "$vpr_cluster_seed_type"						
		);
	}
			
	  					
	if (-e $vpr_route_output_file_path and $q eq "success")
	{
		if (! $keep_intermediate_files)
		{
			system "rm -f $scripts_output_file_path";
			system "rm -f ${temp_dir}*.xml";
			system "rm -f ${temp_dir}*.net";
			system "rm -f ${temp_dir}*.place";
			system "rm -f ${temp_dir}*.route";			
		}
	}
	else
	{
		print ("failed: vpr");
		$error_code = 1;
	}
}

my $EndTime = time;

# Determine running time
my $seconds = ($EndTime - $StartTime);
my $runseconds = $seconds % 60;

# Start collecting results to output.txt
open(RESULTS, "> $results_path");

# Output vpr status and runtime
print RESULTS "vpr_status=$q\n";
print RESULTS "vpr_seconds=$seconds\n";

# Parse VPR output
if(open(VPROUT, "< vpr.out"))
{
    undef $/;
    my $content = <VPROUT>;
    close (VPROUT);
    $/ = "\n";     # Restore for normal behaviour later in script
    
    if ($content =~ m/(.*Error.*)/i) {
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

if (! $error_code)
{
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
sub system_with_timeout
{
    # Check args
    ($#_ > 2) or die "system_with_timeout: not enough args\n";
    (-f $_[0]) or die "system_with_timeout: can't find executable\n";
    ($_[2] > 0) or die "system_with_timeout: invalid timeout\n";

    # Save the pid of child process
    my $pid = fork;
    
    if ($pid == 0)
    {    	 
        # Redirect STDOUT for vpr
        chdir $_[3];
        
        open(STDOUT, "> $_[1]");
        open(STDERR, "> $_[1]");
	

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
		print "$_[0] @VPRARGS\n";
        exec $_[0], @VPRARGS;
    }
    else
    {
        my $timed_out = "false";
        
        # Register signal handler, to kill child process (SIGABRT)
        $SIG{ALRM} = sub { kill 6, $pid; $timed_out = "true"; };

        # Register handlers to take down child if we are killed (SIGHUP)
        $SIG{INTR} = sub { print "SIGINTR\n"; kill 1, $pid; exit; };
        $SIG{HUP} = sub { print "SIGHUP\n"; kill 1, $pid; exit; };
        
        # Set SIGALRM timeout
        alarm $_[2];

        # Wait for child process to end OR timeout to expire
        wait;

        # Unset the alarm in case we didn't timeout
        alarm 0;
        
        # Check if timed out or not
        if ($timed_out eq "true")
        {
            return "timeout";
        }
        else
        {
            my $did_crash = "false";
            if ($? & 127) { $did_crash = "true"; };

            my $return_code = $? >> 8;

            if ($did_crash eq "true")
            {
                return "crashed";
            }
            elsif ($return_code != 0)
            {
                return "exited";
            }
            else
            {
                return "success";
            }
        }
    }
}

sub stage_index
{
	my $stage_name = $_[0];
	
	if (lc($stage_name) eq "odin")
	{
		return $stage_idx_odin;
	}
	if (lc($stage_name) eq "abc")
	{
		return $stage_idx_abc;
	}
	if (lc($stage_name) eq "scripts")
	{
		return $stage_idx_script;
	}
	if (lc($stage_name) eq "vpr")
	{
		return $stage_idx_vpr;
	}
	return -1;	
}

sub file_ext_for_stage
{
	my $stage_idx = $_[0];
	
	if ($stage_idx == 0)
	{
		return ".v";
	}
	elsif ($stage_idx == $stage_idx_odin)
	{
		return ".odin.blif";
	}
	elsif ($stage_idx == $stage_idx_abc)
	{
		return ".abc.blif";
	}
	elsif ($stage_idx == $stage_idx_script)
	{
		return ".pre-vpr.blif";
	}		
}

sub expand_user_path
{
	my $str = shift;
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}

sub file_find_and_replace
{
	my $file_path = shift();
	my $search_string = shift();
	my $replace_string = shift();
	
	open (FILE_IN, "$file_path");
	my $file_contents =  do { local $/; <FILE_IN>};
	close(FILE_IN);
	
	$file_contents =~ s/$search_string/$replace_string/mg;
	
	open (FILE_OUT, ">$file_path");
	print FILE_OUT $file_contents;
	close (FILE_OUT);
}