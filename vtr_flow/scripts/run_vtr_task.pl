#!/usr/bin/perl
###################################################################################
# This script executes one or more VTR tasks
#
# Usage:
#	run_vtr_task.pl <task_name1> <task_name2> ... [OPTIONS]
#
# Options:
#   -s <script_params>:  Treat the remaining command line options as parameters
#               to forward to the VPR calling script (e.g. run_vtr_flow.pl).
# 	-p <N>:  Perform parallel execution using N threads. Note: Large benchmarks
#				will use very large amounts of memory (several gigabytes). Because
#				of this, parallel execution often saturates the physical memory,
#				requiring the use of swap memory, which will cause slower
#				execution. Be sure you have allocated a sufficiently large swap
#				memory or errors may result.
#   -j <N>: Same as -p <N>
#	-l <task_list_file>:  A file containing a list of tasks to execute. Each task
#							name should be on a separate line.
#
#	-hide_runtime: Do not show runtime estimates
#
# Note: At least one task must be specified, either directly as a parameter or
#		through the -l option.
#
# Authors: Jason Luu and Jeff Goeders
#
###################################################################################

use strict;

# This loads the thread libraries, but within an eval so that if they are not available
# the script will not fail.  If successful, $threaded = 1
my $threaded = eval 'use threads; use Thread::Queue; 1';

use Cwd;
use File::Spec;
use File::Basename;
use File::Path qw(make_path);
use List::MoreUtils qw(uniq);
use IPC::Open2;
use POSIX qw(strftime);

# Function Prototypes
sub trim;
sub run_single_task;
sub do_work;
sub ret_expected_runtime;

# Get Absolute Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*vtr_flow)/;
my $vtr_flow_path = $1;

my @tasks;
my @task_files;
my $token;
my $processors             = 1;
my $run_prefix             = "run";
my $show_runtime_estimates = 1;
my $system_type            = "local";
my $shared_script_params = "";

# Parse Input Arguments
while ( $token = shift(@ARGV) ) {

	# Check for -pN
	if ( $token =~ /^-p(\d+)$/ ) {
		$processors = int($1);
	}

	# Check for -jN
	if ( $token =~ /^-j(\d+)$/ ) {
		$processors = int($1);
	}

	# Check for -p N or -j N
	elsif ( $token eq "-p" or $token eq "-j" ) {
		$processors = int( shift(@ARGV) );
	}

    # Treat the remainder of the command line options as script parameters shared by all tasks
    elsif ( $token eq "-s" ) {
        $shared_script_params = join(' ', @ARGV);
        while ($token = shift(@ARGV)) {
            print "adding to shared script params: $token\n"
        }
        print "shared script params: $shared_script_params\n"
    }

	elsif ( $token eq "-system" ) {
		$system_type = shift(@ARGV);
	}

	# Check for a task list file
	elsif ( $token =~ /^-l(.+)$/ ) {
		push( @task_files, expand_user_path($1) );
	}
	elsif ( $token eq "-l" ) {
		push( @task_files, expand_user_path( shift(@ARGV) ) );
	}

	elsif ( $token eq "-hide_runtime" ) {
		$show_runtime_estimates = 0;
	}

	elsif ( $token =~ /^-/ ) {
		die "Invalid option: $token\n";
	}

	# must be a task name
	else {
		if ( $token =~ /(.*)\/$/ ) {
			$token = $1;
		}
		push( @tasks, $token );
	}
}

# Check threaded
if ( $processors > 1 and not $threaded ) {
	print
	  "Multithreaded option specified, but is not supported by this version of perl.  Execution will be single threaded.\n";
	$processors = 1;
}

# Read Task Files
foreach (@task_files) {
	open( FH, $_ ) or die "$! ($_)\n";
	while (<FH>) {
		push( @tasks, $_ );
	}
	close(FH);
}

# Remove duplicate tasks, use uniq() to preserve ordering
@tasks = uniq(@tasks);

#print "Processors: $processors\n";
#print "Tasks: @tasks\n";

if ( $#tasks == -1 ) {
	die "\n"
	  . "Incorect usage.  You must specify at least one task to execute\n"
	  . "\n"
	  . "USAGE:\n"
	  . "run_vtr_task.pl <TASK1> <TASK2> ... \n" . "\n"
	  . "OPTIONS:\n"
	  . "-l <path_to_task_list.txt> - Provides a text file with a list of tasks\n"
	  . "-p <N> - Execution is performed in parallel using N threads (Default: 1)\n";
}

##############################################################
# Run tasks
##############################################################

my $num_total_failures = 0;
foreach my $task (@tasks) {
	chomp($task);
	my $num_failures_in_task = run_single_task($task);
    $num_total_failures += $num_failures_in_task;
}

exit $num_total_failures;

##############################################################
# Subroutines
##############################################################

sub run_single_task {
	my $circuits_dir;
	my $archs_dir;
	my $sdc_dir        = "sdc";
	my $script_default = "run_vtr_flow.pl";
	my $script         = $script_default;
	my $script_path;
	my $script_params = $shared_script_params;  # start with the shared ones then build unique ones
	my @circuits;
	my @archs;
	my $cmos_tech_path = "";

	my $task     = shift(@_);
	my $task_dir = "$vtr_flow_path/tasks/$task";
	chdir($task_dir) or die "Task directory does not exist ($task_dir): $!";

	print "\n$task\n";
	print "-----------------------------------------\n";

	# Get Task Config Info

	my $task_config_file_path = "config/config.txt";
	open( CONFIG_FH, $task_config_file_path )
	  or die
	  "Cannot find task configuration file ($task_dir/$task_config_file_path)";
	while (<CONFIG_FH>) {
		my $line = $_;
		chomp($line);

        #Skip comment-only or blank lines
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

        #Trim off a line-ending comment
        $line =~ s/#.*$//;

		my @data  = split( /=/, $line );
		my $key   = trim( $data[0] );
		my $value = trim( $data[1] );
		if ( $key eq "circuits_dir" ) {
			$circuits_dir = $value;
		}
		elsif ( $key eq "archs_dir" ) {
			$archs_dir = $value;
		}
		elsif ( $key eq "sdc_dir" ) {
			$sdc_dir = $value;
		}
		elsif ( $key eq "circuit_list_add" ) {
			push( @circuits, $value );
		}
		elsif ( $key eq "arch_list_add" ) {
			push( @archs, $value );
		}
		elsif ( $key eq "script_path" ) {
			$script = $value;
		}
		elsif ( $key eq "script_params" ) {
			$script_params .= ' ' . $value;
		}
		elsif ( $key eq "cmos_tech_behavior" ) {
			$cmos_tech_path = $value;
		}
		elsif ($key eq "parse_file"
			or $key eq "qor_parse_file"
			or $key eq "pass_requirements_file" )
		{

			#Used by parser
		}
		else {
			die "Invalid option (" . $key . ") in configuration file.";
		}
	}

	# Using default script
	if ( $script eq $script_default ) {

		# This is hack to automatically add the option '-temp_dir .' if using the run_vtr_flow.pl script
		# This ensures that a 'temp' folder is not created in each circuit directory
		if ( !( $script_params =~ /-temp_dir/ ) ) {
            #-temp_dir must come before the script_params, so that it gets picked up by run_vtr_flow
            # and not passed on as an argument to a tool (e.g. VPR)
 			$script_params = " -temp_dir . " . $script_params;
		}
	}
	else {
		$show_runtime_estimates = 0;
	}

	$circuits_dir = expand_user_path($circuits_dir);
	$archs_dir    = expand_user_path($archs_dir);
	$sdc_dir      = expand_user_path($sdc_dir);

	if ( -d "$vtr_flow_path/$circuits_dir" ) {
		$circuits_dir = "$vtr_flow_path/$circuits_dir";
	}
	elsif ( -d $circuits_dir ) {
	}
	else {
		die "Circuits directory not found ($circuits_dir)";
	}

	if ( -d "$vtr_flow_path/$archs_dir" ) {
		$archs_dir = "$vtr_flow_path/$archs_dir";
	}
	elsif ( -d $archs_dir ) {
	}
	else {
		die "Archs directory not found ($archs_dir)";
	}

	if ( -d "$vtr_flow_path/$sdc_dir" ) {
		$sdc_dir = "$vtr_flow_path/$sdc_dir";
	}
	elsif ( -d $sdc_dir ) {
	}
	else {
		$sdc_dir = "$vtr_flow_path/sdc";
	}

	(@circuits) or die "No circuits specified for task $task";
	(@archs)    or die "No architectures specified for task $task";

	# Check script
	$script = expand_user_path($script);
	if ( -e "$task_dir/config/$script" ) {
		$script_path = "$task_dir/config/$script";
	}
	elsif ( -e "$vtr_flow_path/scripts/$script" ) {
		$script_path = "$vtr_flow_path/scripts/$script";
	}
	elsif ( -e $script ) {
	}
	else {
		die
		  "Cannot find script for task $task ($script).  Looked for $task_dir/config/$script or $vtr_flow_path/scripts/$script";
	}

    # Check architectures
    foreach my $arch (@archs) {
        (-f "$archs_dir/$arch") or die "Architecture file not found ($archs_dir/$arch)";
    }

    # Check circuits
    foreach my $circuit (@circuits) {
        (-f "$circuits_dir/$circuit") or die "Circuit file not found ($circuits_dir/$circuit)";
    }

	# Check CMOS tech behavior
	if ( $cmos_tech_path ne "" ) {
		$cmos_tech_path = expand_user_path($cmos_tech_path);
		if ( -e "$task_dir/config/$cmos_tech_path" ) {
			$cmos_tech_path = "$task_dir/config/$cmos_tech_path";
		}
		elsif ( -e "$vtr_flow_path/tech/$cmos_tech_path" ) {
			$cmos_tech_path = "$vtr_flow_path/tech/$cmos_tech_path";
		}
		elsif ( -e $cmos_tech_path ) {
		}
		else {
			die
			  "Cannot find CMOS technology behavior file for $task ($script). Looked for $task_dir/config/$cmos_tech_path or $vtr_flow_path/tech/$cmos_tech_path";
		}
		$script_params = $script_params . " -cmos_tech $cmos_tech_path";
	}

	# Check if golden file exists
	my $golden_results_file = "$task_dir/config/golden_results.txt";

	##############################################################
	# Create a new experiment directory to run experiment in
	# Counts up until directory number doesn't exist
	##############################################################
	my $experiment_number = 0;

    my $run_dir = "";
    my $run_dir_no_prefix = "";
    do {
        $experiment_number += 1;
        $run_dir = sprintf("run%03d", $experiment_number);
        $run_dir_no_prefix = sprintf("run%d", $experiment_number);
    } while (-e $run_dir or -e $run_dir_no_prefix);

	mkdir( $run_dir, 0775 ) or die "Failed to make directory ($run_dir): $!";
	chmod( 0775, $run_dir );
	chdir($run_dir) or die "Failed to change to directory ($run_dir): $!";

	# Create the directory structure
	# Make this seperately from file script
	# just in case failure occurs creating directory
	foreach my $arch (@archs) {
		make_path( "$arch", { mode => 0775 } ) or die "Failed to create directory ($arch): $!";
		chmod( 0775, "$arch" );
		foreach my $circuit (@circuits) {
			mkdir( "$arch/$circuit", 0775 )
			  or die "Failed to create directory $arch/$circuit: $!";
			chmod( 0775, "$arch/$circuit" );
		}
	}

	##############################################################
    # Build up the list of commands to run
	##############################################################
    my @actions;
    foreach my $circuit (@circuits) {
        foreach my $arch (@archs) {

            #Determine the directory where to run
            my $dir = "$task_dir/$run_dir/${arch}/${circuit}";


            #Build the command to run
            my $command = "$script_path $circuits_dir/$circuit $archs_dir/$arch $script_params" ;

            #Determine the SDC file name
            my $sdc_name = fileparse( $circuit, '\.[^.]+$' ) . ".sdc";
            my $sdc = "$sdc_dir/$sdc_name";
            if( -r $sdc) {
                $command .= " -sdc_file $sdc";
            }

            #Add a hint about the minimum channel width (potentially saves run-time)
            my $expected_min_W = ret_expected_min_W($circuit, $arch, $golden_results_file);
            if($expected_min_W > 0) {
                $command .= " -min_route_chan_width_hint $expected_min_W";
            }

            #Estimate runtime
            my $runtime_estimate = ret_expected_runtime($circuit, $arch, $golden_results_file);

            my @action = [$dir, $command, $runtime_estimate];
            push(@actions, @action);
        }
    }

	##############################################################
	# Run experiment
	##############################################################
    my $num_failures = 0;


	if ( $system_type eq "local" ) {
		if ( $processors == 1 ) {
            foreach my $action (@actions) {
                my ($run_dir, $command, $runtime_estimate) = @$action;
                if($runtime_estimate >= 0) {
                    print strftime "Current time: %b-%d %I:%M %p.  ", localtime;
                    print "Expected runtime of next benchmark: " . $runtime_estimate . "\n";
                }

                chdir( $run_dir );

                my $status = system( $command );

                my $return_code = $status >> 8; #Must shift by 8 bits to get real exit code

                if($return_code != 0) {
                    $num_failures += 1;
                }
			}
		} else {
			my $thread_work   = Thread::Queue->new();
			my $thread_result = Thread::Queue->new();
			my $thread_return_code = Thread::Queue->new();
			my $threads       = $processors;

            foreach my $action (@actions) {
                my ($run_dir, $command, $runtime_estimate) = @$action;

                $thread_work->enqueue("$run_dir||||$command");

            }

			my @pool = map { threads->create( \&do_work, $thread_work, $thread_result, $thread_return_code ) } 1 .. $threads;

			for ( 1 .. $threads ) {
				while ( my $result = $thread_result->dequeue ) {
					chomp($result);
					print $result . "\n";
				}
                while (my $return_code = $thread_return_code->dequeue) {
                    if($return_code != 0) {
                        $num_failures += 1;
                    }
                }
			}

			$_->join for @pool;
		}
	} else {
        die("Unrecognized job system '$system_type'");
    }
    return $num_failures;
}

sub do_work {
	my ( $work_queue, $return_queue, $return_code_queue ) = @_;
	my $tid = threads->tid;

	while (1) {
		my $work    = $work_queue->dequeue_nb();
		my @work    = split( /\|\|\|\|/, $work );
		my $dir     = @work[0];
		my $command = @work[1];

		if ( !$dir ) {
			last;
		}

		my $return_status = system "cd $dir; $command > thread_${tid}.out";
        my $exit_code = $return_status >> 8; #Shift to get real exit code

		open( OUT_FILE, "$dir/thread_${tid}.out" )
		  or die "Cannot open $dir/thread_${tid}.out: $!";
		my $sys_output = do { local $/; <OUT_FILE> };

		#$sys_output =~ s/\n//g;
		$return_queue->enqueue($sys_output);

        $return_code_queue->enqueue($exit_code);
	}
    #Need to put undef in queues to indicate end and prevent blocking forever
	$return_queue->enqueue(undef);
    $return_code_queue->enqueue(undef);

	#print "$tid exited loop\n"
}

# trim whitespace
sub trim($) {
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

sub expand_user_path {
	my $str = shift;
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}

sub ret_expected_runtime {
	my $circuit_name             = shift;
	my $arch_name                = shift;
	my $golden_results_file_path = shift;
	my $seconds                  = 0;

    if( not -r $golden_results_file_path) {
        return "Unkown";
    }

	open( GOLDEN, $golden_results_file_path );
	my @lines = <GOLDEN>;
	close(GOLDEN);

	my $header_line = shift(@lines);
	my @headers     = split( /\s+/, $header_line );

	my %index;
	@index{@headers} = ( 0 .. $#headers );

	my @line_array;
	my $found = 0;
	foreach my $line (@lines) {
		@line_array = split( /\s+/, $line );
		if ( $arch_name eq @line_array[0] and $circuit_name eq @line_array[1] )
		{
			$found = 1;
			last;
		}
	}

	if ( not $found ) {
		return "Unknown";
	}

	my $location = $index{"pack_time"};
	if ($location) {
        my $val = @line_array[$location];
        if($val > 0.) {
            $seconds += $val;
        }
	}
	my $location = $index{"place_time"};
	if ($location) {
        my $val = @line_array[$location];
        if($val > 0.) {
            $seconds += $val;
        }
	}
	my $location = $index{"min_chan_width_route_time"};
	if ($location) {
        my $val = @line_array[$location];
        if($val > 0.) {
            $seconds += $val;
        }
	}
	my $location = $index{"crit_path_route_time"};
	if ($location) {
        my $val = @line_array[$location];
        if($val > 0.) {
            $seconds += $val;
        }
	}
	my $location = $index{"route_time"};
	if ($location) {
        my $val = @line_array[$location];
        if($val > 0.) {
            $seconds += $val;
        }
	}

	if ( $seconds != 0 ) {
		if ( $seconds < 60 ) {
			my $str = sprintf( "%.0f seconds", $seconds );
			return $str;
		}
		elsif ( $seconds < 3600 ) {
			my $min = $seconds / 60;
			my $str = sprintf( "%.0f minutes", $min );
			return $str;
		}
		else {
			my $hour = $seconds / 60 / 60;
			my $str = sprintf( "%.0f hours", $hour );
			return $str;
		}
	}
	else {
		return "Unknown";
	}
}

sub ret_expected_min_W {
	my $circuit_name             = shift;
	my $arch_name                = shift;
	my $golden_results_file_path = shift;
	my $seconds                  = 0;

    my $expected_min_W = -1;

    if( -r $golden_results_file_path) {
        #File exists, look-up the golden min channel width
        open( GOLDEN, $golden_results_file_path );
        my @lines = <GOLDEN>;
        close(GOLDEN);

        my $header_line = shift(@lines);
        my @headers     = split( /\s+/, $header_line );

        my %index;
        @index{@headers} = ( 0 .. $#headers );


        my @line_array;
        my $found = 0;
        foreach my $line (@lines) {
            @line_array = split( /\s+/, $line );
            if ( $arch_name eq @line_array[0] and $circuit_name eq @line_array[1] )
            {
                $found = 1;
                last;
            }
        }

        if ($found) {
            my $location = $index{"min_chan_width"};
            if ($location) {
                $expected_min_W = @line_array[$location];
            } 
        }
    }

    return $expected_min_W;
}
