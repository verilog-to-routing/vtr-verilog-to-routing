#!/usr/bin/perl

################################################################################################
#
# AUTHOR: Peter Jamieson (paj)
# DATE: Started Nov_29_2004
#
# DESCRITION:
#	This script goes through a list of verilog files and takes each file, and breaks the file
#	into the hierarchy of modules (pieces).  Each module is appended with the name of the original 
#	file.
#
# PARAMETERS:
#	1. A file that holds the list of files that have the values we're interested in
#
################################################################################################

################################################################################################
# GLOBALS
################################################################################################
$SCRIPT_DIR="/nfs/eecg/q/grads10/jamieson/WORKSPACE/CAD_FLOW/PERL_SCRIPTS/GENERAL_SCRIPTS";

#-----------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------
# MAIN SCRIPT:
#	DESCRIPTION:
#-----------------------------------------------------------------------------------------------
#-----------------------------------------------------------------------------------------------

# check the parametes.  Note PERL does not consider the script itself a parameter.
$program_specified_arguments = 1;
$number_arguments = @ARGV;
if ($number_arguments != $program_specified_arguments)
{
	print("usage: split_up_verilog_file_into_module.pl [file list - first line is comment]\n");
	exit(-1);
}

# record the present directory in case we start moving around
$LOCAL_DIR=`pwd`;
chop($LOCAL_DIR);

# open the list file.
if((!(-e $ARGV[0])) || (open (LIST_FILE, "$ARGV[0]") != 1))
{
	print("file $ARGV[0] cannot be opened\n");
	exit(-1);
}

# skip first line 
$NEXT_FILE = <LIST_FILE>;

# store all the files in an array
while ($NEXT_FILE = <LIST_FILE>)
{
	chop($NEXT_FILE);

	# record the name of this file so we can process it
	push(@FILES, $NEXT_FILE);
}

close LIST_FILE;

for ($i = 0; $i < @FILES; $i++)
{
	printf("Next file: $FILES[$i]\n");

	# take out all the ` chracters
	$COMMAND = "sed -e \"s/\\`/apostpajrophe/g\" $FILES[$i] > $FILES[$i]_tmp";
	COMMANDS($COMMAND);

	# check and open each file
	if((!(-e $FILES[$i])) || (open (V_FILE, "$FILES[$i]_tmp") != 1))
	{
		print("file $FILES[$i]_tmp cannot be opened\n");
		exit(-1);
	}

	# Get the root name without .v
	$_ = $FILES[$i];
	s/\.v//g;	

	$BASE_NAME = $_;
	printf("Base_name:$BASE_NAME\n");

	$DIR_NAME = $BASE_NAME."_tmp";
	printf("Creating dir: $DIR_NAME in $LOCAL\n");
	# create a directory for this base name
	mkdir("$LOCAL_DIR/$DIR_NAME");

	# initialize the writing to file flag
	$FILE_WRITING = 0;
	# empty the module list
	@MODULE_NAMES = ();

	$MODULE_COUNT = 1;

	# clean arrays
	@NUM_OF_CHILD_MODULES = ();
	@MODULE_NAMES = ();
	@MODULE_HAS_CHILDREN = ();
	@MODULE_PROCESSED = ();
	@INDEX_FOR_MODULE = ();
	@CHILD_MODULE = ();

	# make the first lines in a header file that can be joined with all so that the defines are included everywhere
	$MODULE_FILE_NAME = $DIR_NAME."/header.v";
	# make the local file for describing the list
	$NEW_FILE_LIST_NAME = "FILES.list";
	$COMMAND="echo \"# Comment line\" >> $DIR_NAME/$NEW_FILE_LIST_NAME";
	COMMANDS($COMMAND);

	# read line by line the verilog file
	while ($NEXT_V_LINE = <V_FILE>)
	{
		chop($NEXT_V_LINE);
		if ($NEXT_V_LINE =~ /^[ 	]*\/\//)
		{
			# skip comments
		}
		#check if the line is endmodule
		elsif ($NEXT_V_LINE =~ /endmodule/ )
		{
			# pass the enmodule line through
			$COMMAND="echo \"$NEXT_V_LINE\" >> $MODULE_FILE_NAME";
			COMMANDS_NO_PRINT($COMMAND);

			# store the list of these child modules
			push (@NUM_OF_CHILD_MODULES, $NUM_CHILD_MODULES);

			# indicate that another module is fully analyzed
			$MODULE_COUNT ++;
		}
		# check if the line is module X (.*
		elsif ($NEXT_V_LINE =~ /(module)[ 	]*([(a-z)(A-Z)_(0-9)]*)[ 	]*(\(.*)/ )
		{
			# extracted module name
			$MODULE_NAME = $2;
			printf("Found module: $2 in $FILES[$i]\n$1|\n $3|\n");

			# record the module name
			push(@MODULE_NAMES, $MODULE_NAME);
			# add an index with this name to a hash
			$INDEX_FOR_MODULE{$MODULE_NAME} = $MODULE_COUNT;
			# push an initializer for a true false list that says whether this module has children
			push(@MODULE_HAS_CHILDREN, -1);
			# push an initializer to indicate that this MODULE has not been fully defined yet
			push(@MODULE_PROCESSED, -1);
			$NUM_CHILD_MODULES = 0;

			# what I want to name the file
			$MODULE_FILE_NAME = $DIR_NAME."/".$MODULE_NAME.".v";
			printf("Dir Name: $MODULE_FILE_NAME\n");

			# erase the old version
			$COMMAND="rm -f $MODULE_FILE_NAME";
			COMMANDS($COMMAND);

			# print what we want to a file that represents this module
			$COMMAND="echo \"$1 $2 $3\" >> $MODULE_FILE_NAME";
			COMMANDS_NO_PRINT($COMMAND);
		}
		# check if a module uses a module
		elsif ($NEXT_V_LINE =~ /([(a-z)(A-Z)_(0-9)]*)[ 	]*([(a-z)(A-Z)_(0-9)]*)[ 	]*\(.*/ )
		{
		#	printf("Found module call:\n");
			# record that this module has children
			$MODULE_HAS_CHILDREN[$MODULE_COUNT-1] = 1;

			# record that there is a child for this module
			$NUM_CHILD_MODULES ++;
			
			# record the child module name
			push (@CHILD_MODULE, $1);

			# pass this info out
			$COMMAND="echo \"$NEXT_V_LINE\" >> $MODULE_FILE_NAME";
			COMMANDS_NO_PRINT($COMMAND);
		}
		else 
		{
#			printf("ELSE: $NEXT_V_LINE\n");
			$COMMAND="echo \"$NEXT_V_LINE\" >> $MODULE_FILE_NAME";
			COMMANDS_NO_PRINT($COMMAND);
		}
	}

	# now that we've read all the module names, we now go through and concat ones that depend on others

	# close the entire verilog file
	close V_FILE;

	# print out some lists to check if they're ok
	printf("-----------------\n");
	print "@MODULE_NAMES";
	printf("-----------------\n");
	print "@NUM_OF_CHILD_MODULES";
	printf("-----------------\n");
	print "@$MODULE_PROCESSED";
	printf("-----------------\n");

	$STILL_TO_DO = 1;
	while ($STILL_TO_DO == 1)
	{
		$STILL_TO_DO = 0;
		$CURRENT_CHILD_MODULE_SPOT = 0;

		for ($j = 0; $j < @MODULE_NAMES; $j++)
		{
			if ($MODULE_PROCESSED[$j] == -1)
			{
				# IF - this module still hasn't been processed, check if all children are processed
				$STILL_TO_DO = 1;
				printf("[$j] PROCESSING module name:$MODULE_NAMES[$j]\n");
				
				$STILL_CAN_PROCESS = 1;

				for ($k = 0; $k < $NUM_OF_CHILD_MODULES[$j]; $k++) 
				{
					$NAME = $CHILD_MODULE[$CURRENT_CHILD_MODULE_SPOT+$k];
					$INDEX = $INDEX_FOR_MODULE{$NAME};
					printf("[$j,$k] NAME $NAME and INDEX $INDEX\n");

					if (($INDEX >= 1) && ($MODULE_PROCESSED[$INDEX-1] == 1))
					{
						# IF - this is a proper reference to a module check and the child has been proceesed
						$STILL_CAN_PROCESS = 1;
					}
					elsif ($INDEX >= 1)
					{	
						# ELSE IF - this module depends on still to be defined children
						printf("Can't Process $$STILL_CAN_PROCESS\n");
						$STILL_CAN_PROCESS = 0;
						last;
					}
					else
					{
						# ELSE - ignore this regexpression catch
					}
				}	
				
				if ($STILL_CAN_PROCESS == 1)
				{
					# if all children defined, then concat each of the modules into one file
					printf("Still can process - processing\n");

					# when we can define the module produce the name and temp_name file
					$ORIGINAL_NAME = $MODULE_NAMES[$j].".v";
					$TEMP_NAME = $ORIGINAL_NAME."_tmp";

					# add the header file
					$MODULE_FILE_NAME = $DIR_NAME."/header.v";
					if (-e $MODULE_FILE_NAME)
					{
						$COMMAND = "cat $MODULE_FILE_NAME $DIR_NAME/$ORIGINAL_NAME > $DIR_NAME/$TEMP_NAME";
						COMMANDS($COMMAND);
						$COMMAND = "mv $DIR_NAME/$TEMP_NAME $DIR_NAME/$ORIGINAL_NAME";
						COMMANDS($COMMAND);
					}

					# initialize a loop hash so we don't multiply concat a module
					@LOOP_HASH = ();

					# now add all the children
					for ($k = 0; $k < $NUM_OF_CHILD_MODULES[$j]; $k++) 
					{
						# generate the name and find an INDEX in the hash 
						$NAME = $CHILD_MODULE[$CURRENT_CHILD_MODULE_SPOT+$k];
						$INDEX = $INDEX_FOR_MODULE{$NAME};
						$TO_CONCAT_NAME = $NAME.".v";

						# check if this name is already been processed
						$ALREADY_INDEXED = $LOOP_HASH{$NAME};
						if ($ALREADY_INDEXED == 1)
						{
							next;
						}
						else
						{
							# ELSE - add to hash for next time
							$LOOP_HASH{$NAME} = 1;
						}

						# if this module exists and has been defined then concat it
						if (($INDEX >= 1) && ($MODULE_PROCESSED[$INDEX-1] == 1))
						{
							$COMMAND = "cat $DIR_NAME/$ORIGINAL_NAME $DIR_NAME/$TO_CONCAT_NAME > $DIR_NAME/$TEMP_NAME";
							COMMANDS($COMMAND);
							$COMMAND = "mv $DIR_NAME/$TEMP_NAME $DIR_NAME/$ORIGINAL_NAME";
							COMMANDS($COMMAND);
						}
					}

					# put all the ` characters back
					$COMMAND = "sed -e \"s/apostpajrophe/\\`/g\" $DIR_NAME/$ORIGINAL_NAME > $DIR_NAME/$TEMP_NAME";
					COMMANDS($COMMAND);
					$COMMAND = "mv $DIR_NAME/$TEMP_NAME $DIR_NAME/$ORIGINAL_NAME";
					COMMANDS($COMMAND);

					# add this module to a list which defines all the modules that need to be compiled
					$COMMAND="echo \"$MODULE_NAMES[$j].v\" >> $DIR_NAME/$NEW_FILE_LIST_NAME";
					COMMANDS($COMMAND);

					# Flag that this module is now fully defined
					$MODULE_PROCESSED[$j] = 1;
				}
			}

			# move the wandering count up
			$CURRENT_CHILD_MODULE_SPOT += $NUM_OF_CHILD_MODULES[$j];
		}
		printf("-----------------\n");
	}
}

sub COMMANDS_NO_PRINT
{ 
#	printf("\n");
#	printf("--------------------------------------------------------------\n");
#	print("$_[0]\n");
	system("$_[0]");
#	printf("--------------------------------------------------------------\n");
#	printf("\n");
}

sub COMMANDS 
{ 
	printf("\n");
	printf("--------------------------------------------------------------\n");
	print("$_[0]\n");
	system("$_[0]");
	printf("--------------------------------------------------------------\n");
	printf("\n");
}
