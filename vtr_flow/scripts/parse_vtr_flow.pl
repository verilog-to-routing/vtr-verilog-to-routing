#!/usr/bin/perl
###################################################################################
# This script is used to extract statistics from a single execution of the VTR flow
#
# Usage:
#	parse_vtr_flow.pl <parse_path> <parse_config_file>
#
# Parameters:
#	parse_path: Directory path that contains the files that will be 
#					parsed (vpr.out, odin.out, etc).
#	parse_config_file: Path to the Parse Configuration File
###################################################################################


use strict;
use Cwd;
use File::Spec;

sub expand_user_path;

my $parse_path = expand_user_path(shift);
my $parse_config_file = expand_user_path(shift);

if (! -r $parse_config_file)
{
	die "Cannot find parse file ($parse_config_file)\n";
}

open (PARSE_FILE, $parse_config_file);
my @parse_lines = <PARSE_FILE>;
close (PARSE_FILE);

my @parse_data;
my $file_to_parse;
foreach my $line (@parse_lines)
{
	chomp($line);
	
	# Ignore comments
	if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }
	
	my @name_file_regexp = split( /;/, $line, 3 );
	push(@parse_data, [@name_file_regexp]);	
}

for my $parse_entry (@parse_data)
{
	print @$parse_entry[0] . "\t";
}
print "\n";

for my $parse_entry (@parse_data)
{
	my $file_to_parse = @$parse_entry[1];
	if (not -r "${parse_path}/${file_to_parse}")
	{
		die "Cannot open file to parse (${parse_path}/${file_to_parse})";
	}
	undef $/;
	open (DATA_FILE, "<$parse_path/$file_to_parse");
	my $parse_file_lines = <DATA_FILE>;
	close(DATA_FILE);
	$/ = "\n";
	
	my $regexp = @$parse_entry[2];
	if ($parse_file_lines =~ m/$regexp/g)
	{
		print $1;
	}
	print "\t";
}
print "\n";

sub expand_user_path
{
	my $str = shift;	
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}