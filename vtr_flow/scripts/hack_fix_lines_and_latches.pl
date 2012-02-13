#!/usr/bin/perl
###################################################################################
# This script prepares the ABC created blif for VPR by fixing lines that are too 
# long and adding the clock signals to the latches.
#
# Usage:
#	hack_fix_lines_and_latches.pl abc_blif output_blif clock_file1 clock_file2
#
# Parameters:
# 	abc_blif: The input blif that was created by ABC
#   output_blif: The blif output by this script, to be used in VPR
#   clock_file1: File that contains a list of benchmarks and their clock signals
#					Each line of the file contains the benchmark name, then the
#					clock signal name, separated by white space.
#	clock_file2: A additional clock file to check
# 
###################################################################################

use strict;

sub find_clock;

# check the parametes.  Note PERL does not consider the script itself a parameter.
my $number_arguments = @ARGV;
if ($number_arguments != 3 and $number_arguments != 4)
{
	print("usage: hack_fix_latches.pl [input_blif] [output_blif] [clock_file] [optional_2nd_clock_file] \n");
	exit(-1);
}

# Read input/output file parameters
my $input_file_path = shift(@ARGV);
(-f $input_file_path) or die "Input file does not exists ($input_file_path)";

my $output_file_path = shift(@ARGV);
my $clock_file_path = shift(@ARGV);
my $clock2_file_path = shift(@ARGV);

# Get benchmark name
$input_file_path =~ /(.*?\/)+(.*?)\./;
my $bench_name = $2;
print "Benchmark name: $bench_name\n";

##########################################################################
############################ FIX LINE LENGTHS ############################
##########################################################################

open(IN_FILE_HDL, "<$input_file_path");
open(OUT_FILE_HDL, ">$output_file_path");

while (<IN_FILE_HDL>)
{
	my $line = $_;
	chomp($line);
	
	my $i = 0;
	my $start;
	while ((length($line) - $i) > 1500)
	{
		$start = $i;
		$i += 1000;
		while (substr($line, $i, 1) ne " ")
		{
			$i++;
		}
		print OUT_FILE_HDL substr($line, $start, $i - $start) . " \\\n";
	}
	print OUT_FILE_HDL substr($line, $i) . "\n";	
}
close (IN_FILE_HDL);
close (OUT_FILE_HDL);


##########################################################################
############################ ADD CLOCKS TO LATCHES #######################
##########################################################################

my $found = 0;
my $clock = "";

find_clock(\$found, \$clock, $clock_file_path, $bench_name);
find_clock(\$found, \$clock, $clock2_file_path, $bench_name);

if (! $found)
{
	print ("Could not find benchmark in bench/clock file(s): $clock_file_path,$clock2_file_path\nSee http://code.google.com/p/vtr-verilog-to-routing/wiki/ClocksFile\n");
	exit(-1);
}

if ($clock ne "")
{
	print "Clock Found:$clock\n";	
}
else
{
	print "Invalid clock found.\n";
	exit(-1);
}
	
open (OUT_FILE_HDL, "<$output_file_path");
my @lines = <OUT_FILE_HDL>;
close (OUT_FILE_HDL);

open (OUT_FILE_HDL, ">$output_file_path");

my $line;
foreach $line (@lines)
{
	chomp($line);
	$line =~ s/(.latch.*)( [012])/\1 re top\^$clock\2/;
	print OUT_FILE_HDL $line . "\n";
}
close (OUT_FILE_HDL);

print "\nSuccess\n";
# sed command to substitute the details in...note the \\ is used since passed to command prompt which chops one \
#print "sed -e \"s/\\(.latch.*\\)\\( [012]\\)/\\1 re $clock\\2/\" $output_file_path > $output_file_path";
#system "sed -e \"s/\\(.latch.*\\)\\( [012]\\)/\\1 re $clock\\2/\" $output_file_path > $output_file_path";


sub find_clock
{
	my $found_ref = shift();
	my $clock_name_ref = shift();
	my $clock_file = shift();
	my $bench_name = shift();
	
	my $line;
	
	if (-r $clock_file)
	{
		# open the bench/clock file.
		open (LIST_FILE, "<$clock_file");
	
		# loop looking for benchmark
		while ($line = <LIST_FILE>)
		{
			chomp($line);
		
			# Skip comments and empty lines
			if ($line =~ /^\w*#/ or $line =~ /^\w*$/)
			{
				next;	
			}	
			
			$line =~ m/^(\S+)\s+(\S+)\s*$/;
			if ($bench_name eq $1)
			{
				${$found_ref} = 1;
				${$clock_name_ref} = $2;			
				last;
			} 
		} 
		close (LIST_FILE);
	}
	
	return;
}	
