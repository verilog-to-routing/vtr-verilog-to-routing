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

	Usage:	./exec <input_blif> [ <output_blif> [ --all/--vanilla/<clocks_not_to_black_box(comma separated list)> ] ]

		./exec <input_blif>
			-> comma separated list of clks in the files

		./exec <input_blif> <output_blif> [--all]
			-> write to file the result (all latches with clocks are black boxed) and report
		
		./exec <input_blif> <output_blif> --vanilla
			-> write to file the result (all latches are restored to vanilla) and report

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
my $all_bb = 0;
my $respect_init = 0;
my $respect_edge = 0;

my $clkfile = "";
my $has_output = 0;
my $parse_input = 0;
my $parse_output = 0;
my $parse_clk_list =0;
# this builds the truth table for the bb latch
# is it necessary ?						
# driver|clk output
#my $bb_latch_truth_table ="11 1\n";
my $bb_latch_truth_table ="";

if ($ARGC < 1 )
{
	print_help();
	exit(-1);
}

foreach my $cur_arg (@ARGV)
{
	if ($parse_input)
	{
		open($InFile, "<".$cur_arg) || die "Error Opening BLIF input File $cur_arg: $!\n";
		$clkfile = $cur_arg.".clklist";
		$parse_input = 0;
	}
	elsif ($parse_output)
	{
		$has_output = 1;
		open($OutFile, ">" .$cur_arg) || die "Error Opening BLIF output File $cur_arg: $!\n";
		$parse_output = 0;
	}
	elsif ($parse_clk_list)
	{
		foreach my $input_clocks (split(/,/, $cur_arg))
		{
			$clocks_not_to_bb{$input_clocks} = 1;
		}
		$parse_clk_list = 0;
	}	
	elsif ($cur_arg =~ /\-\-input/)
	{
		$parse_input = 1;
	}
	elsif ($cur_arg =~ /\-\-output/)
	{
		$parse_output = 1;
	}
	elsif ($cur_arg =~ /\-\-clk_list/)
	{
		$parse_clk_list = 1;
	}
	elsif ($cur_arg =~ /\-\-respect_init/)
	{
		$respect_init = 1;
	}
	elsif ($cur_arg =~ /\-\-respect_edge/)
	{
		$respect_edge = 1;
	}
	elsif ($cur_arg =~ /\-\-vanilla/)
	{
		$vanilla = 1;
	}
	elsif ($cur_arg =~ /\-\-all/)
	{
		$all_bb = 1;
	}
	else
	{
		print "Error wrong argument kind $cur_arg\n";
		print_help();
		exit(-1);
	}
}
if ($parse_input || $parse_output)
{
	print "Error wrong argument input\n";
	print_help();
	exit(-1);
}

#################################
# a clock name is defined in the map as <latch|$edge_type|$clk|$initial_value>
# build clocks names list and latches location list
# this also converts all black boxed latches to regular latches in one pass
#################################

my %bb_clock_domain;
my %vanilla_clk_domain;
my %unusable_clk_domain;

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
			if( $size == 6 )
			{
				my $clk_domain_name = "latch";
				my $display_domain_name = "latch";
				#use everything after the output to create a clk name, which translate to a clock domain
				for (my $i=3; $i < $size; $i++)
				{
					#abc sets these to 0
					if( (!$respect_init || $i != 5) && ( !$respect_edge || $i != 3 ) )
					{
						$clk_domain_name .= $uniqID_separator.@tokens[$i];
					}
					$display_domain_name .= $uniqID_separator.@tokens[$i];
				}

				#if we have an output file then we can black box some clocks
				if( (!$all_bb) && ($vanilla || exists($clocks_not_to_bb{$clk_domain_name})) )
				{
					if( !$vanilla )
					{
						#show that we have found this clock
						$clocks_not_to_bb{$clk_domain_name} += 1;
					}

					if(!exists($vanilla_clk_domain{$display_domain_name}))
					{
						$vanilla_clk_domain{$display_domain_name} = 0;	
					}
					$vanilla_clk_domain{$display_domain_name} += 1;
				}
				else
				{
					$line = ".subckt ".$display_domain_name." i[0]=".@tokens[1]." i[1]=".@tokens[4]." o[0]=".@tokens[2]."\n";
					if(!exists($bb_clock_domain{$display_domain_name}))
					{
						$bb_clock_domain{$display_domain_name} = 0;	
					}
					$bb_clock_domain{$display_domain_name} += 1;
				}
			}
			else
			{
				$unusable_clk_domain{$line} .= "${lineNum}, ";
			}
		}
		
		# if we have an output file, print the line to it
		if($has_output)
		{
			print $OutFile $line
		}
	}
	
	#if the Line is a .end after a bb module declaration dont skip thereafter
	$skip = ( ($skip) && ($line =~ /^\.end/))? 0: $skip;
}

# if we have an output file print the .module for bb latches at the end of the file
if( $has_output )
{
	foreach my $module (keys %bb_clock_domain) 
	{
		print $OutFile ".model ${module}\n.inputs i[0] i[1]\n.outputs o[0]\n${bb_latch_truth_table}.blackbox\n.end\n\n";
	}
	close($OutFile);
}
# else create a file wich contains all the clock domains in the file (1/line)
else
{
	open($OutFile, ">" .$clkfile) || die "Error Opening clock list output File $clkfile: $!\n";
	print $OutFile (join("\n", keys %vanilla_clk_domain))."\n".(join("\n", keys %bb_clock_domain))."\n";
	close($OutFile);
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

print "\n____\nUnusable Latches:";
if( ($size = %unusable_clk_domain) == 0)
{
	print "\n\t-None-";
}
foreach my $clks (keys %unusable_clk_domain) 
{
	print "\n\t".$clks."\tlines:: ".$unusable_clk_domain{$clks};
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
close($InFile);

exit(0);