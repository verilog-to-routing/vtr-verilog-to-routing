#!/usr/bin/perl
#------------------------------------------------------------------------#
#  Run a regression test on the batch system                             #
#   - runs the regression tests given as input parameters                #
# 	- parses and checks results upon completion							 #
#   - tests can be found in <vtr_flow_path>/tasks/regression_tests/      #
#                                                                        #
#  Usage:                                                                #
#  <While in the top-level directory>                                    #
#  run_reg_test.pl <TEST1> <TEST2> ...                                   #
#  									                                     #
#  Options:								                                 #
#	-create_golden:  Will create/overwrite the golden results with       #
#	those of the most recent execution				                     #
#	quick_test: Will run quick test in top-level directory before 	     #
# 	running specified regression tests.	                                 #
#	-display_qor: Will display quality of results of most recent         #
# 	build of specified regression test.				                     #
#									                                     #
#  Notes: <TEST> argument is of the format: <project>_reg_<suite>   	 #
#  See <vtr_flow_path>/tasks/regression_tests for more information.	     #
#									                                     #
#  Currently available:							                         #
#		- vtr_reg_basic						                             #
#		- vtr_reg_strong					                             #
#		- vtr_reg_nightly					                             #
#		- vtr_reg_weekly                                                 #
#									                                     #
#------------------------------------------------------------------------#

use strict;
use Cwd;
use File::Spec;
use List::Util;
use Scalar::Util;

# Function Prototypes
sub setup_single_test;
sub check_override;
sub run_single_test;
sub parse_single_test;
sub run_quick_test;
sub run_odin_test;

# Get absolute path of 'vtr_flow'
my $vtr_flow_path = getcwd;
$vtr_flow_path = "$vtr_flow_path/vtr_flow";

# Get absolute path of 'regression_tests'
my $reg_test_path = "$vtr_flow_path/tasks/regression_tests";

my @tests;
my $token;
my $test_dir;
my $test_name;
my $run_params = "";
my $parse_params = "";
# Override variables
my $first = 1;
my $parse_only = 0;
my $create_golden = 0;
my $check_golden = 0;
my $calc_geomean = 0;
my $display_qor = 0;
my $can_quit = 0;

# Parse Input Arguments
while ( $token = shift(@ARGV) ) {
	if ( $token eq "-parse") {
		$parse_only = 1;
	}
	elsif ( $token eq "-create_golden" ) {
		$create_golden = 1;
	}
	elsif ( $token eq "-check_golden" ) {
		$check_golden = 1;
	}
	elsif ( $token eq "-calc_geomean" ) {
		$calc_geomean = 1;
	}
	elsif ( $token eq "-display_qor" ) {
		$display_qor = 1;
	}
	elsif ($token eq "quick_test") {
		run_quick_test();
		$can_quit = 1;
	}
	elsif ($token eq "odin_reg_micro") {
		run_odin_test( "micro" );
		$can_quit = 1;
	}
	elsif ($token eq "odin_reg_full") {
		run_odin_test( "full" );
		$can_quit = 1;
	}
	else {	
		if ($token =~ /(.*)\//) {
			$token = $1;
		}
		push( @tests, $token );
	}
}

# Remove duplicate tests
my %hash = map { $_, 1 } @tests;
@tests = keys %hash;

if ( $#tests == -1 and !$can_quit ) {
	die "\n"
	  . "Incorrect usage.  You must specify at least one test to execute.\n"
	  . "\n"
	  . "USAGE:\n"
	  . "run_reg_test.pl <TEST1> <TEST2> ... \n"
	  . "\n"
	  . "OPTIONS:\n"
	  . "	-create_golden: Will create/overwrite the golden results with\n"
	  . "	those of the most recent execution\n"
	  . "	-quick_test: Will run quick test in top-level directory before\n"
	  . "	running specified regression tests.\n"
	  . "	-display_qor: Will display quality of results of most recent build \n"
	  . "	of specified regression test.\n"
	  . "\n"
	  . "Notes: <TEST> argument is of the format: <project>_reg_<suite>\n"
	  . "See <vtr_flow_path>/tasks/regression_tests for more information.\n"
	  . "\n"	  
	  . "Currently available:\n"
	  . "	- vtr_reg_basic\n"
	  . "	- vtr_reg_strong\n"
	  . "	- vtr_reg_nightly\n"
	  . "	- vtr_reg_weekly\n"
	  . "\n"
	  . "If you wish to add your own test, place it in 
      . <vtr_flow_path>/tasks/regression_tests\n"
	  . "\n";
}


##############################################################
# Run regression tests
##############################################################

if ( $#tests > -1 ) {

	foreach my $test (@tests) {
		chomp($test);
		# Set up test
		setup_single_test($test);
		# Check for user overrides
		check_override;
		# Run regression test
		run_single_test;
		$first = 0;
		# Parse regression test
		parse_single_test(" ");
		# Create/Check golden results
		if ($create_golden) {
			parse_single_test("create", "calculate");
		}
		else {
			parse_single_test("check", "calculate");
		}
	}
	print "\nTest complete\n\n";
}


##############################################################
# Subroutines
##############################################################

sub setup_single_test {
	$test_name = shift;
	$test_dir = "$reg_test_path/$test_name";

	# Task list exists. Execute script with list instead.
	if ( -e "$test_dir/task_list.txt" ) {
		$run_params = "-l $test_dir/task_list.txt";
		$parse_params = "-l $test_dir/task_list.txt ";
	}
	else {
		$run_params = "$test_dir";
		$parse_params = "$test_dir ";
	}
}

sub check_override {

	if ($parse_only) {
		parse_single_test;
		exit "Parsed results";
	}

	if ($check_golden) {
		parse_single_test("check");
		exit "Checked results.";
	}
	
	if ($calc_geomean) {
		parse_single_test("calculate");
		exit "Calculated results.";
	}

	if ($display_qor) {
		if (!-e "$test_dir/qor_geomean.txt") {
			die "QoR results do not exist ($test_dir/qor_geomean.txt)";
		}
		elsif (!-r "$test_dir/qor_geomean.txt") {
			die "[ERROR] Failed to open $test_dir/qor_geomean.txt: $!";
		}
		else {
			print "=" x 121 . "\n";
			print "\t" x 6 . "$test_name QoR Results \n";
			print "=" x 121 . "\n";

			my @data = (
						"revision"		,
						"date"			,
						"total_runtime"		,
						"total_wirelength"	,
						"num_clb"		,
						"min_chan_width"	,
						"crit_path_delay"
						);
			
			my %units = (
						"revision"		, ""		,
						"date"			, ""		,
						"total_runtime" 	, " s"		,
						"total_wirelength" 	, " units"	,
						"num_clb" 		, " blocks"	,
						"min_chan_width" 	, " tracks"	,
						"crit_path_delay" 	, " ns"
						);

			my %precision = (
							"revision"		, "%s"	,
							"date" 			, "%s"	,
							"total_runtime"		, "%.3f",
							"total_wirelength" 	, "%.0f",
							"num_clb" 		, "%.2f",
							"min_chan_width" 	, "%.2f",
							"crit_path_delay" 	, "%.3f"
							);

			open( QOR_FILE, "$test_dir/qor_geomean.txt" );
			my $output = <QOR_FILE>;
			my @first_line = split( /\t/, trim($output) );			
			my @backwards = reverse <QOR_FILE>;

format STDOUT_TOP =
| @||||||| | @||||||||| | @|||||||||||||| | @||||||||||||||||||| | @|||||||||||| | @||||||||||||||| | @|||||||||||||||| |
@data;
-------------------------------------------------------------------------------------------------------------------------
.
write;

			while( @backwards ) {		
				my @last_line = split( /\t/, trim( shift(@backwards) ) );
				my @new_last_line;

				foreach my $param (@data) {
					
					# Get column (index) of each qor metric
					my $index = List::Util::first { @first_line[$_] eq $param } 0 .. $#first_line;

					# If index is out-of-bounds or metric cannot be found
					if ( $index > @last_line or $index eq "" ) {
						push( @new_last_line, sprintf( $precision{$param}, "-" ) . $units{$param} );
					}	
					# If valid number, add it onto line to be printed with appropriate sig figs. and units
					elsif ( Scalar::Util::looks_like_number(@last_line[$index]) ) {
						push( @new_last_line, sprintf( $precision{$param}, @last_line[$index] ) . $units{$param} );
					}
					# For dates of format dd/mm/yy
					elsif ( @last_line[$index] =~ m/^\d{2}\/\d{2}\/\d{2}$/ or @last_line[$index] ne "" ) { 
						push( @new_last_line, sprintf( $precision{$param}, @last_line[$index] ) . $units{$param} );
					}
					else {
						push( @new_last_line, sprintf( $precision{$param}, "-" ) . $units{$param} );
					}
				}
 
format STDOUT =
| @||||||| | @||||||||| | @|||||||||||||| | @||||||||||||||||||| | @|||||||||||| | @||||||||||||||| | @|||||||||||||||| |
@new_last_line;
.
write;
			}

			close( QOR_FILE );
			exit "QoR results displayed";
		}
	}
}

sub run_single_test {
	if ($first) { 
		print "=============================================\n";
		print "    Verilog-to-Routing Regression Testing    \n";
		print "=============================================\n";
		$first = 0;
	}

	print "\nRunning regression test... \n";
	chdir("$vtr_flow_path");
	print "scripts/run_vtr_task.pl $run_params \n";
	system("scripts/run_vtr_task.pl $run_params \n");
	chdir("..");
}

sub parse_single_test {
	while ( my $golden = shift(@_) ) {
		if ( $golden eq "create" ) {
			print "\nCreating golden results... \n";
			$parse_params = $parse_params . "-create_golden ";
		}
		elsif ( $golden eq "check" ) {
			print "\nChecking test results... \n";
			$parse_params = $parse_params . "-check_golden ";
		}
		elsif ( $golden eq "calculate" ) {
			print "\nCalculating QoR results... \n";
			$parse_params = $parse_params . "-calc_geomean ";
		}
		else {
			print "\nParsing test results... \n";
		}
	}
	chdir("$vtr_flow_path");
	print "scripts/parse_vtr_task.pl $parse_params \n";
	system("scripts/parse_vtr_task.pl $parse_params \n");
	chdir("..");
}

sub run_quick_test {

	print "=============================================\n";
	print "   Test Verilog-to-Routing Infrastructure    \n";
	print "=============================================\n\n";

	system("rm -f quick_test/depth_split.blif");
	system("rm -f quick_test/abc_out.blif");
	system("rm -f quick_test/route.out");


	chdir ("quick_test") or die "Failed to change to directory ./quick_test: $!";

	print "Testing ODIN II: ";
	system("../ODIN_II/odin_II.exe -c depth_split.xml > odin_ii_out.txt 2>&1");
	if(-f "depth_split.blif") {
		print " [PASSED]\n\n";
	} else {
		print " [FAILED]\n\n";
	}

	print "Testing ABC: ";
	system("../abc_with_bb_support/abc -c \"read abc_test.blif;sweep;write_hie abc_test.blif abc_out.blif\" > abc_out.txt  2>&1" );
	if(-f "abc_out.blif") {
		print " [PASSED]\n\n";
	} else {
		print " [FAILED]\n\n";
	}

	print "Testing VPR: ";
	system("../vpr/vpr sample_arch.xml vpr_test -blif_file vpr_test.blif -route_file route.out -nodisp > vpr_out.txt 2>&1");
	if(-f "route.out") {
		print " [PASSED]\n\n";
	} else {
		print " [FAILED]\n\n";
	}

	chdir ("..");	
}

sub run_odin_test {
	$token = shift;

	chdir ("ODIN_II") or die "Failed to change to directory ./ODIN_II: $!";

	if ( $token eq "micro" ) {
		system("./verify_microbenchmarks.sh");
	}
	elsif ( $token eq "full" ) {
		system("./verify_regression_tests.sh");
	}

	chdir ("..");	
}

sub trim($) {
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}
