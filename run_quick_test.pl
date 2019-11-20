#!/usr/bin/perl
#------------------------------------------------------------------------#
#  Run an experiment on the batch system                                 #
#   - runs an experiment script                                          #
#     creates seperate directory for each circuit                        #
#   - uses a configuration file for experiment setup                     #
#                                                                        #
#  Usage:                                                                #
#  <While in the experiment directory with directory base>               #
#  run_experiment.pl [local|condor]                                      #
#                                                                        #
#     local|condor - an optional string that is either local to run      #
#                    locally or condor to run on condor batch system,    #
#					 defaults to condor                                  #
#    Options: -machines [fast|slow|both]                                 #
#------------------------------------------------------------------------#

use strict;
use Cwd;

print "=============================================\n";
print "Test Verilog-to-Routing Build Infrastructure\n";
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

print "Test complete\n";

