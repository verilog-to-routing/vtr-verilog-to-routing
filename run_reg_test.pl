#!/usr/bin/perl
#------------------------------------------------------------------------#
#  Run a regression test on the batch system                             #
#   - runs the regression tests given as input parameters                #
# 	- parses and checks results upon completion			 #
#   - tests can be found in <vtr_flow_path>/tasks/regression_tests/      #
#                                                                        #
#  Usage:                                                                #
#  <While in the top-level directory>               			 #
#  run_reg_test.pl <TEST1> <TEST2> ...                                   #
#  									 #
#  Options:								 #	
#	-create_golden:  Will create/overwrite the golden results with   #
#	those of the most recent execution				 #
#	quick_test: Will run quick test in top-level directory before 	 #
# 	running specified regression tests.				 #
#	-display_qor: Will display quality of results of most recent build \n"
#	of specified regression test.\n
#									 #
#  Notes: <TEST> argument is of the format: <project>_reg_<suite>   	 #
#  See <vtr_flow_path>/tasks/regression_tests for more information.	 #	
#									 #	
#  Currently available:							 #
#		- vtr_reg_basic						 #	
#		- vtr_reg_strong					 #
#		- vtr_reg_nightly					 #	
#		- vtr_reg_weekly					 #
#									 #
#------------------------------------------------------------------------#

use strict;
use Cwd;
use File::Spec;

# Function Prototypes
sub setup_single_test;
sub check_override;
sub run_single_test;
sub parse_single_test;
sub run_quick_test;
sub run_odin_test;

# Get absolute path of 'vtr_flow'
# Cwd::abs_path($0) =~ m/(.*\/vtr_flow)\//;
my $vtr_flow_path = "./vtr_flow";

# Get absolute path of 'regression_tests'
# Cwd::abs_path($0) =~ m/(.*regression_tests)/;
my $reg_test_path = "./vtr_flow/tasks/regression_tests";

my @tests;
my $token;
my $test_dir;
my $script_params = "";
# Override variables
my $parse_only = 0;
my $create_golden = 0;
my $check_golden = 0;
my $calc_qor = 0;
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
	elsif ( $token eq "-calc_qor" ) {
		$calc_qor = 1;
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
	  . "If you wish to add your own test, place it in <vtr_flow_path>/tasks/regression_tests\n"
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
		# Parse regression test
		parse_single_test("");
		# Check golden results
		parse_single_test("check", "calculate");
	}
	print "\nTest complete\n\n";
}


##############################################################
# Subroutines
##############################################################

sub setup_single_test {
	my $test_name = shift;
	$test_dir = "$reg_test_path/$test_name";

	# Task list exists. Execute script with list instead.
	if ( -e "$test_dir/task_list.txt" ) {
		$script_params = "-l $test_dir/task_list.txt";
	}
	else {
		$script_params = "$test_dir";
	}
}

sub check_override {

	if ($parse_only) {
		parse_single_test;
		die "Parsed results";
	}

	if ($create_golden) {
		parse_single_test("create");
		die "Created golden results";
	}

	if ($check_golden) {
		parse_single_test("check");
		die "Checked results.";
	}
	
	if ($calc_qor) {
		parse_single_test("calculate");
		die "Calculated results.";
	}

	if ($display_qor) {
		if (!-e "$test_dir/qor_geomean.txt") {
			die "QoR results do not exist ($test_dir/qor_geomean.txt)";
		}
		elsif (!-r "$test_dir/qor_geomean.txt") {
			die "[ERROR] Failed to open $test_dir/qor_geomean.txt: $!";
		}
		else {
			print "============================================================================================================================\n";
			print "        				   Verilog-to-Routing QoR Results       					\n";
			print "============================================================================================================================\n";

			open( QOR_FILE, "$test_dir/qor_geomean.txt" );
			my $output = <QOR_FILE>;

			my @first_line = split( /\t/, trim($output) );
			print "@first_line[1]" . "\t\t " . "@first_line[2]" . "\t\t    " . "@first_line[3]" . "\t\t " . "@first_line[4]" . "\t\t" . "@first_line[5] \n";
			print "-----------------------" . "\t\t" . "----------------" . "\t" . "----------------" . "\t" . "----------------" . "\t" . "--------------------" . "\n";
			my $last_line;
			while(<QOR_FILE>) {
				   $last_line = $_ if eof;
			}
			my @last_line = split( /\t/, trim($last_line) );
			print "@last_line[1]" . "\t\t" . "@last_line[2]" . "\t" . "@last_line[3]" . "\t" . "@last_line[4]" . "\t" . "@last_line[5] \n\n";
			die "QoR results displayed";
		}
	}
}

my $first = 1; 

sub run_single_test {
	if ($first) { 
		print "=============================================\n";
		print "    Verilog-to-Routing Regression Testing    \n";
		print "=============================================\n";
		$first = 0;
	}

	print "\nRunning regression test... \n";
	system("$vtr_flow_path/scripts/run_vtr_task.pl $script_params \n");
}

sub parse_single_test {
	while ( my $golden = shift(@_) ) {
		if ( $golden eq "create" ) {
			print "\nCreating golden results... \n\n";
			$script_params = $script_params . " -create_golden";
		}
		elsif ( $golden eq "check" ) {
			print "\nChecking test results... \n\n";
			$script_params = $script_params . " -check_golden";
		}
		elsif ( $golden eq "calculate" ) {
			print "\nCalculating QoR results... \n\n";
			$script_params = $script_params . " -calc_qor";
		}
		else {
			print "\nParsing test results... \n\n";
		}
	}
	system("$vtr_flow_path/scripts/parse_vtr_task.pl $script_params \n");
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