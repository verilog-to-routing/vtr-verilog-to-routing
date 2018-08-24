#!/usr/bin/perl
################################################################################
# Developed by Jean-Philippe Legault (jeanlego@github) and
#  Dr. Kenneth B. Kent (ken@unb.ca) for ODIN II at the
#  University of New Brunswick, Fredericton
# 2018
################################################################################

sub print_help(){
print 
"
	This script is necessary because ABC Does not support more then one clock and drops all other clock information. 
	This script black boxes latches unless specified and vice versa.

	Usage:	./exec <input_blif> [ <output_blif> [ <clocks_not_to_black_box(comma separated list)> ] ]

		./exec <input_blif>
			-> comma separated list of clks in the files

		./exec <input_blif> <output_blif>
			-> write to file the result (all latches with clocks are black boxed) and report

		./exec <input_blif> <output_blif> <clocks_not_to_black_box(comma separated list)>
			-> write to file the result (all latches with clocks except those specified are black boxed) and report

";
}

my $ARGC = @ARGV;
my %clocks_not_to_bb;
my $OutFile;
my $InFile;

if ( $ARGC > 3 || $ARGC < 1 )
{
	print_help();
	exit(-1);
}

open($InFile, "<".$ARGV[0]) || die "Error Opening BLIF input File $ARGV[0]: $!\n";

if ( $ARGC > 1 )
{
	open($OutFile, ">" .$ARGV[1]) || die "Error Opening BLIF output File $ARGV[1]: $!\n";
}
else
{
	my $clkfile = $ARGV[0].".clklist";
	open($OutFile, ">" .$clkfile) || die "Error Opening clock list output File $ARGV[0].clklist: $!\n";
}

if ( $ARGC > 2 )
{
	for my $clks (split(/[\s]*,[\s]*/,$ARGV[2])) 
	{
		$clocks_not_to_bb{$clks} = 1;
	}
}

#################################
# a clock name is defined in the map as <latch|$edge_type|$clk|$initial_value>
# build clocks names list and latches location list
# this also converts all black boxed latches to regular latches in one pass
#################################

my %bb_clocks_map;
my %clocks_map;
my %latches_location_map;

my $skip = 0;
my $lineNum = 0;

#only print blackboxed version
while( ($line = <$InFile>) && !eof ($InFile))
{
	$lineNum += 1;
	
	#If the Line is a bb module declaration skip it
	$skip = ( (!$skip) && (index(lc($line), ".model latch") != -1) )? 1: $skip;

	#if the Line is a .end after a bb module declaration dont skip it
	$skip = ( ($skip) && (index(lc($line), ".end") != -1) )? 0: $skip;


	if (!$skip)
	{
		if ($line =~ /^\.subckt[\s]+latch\|/)
		{
			my @tokens = split(/[\s]+/, $line);

			#   blackboxed latch
			#		[0]		[1]							[2]				[3]
			#       .subckt	latch|<type>|<clk>|<init>	i[0]=<driver> 	o[0]=<output>
			
			$size = @tokens;

			my @reformat_clk = split(/\|/, @tokens[1]);
			my @reformat_driver = split(/\=/, @tokens[2]);
			my @reformat_output = split(/\=/, @tokens[3]);

			if(  $ARGC > 1 && (exists($clocks_not_to_bb{@tokens[1]})) )
			{
				$clocks_not_to_bb{@tokens[1]} += 1;
			}

			if(  $ARGC > 1 && (exists($clocks_not_to_bb{@tokens[1]})) )
			{
				print $OutFile ".latch ".@reformat_driver[1]." ".@reformat_output[1]." ".@reformat_clk[1]." ".@reformat_clk[2]." ".@reformat_clk[3]."\n";
				if( ! exists( $clocks_map{@tokens[1]} ) )
				{
					$clocks_map{@tokens[1]} = 1;
				}
			}
			elsif( ! exists( $bb_clocks_map{@tokens[1]} ) )
			{
				$bb_clocks_map{@tokens[1]} = 1;
			}
		}
		elsif ($line =~ /^\.latch/)
		{
			#   original latch with clock, 6 tokens only!
			#		[0]		[1]			[2]			[3]		[4]		[5]
			#       .latch	<driver>	<output>	<type>	<clk>	<init>
			my @tokens = split(/[\s]+/, $line);
			$size = @tokens;
			my $latch_clk_name = "latch";

			#use everything after the output to create a clk name, which translate to a clock domain
			for (my $i=3; $i < $size; $i++)
			{
				$latch_clk_name .= "|".@tokens[$i];
			}

			if( $size > 3 )
			{

				if(  $ARGC > 1 && (exists($clocks_not_to_bb{$latch_clk_name})) )
				{
					$clocks_not_to_bb{$latch_clk_name} += 1;
				}

				if( $ARGC > 1 && (!exists($clocks_not_to_bb{$latch_clk_name})) )
				{
					print $OutFile ".subckt ".$latch_clk_name." i[0]=".@tokens[1]." o[0]=".@tokens[2]."\n";
					if( ! exists( $bb_clocks_map{$latch_clk_name} ) )
					{
						$bb_clocks_map{$latch_clk_name} = 1;
					}
				}
				elsif( ! exists( $clocks_map{$latch_clk_name} ) )
				{
					$clocks_map{$latch_clk_name} = 1;
				}
			}
		}
		elsif( $ARGC > 1 )
		{
			print $OutFile $line;
		}

	}
}

#print the .module for bb latches at the end of the file
foreach my $module (keys %bb_clocks_map) 
{
	print $OutFile ".module ".$module."\n.input\n i[0]\n.output\n o[0]\n.blackbox\n.end\n\n";
}

my $clocked = join(", ", keys %clocks_map);
my $bb_clocked = join(", ", keys %bb_clocks_map);
print "clocks: ".$clocked."\n";
print "black_boxed_clocks: ".$bb_clocked."\n";

if ( $ARGC > 2 )
{
	foreach my $clks (keys %clocks_not_to_bb) 
	{
		if ( $clocks_not_to_bb{$clks} != 1 )
		{
			delete $clocks_not_to_bb{$clks}
		}
	}
	if( ($size = keys %clocks_not_to_bb) > 0 )
	{
		my $wrong_input_clk = join(", ", keys %clocks_not_to_bb);
		print "malformed or non-existant input clock: ".$wrong_input_clk."\n";

	}

}
if ( $ARGC == 1)
{
	my $clocked = join("\n", keys %clocks_map);
	my $bb_clocked = join("\n", keys %bb_clocks_map);
	print $OutFile $clocked."\n".$bb_clocked."\n";
}

close($OutFile);
close($InFile);

exit(0);