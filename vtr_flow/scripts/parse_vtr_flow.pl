#!/usr/bin/perl
###################################################################################
# This script is used to extract statistics from a single execution of the VTR flow
#
# Usage:
#	parse_vtr_flow.pl <parse_path> <parse_config_file> [<extra_param> ...]
#
# Parameters:
#	parse_path: Directory path that contains the files that will be 
#					parsed (vpr.out, odin.out, etc).
#	parse_config_file: Path to the Parse Configuration File
#	extra_param: Extra key=value pairs to be added to the output
###################################################################################


use strict;
use Cwd;
use File::Spec;
use File::Basename;

sub expand_user_path;

if ( $#ARGV + 1 < 2 ) {
	print "usage: parse_vtr_flow.pl <path_to_output_files> <config_file> [<extra_param> ...]\n";
	exit(-1);
}
my $parse_path = expand_user_path(shift(@ARGV));
my $parse_config_file = expand_user_path(shift(@ARGV));
my @extra_params = @ARGV;

if ( !-r $parse_config_file ) {
	die "Cannot find parse file ($parse_config_file)\n";
}

my @parse_lines = load_file_with_includes($parse_config_file);

my @parse_data;
my $file_to_parse;
foreach my $line (@parse_lines) {
	chomp($line);

	# Ignore comments
	if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }
	
	my @name_file_regexp = split( /;/, $line );
	push(@parse_data, [@name_file_regexp]);	
}

# attributes to parse become headings
for my $extra_param (@extra_params) {
    my ($param_name, $param_value) = split('=', $extra_param);
    print $param_name . "\t";
}

for my $parse_entry (@parse_data) {
    print @$parse_entry[0] . "\t";
}
print "\n";

for my $extra_param (@extra_params) {
    my ($param_name, $param_value) = split('=', $extra_param);
    print $param_value . "\t";
}
my $count = 0;
for my $parse_entry (@parse_data) {
	my $file_to_parse = "@$parse_entry[1]";
	my $file_to_parse_path =
	  File::Spec->catdir( ${parse_path}, ${file_to_parse} );
	my $default_not_found = "-1";
	if (scalar @$parse_entry > 3) {
	    $default_not_found = "@$parse_entry[3]";
    }

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
		print $default_not_found;
		print "\t";
	}
	else {
		open( DATA_FILE, "<$file_to_parse_path" );
		my @parse_file_lines = <DATA_FILE>;
		close(DATA_FILE);

        chomp(@parse_file_lines);
		
		my $regexp = @$parse_entry[2];
		$regexp =~ s/\s+$//;

        my $result = $default_not_found;
        for my $line (@parse_file_lines) {
            if ( $line =~ m/$regexp/gm ) {
                #Match
                $result = $1;

                #Note that we keep going even if a match is found,
                #so that the recorded value is the *last* match in
                #the file
            }
        }
        # tab separation even at end of line to indicate last element
        print "$result\t";
	}
}
print "\n";

sub expand_user_path {
	my $str = shift;	
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
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

	open(FILE, "<$filepath" ) or die("Failed to open file $filepath");
	my @lines = <FILE>;
	close(FILE);

    return @lines;
}
