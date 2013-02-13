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
use List::Util;
use Math::BigInt;
use POSIX qw/strftime/;

# Function Prototypes
sub trim;
sub parse_single_task;
sub summarize_qor;
sub calc_geomean;
sub check_golden;
sub expand_user_path;

# Get Absolute Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*vtr_flow)/;
my $vtr_flow_path = $1;

my $run_prefix = "run";

# Parse Input Arguments
my @tasks;
my @task_files;
my $token;
my $create_golden = 0;
my $check_golden  = 0;
my $parse_qor 	  = 1;  # QoR file is parsed by default; turned off if 
						# user does not specify QoR parse file in config.txt
my $calc_geomean  = 0;  # QoR geomeans are not computed by default;
my $revision;

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
	elsif ( $token eq "-revision" ) {
		$revision = shift(@ARGV);
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

foreach my $task (@tasks) {
	chomp($task);
	parse_single_task($task);
}

if ($calc_geomean) {
	summarize_qor;
	calc_geomean;
}

sub parse_single_task {
	my $task_name = shift;
	my $task_path = "$vtr_flow_path/tasks/$task_name";

	open( CONFIG, "<$task_path/config/config.txt" )
	  or die "Failed to open $task_path/config/config.txt: $!";
	my @config_data = <CONFIG>;
	close(CONFIG);

	my @circuits;
	my $parse_file;
	my $qor_parse_file;
	my @archs;
	foreach my $line (@config_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

		my @data  = split( /=/, $line );
		my $key   = trim( $data[0] );
		my $value = trim( $data[1] );

		if ( $key eq "circuit_list_add" ) {
			push( @circuits, $value );
		}
		elsif ( $key eq "arch_list_add" ) {
			push( @archs, $value );
		}
		elsif ( $key eq "parse_file" ) {
			$parse_file = expand_user_path($value);
		}
		elsif ( $key eq "qor_parse_file" ) {
			$qor_parse_file = expand_user_path($value);
		}
	}

	# PARSE CONFIG FILE
	if ( -e "$task_path/config/$parse_file" ) {
		$parse_file = "$task_path/config/$parse_file";
	}
	elsif ( $parse_file eq "" ) {
		die "Task $task_name has no parse file specified.\n";
	}
	elsif ( -e "$vtr_flow_path/parse/parse_config/$parse_file" ) {
		$parse_file = "$vtr_flow_path/parse/parse_config/$parse_file";
	}
	else {
		die "Parse file does not exist ($parse_file)";
	}
	
	# Get Max Run #
	opendir(DIR, $task_path);
	my @folders = readdir(DIR);
	closedir(DIR);
	# QOR PARSE CONFIG FILE
	if ( -e "$task_path/config/$qor_parse_file" ) {
		$qor_parse_file = "$task_path/config/$qor_parse_file";
	}
	elsif ( $qor_parse_file eq "" ) {
		print "Task $task_name has no QoR parse file specified. Skipping QoR.\n";
		$parse_qor = 0;
		$calc_geomean = 0;
	}
	elsif ( -e "$vtr_flow_path/parse/qor_config/$qor_parse_file" ) {
		$qor_parse_file = "$vtr_flow_path/parse/qor_config/$qor_parse_file";
	}
	else {
		die "QoR parse file does not exist ($qor_parse_file)";
	}

	my $exp_num = 1;
	foreach my $folder_name (@folders) {
		$folder_name =~ /${run_prefix}(\d+)/;
		if (int($1) > $exp_num) {
			$exp_num = int($1);
		}
	}
	
	my $run_path = "$task_path/${run_prefix}${exp_num}";

	my $first = 1;
	open( OUTPUT_FILE, ">$run_path/parse_results.txt" );
	foreach my $arch (@archs) {
		foreach my $circuit (@circuits) {
			system(
				"$vtr_flow_path/scripts/parse_vtr_flow.pl $run_path/$arch/$circuit $parse_file > $run_path/$arch/$circuit/parse_results.txt"
			);
			open( RESULTS_FILE, "$run_path/$arch/$circuit/parse_results.txt" );
			my $output = <RESULTS_FILE>;
			if ($first) {
				print OUTPUT_FILE "arch\tcircuit\t$output";
				$first = 0;
			}
			my $output = <RESULTS_FILE>;
			close(RESULTS_FILE);
			print OUTPUT_FILE $arch . "\t" . $circuit . "\t" . $output;
		}
	}
	close(OUTPUT_FILE);

	if ($parse_qor) {
		my $first = 1;
		open( OUTPUT_FILE, ">$run_path/qor_results.txt" );
		foreach my $arch (@archs) {
			foreach my $circuit (@circuits) {
				system(
					"$vtr_flow_path/scripts/parse_vtr_flow.pl $run_path/$arch/$circuit $qor_parse_file > $run_path/$arch/$circuit/qor_results.txt"
				);
				open( RESULTS_FILE, "$run_path/$arch/$circuit/qor_results.txt" );
				my $output = <RESULTS_FILE>;
				if ($first) {
					print OUTPUT_FILE "arch\tcircuit\t$output";
					$first = 0;
				}
				my $output = <RESULTS_FILE>;
				close(RESULTS_FILE);
				print OUTPUT_FILE $arch . "\t" . $circuit . "\t" . $output;
			}
		}
		close(OUTPUT_FILE);
	}

	if ($create_golden) {
		copy( "$run_path/parse_results.txt",
			"$run_path/../config/golden_results.txt" );
	}
	if ($check_golden) {
		check_golden( $task_name, $task_path, $run_path );
	}
}

sub summarize_qor {

	##############################################################
	# Set up output file
	##############################################################

	my $first = 1;
	
	my $task = @tasks[0];
	my $task_path = "$vtr_flow_path/tasks/$task";
	
	my $output_path = $task_path;
	my $exp_num = last_exp($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		$output_path = "$task_path/../";
	}
	if ( !-e "$output_path/task_summary" ) {
		mkdir "$output_path/task_summary";
	}
	if ( -e "$output_path/task_summary/${run_prefix}${exp_num}_summary.txt" ) {
	}
	open( OUTPUT_FILE, ">$output_path/task_summary/${run_prefix}${exp_num}_summary.txt" );
	
	##############################################################
	# Append contents of QoR files to output file
	##############################################################

	foreach my $task (@tasks) {
		chomp($task);
		$task_path = "$vtr_flow_path/tasks/$task";
		$exp_num = last_exp($task_path);
		my $run_path = "$task_path/${run_prefix}${exp_num}";

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
	my $task_path = "$vtr_flow_path/tasks/$task";
	
	my $output_path = $task_path;
	my $exp_num = last_exp($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		$output_path = "$task_path/../"; 
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

	my $summary_file = "$output_path/task_summary/${run_prefix}${exp_num}_summary.txt";

	if ( !-r $summary_file ) {
		print "[ERROR] Failed to open $summary_file: $!";
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

	print OUTPUT_FILE "\n${exp_num}";

	##############################################################
	# Compute & write geomean to output file
	##############################################################

	my $index = 4;
	my @summary_params = split( /\t/, trim($summary_params) );
	
	while ( $#summary_params  >= $index ) {
		my $geomean = 1; my $num = 0;
		foreach my $line (@summary_data) {
			my @test_line = split( /\t/, $line );
			if ( trim( @test_line[$index] ) > 0 ) {
				$geomean *= trim( @test_line[$index] );
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

sub last_exp {
	my $path = shift;
	my $num = 1;
	while ( -e "$path/${run_prefix}${num}" ) {
		++$num;
	}
	--$num;
	return $num;
} 

sub check_golden {
	my $task_name = shift;
	my $task_path = shift;
	my $run_path  = shift;

	print "$task_name...";

	# Code to check the results against the golden results
	my $golden_file = "$task_path/config/golden_results.txt";
	my $test_file   = "$run_path/parse_results.txt";

	my $pass_req_file;
	open( CONFIG_FILE, "$task_path/config/config.txt" );
	my $lines = do { local $/; <CONFIG_FILE>; };
	close(CONFIG_FILE);

	# Search config file
	if ( $lines =~ /^\s*pass_requirements_file\s*=\s*(\S+)\s*$/m ) { }
	else {
		print
		  "[ERROR] No 'pass_requirements_file' in task configuration file ($task_path/config/config.txt)\n";
		return;
	}

	my $pass_req_filename = $1;

	# Search for pass requirement file
	$pass_req_filename = expand_user_path($pass_req_filename);
	if ( -e "$task_path/config/$pass_req_filename" ) {
		$pass_req_file = "$task_path/config/$pass_req_filename";
	}
	elsif ( -e "$vtr_flow_path/parse/pass_requirements/$pass_req_filename" ) {
		$pass_req_file =
		  "$vtr_flow_path/parse/pass_requirements/$pass_req_filename";
	}
	elsif ( -e $pass_req_filename ) {
		$pass_req_file = $pass_req_filename;
	}
	else {
		print
		  "[ERROR] Cannot find pass_requirements_file.  Checked for $task_path/config/$pass_req_filename or $vtr_flow_path/parse/$pass_req_filename or $pass_req_filename\n";
		return;
	}

	my $line;
	my $pass = 1;

	my @golden_data;
	my @test_data;
	my @pass_req_data;

	my @params;
	my %type;
	my %min_threshold;
	my %max_threshold;

	##############################################################
	# Read files
	##############################################################
	if ( !-r $golden_file ) {
		print "[ERROR] Failed to open $golden_file: $!";
		return;
	}
	open( GOLDEN_DATA, "<$golden_file" );
	@golden_data = <GOLDEN_DATA>;
	close(GOLDEN_DATA);

	if ( !-r $pass_req_file ) {
		print "[ERROR] Failed to open $pass_req_file: $!";
		return;
	}
	open( PASS_DATA, "<$pass_req_file" );
	@pass_req_data = <PASS_DATA>;
	close(PASS_DATA);

	if ( !-r $test_file ) {
		print "[ERROR] Failed to open $test_file: $!";
		return;
	}
	open( TEST_DATA, "<$test_file" );
	@test_data = <TEST_DATA>;
	close(TEST_DATA);

	##############################################################
	# Process and check all parameters for consistency
	##############################################################
	my $golden_params = shift @golden_data;
	my $test_params   = shift @test_data;

	my @golden_params =
	  split( /\t/, trim($golden_params) );    # get parameters of golden results
	my @test_params =
	  split( /\t/, trim($test_params) );      # get parameters of test results

	if ( $golden_params ne $test_params ) {
		print "[ERROR] Different parameters in golden and result file. $golden_params different from $test_params\n";
		return;
	}

	# Check to ensure all parameters to compare are consistent
	foreach $line (@pass_req_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

		my @data = split( /;/, $line );
		my $name = trim( $data[0] );
		$type{$name} = trim( $data[1] );
		if ( trim( $data[1] ) eq "Range" ) {
			$min_threshold{$name} = trim( $data[2] );
			$max_threshold{$name} = trim( $data[3] );
		}

		#Ensure item is in golden results
		if ( !grep { $_ eq $name } @golden_params ) {
			print "[ERROR] $name is not in the golden file.\n";
			return;
		}

		# Ensure item is in new results
		if ( !grep { $_ eq $name } @test_params ) {
			print "[ERROR] $name is not in the results file.\n";
		}

		push( @params, $name );
	}

	##############################################################
	# Compare test data with golden data
	##############################################################
	if ( ( scalar @test_data ) != ( scalar @golden_data ) ) {
		print
		  "[ERROR] Different number of entries in golden and result files.\n";
	}
	@test_data   = sort (@test_data);
	@golden_data = sort (@golden_data);

	# Iterate through each line of the test results data and compare with the golden data
	foreach $line (@test_data) {
		my @test_line   = split( /\t/, $line );
		my @golden_line = split( /\t/, shift @golden_data );

		if (   ( @test_line[0] ne @golden_line[0] )
			or ( @test_line[1] ne @golden_line[1] ) )
		{
			print
			  "[ERROR] Circuit/Architecture mismatch between golden and result file.\n";
			return;
		}
		my $circuitarch = "@test_line[0]-@test_line[1]";

		# Check each parameter where the type determines what to check for
		foreach my $value (@params) {
			my $index =
			  List::Util::first { $golden_params[$_] eq $value }
			  0 .. $#golden_params;
			my $test_value   = @test_line[$index];
			my $golden_value = @golden_line[$index];
			if ( $type{$value} eq "Range" ) {

				# Check because of division by 0
				if ( $golden_value == 0 ) {
					if ( $test_value != 0 ) {
						print
						  "[Fail] $circuitarch $value: result = $test_value golden = $golden_value\n";
						$pass = 0;
						return;
					}
				}
				else {
					my $ratio = $test_value / $golden_value;
					if (   $ratio < $min_threshold{$value}
						or $ratio > $max_threshold{$value} )
					{
						print
						  "[Fail] $circuitarch $value: result = $test_value golden = $golden_value\n";
						$pass = 0;
						return;
					}
				}
			}
			else {

				# If the type is unknown, check for an exact match
				if ( $test_value ne $golden_value ) {
					$pass = 0;
					print
					  "[Fail] $circuitarch $value: result = $test_value golden = $golden_value\n";
				}
			}
		}
	}

	if ($pass) {
		print "[Pass]\n";
	}
}

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
