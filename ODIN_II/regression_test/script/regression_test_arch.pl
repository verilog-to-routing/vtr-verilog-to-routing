#!/usr/bin/perl

##############################################################################
#
# AUTHOR: Peter Jamieson (paj)
# DATE: Started 01.23.2003
#
# Edited by Ken Kent (06.18.2009)
#  - Added ability to test configuration files instead of command line
#
# DESCRITION:
# 	This script executes each of the benchmarks through the verilog compiler.
#
# PARAMETERS:
#	1. The file list of benchmarks to execute
#	2. Path where benchmarks can be found
#	3. Path where to put ouput of benchmarks
#	4. Interactive option (y or n)
#	5. With valgrind option (y or n) // defunct
#
#	Sample:  bm_verilog_synthesize.pl sample_file_list.list . . n n
#
##############################################################################

##############################################################################
# GLOBALS
##############################################################################

# Directory Locations for all the CAD programs
$VERILOG_SYNTH="../odin_II.exe ";

use Cwd;
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------
# MAIN SCRIPT:
#	DESCRIPTION:
#-----------------------------------------------------------------------------
#-----------------------------------------------------------------------------

# check the parametes. Note PERL does not consider the script name a parameter.
$program_specified_arguments = 5;
$number_arguments = @ARGV;
if ($number_arguments != $program_specified_arguments)
{
	print("usage: regression_test_no_args.pl [golden_commit - yes or no] [file list - first line is comment] [path where] [path to] [path golden_results]\n");
	print("All paths should be relative to where this script is executed from\n");
	exit(-1);
}

$WHERE = 2;
$TO = 3;
$GOLDEN = 4;

$LOCAL_DIR = `pwd`;
chop($LOCAL_DIR);

# check if where exists 
if (chdir("$ARGV[$WHERE]") != 1)
{
	print("The where directory $ARGV[$WHERE] does not exist\n");
	exit(-1);
}

chdir($LOCAL_DIR);
# check if to exists.  If it doesn't, create it.
if (chdir("$ARGV[$TO]") != 1)
{
	# IF - does not exist then create directory
	printf("making $LOCAL_DIR - $ARGV[$TO]\n");
	mkdir("$ARGV[$TO]");
}

chdir($LOCAL_DIR);

# open the list file.
if(!(-e $ARGV[1]))
{
	print("file $ARGV[1] does not exist\n");
	exit(-1);
}
if((!(-e $ARGV[1])) || (open (LIST_FILE, "$ARGV[1]") != 1))
{
	print("file $ARGV[1] cannot be opened\n");
	exit(-1);
}
# open the corresponding file list
if(!(-e $ARGV[2]))
{
	print("file $ARGV[2] does not exist\n");
	exit(-1);
}
if((!(-e $ARGV[2])) || (open (ARG_FILE, "$ARGV[2]") != 1))
{
	print("file $ARGV[2] cannot be opened\n");
	exit(-1);
}

# go to the list dir
chdir("$ARGV[$TO]");

# skip first line 
$NEXT_FILE = <LIST_FILE>;
$NEXT_ARG = <ARG_FILE>;

# read the next file on the list.
while ($NEXT_FILE = <LIST_FILE>)
{
	$NEXT_ARG = <ARG_FILE>;

	chop($NEXT_FILE);
	chop($ARG_FILE);

	$FILE_WITH_DIR = "$ARGV[$WHERE]" . "/$NEXT_FILE";

	printf("SCRIPT_OUT - - $LOCAL_DIR/$ARGV[$WHERE]/$NEXT_FILE\n");
	# check if the file exists.
	if (-e "$LOCAL_DIR/$ARGV[$WHERE]/$NEXT_FILE")
	{
		# set up the regex buffer and extract the name without .v extension
		$_ = $NEXT_FILE;
		s/\.xml//g;	
		$NAME_WITHOUT_EXTENSION = $_;

		printf("--------------------------------------------------------------\n");
		# synthisize
		# Paths are quoted to support paths that include spaces
		$COMMAND="'$LOCAL_DIR/$VERILOG_SYNTH' -c '$LOCAL_DIR/$ARGV[$WHERE]/$NEXT_FILE' >& '$LOCAL_DIR/$ARGV[$TO]/$NAME_WITHOUT_EXTENSION.log'";
		printf("$COMMAND\n");
		# if not equal 0 then exited prematurely
		$RESULT = system("$COMMAND");
		printf("$RESULT\n");
		if ("$RESULT" ne '0')
		{
			printf("COMPILING: $NEXT_FILE Failed!!\n");
			exit(-1);
		}

		if ($ARGV[0] eq 'yes')
		{

		}
		else
		{
			$COMMAND="diff -q $ARGV[$TO]/$NAME_WITHOUT_EXTENSION.out $ARGV[$GOLDEN]/$ AME_WITHOUT_EXTENSION.out";
			printf("$COMMAND\n");
			# if result = 0 means that the files are the same
			$RESULT = system("$COMMAND");
			printf("$RESULT\n");
			if ("$RESULT" ne "0")
			{
				printf("DIFF: $NEXT_FILE Different Output...regression failed\n");
				exit(-1);
			}
		}
	}
	else
	{
		printf("File not found\n");
	}
}

sub COMMANDS 
{ 
	print("$_[0]\n");
	system("$_[0]");
}
