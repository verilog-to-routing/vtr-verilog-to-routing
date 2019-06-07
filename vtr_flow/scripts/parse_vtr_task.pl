#!/usr/bin/perl

###################################################################################
# This script is used to extract and verify statistics of one or more VTR tasks.
#
# Usage:
#	parse_vtr_task.pl <task_name1> <task_name2> ... [OPTIONS]
#
# Options:
# 	-l <task_list_file>: Used to provide a test file containing a list of tasks
#   -create_golden:  Will create/overwrite the golden results with those of the
#						most recent execution
#   -check_golden:  Will verify the results of the most recent execution against
#						the golden results for each task and report either a
#						[Pass] or [Fail]
#   -parse_qor:  	Used for the purposes of parsing quality of results of the
#					most recent execution.
#   -calc_geomean:  Used for the purposes of computing quality of results geomeans 
# 					of the most recent execution.
#
# Authors: Jason Luu and Jeff Goeders
###################################################################################

use strict;
use Cwd;
use File::Spec;
use File::Copy;
use File::Basename;
use List::Util;
use Math::BigInt;
use POSIX qw/strftime/;

# Function Prototypes
sub trim;
sub parse_single_task;
sub pretty_print_table;
sub summarize_qor;
sub calc_geomean;
sub check_golden;
sub expand_user_path;
sub get_important_file;
sub parse_single_flow_run;
sub aggregate_single_flow_runs;

# Get Absolute Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*vtr_flow)/;
my $vtr_flow_path = $1;

my $run_prefix = "run";

my $FAILED_PARSE_EXIT_CODE = -1;

# Parse Input Arguments
my @tasks;
my @task_files;
my $token;
my $create_golden = 0;
my $check_golden  = 0;
my $parse_qor 	  = 1;  # QoR file is parsed by default; turned off if 
						# user does not specify QoR parse file in config.txt
my $calc_geomean  = 0;  # QoR geomeans are not computed by default;
my $override_exp_id       = 0;
my $revision;
my $verbose       = 0;
my $pretty_print_results = 1;

while ( $token = shift(@ARGV) ) {

	# Check for a task list file
	if ( $token =~ /^-l(.+)$/ ) {
		push( @task_files, expand_user_path($1) );
	}
	elsif ( $token eq "-l" ) {
		push( @task_files, expand_user_path( shift(@ARGV) ) );
	}
	elsif ( $token eq "-create_golden" ) {
		$create_golden = 1;
	}
	elsif ( $token eq "-check_golden" ) {
		$check_golden = 1;
	}
	elsif ( $token eq "-parse_qor" ) {
		$parse_qor = 1;
	}
	elsif ( $token eq "-calc_geomean" ) {
		$calc_geomean = 1;
	}
	elsif ( $token eq "-run") {
	    $override_exp_id = shift(@ARGV);
    }
	elsif ( $token eq "-revision" ) {
		$revision = shift(@ARGV);
	}
	elsif ( $token eq "-v" ) {
		$verbose = 1;
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

# Read Task Files
foreach (@task_files) {
	open( FH, $_ ) or die "$! ($_)\n";
	while (<FH>) {
		push( @tasks, $_ );
	}
	close(FH);
}

my $num_golden_failures = 0;
foreach my $task (@tasks) {
	chomp($task);
	my $failures = parse_single_task($task);
    $num_golden_failures += $failures; 
}

if ($calc_geomean) {
	summarize_qor;
	calc_geomean;
}

exit $num_golden_failures;

sub parse_single_task {
	my $task_name = shift;
	(my $task_path = $task_name) =~ s/\s+$//;

    # first see if task_name is the task path
	if (! -e "$task_path/config/config.txt") {
	    ($task_path = "$vtr_flow_path/tasks/$task_name") =~ s/\s+$//;
    }

	open( CONFIG, "<$task_path/config/config.txt" )
	  or die "Failed to open $task_path/config/config.txt: $!\n";
	my @config_data = <CONFIG>;
	close(CONFIG);

	my @archs_list;
	my @circuits_list;
	my @script_params_list;
	my $parse_file;
	my $qor_parse_file;
        my $second_parse_file;
        my $counter = 0;
	foreach my $line (@config_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

        #Trim off a line-ending comment
        $line =~ s/#.*$//;

		my @data  = split( /=/, $line );
		my $key   = trim( $data[0] );
		my $value = trim( $data[1] );

		if ( $key eq "circuit_list_add" ) {
			push( @circuits_list, $value );
		} elsif ( $key eq "arch_list_add" ) {
			push( @archs_list, $value );
		} elsif ( $key eq "script_params_list_add" ) {
			push( @script_params_list, $value );
		} elsif ( $key eq "parse_file" ) {
                        if ($counter eq 1){
                                    #second time parse file
                                    $second_parse_file = expand_user_path($value);
                                    $counter = 0;
                                    #don't need to check golden, only compare between two files
                                    $check_golden = 0;
                        }
                        else{
                            $parse_file = expand_user_path($value);
                            $counter = $counter + 1;
                        }
		} elsif ( $key eq "qor_parse_file" ) {
			$qor_parse_file = expand_user_path($value);
		}
	}

    if (!@script_params_list) {
        #Add a default empty param if none otherwise specified
        #(i.e. only base params)
        push(@script_params_list, "");
    }

	# PARSE CONFIG FILE
	if ( $parse_file eq "" ) {
		die "Task $task_name has no parse file specified.\n";
	}
	$parse_file = get_important_file($task_path, $vtr_flow_path, $parse_file);

        if ($second_parse_file){
            $second_parse_file = get_important_file($task_path, $vtr_flow_path, $second_parse_file);
	}

	# Get Max Run #
	opendir(DIR, $task_path);
	my @folders = readdir(DIR);
	closedir(DIR);
	# QOR PARSE CONFIG FILE
	if ( $qor_parse_file eq "" ) {
		print "Task $task_name has no QoR parse file specified. Skipping QoR.\n";
		$parse_qor = 0;
		$calc_geomean = 0;
	}
	else {
	    $qor_parse_file = get_important_file($task_path, $vtr_flow_path, $qor_parse_file);
    }

    my $exp_id = 0;
    if($override_exp_id != 0) {
        #explicitely specified via -run parameter
        $exp_id = $override_exp_id;
    } else {
        # haven't explicitely specified via -run parameter
        $exp_id = last_exp_id(${task_path});
    }
	
	my $run_path = "$task_path/${run_prefix}${exp_id}";

    my @first_run_result_files;
    my @second_run_result_files;
    my @qor_run_result_files;

	foreach my $arch (@archs_list) {
		foreach my $circuit (@circuits_list) {
            foreach my $script_params (@script_params_list) {
                my $script_params_dirname = "common";
                if ($script_params ne "") {
                    $script_params_dirname .= "_" . $script_params;
                    $script_params_dirname =~ s/ /_/g;
                }

                #First run parse
                my $work_dir = "$run_path/$arch/$circuit/$script_params_dirname";
                my $output_filepath = "$work_dir/parse_results.txt";
                my $config_prefix = "arch\tcircuit\tscript_params";
                my $config_values = "arch=$arch" . " circuit=$circuit" . " script_params=$script_params_dirname";
                parse_single_flow_run($work_dir, $config_values, $parse_file, $output_filepath);
                push(@first_run_result_files, $output_filepath);

                if($second_parse_file){
                    my $output_filepath = "$work_dir/parse_results_2.txt";
                    parse_single_flow_run($work_dir, $config_values, $second_parse_file, $output_filepath);
                    push(@second_run_result_files, $output_filepath);
                }

                if ($parse_qor) {
                    my $output_filepath = "$work_dir/qor_results.txt";
                    parse_single_flow_run($work_dir, $config_values, $qor_parse_file, $output_filepath);
                    push(@qor_run_result_files, $output_filepath);
                }
            }
		}
	}

    if (@first_run_result_files) {
        aggregate_single_flow_runs(\@first_run_result_files, "$run_path/parse_results.txt");
    }
    if (@second_run_result_files) {
        aggregate_single_flow_runs(\@second_run_result_files, "$run_path/parse_results_2.txt");
    }
    if (@qor_run_result_files) {
        aggregate_single_flow_runs(\@qor_run_result_files, "$run_path/qor_results.txt");
    }

	if ($create_golden) {
		copy( "$run_path/parse_results.txt", "$run_path/../config/golden_results.txt" );
	}

    if ($second_parse_file){
        #don't check with golden results, just check the two files
        return check_two_files ( $task_name, $task_path, $run_path, "$run_path/parse_results.txt", "$run_path/parse_results_2.txt", 0);
        
    }

	if ($check_golden) {
        #Returns 1 if failed
        if ($second_parse_file eq ""){
            return check_two_files( $task_name, $task_path, $run_path, "$run_path/parse_results.txt", "$task_path/config/golden_results.txt", $check_golden);
        }
	}

    return 0; #Pass
}

sub parse_single_flow_run {
    my ($run_dir, $config_values, $parse_file, $output_file_path) = @_;

    my $cmd = join(" ", "$vtr_flow_path/scripts/parse_vtr_flow.pl", $run_dir, $parse_file, $config_values, "> $output_file_path");

    my $ret = system($cmd);
    if ($ret != 0) {
        print "System command '$cmd' failed\n";
        exit $FAILED_PARSE_EXIT_CODE;
    }
    pretty_print_table($output_file_path);
}

sub aggregate_single_flow_runs {
    my ($parse_result_files, $output_file_path) = @_;

    my $first = 0;

	open(OUTPUT_FILE, ">$output_file_path" );

    my $size = @$parse_result_files;
    for (my $i=0; $i < $size; $i++) {
        my $result_file = @$parse_result_files[$i];

        open(RESULT_FILE, $result_file);
        my @lines = <RESULT_FILE>;
        close(RESULT_FILE);

        if ($i == 0) {
            #First file add header
            print OUTPUT_FILE $lines[0];
        } 

        #Data
        print OUTPUT_FILE $lines[1];

    }
    close(OUTPUT_FILE);
    pretty_print_table($output_file_path);
}

sub summarize_qor {

	##############################################################
	# Set up output file
	##############################################################

	my $first = 1;
	
	my $task = @tasks[0];
	(my $task_path = $task) =~ s/\s+$//;

    # first see if task_name is the task path
	if (! -e "$task_path/config/config.txt") {
	    ($task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
    }

	my $output_path = $task_path;
	my $exp_id = last_exp_id($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		$output_path = "$task_path/../";
	}
	if ( !-e "$output_path/task_summary" ) {
		mkdir "$output_path/task_summary";
	}
	if ( -e "$output_path/task_summary/${run_prefix}${exp_id}_summary.txt" ) {
	}
	open( OUTPUT_FILE, ">$output_path/task_summary/${run_prefix}${exp_id}_summary.txt" );
	
	##############################################################
	# Append contents of QoR files to output file
	##############################################################

	foreach my $task (@tasks) {
		chomp($task);
        ($task_path = $task) =~ s/\s+$//;

        # first see if task_name is the task path
        if (! -e "$task_path/config/config.txt") {
            ($task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
        }
		$exp_id = last_exp_id($task_path);
		(my $run_path = "$task_path/${run_prefix}${exp_id}") =~ s/\s+$//;

		open( RESULTS_FILE, "$run_path/qor_results.txt" );
		my $output = <RESULTS_FILE>;

		if ($first) {
			print OUTPUT_FILE "task_name\t$output";
			$first = 0;
		}
		
		while ($output = <RESULTS_FILE>) {
			print OUTPUT_FILE $task . "\t" . $output;
		}
		close(RESULTS_FILE);
	}
	close(OUTPUT_FILE);
}

sub calc_geomean {

	##############################################################
	# Set up output file
	##############################################################

	my $first = 0;

	my $task = @tasks[0];
	(my $task_path = $task) =~ s/\s+$//;

    # first see if task_name is the task path
	if (! -e "$task_path/config/config.txt") {
	    ($task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
    }
	my $output_path = $task_path;
	my $exp_id = last_exp_id($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		($output_path = "$task_path/../") =~ s/\s+$//; 
	}
	if ( !-e "$output_path/qor_geomean.txt" ) {
		open( OUTPUT_FILE, ">$output_path/qor_geomean.txt" );
		$first = 1;
	}
	else {
		open( OUTPUT_FILE, ">>$output_path/qor_geomean.txt" );
	}
	
	##############################################################
	# Read summary file
	##############################################################

	my $summary_file = "$output_path/task_summary/${run_prefix}${exp_id}_summary.txt";

	if ( !-r $summary_file ) {
		print "[ERROR] Failed to open $summary_file: $!\n";
		return;
	}
	open( SUMMARY_FILE, "<$summary_file" );
	my @summary_data = <SUMMARY_FILE>;
	close(SUMMARY_FILE);

	my $summary_params = shift @summary_data;
	my @summary_params = split( /\t/, trim($summary_params) );
	
	if ($first) {
		# Hack - remove unwanted labels
		my $num = 4;
		while ($num) {
			shift @summary_params;
			--$num;
		}
		print OUTPUT_FILE "run";
		my @temp = @summary_params;
		while ( $#temp >= 0 ) {
			my $label = shift @temp;
			print OUTPUT_FILE "\t" . "$label";
		}
		print OUTPUT_FILE "\t" . "date" . "\t" . "revision";
		$first = 0;
	}
	else {
	}

	print OUTPUT_FILE "\n${exp_id}";

	##############################################################
	# Compute & write geomean to output file
	##############################################################

	my $index = 4;
	my @summary_params = split( /\t/, trim($summary_params) );
	
	while ( $#summary_params  >= $index ) {
		my $geomean = 1; my $num = 0;
		foreach my $line (@summary_data) {
			my @first_file_line = split( /\t/, $line );
			if ( trim( @first_file_line[$index] ) > 0 ) {
				$geomean *= trim( @first_file_line[$index] );
				$num++;
			}
		}
		if ($num) {
			$geomean **= 1/$num;
			print OUTPUT_FILE "\t" . "${geomean}";
		}
		else {
			print OUTPUT_FILE "\t" . "-1";
		}
		$index++;
	}
	my $date = strftime( '%D', localtime );
	print OUTPUT_FILE "\t" . "$date" . "\t" . "$revision";
	close(OUTPUT_FILE);
}

sub max {
    my $x = shift;
    my $y = shift;

    return ($x < $y) ? $y : $x;
}

sub pretty_print_table {
    my $file_path = shift;


    #Read the input file
    my @file_data;
    open(INFILE,"<$file_path");
    while(<INFILE>) {
        chomp;
        push(@file_data, [split /\t/])
    }

    #Determine the maximum column width for pretty formatting
    my %col_widths;
    for my $row (0 .. $#file_data) {
        for my $col (0 .. $#{$file_data[$row]}) {

            my $col_width = length $file_data[$row][$col];

            #Do we have a valid column width?
            if (not exists $col_widths{$col}) {
                #Initial width
                $col_widths{$col} = $col_width;
            } else {
                #Max width
                $col_widths{$col} = max($col_widths{$col}, $col_width);
            }

        }
    }

    #Write out in pretty format
    open(OUTFILE,">$file_path");
    for my $row (0 .. $#file_data) {
        for my $col (0 .. $#{$file_data[$row]}) {
            printf OUTFILE "%-*s", $col_widths{$col}, $file_data[$row][$col];

            if($col != $#{$file_data[$row]}) {
                printf OUTFILE "\t";
            }
        }
        printf OUTFILE "\n";
    }
    close(OUTFILE);

}

sub last_exp_id {
	my $path = shift;
	my $num = 0;
    my $run_id = "";
    my $run_id_no_pad = "";
    do {
		++$num;
        $run_id = sprintf("%03d", $num);
        $run_id_no_pad = sprintf("%d", $num);
    } while ( -e "$path/${run_prefix}${run_id}" or -e "$path/${run_prefix}${run_id_no_pad}");
    --$num;
    $run_id = sprintf("%03d", $num);
    $run_id_no_pad = sprintf("%d", $num);

    if( -e "$path/${run_prefix}${run_id}" ) {
        return $run_id;
    } elsif (-e "$path/${run_prefix}${run_id_no_pad}") {
        return $run_id_no_pad;
    }

    die("Unknown experiment id");
} 

sub check_two_files {
	my $task_name = shift;
	my $task_path = shift;
	my $run_path  = shift;
        my $first_test_file_dir = shift;
        my $second_test_file_dir = shift;
        my $is_golden = shift;

    #Did this check pass?
	my $failed = 0;

    print "$task_name...";
    print "\n" if $verbose;

	# Code to check the results of the two files
	(my $test_file_1 = "$first_test_file_dir") =~ s/\s+$//;
	(my $test_file_2 = "$second_test_file_dir") =~ s/s+$//;

	my $pass_req_file;
	open( CONFIG_FILE, "$task_path/config/config.txt" );
	my $lines = do { local $/; <CONFIG_FILE>; };
	close(CONFIG_FILE);

	# Search config file
	if ( $lines =~ /^\s*pass_requirements_file\s*=\s*(\S+)\s*$/m ) {
        my $pass_req_filename = $1;

        if ($pass_req_filename !~ /^\s*%/) { #Not blank

            # Search for pass requirement file
            $pass_req_filename = expand_user_path($pass_req_filename);
            if ( -e "$task_path/config/$pass_req_filename" ) {
                $pass_req_file = "$task_path/config/$pass_req_filename";
            } elsif ( -e "$vtr_flow_path/parse/pass_requirements/$pass_req_filename" ) {
                $pass_req_file =
                  "$vtr_flow_path/parse/pass_requirements/$pass_req_filename";
            } elsif ( -e $pass_req_filename ) {
                $pass_req_file = $pass_req_filename;
            }

            if ( -e $pass_req_file ) {
                $failed += check_pass_requirements($pass_req_file, $test_file_1, $test_file_2, $is_golden);
            } else {
                print "[ERROR] Cannot find pass_requirements_file.  Checked for $task_path/config/$pass_req_filename or $vtr_flow_path/parse/$pass_req_filename or $pass_req_filename\n";
                $failed += 1;
            }
        }
    } else {
		print
		  "[Warning] No 'pass_requirements_file' in task configuration file ($task_path/config/config.txt)\n";
	}


	if ($failed == 0) {
		print "[Pass]\n";
	}
    return $failed;
}

sub check_pass_requirements() {
    my ($pass_req_file, $test_file_1, $test_file_2, $is_golden) = @_;

	my $line;

	my @first_test_data;
    my @second_test_data;
	my @pass_req_data;

	my @params;
	my %type;
	my %min_threshold;
	my %max_threshold;
	my %abs_diff_threshold;
    my $failed = 0;

	##############################################################
	# Read files
	##############################################################
	if ( !-r $test_file_2 ) {
		print "[ERROR] Failed to open $test_file_2: $!\n";
        $failed += 1;
		return $failed;
	}
	open( GOLDEN_DATA, "<$test_file_2" );
	@second_test_data = <GOLDEN_DATA>;
	close(GOLDEN_DATA);

	if ( !-r $pass_req_file ) {
		print "[ERROR] Failed to open $pass_req_file: $!\n";
        $failed += 1;
		return $failed;
	}

	@pass_req_data = load_file_with_includes($pass_req_file);

	if ( !-r $test_file_1 ) {
		print "[ERROR] Failed to open $test_file_1: $!\n";
        $failed += 1;
		return $failed;
	}
	open( TEST_DATA, "<$test_file_1" );
	@first_test_data = <TEST_DATA>;
	close(TEST_DATA);

	##############################################################
	# Process and check all parameters for consistency
	##############################################################
	my $second_test_params = shift @second_test_data;
	my $first_test_params   = shift @first_test_data;

	my @second_test_params = split( /\t/, $second_test_params );    # get parameters of the second file results
	my @first_test_params = split( /\t/, $first_test_params );      # get parameters of the first file results

    my @second_test_params = map(trim($_), @second_test_params);
    my @first_test_params = map(trim($_), @first_test_params);

    my %first_params_index;
    my $i = 0;
    foreach my $param (@first_test_params) {
        $first_params_index{$param} = $i;
        $i++;
    }
    my %second_params_index;
    $i = 0;
    foreach my $param (@second_test_params) {
        $second_params_index{$param} = $i;
        $i++;
    }

	# Check to ensure all parameters to compare are consistent
	foreach $line (@pass_req_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

		my @data = split( /;/, $line );
		my $name = trim( $data[0] );
        my $check_arg = trim($data[1]);
        my ($check_func) = $check_arg =~ m/(.*)\(/;
        my ($check_func_args_str) = $check_arg =~ m/\((.*)\)/;
        my @check_func_args = split( /,/, $check_func_args_str);
		$type{$name} = trim($check_func);
		if ( $check_func eq "Range" ) {
			$min_threshold{$name} = trim( $check_func_args[0] );
			$max_threshold{$name} = trim( $check_func_args[1] );
		} elsif ($check_func eq "RangeAbs") {
			$min_threshold{$name} = trim( $check_func_args[0] );
			$max_threshold{$name} = trim( $check_func_args[1] );
			$abs_diff_threshold{$name} = trim( $check_func_args[2] ); #Third element is absolute threshold
        } elsif ($check_func eq "Equal") {
            #Pass
        } else {
			print "[ERROR] $name has no valid comparison check specified (e.g. Range, RangeAbs, Equal) (was '$check_func').\n";
            $failed += 1;
			return $failed;
        }

		#Ensure item is in the first file
		if (!exists $second_params_index{$name}) {
                    if ($is_golden){
                        print "[ERROR] $name is not in the golden results file.\n";
                    }else{
                        print "[ERROR] $name is not in the second parse file.\n";
                    }
                    $failed += 1;
		}

		# Ensure item is in new results
		if (!exists $first_params_index{$name}) {
		    if ($is_golden){
                        print "[ERROR] $name is not in the results file.\n";
                    }else{
                        print "[ERROR] $name is not in the first parse file.\n";
                    }
                    $failed += 1;
		}

		push( @params, $name );
	}

	##############################################################
	# Compare first file data data with second file data
	##############################################################
	if ( ( scalar @first_test_data ) != ( scalar @second_test_data ) ) {
		print
		  "[ERROR] Different number of entries in the two files.\n";
          $failed += 1;
	}

	# Iterate through each line of the test results data and compare with the golden data
	foreach $line (@first_test_data) {
		my @first_file_line   = split( /\t/, $line );
		my @second_file_line = split( /\t/, shift @second_test_data );


        my $second_file_arch = trim(@second_file_line[$second_params_index{'arch'}]);
        my $second_file_circuit = trim(@second_file_line[$second_params_index{'circuit'}]);
        my $second_file_script_params = trim(@second_file_line[$second_params_index{'script_params'}]);
        my $first_file_arch = trim(@first_file_line[$first_params_index{'arch'}]);
        my $first_file_circuit = trim(@first_file_line[$first_params_index{'circuit'}]);
        my $first_file_script_params = trim(@first_file_line[$first_params_index{'script_params'}]);

		if (   ( $first_file_circuit ne $first_file_circuit )
			or ( $first_file_arch ne $first_file_arch ) 
			or ( $first_file_script_params ne $first_file_script_params )) {
            if ($is_golden){
                print "[ERROR] Circuit/Architecture/Script-Params mismatch between golden results ($second_file_arch/$second_file_circuit/$second_file_script_params) and result ($first_file_arch/$first_file_circuit/$first_file_script_params).\n";
            } else{
                print "[ERROR] Circuit/Architecture/Script-Params mismatch between first result file($second_file_arch/$second_file_circuit/$second_file_script_params) and second result file ($first_file_arch/$first_file_circuit/$first_file_script_params).\n";
            }
            $failed += 1;
			return $failed;
		}
		my $circuitarch = "$first_file_arch/$first_file_circuit/$first_file_script_params";

		# Check each parameter where the type determines what to check for
		foreach my $value (@params) {
			my $first_file_index = $first_params_index{$value};
			my $second_file_index = $second_params_index{$value};
			my $first_file_value   = trim(@first_file_line[$first_file_index]);
			my $second_file_value = trim(@second_file_line[$second_file_index]);


			if ( $type{$value} eq "Range" or $type{$value} eq "RangeAbs" ) {
                my $abs_diff = abs($first_file_value - $second_file_value);
                my $ratio;
                if ($second_file_value == 0) {
                    $ratio = "inf";
                } else {
                    $ratio = $first_file_value / $second_file_value;
                }

                if($verbose) {
                    print "\tParam: $value\n";
                    print "\t\tTest: $first_file_value\n";
                    print "\t\tGolden Value: $second_file_value\n";
                    print "\t\tRatio: $ratio\n";
                    print "\t\tAbsDiff $abs_diff\n";
                    print "\t\tMinRatio $min_threshold{$value}\n";
                    print "\t\tMaxRatio $max_threshold{$value}\n";
                    print "\t\tAbsThreshold $abs_diff_threshold{$value}\n";
                }

                if (    exists $abs_diff_threshold{$value}
                    and $abs_diff <= $abs_diff_threshold{$value}) {
                    #Within absolute threshold
                    next;
                }

                if (    $ratio >= $min_threshold{$value}
                    and $ratio <= $max_threshold{$value}) {
                    #Within relative thershold  
                    next;
                }

                if ($first_file_value == $second_file_value) {
                    #Equal (e.g. both zero)
                    next;
                }

                if (    $first_file_value eq 'nan'
                    and $second_file_value eq 'nan') {
                    #Both literal Not-a-Numbers
                    next;
                }

                #Beyond absolute and relative thresholds
                if ($is_golden){
                    print
                        "[Fail] \n $circuitarch $value: golden = $second_file_value result = $first_file_value\n";
                }else{
                    print
                        "[Fail] \n $circuitarch $value: first result = $first_file_value second result = $second_file_value\n";
                }
                $failed += 1;
			} elsif ($type{$value} eq "Equal") {
				if ( $first_file_value ne $second_file_value ) {
                    if ($is_golden) {
                        print "[Fail] \n $circuitarch $value: golden = $second_file_value result = $first_file_value\n";
                    } else {
                        print "[Fail] \n $circuitarch $value: first result = $first_file_value second result = $second_file_value\n";
                    }
					$failed += 1;
				}
			} else {
				# If the check type is unknown
                $failed += 1;
                print "[Fail] \n $circuitarch $value: unrecognized check type '$type{$value}' (e.g. Range, RangeAbs, Equal)\n";

			}
		}
	}
    return $failed;
}

sub trim() {
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

sub get_important_file {
    my $task_path = shift;
    my $vtr_flow_path = shift;
    my $file = shift;
	if ( -e "$task_path/config/$file" ) {
		return "$task_path/config/$file";
	}
	elsif ( -e "$vtr_flow_path/parse/parse_config/$file" ) {
		return "$vtr_flow_path/parse/parse_config/$file";
	}
	elsif ( -e "$vtr_flow_path/parse/qor_config/$file" ) {
	    return "$vtr_flow_path/parse/qor_config/$file";
    }
	elsif ( $file !~ /^\/.*/ ) {
		die "Important file does not exist ($file)";
	}
}

sub load_file_with_includes {
    my ($filepath) = @_;

    my @lines;

    foreach my $line (load_file_lines($filepath)) {
        if ($line =~ m/^\s*%include\s+"(.*)"\s*$/) {
            #Assume the included file is in the same direcotry
            my $include_filepath = File::Spec->catfile(dirname($filepath), $1);
            $include_filepath = Cwd::realpath($include_filepath);

            #Load it's lines, note that this is done recursively to resolve all includes
            my @included_file_lines = load_file_with_includes($include_filepath);
            push(@lines, "#Starting %include $include_filepath\n");
            push(@lines, @included_file_lines);
            push(@lines, "#Finished %include $include_filepath\n");
        } else {
            push(@lines, $line);
        }
    }

    return @lines;
}

sub load_file_lines {
    my ($filepath) = @_;

	open(FILE, "<$filepath" ) or die("Failed to open file $filepath\n");
	my @lines = <FILE>;
	close(FILE);

    return @lines;
}
