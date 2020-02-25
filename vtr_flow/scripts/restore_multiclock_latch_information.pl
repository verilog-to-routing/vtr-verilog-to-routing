#!/usr/bin/env perl
################################################################################
# Developed by Aaron G. Graham (aaron.graham@unb.ca) and
#  Dr. Kenneth B. Kent (ken@unb.ca) for ODIN II at the
#  University of New Brunswick, Fredericton
# 2018
#
# This script is necessary because ABC Does not support more then one clock
#  and drops all other clock information. This script restores the dropped
#  multi-clock information (from the ODIN II BLIF to the ABC Optimized BLIF)
#
# Inputs: <ODIN_II_BLIF_IN_FILE> <ABC_BLIF_IN_FILE> <ABC_BLIF_OUT_FILE>
#
################################################################################
#Open the ODIN II BLIF File
open(my $odinInFile, "<".$ARGV[0]) || die "Error Opening ODIN II File $ARGV[0]: $!\n";

#Open the ABC Optimized (ODIN II) BLIF File
open(my $abcInFile, "<".$ARGV[1]) || die "Error Opening ABC Input File $ARGV[1]: $!\n";

#Open the ABC Optimized (ODIN II) BLIF File
open(my $abcOutFile, ">".$ARGV[2]) || die "Error Opening Output File $ARGV[2]: $!\n";

#While there are lines in the ABC Optimized (ODIN II) BLIF File
while(($line = <$abcInFile>))
{
	#If the Line is a Latch
	if ($line =~ /^.latch/ )
	{
		#Tokenize the Line
		my @tokens = split(/[ \t\n\r]+/, $line);

		#Get the Number of Tokens
		$size = @tokens;

		#The Latch Already has Clock Information; Print as is
		if ($size >= 5)
		{
			print $abcOutFile $line;
		}
		#The Latch Is Missing the Clock Information
		elsif ($size >= 3)
		{
			#Find the Clock Info to Restore From the ODIN II BLIF File
			$found = 0;

			#Seek to the head of the ODIN II BLIF File
			seek odinInFile, 0, 0;

			#While there are lines in the ODIN II BLIF File and We Haven't
			# Found the Clock Info We're Looking For
			while(($lineOdn = <$odinInFile>) && !$found)
			{
				#If the Line is a Latch
				if ($lineOdn =~ /^.latch/)
				{
					#Tokenize the Line
					my @tokensOdn = split(/[ \t\n\r]+/, $lineOdn);

					#Get the Number of Tokens
					$sizeOdn = @tokensOdn;

					#If the Latch we've Found has Clock Information and
					# The Output Pin Matches the Latch We're Looking For
					if ($sizeOdn >=5 && $tokens[2] == $tokensOdn[2])
					{

						#Print Out the Line with the Restored Latch Information Included
					    print $abcOutFile join(" ", (@tokens[0], @tokens[1], @tokens[2], @tokensOdn[3], @tokensOdn[4], @tokens[3]) ), "\n";

						#Say that we Found the Droids we're Looking For
						$found = 1;
					}
				}
			}

			#Error out if we Didn't Find the Droids we're Looking for
			if(!$found)
			{
				die "Latch ".$tokens[2]." not found in Odin II BLIF!\n".$line."\n";
				exit(-1);
			}
		}
		#The Latch has an Un-Expected Number of Tokens
		else
		{
			die "Unexpected latch format: ".$size." tokens\n$line\n";
		}
	}
	#Line isn't a Latch; Print as is
	else
	{
		print $abcOutFile $line;
	}
}

#Close the ODIN II BLIF File
close($odin2File);

#Close the ABC Optimized (ODIN II) BLIF File
close($abcInFile);

#Close the processed BLIF file with restored clocks
close($abcOutFile);