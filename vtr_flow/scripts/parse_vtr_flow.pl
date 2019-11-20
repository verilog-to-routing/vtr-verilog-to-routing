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

if ( $#ARGV + 1 < 2 ) {
	print "usage: parse_vtr_flow.pl <path_to_output_files> <config_file>\n";
	exit(-1);
}
my $parse_path = expand_user_path(shift);
my $parse_config_file = expand_user_path(shift);

if ( !-r $parse_config_file ) {
	die "Cannot find parse file ($parse_config_file)\n";
}

open (PARSE_FILE, $parse_config_file);
my @parse_lines = <PARSE_FILE>;
close (PARSE_FILE);

my @parse_data;
my $file_to_parse;
foreach my $line (@parse_lines) {
	chomp($line);

	# Ignore comments
	if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }
	
	my @name_file_regexp = split( /;/, $line, 3 );
	push(@parse_data, [@name_file_regexp]);	
}

my $count = 0;
for my $parse_entry (@parse_data) {
	$count++;
	if($count < scalar(@parse_data))
	{
		print @$parse_entry[0] . "\t";
	}
	else
	{
		print @$parse_entry[0];
	}
}
print "\n";

$count = 0;
for my $parse_entry (@parse_data) {
	my $file_to_parse = "@$parse_entry[1]";
	my $file_to_parse_path =
	  File::Spec->catdir( ${parse_path}, ${file_to_parse} );

	$count++;	
	if ( $file_to_parse =~ /\*/ ) {
		my @files = glob($file_to_parse_path);
		if ( @files == 1 ) {
			$file_to_parse_path = $files[0];
		}
		else {
			die "Wildcard in filename ($file_to_parse) matched " . @files
			  . " files.  There must be exactly one match.\n";
		}
	}
	if ( not -r "$file_to_parse_path" ) {
		print "-1";
		print "\t";
	}
	else {
		undef $/;
		open( DATA_FILE, "<$file_to_parse_path" );
		my $parse_file_lines = <DATA_FILE>;
		close(DATA_FILE);
		$/ = "\n";
		
		my $regexp = @$parse_entry[2];
		if ( $parse_file_lines =~ m/$regexp/gm ) {
			print $1;
		}
		else {
			print "-1";
		}
		if($count < scalar(@parse_data)) {
			print "\t";
		}
	}
}
print "\n";

sub expand_user_path {
	my $str = shift;	
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}