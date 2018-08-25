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

		./exec <input_blif> <output_blif> <clocks_not_to_black_box>
			-> write to file the result (all latches with clocks except those specified are black boxed) and report

";
}

my $ARGC = @ARGV;
my %clocks_not_to_bb;
my $OutFile;
my $InFile;
my $uniqID_separator = "_^_";
my $vanilla = 0;

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
	if ($ARGV[2] =~ /\-\-vanilla/)
	{
		$vanilla = 1;
	}
	else
	{
		foreach my $input_clocks (split(/,/, $ARGV[2]))
		{
			$clocks_not_to_bb{$input_clocks} = 1;
		}
	}
}

#################################
# a clock name is defined in the map as <latch|$edge_type|$clk|$initial_value>
# build clocks names list and latches location list
# this also converts all black boxed latches to regular latches in one pass
#################################

my %bb_clock_domain;
my %vanilla_clk_domain;

my %latches_location_map;

my $skip = 0;
my $lineNum = 0;
while( ($line = <$InFile>))
{
	$lineNum += 1;
	
	#If the Line is a bb latch module declaration skip it
	$skip = ( (!$skip) && ($line =~ /^\.model[\s]+latch/) )? 1: $skip;

	if (!$skip)
	{
		##############################
		# if it finds a blackboxed latch restore the line read to a vanilla latch
		#   blackboxed latch
		#		[0]		[1]							[2]				[3]			[4]
		#       .subckt	latch|<type>|<clk>|<init>	i[0]=<driver> 	i[1]=clk	o[0]=<output>
		if ($line =~ /^\.subckt[\s]+latch/)
		{
			my @tokens = split(/[\s]+/, $line);
			my @reformat_clk = split(/\Q$uniqID_separator\E/, @tokens[1]);
			my @reformat_driver = split(/\=/, @tokens[2]);
			my @reformat_output = split(/\=/, @tokens[4]);

			$line = ".latch ".@reformat_driver[1]." ".@reformat_output[1]." ".@reformat_clk[1]." ".@reformat_clk[2]." ".@reformat_clk[3]."\n";
		}

		##############################
		# if it finds a vanilla latch and if it has a clock domain use, otherwise it is a flip flop and just print it
		#   vanilla latches tokens
		#		[0]		[1]			[2]			[3]			[4]		[5]
		#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
		if($line =~ /^\.latch/)
		{
			$size = my @tokens = split(/[\s]+/,$line);
			if($size > 4)
			{

				my $clk_domain_name = "latch";
				#use everything after the output to create a clk name, which translate to a clock domain
				for (my $i=3; $i < $size; $i++)
				{
					$clk_domain_name .= $uniqID_separator.@tokens[$i];
				}

				#if we have an output file then we can black box some clocks
				if($vanilla || exists($clocks_not_to_bb{$clk_domain_name}))
				{
					if( !$vanilla )
					{
						#show that we have found this clock
						$clocks_not_to_bb{$clk_domain_name} += 1;
					}

					if(!exists($vanilla_clk_domain{$clk_domain_name}))
					{
						$vanilla_clk_domain{$clk_domain_name} = 0;	
					}
					$vanilla_clk_domain{$clk_domain_name} += 1;
				}
				else
				{
					$line = ".subckt ".$clk_domain_name." i[0]=".@tokens[1]." i[1]=".@tokens[4]." o[0]=".@tokens[2]."\n";
					if(!exists($bb_clock_domain{$clk_domain_name}))
					{
						$bb_clock_domain{$clk_domain_name} = 0;	
					}
					$bb_clock_domain{$clk_domain_name} += 1;
				}
			}
		}
		
		# if we have an output file, print the line to it
		if( $ARGC > 1 )
		{
			print $OutFile $line
		}

	}
	
	#if the Line is a .end after a bb module declaration dont skip thereafter
	$skip = ( ($skip) && ($line =~ /^\.end/))? 0: $skip;
}

# if we have an output file print the .module for bb latches at the end of the file
if( $ARGC > 1 )
{
	foreach my $module (keys %bb_clock_domain) 
	{
		print $OutFile ".model ".$module."\n.inputs i[0] i[1]\n.outputs o[0]\n11 1\n.blackbox\n.end\n\n";
	}
}
# else create a file wich contains all the clock domains in the file (1/line)
else
{
	print $OutFile (join("\n", keys %vanilla_clk_domain))."\n".(join("\n", keys %bb_clock_domain))."\n";
}

####################################################
#begin reporting
####################################################

print "Usage Statistic:\n\t<Clock UniqID>\t#<Number of Latches impacted>";

#report number of latches per clock domain and if the clock domain is black boxed or not
print "\n____\nVanilla latches:";
if( ($size = %vanilla_clk_domain) == 0)
{
	print "\n\t-None-";
}
foreach my $clks (keys %vanilla_clk_domain) 
{
	print "\n\t".$clks."\t#".$vanilla_clk_domain{$clks};
}

print "\n____\nBlack Boxed Latches:";
if( ($size = %bb_clock_domain) == 0)
{
	print "\n\t-None-";
}
foreach my $clks (keys %bb_clock_domain) 
{
	print "\n\t".$clks."\t#".$bb_clock_domain{$clks};
}

#report wrongly typed/ unexistant input clock domain to keep vanilla
foreach my $clks (keys %clocks_not_to_bb) 
{
	if ( $clocks_not_to_bb{$clks} != 1 )
	{
		delete $clocks_not_to_bb{$clks}
	}
}
if( ($size = keys %clocks_not_to_bb) > 0 )
{
	print "\n####\nERROR: malformed or non-existant input clock:\n\t<".(join(">\n\t<", keys %clocks_not_to_bb)).">\n####\n";
	exit(-1);
}

print "\n\n";
close($OutFile);
close($InFile);

exit(0);