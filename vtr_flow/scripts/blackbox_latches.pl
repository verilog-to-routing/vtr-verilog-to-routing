#!/usr/bin/perl
################################################################################
# Developed by Jean-Philippe Legault (jeanlego@github) and
#  Dr. Kenneth B. Kent (ken@unb.ca) for ODIN II at the
#  University of New Brunswick, Fredericton
# 2018
################################################################################

sub print_help{
print 
"
	This script is necessary because ABC Does not support more then one clock and drops all other clock information. 
	This script black boxes latches unless specified and vice versa.

	Usage:	./exec 
				--input				Bliff input file
				--output_list			Clock list output file
				--output			Bliff output file
				--clk_list			List of clocks not to black box
				--vanilla			Convert the bliff file to non boxed latches
				--all				Convert the bliff file to all boxed latches
";
}

sub print_stats{
	my %domain = %{$_[0]};
	print "\n____\n".$_[1]." Latches:";
	if( ($size = %domain) == 0)
	{
		print "\n\t-None-";
	}
	foreach my $clks (keys %domain) 
	{
		print "\n\t".$clks."\t#".$domain{$clks};
	}
}

my %clocks_not_to_bb;
my %bb_clock_domain;
my %vanilla_clk_domain;

my $OutFile;
my $InFile;
my $ClkFile;

my $uniqID_separator = "_^_";

my $vanilla = 0;
my $all_bb = 0;

my $has_output = 0;
my $has_input = 0;
my $has_output_clk = 0;
my $has_clk_list = 0;

my $parse_input = 0;
my $parse_output = 0;
my $parse_clk_list = 0;
my $parse_output_clk_file = 0;

foreach my $cur_arg (@ARGV)
{
	if ($parse_input)
	{
		$parse_input = 0;

		open($InFile, "<".$cur_arg) || die "Error Opening BLIF input File $cur_arg: $!\n";
		print "using input blif: ".$cur_arg."\n";

		$has_input = 1;
	}
	elsif ($parse_output) 
	{
		$parse_output = 0;

		open($OutFile, ">" .$cur_arg) || die "Error Opening BLIF output File $cur_arg: $!\n";
		print "using output blif: ".$cur_arg."\n";

		$has_output = 1;
	}
	elsif ($parse_output_clk_file) 
	{
		$parse_output_clk_file = 0;

		open($ClkFile, ">" .$cur_arg) || die "Error Opening Clock list output File $cur_arg: $!\n";
		print "using output clock file: ".$cur_arg."\n";
		
		$has_output_clk = 1;
	}
	elsif ($parse_clk_list) {
		$parse_clk_list = 0;

		foreach my $input_clocks (split(/,/, $cur_arg)) {
			$clocks_not_to_bb{$input_clocks} = 1;
		}

		print "using folowing clk domain: ";
		foreach my $clks (keys %clocks_not_to_bb) {
			print $clks." ";
		}
		print "\n";

		$has_clk_list = 1;
	} else {
		if ($cur_arg =~ /\-\-input/) {
			$parse_input = 1;
		}elsif ($cur_arg =~ /\-\-output_list/) {
			$parse_output_clk_file = 1;	
		}elsif ($cur_arg =~ /\-\-output/) {
			$parse_output = 1;			
		}elsif ($cur_arg =~ /\-\-clk_list/) {
			$parse_clk_list = 1;
		}elsif ($cur_arg =~ /\-\-vanilla/) {
			$vanilla = 1;
		}elsif ($cur_arg =~ /\-\-all/) {
			$all_bb = 1;
		}else {
			print "Error wrong argument kind $cur_arg\n";
			print_help();
			exit(-1);
		}
	}
}

#default is vanilla all latches
if(!$vanilla && !$all_bb && !$has_clk_list)
{
	$vanilla = 1;
}

if(!$has_output && ($all_bb || $has_clk_list))
{
	print "Cannot specify a rewrite directive without an output file\n";
	print_help();
	exit(-1);
}

if( ($vanilla && $all_bb) || ($all_bb && $has_clk_list) || ($vanilla && $has_clk_list))
{
	print "Cannot specify more than one output rewrite directive\n";
	print_help();
	exit(-1);
}

if($parse_input || $parse_clk_list || $parse_output || $parse_output_clk_file)
{
	print "Missing filename for I/O files argument\n";
	print_help();
	exit(-1);
}

if(! $has_input )
{
	print "Missing input blif file\n";
	print_help();
	exit(-1);
}


#################################
# a clock name is defined in the map as <latch|$edge_type|$clk|$initial_value>
# build clocks names list and latches location list
# this also converts all black boxed latches to regular latches in one pass
#################################

my $skip = 0;
my $lineNum = 0;
while( (my $line = <$InFile>) )
{
	$lineNum += 1;
	
	#If the Line is a bb latch module declaration skip it
	$skip = ( (!$skip) && ($line =~ /^\.model[\s]+latch/) )? 1: $skip;

	if (!$skip)
	{
		my @latch_clk_tokens;

		# BLACK BOX
		if ($line =~ /^\.subckt[\s]+latch/)
		{
			#   blackboxed latch (creted by this script, so it will always respect this format)
			#		[0]		[1]							[2]				[3]			[4]
			#       .subckt	latch|<type>|<clk>|<init>	I=<driver> 	C=clk	O=<output>
			my @tokens = split(/[\s]+/, $line);
			my @reformat_clk = split(/\Q$uniqID_separator\E/, @tokens[1]);
			my @reformat_driver = split(/\=/, @tokens[2]);
			my @reformat_output = split(/\=/, @tokens[4]);
			
			@latch_clk_tokens = (".latch", @reformat_driver[1] ,@reformat_output[1], @reformat_clk[1], @reformat_clk[2], @reformat_clk[3]);
		}
		#LATCHES
		elsif($line =~ /^\.latch/)
		{
			#   vanilla latches tokens
			#		[0]		[1]			[2]			[3]			[4]		[5]
			#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
			@latch_clk_tokens = split(/[\s]+/,$line);
		}


		#check if we have a match if so process it and that the match has a domain
		if((my $size = @latch_clk_tokens) == 6)
		{
			#build the domain map ####################
			my $clk_domain_name = "latch";
			my $display_domain_name = "latch";
			#use everything after the output to create a clk name, which translate to a clock domain
			for (my $i=3; $i < $size; $i++)
			{
				#keep full ref name for display purposes
				$display_domain_name.=$uniqID_separator.@latch_clk_tokens[$i];
			}

			if(!$all_bb && ($vanilla || exists($clocks_not_to_bb{$display_domain_name})))
			{
				#register the clock domain in the used clk map from the input list 
				if(!$vanilla) {
					$clocks_not_to_bb{$display_domain_name} += 1;
				}

				$vanilla_clk_domain{$display_domain_name} += 1;
				$line = join(" ",@latch_clk_tokens)."\n";
			} 
			else
			{
				$bb_clock_domain{$display_domain_name} += 1;
				$line = ".subckt ".$display_domain_name." I=".@latch_clk_tokens[1]." C=".@latch_clk_tokens[4]." O=".@latch_clk_tokens[2]."\n";
			} 
		}

		# if we have an output file, print the line to it
		if($has_output) {
			print $OutFile $line;
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
		print $OutFile 	".model ${module}\n".
						".inputs I C\n".
						".outputs O\n".
						".blackbox\n".
						".end\n\n";
	}
	close($OutFile);
}

# write out all the clock domains in the file (1/line)
if( $has_output_clk )
{
	print $ClkFile join("\n", keys %vanilla_clk_domain).join("\n", keys %bb_clock_domain)."\n";
	close($ClkFile);
}

####################################################
#begin reporting
####################################################

print "Usage Statistic:\n\t<Clock UniqID>\t#<Number of Latches impacted>";
print_stats(\%vanilla_clk_domain, "Vanilla");
print_stats(\%bb_clock_domain, "Black Boxed");

#report wrongly typed/ unexistant input clock domain to keep vanilla
foreach my $clks (keys %clocks_not_to_bb) {
	if ( $clocks_not_to_bb{$clks} != 1 ){
		delete $clocks_not_to_bb{$clks}
	}
}
if( ($size = keys %clocks_not_to_bb) ) {
	print "\n####\nERROR: malformed or non-existant input clock:\n\t<".(join(">\n\t<", keys %clocks_not_to_bb)).">\n####\n";
	exit(-1);
}

print "\n\n";
close($InFile);

exit(0);