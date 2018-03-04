#!/usr/bin/perl
################################################################################
# Developed by Aaron G. Graham (aaron.graham@unb.ca) and
#  Dr. Kenneth B. Kent (ken@unb.ca) for ODIN II at the
#  University of New Brunswick, Fredericton
# 2018
#
# Inputs: <ODIN_II_BLIF_FILE> <ABC_BLIF_FILE>
#
################################################################################

open(odin2File, "<".$ARGV[0]) || die "Error Opening ODIN II File: $!\n";

open(abcFile, "<".$ARGV[1]) || die "Error Opening ABC File: $!\n";

while(($line = <abcFile>))
{
	if ($line =~ /^.latch/ )
	{
		my @tokens = split(/[ \t\n\r]+/, $line);

		$size = @tokens;

		if ($size >= 5)
		{
			print $line;
		}
		elsif ($size >= 3)
		{
			#Find the Clock Info to Restore From ODIN II BLIF File
			$found = 0;

			seek odin2File, 0, 0;

			while(($lineOdn = <odin2File>) && !$found)
			{
				if ($lineOdn =~ /^.latch/)
				{
					my @tokensOdn = split(/[ \t\n\r]+/, $lineOdn);

					$sizeOdn = @tokensOdn;

					if ($sizeOdn >=5 && $tokens[2] == $tokensOdn[2])
					{
						#Echo the Updated Line
					    print join(" ", (@tokens[0], @tokens[1], @tokens[2], @tokensOdn[3], @tokensOdn[4], @tokens[3]) ), "\n";

						$found = 1;
					}
				}
			}

			if(!$found)
			{
				die "Latch ".$tokens[2]." not found in Odin II BLIF!\n".$line."\n";
				exit -1;
			}
		}
		else
		{
			die "Unexpected latch format: ".$size." tokens\n$line\n";
		}
	}
	else
	{
		print $line;
	}
}

close(odin2File);

close(abcFile);
