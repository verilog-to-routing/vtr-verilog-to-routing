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

	Usage:	./exec 
				--input				Bliff input file
				--output			Bliff output file
				--clk_list			List of clocks not to black box
				--respect_init		Use the initial value as part of the clock domain
				--respect_edge		Use the edges type as the clock domain
				--vanilla			Convert the bliff file to non boxed latches
				--all				Convert the bliff file to all boxed latches
				--use_white_box		Use White boxes instead of Black boxes (default) 
				--use_flop			Use .flops instead of .latch
";
}

my $ARGC = @ARGV;
my %clocks_not_to_bb;
my $OutFile;
my $InFile;
my $uniqID_separator = "_^_";
my $vanilla = 0;
my $all_bb = 0;
my $use_white_boxes = 0;
my $use_flop = 0;
my $respect_init = 0;
my $respect_edge = 0;
my $has_output = 0;

my $clkfile = "";
my $parse_input = 0;
my $parse_output = 0;
my $parse_clk_list =0;

my $box_type = "black";
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
		print $cur_arg."\n";
	}elsif ($parse_output) {
		open($OutFile, ">" .$cur_arg) || die "Error Opening BLIF output File $cur_arg: $!\n";
		$parse_output = 0;
		print $cur_arg."\n";
	}elsif ($parse_clk_list) {
		foreach my $input_clocks (split(/,/, $cur_arg))
		{
			$clocks_not_to_bb{$input_clocks} = 1;
		}
		$parse_clk_list = 0;
	}elsif ($cur_arg =~ /\-\-input/) {
		$parse_input = 1;
		print "using input: ";
	}elsif ($cur_arg =~ /\-\-output/) {
		$parse_output = 1;
		$has_output = 1;
		print "using output: ";
	}elsif ($cur_arg =~ /\-\-clk_list/) {
		$parse_clk_list = 1;
	}elsif ($cur_arg =~ /\-\-respect_init/) {
		$respect_init = 1;
	}elsif ($cur_arg =~ /\-\-respect_edge/) {
		$respect_edge = 1;
	}elsif ($cur_arg =~ /\-\-vanilla/) {
		$vanilla = 1;
	}elsif ($cur_arg =~ /\-\-all/) {
		$all_bb = 1;
	}elsif ($cur_arg =~ /\-\-use_white_box/) {
		$box_type = "white";
	}elsif ($cur_arg =~ /\-\-use_flop/) {
		$use_flop = 1;
	}else {
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

if ( $all_bb )
{
	%clocks_not_to_bb = ();
}

#################################
# a clock name is defined in the map as <latch|$edge_type|$clk|$initial_value>
# build clocks names list and latches location list
# this also converts all black boxed latches to regular latches in one pass
#################################

my %bb_clock_domain;
my %vanilla_clk_domain;

my $skip = 0;
my $lineNum = 0;
while( ($line = <$InFile>))
{
	$lineNum += 1;
	
	#If the Line is a bb latch module declaration skip it
	$skip = ( (!$skip) && ($line =~ /^\.model[\s]+latch/) )? 1: $skip;

	if (!$skip)
	{
		#		[0]		[1]			[2]			[3]			[4]		[5]
		#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
		#	type = {fe, 			re, 			ah, 			al, 			asg}, 
		#			falling edge	rising edge		active high 	active low 		asynchronous
		my @latch_clk_tokens = ("undef", "undef", "undef", "", "", "");

		# BLACK OR WHITE BOX
		if ($line =~ /^\.subckt[\s]+latch/)
		{
			#   blackboxed or white box latch (creted by this script, so it will always respect this format)
			#		[0]		[1]							[2]				[3]			[4]
			#       .subckt	latch|<type>|<clk>|<init>	I=<driver> 	C=clk	O=<output>
			my @tokens = split(/[\s]+/, $line);
			my @reformat_clk = split(/\Q$uniqID_separator\E/, @tokens[1]);
			my @reformat_driver = split(/\=/, @tokens[2]);
			my @reformat_output = split(/\=/, @tokens[4]);
			
			@latch_clk_tokens = (".latch", @reformat_driver[1] ,@reformat_output[1], @reformat_clk[1], @reformat_clk[2], @reformat_clk[3]);
		}
		#FLIP FLOP
		elsif($line =~ /^\.flop/)
		{
			#   flop latches tokens
			#	these ones are a bit less fun as they can be in any perticular order
			#		.flop D=in Q=out [C=clk] [S=set] [R=reset] [E=enable] [init=init] [async] [negedge]
			my @tokens = split(/[\s]+/,$line);
			@latch_clk_tokens[0] = ".latch";
			foreach my $item (@tokens)
			{
				my $tk_item = split(/\=/, $item);

				if ($tk_item[0]  =~ /D/) {
					@latch_clk_tokens[1] = $tk_item[1];
				}elsif ($tk_item[0] =~ /Q/) {
					@latch_clk_tokens[2] = $tk_item[1];
				}elsif ($tk_item[0] =~ /C/) {
					@latch_clk_tokens[4] = $tk_item[1];
				}elsif ($tk_item[0] =~ /S/) {
					#do nothing
				}elsif ($tk_item[0] =~ /R/) {
					#do nothing
				}elsif ($tk_item[0] =~ /E/) {
					#do nothing
				}elsif ($tk_item[0] =~ /init/) {
					@latch_clk_tokens[5] = $tk_item[1];
				}elsif ($tk_item[0] =~ /async/) {
					@latch_clk_tokens[3] = "asg";
				}elsif ($tk_item[0] =~ /negedge/) {
					@latch_clk_tokens[3] = "fe";
				}else{
					die "unsupported token!";
				}
			}
		}
		#LATCHES
		elsif($line =~ /^\.latch/)
		{
			#   vanilla latches tokens
			#		[0]		[1]			[2]			[3]			[4]		[5]
			#       .latch	<driver>	<output>	[<type>]	[<clk>]	[<init>]
			@latch_clk_tokens = split(/[\s]+/,$line);
		}

		

		my $size = @latch_clk_tokens;
		#check if we have a match if so process it and that the match has a domain
		if(@latch_clk_tokens[0] != "undef")
		{
			#build the domain map ####################
			my $clk_domain_name = "latch";
			my $display_domain_name = "latch";
			#use everything after the output to create a clk name, which translate to a clock domain
			for (my $i=3; $i < $size; $i++)
			{
				#abc sets these to 0
				if( (!$respect_init || $i != 5) && ( !$respect_edge || $i != 3 ) )
				{
					$clk_domain_name .= " ".@tokens[$i];
				}
				#keep full ref name for display purposes
				$display_domain_name .= " ".@tokens[$i];
			}

			if(!$all_bb && ($vanilla || exists($clocks_not_to_bb{$clk_domain_name})))
			{
				#register the clock domain in the used clk map from the input list 
				if(!$vanilla)
				{
					$clocks_not_to_bb{$clk_domain_name} += 1;
				}
				
				if(!exists($vanilla_clk_domain{$display_domain_name}))
				{
					$vanilla_clk_domain{$display_domain_name} = 0;	
				}
				$vanilla_clk_domain{$display_domain_name} += 1;

				if($use_flop)
				{
					$line = ".flop ".$display_domain_name;
					if(@latch_clk_tokens[1] != ""){ $line .= " D=".@latch_clk_tokens[1];}
					if(@latch_clk_tokens[2] != ""){ $line .= " Q=".@latch_clk_tokens[2];}
					if(@latch_clk_tokens[3] != ""){ $line .= " ".@latch_clk_tokens[3];}
					if(@latch_clk_tokens[4] != ""){ $line .= " C=".@latch_clk_tokens[4];}
					if(@latch_clk_tokens[5] != ""){ $line .= " init=".@latch_clk_tokens[5];}
				}
				else
				{
					$line = ".latch ".join(" ",@latch_clk_tokens)."\n";
				}
			} 
			else
			{
				if(!exists($bb_clock_domain{$display_domain_name}))
				{
					$bb_clock_domain{$display_domain_name} = 0;	
				}
				$bb_clock_domain{$display_domain_name} += 1;
				
				$line = ".subckt ".$display_domain_name." I=".@tokens[1]." C=".@tokens[4]." O=".@tokens[2]."\n";
			} 
		}

		# if we have an output file, print the line to it
		if($has_output)
		{
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
		my $latch_label = "";
		if($box_type == "white")
		{
			my @reformat_clk = split(/\Q$uniqID_separator\E/, $module);
			$latch_label = ".latch I O ".@reformat_clk[0]." C ".@reformat_clk[2]."\n";
		}
		
		print $OutFile 	".model ${module}\n".
						".attrib ${box_type} box seq no_merge keep\n".
						".inputs I C\n".
						".outputs O\n".
						".input_required C 0.0\n".
						".input_required I 0.0\n".
						".output_arrival O 0.1\n".
						${latch_label}.
						".end\n\n";
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

print "\n____\nBoxed Latches:";
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
close($InFile);

exit(0);