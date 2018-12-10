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
				--clk_list			clock not to black box
				--vanilla			Convert the bliff file to non boxed latches
				--restore			Reatach given clock to all latches found
				--padding			insert_padding for abc renamed pins
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
my @clocks_to_restore;

my $OutFile;
my $InFile;
my $ClkFile;

my $uniqID_separator = "_^_";

my $vanilla = 0;

my $has_output = 0;
my $has_input = 0;
my $has_output_clk = 0;
my $has_clk_list = 0;
my $has_restore_clk = 0;
my $abc_padding = 0;

my $parse_input = 0;
my $parse_output = 0;
my $parse_clk_list = 0;
my $parse_restore_clock = 0;
my $parse_output_clk_file = 0;
my $abc_parse_padding = 0;

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
	elsif ($parse_clk_list) 
	{
		$parse_clk_list = 0;

		foreach my $input_clocks (split(/,/, $cur_arg)) {
			my @tmp_clocks = split(/\Q$uniqID_separator\E/, $input_clocks);
			my $restructured_input_clk = @tmp_clocks[1]." ".@tmp_clocks[2];
			$clocks_not_to_bb{$restructured_input_clk} = 1;
		}

		print "using folowing clk domain: ";
		foreach my $clks (keys %clocks_not_to_bb) {
			print $clks." ";
		}
		print "\n";

		$has_clk_list = 1;
	} 
	elsif ($abc_parse_padding) 
	{
		$abc_parse_padding = 0;
		$abc_padding = $cur_arg;
		print "using ".$abc_padding." to pad current nets\n";
	} 

	elsif ($parse_restore_clock) 
	{
		$parse_restore_clock = 0;
		my @tmp_clocks = split(/\Q$uniqID_separator\E/, $cur_arg);
		#default init to 0 since abc will build the PIand PO to for init 1 so that it behaves as such
		$clocks_to_restore = @tmp_clocks[1]." ".@tmp_clocks[2]." 0";
		print "using ".$clocks_to_restore." to restore latches\n";
		$has_restore_clk = 1;
	}
	else {
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
		}elsif ($cur_arg =~ /\-\-restore/) {
			$parse_restore_clock = 1;
		}elsif ($cur_arg =~ /\-\-padding/) {
			$abc_parse_padding = 1;
		}else {
			print "Error wrong argument kind $cur_arg\n";
			print_help();
			exit(-1);
		}
	}
}

#default is vanilla all latches
if(!$vanilla && !$has_clk_list && !$has_restore_clk)
{
	$vanilla = 1;
}

if(!$has_output && ($has_restore_clk || $has_clk_list))
{
	print "Cannot specify a rewrite directive without an output file\n";
	print_help();
	exit(-1);
}
if( ($vanilla && $has_restore_clk) || ($has_restore_clk && $has_clk_list) || ($vanilla && $has_clk_list))
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
my $skip_once = 0;
my $lineNum = 0;
my %clocks_in_model = ();
my $line;
while( (my $cur_line = <$InFile>) )
{
	$lineNum += 1;

	#remove newline
	chomp($cur_line);

	#remove leading and trailing whitespace
	$cur_line=~ s/^\s+|\s+$//g; 

	#truncate duplicate whitespace
	$cur_line =~ s/\h+/ /g;

    if ($cur_line =~ /\\$/) 
	{
		chop($cur_line);
		$cur_line=~ s/^\s+|\s+$//g; 
		# continue reading
        $line .= $cur_line." ";
    }
	else
	{
		$line .= $cur_line;

		#we need to rebrand all the nets to prevent collision during reoptimization flow
		my @line_tokenized = split(/[\s]+/,$line);
		$line = "";
		foreach my $line_tok (@line_tokenized)
		{
			if ($line_tok =~ /^lo[0-9]+/
			or $line_tok =~ /^n[0-9]+/
			or $line_tok =~ /^li[0-9]+/)
			{
				$line_tok = $abc_padding.$line_tok;
			}

			$line .= $line_tok." ";
		}
		$line .= "\n";  

		
		#If the Line is a bb latch module declaration skip it
		$skip = ( (!$skip) && ($line =~ /^\.model[\s]+latch/) )? 1: $skip;

		#if the Line is a Dummy clock keeper (prevents clk name mangling by abc) skip it and we'll rewrite it
		$skip = ( (!$skip) && ($line =~ /^\.model[\s]+CLOCK_GATING/) )? 1: $skip;

		$skip_once = ( $line =~ /^\.subckt[\s]+CLOCK_GATING/ )? 1: 0; 

		if (!$skip
		and !$skip_once)
		{
			# if this is the end of a model dump all the clocks in this model onto a dummy black box 
			# this is used so that abc does not mangle the clock names if the latches are internally driven
			# write the clock dump just before the end and empty the hash
			if($has_output
			and not $vanilla
			and $line =~ /^\.end/) 
			{
				foreach my $clock_name (keys %clocks_in_model)
				{
					print $OutFile 	"\n.subckt CLOCK_GATING I=".$clock_name." O=GATED_CLOCK\n";
				}
				%clocks_in_model = ();
			}

			my @latch_clk_tokens;
			if( $has_restore_clk )
			{
				if($line =~ /^\.latch/)
				{
					#   vanilla latches tokens
					#		[0]		[1]			[2]			[3]			[4]		[5]
					#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
					@latch_clk_tokens = split(/[\s]+/,$line);
					$line = @latch_clk_tokens[0]." ".
							@latch_clk_tokens[1]." ".
							@latch_clk_tokens[2]." ".
							$clocks_to_restore."\n"; # we set the init value to 0 as ABC will have inserted the necessary PI/PO to use a 0 init latch
				}
			}
			else
			{
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
				if((my $size = scalar @latch_clk_tokens) == 6 )
				{
					#build the domain map ####################
					#do not include the init value in the map since abc will build the necessary facilities around init 1
					my $clk_domain_name = @latch_clk_tokens[3]." ".@latch_clk_tokens[4];

					#keep full ref name for display purposes
					my $display_domain_name = "latch".$uniqID_separator.@latch_clk_tokens[3].$uniqID_separator.@latch_clk_tokens[4].$uniqID_separator.@latch_clk_tokens[5];
					#use everything after the output to create a clk name, which translate to a clock domain

					if($vanilla
					or exists($clocks_not_to_bb{$clk_domain_name}))
					{
						#insert the clock in the list for the .model
						$clocks_in_model{@latch_clk_tokens[4]} = @latch_clk_tokens[3];
						#register the clock domain in the used clk map from the input list 
						if( exists $clocks_not_to_bb{$clk_domain_name} ) {
							$clocks_not_to_bb{$clk_domain_name} += 1;
							$line = @latch_clk_tokens[0]." ".@latch_clk_tokens[1]." ".@latch_clk_tokens[2]." ".@latch_clk_tokens[3]." GATED_CLOCK ".@latch_clk_tokens[5]."\n";
						}
						else
						{
							$line = join(" ",@latch_clk_tokens)."\n";
						}

						$vanilla_clk_domain{$display_domain_name} += 1;
					} 
					else
					{
						$bb_clock_domain{$display_domain_name} += 1;
						$line = ".subckt ".$display_domain_name." I=".@latch_clk_tokens[1]." C=".@latch_clk_tokens[4]." O=".@latch_clk_tokens[2]."\n";
					} 
				}
			}

			# if we have an output file, print the line to it
			if($has_output) {
				print $OutFile $line;
			}
		}

		$skip_once = 0;
		#if the Line is a .end after a bb module declaration dont skip thereafter
		$skip = ( ($skip) && ($line =~ /^\.end/))? 0: $skip;

		$line = "";
	}
}

# if we have an output file print the .module for bb latches at the end of the file
if( $has_output 
and not $vanilla)
{
	foreach my $module (keys %bb_clock_domain) 
	{
		print $OutFile 	"\n".
						".model ${module}\n".
						".inputs I C\n".
						".outputs O\n".
						".blackbox\n".
						".end\n".
						"\n";
	}

	print $OutFile 	"\n".
					".model CLOCK_GATING\n".
					".inputs I\n".
					".outputs O\n".
					".blackbox\n".
					".end\n".
					"\n";

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