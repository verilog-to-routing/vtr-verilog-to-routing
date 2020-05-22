#!/usr/bin/env perl
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
				--input				Blif input file
				--output_list		Clock list output file
				--output			Blif output file
				--clk_list			clock not to black box
				--vanilla			Convert the bliff file to non boxed latches
				--restore			Reatach given clock to all latches found
";
}

sub print_stats{
	my %domain = %{$_[0]};
	print "\n____\n".$_[1]." Latches:";
	if( ($size = %domain) == 0)
	{
		print "\n\t-None-";
	}
	foreach my $clks (sort(keys %domain))
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

my $parse_input = 0;
my $parse_output = 0;
my $parse_clk_list = 0;
my $parse_restore_clock = 0;
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
	elsif ($parse_clk_list) 
	{
		$parse_clk_list = 0;

		foreach my $input_clocks (split(/,/, $cur_arg)) {
			$clocks_not_to_bb{$input_clocks} = 1;
		}

		print "using folowing clk domain: ";
		foreach my $clks (sort(keys %clocks_not_to_bb)) {
			print $clks." ";
		}
		print "\n";

		$has_clk_list = 1;
	} 
	elsif ($parse_restore_clock) 
	{
		$parse_restore_clock = 0;
		#default init to 0 since abc will build the PIand PO to for init 1 so that it behaves as such
		my @split_clock_name = split(/\Q$uniqID_separator\E/, $cur_arg);
		$clocks_to_restore = @split_clock_name[1]." ".@split_clock_name[2]." ".@split_clock_name[3];
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
		}else {
			print "Error wrong argument kind $cur_arg\n";
			print_help();
			exit(-1);
		}
	}
}

#default is blackbox all latches
if(!$vanilla && !$has_clk_list && !$has_restore_clk)
{
	print "blackboxing all latches in file\n";
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
my $lineNum = 0;
my %clocks_in_model = ();
my $parsed_line = "";
while( (my $cur_line = <$InFile>) )
{
	#######################
	# chomp the line
	$lineNum += 1;

	# if is terminated by a backslash
	$parsed_line .= $cur_line;
	if ( $parsed_line =~ /\\\n$/ )
	{
		$parsed_line =~ s/\\\n$//g;
	}
	else
	{
		#dump line to parse
		my $line = $parsed_line;
		$parsed_line = "";

		#truncate duplicate whitespace
		$line =~ s/\s{2,}/ /g;
		#remove leading and trailing whitespace
		$line =~ s/^\s+|\s+$//g; 

		########################
		# check if we need to skip this line
		my $skip_this_line = 0;
		#If the Line is a bb latch module declaration 
		#	skip it
		#if the Line is a Dummy clock keeper (prevents clk name mangling by abc) 
		#	skip it and we'll rewrite it
		if ( not $skip )
		{
			# multiline skip
			if ( $line =~ /^\.model[\s]+latch/
			or	 $line =~ /^\.model[\s]+CLOCK_GATING/)
			{
				$skip = 1;
				$skip_this_line = 1;
			}
		}
		else
		{
			if ( $line =~ /^\.end/ )
			{
				$skip = 0;
			}
			$skip_this_line = 1;
		}

		#single lines skip
		if( $line =~ /^\.subckt[\s]+CLOCK_GATING/ )
		{
			$skip_this_line = 1;
		}

		###########################
		if ( not $skip_this_line )
		{
			# print "current line: <$line>\n";

			if ($line =~ /^\.end/)
			{
				if ( $has_output
				and not $vanilla)
				{
					print $OutFile "\n";
					foreach my $clock_name (keys %clocks_in_model)
					{
						print $OutFile 	".subckt CLOCK_GATING I=".$clock_name."\n";
					}
					%clocks_in_model = ();
				}
			}
			else
			{
				#we need to rebrand all the nets to prevent collision during reoptimization flow
				if( $has_clk_list )
				{
					my @line_tokenized = split(/[\s]+/,$line);
					$line = "";
					foreach my $line_tok (@line_tokenized)
					{
						if ($line_tok =~ /^[_]*lo[0-9]+/
						or $line_tok =~ /^[_]*n[0-9]+/
						or $line_tok =~ /^[_]*li[0-9]+/)
						{
							$line_tok = "_".$line_tok;
						}
						elsif($line_tok =~ /=/)
						{
							my @equal_tok = split(/=/,$line_tok);
							if (@equal_tok[1] =~ /^[_]*lo[0-9]+/
							or @equal_tok[1] =~ /^[_]*n[0-9]+/
							or @equal_tok[1] =~ /^[_]*li[0-9]+/)
							{
								@equal_tok[1] = "_".@equal_tok[1];
							}
							$line_tok = @equal_tok[0]."=".@equal_tok[1];
						}

						$line .= $line_tok." ";
					}  
				}

				if( $has_restore_clk )
				{
					my @latch_clk_tokens;
					if($line =~ /^\.latch/)
					{
						#   vanilla latches tokens
						#		[0]		[1]			[2]			[3]			[4]		[5]
						#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
						@latch_clk_tokens = split(/[\s]+/,$line);
						$line = @latch_clk_tokens[0]." ".
								@latch_clk_tokens[1]." ".
								@latch_clk_tokens[2]." ".
								$clocks_to_restore;
					}
				}
				else
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

						@latch_clk_tokens = 
						(
							".latch", 
							@reformat_driver[1] ,
							@reformat_output[1], 
							@reformat_clk[1], 
							@reformat_clk[2], 
							@reformat_clk[3]
						);
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
						#switch init to 0 ############ fix init bug
						if(@latch_clk_tokens[5] ne "1")
						{
							@latch_clk_tokens[5] = "0";
						}

						#keep full ref name for display purposes
						my $display_domain_name = "latch".
							$uniqID_separator.@latch_clk_tokens[3].
							$uniqID_separator.@latch_clk_tokens[4].
							$uniqID_separator.@latch_clk_tokens[5];
						
						$clocks_in_model{@latch_clk_tokens[4]} = 1;

						if ( exists ( $clocks_not_to_bb{$display_domain_name} ) )
						{
							$clocks_not_to_bb{$display_domain_name} += 1;
							$vanilla_clk_domain{$display_domain_name} += 1;

							$line = join(" ",@latch_clk_tokens);
						}
						elsif( $vanilla )
						{
							$vanilla_clk_domain{$display_domain_name} += 1;

							$line = join(" ",@latch_clk_tokens);
						} 
						else
						{
							$bb_clock_domain{$display_domain_name} += 1;

							$line = ".subckt ".
								$display_domain_name.
								" I=".@latch_clk_tokens[1].
								" C=".@latch_clk_tokens[4].
								" O=".@latch_clk_tokens[2];
						} 
					}
				}
			}

			# if we have an output file, print the line to it
			if($has_output) 
			{
				# max line length is 256 chars to make sure it fits in most reader buffer
				my $shrunk_text = "";
				# add some spacing before items
				if($line =~ /^\./)
				{
					$shrunk_text = "\n";
				}

				my @line_tokenized = split(/[\s]+/,$line);
				foreach my $string_token (@line_tokenized)
				{
					if ( length( $shrunk_text.$string_token." " ) >= 128 )
					{
						print $OutFile $shrunk_text."\\\n";
						$shrunk_text = " ";
					}

					$shrunk_text .= $string_token." ";
				}
				print $OutFile $shrunk_text."\n";
			}
		}
	}
}

# if we have an output file print the .module for bb latches at the end of the file
if( $has_output 
and not $vanilla)
{
	foreach my $module (sort(keys %bb_clock_domain)) 
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
					".blackbox\n".
					".end\n".
					"\n";

	close($OutFile);
}

# write out all the clock domains in the file (1/line)
if( $has_output_clk )
{
	print $ClkFile join("\n", sort(keys %vanilla_clk_domain)).join("\n", sort(keys %bb_clock_domain))."\n";
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
	print "\n####\nERROR: malformed or non-existant input clock:\n";
	print "\t<".(join(">\n\t<", sort(keys %clocks_not_to_bb))).">\n####\n";
	exit(-1);
}

print "\n\n";
close($InFile);

exit(0);
