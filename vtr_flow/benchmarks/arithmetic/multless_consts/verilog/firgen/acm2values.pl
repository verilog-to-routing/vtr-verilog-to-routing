#!/usr/bin/perl

#------------------------------------------------------------------------------
# acm2values.pl: generates calls acm on the commandline
#                replaces the output variables with descriptive wirenames
#                in the form of: 
#                wX = positive integer
#                wX_ = negative integer
#                all values are in hex
# author: jtrapass (Joseph Paul Trapasso)
# based on the mcm tool by Yevgen Voronenko
# under the guidance of James Hoe and Markus Pueschel

# This script requires the Spiral Multiplier Block generator,
# available at http://spiral.ece.cmu.edu/mcm/gen.html
# The variable below "$acm_path" must be set to the path of its "acm"
# executable.

#------------------------------------------------------------------------------

#use warnings 'all';
#use strict;
#use FindBin;

sub val2name {
    my ($val) = @_;
    
    if($val < 0){
	my $temp = -1 * $val;
	return "w" . $temp . "_";
    }
    else{
	return "w" . $val;
    }
	
}

sub main {
    my @constants = ();
    my $acmOutput;
    my $acm_path = "../synth/acm";

    #parse args
    for(my $i = 0; $i < scalar(@ARGV); $i++){
	#verify we were given a number:
	if($ARGV[$i] eq "-acmOutput"){
	    $i++;
	    $acmOutput = $ARGV[$i];
	}
	elsif( $ARGV[$i] eq ( $ARGV[$i] + 0)){
	    push(@constants, $ARGV[$i]);
	}
	else{
	    print "Non-numerical constant found: $ARGV[$i]\n";
	    printUsage();
	    return ;	
	}
    }

    my $acm_args = join(" ", @constants);
    my %wireValues = ("t0" => 1);
    my %wireDone = (val2name(1) => 1);
    
    if(defined($acmOutput)){
	if(!open(ACM_OUT, "< " . $acmOutput)){
	    print "Error opening acm output file: $acmOutput\n";
	    return -1;
	}
    }elsif(!open(ACM_OUT, "$acm_path $acm_args 2>&1 |")){
	print "Error excecuting acm\n";
	return -1;
    }
    
    while(my $line = <ACM_OUT>){
	#skip comments
	if($line =~ /^\s*\/\//){
	    next;
	}
	#assignments via function eg: t8 = shl(t7, 6);
	if($line =~ /\s*(\w+)\s*=\s*(\w+)\((\w+)\s*,\s*(-?)(\w+)\);/){
	    my $y_value = 0;
	    my ($var, $op, $x, $neg, $y) = ($1, $2, $3, $4, $5);

	    $y = -1 * $y if($neg);

	    my $x_val = $wireValues{$x};
	    my $y_val = $wireValues{$y};

	    my $wire_val;

	    if(!defined($x_val)){
		print "Error, non-previously seen variable: $x\n";
		do{
		    print $line;
		}while($line = <ACM_OUT>);
		return -1;
	    }
	    
	    if($op eq "add"){
		if(!defined($y_val)){
		    print "Error, non-previously seen variable: $y\n";
		    return -1;
		}
		$wire_val = $x_val + $y_val;
	    }
	    elsif($op eq "sub"){
		if(!defined($y_val)){
		    print "Error, non-previously seen variable: $y\n";
		    return -1;
		}
		$wire_val = $x_val - $y_val;
	    }
	    elsif($op eq "shl"){  # shift left
		if($x_val < 0){
		    #left shifted negatives are unsupported
		    print "left shifted negative numbers are unsupported: shl( ${x_val}, ${y})\n";
		    return -1;
		}

		if($y == 0){
		    $wire_val = $x_val;
		}
		else{
		    $wire_val = $x_val  << $y;
		}
		$y_value = 1;
	    }
	    elsif($op eq "shr"){ #shift right
		if($y == 0){
		    $wire_val = $x_val;
		}
		else{
		    $wire_val = $x_val  >> $y;
		}
		$y_value = 1;
	    }
	    elsif($op eq "cmul"){ #constant multiply
		if($y != -1){
		    print "cmul by non -1 value ($y) ... WHY?";
		    return -1;
		}
		$wire_val = $x_val * -1;
		$y_value = 1;
	    }
	    else{
		print "Error, unkown acm operation: $op\n";
		print $line;
		close(ACM_OUT);
		return -1;
	    }
	    
	    $wireValues{$var} = $wire_val;

	    my $new_wireName = val2name($wire_val);
	    
	    if(!defined($wireDone{$new_wireName})){
		$wireDone{$new_wireName} = $wire_val;
		my $x_wireName =val2name($x_val);
		my $y_wireName = ($y_value?$y:val2name($y_val));
		
		print "$new_wireName = ${op}(${x_wireName}, ${y_wireName});\n";
	    }

	}
	#standard assignment eg:  "t8 = t0;"
	elsif($line =~ /\s*(\w+)\s*=\s*(\w+)\s*;/){
	    my ($var, $x) = ($1, $2);
	    my $x_val = $wireValues{$x};

	    if(!defined($x_val)){
		print "Error, non-previously seen variable: $x\n";
		do{
		    print $line;
		}while($line = <ACM_OUT>);
		return -1;
	    }

	    my $wire_val = $x_val;
	    $wireValues{$var} = $wire_val;
	    
	    my $new_wireName = val2name($wire_val);
	    
	    if(!defined($wireDone{$new_wireName})){
		
		print "Error, Assignment, yet previous wire not in list\n";
		do{
		    print $line;
		}while($line = <ACM_OUT>);
		return -1;

	    }

	}
	elsif($line =~ /\s*(\w+)\s*=\s*neg\(\s*(\w+)\s*\);/){
	    my ($var, $x) = ($1, $2);
	    my $x_val = $wireValues{$x};

	    if(!defined($x_val)){
		print "Error, non-previously seen variable: $x\n";
		do{
		    print $line;
		}while($line = <ACM_OUT>);
		return -1;
	    }

	    my $wire_val = -1 * $x_val;
	    $wireValues{$var} = $wire_val;

	    my $new_wireName = val2name($wire_val);
	    
	    if(!defined($wireDone{$new_wireName})){

		$wireDone{$new_wireName} = $wire_val;
		my $x_wireName =val2name($x_val);
		
		print "$new_wireName = neg(${x_wireName});\n";
	    }

	}
	else{
	    print "Error, unkown acm output:\n";
	    do{
		print $line;
	    }while($line = <ACM_OUT>);
	    close(ACM_OUT);
	    return -1;
	}
    }

    if(!close(ACM_OUT)){ #check exit code
	print "Error executing acm: nonzero exit code\n";
	return -1;
    }
    
    return 0;
}

sub printUsage(){

    print <<EOF;

\#------------------------------------------------------------------------------
\# acm2values.pl: generates calls acm on the commandline
\#                replaces the output variables with descriptive wirenames
\#                in the form of: 
\#                wX = positive integer
\#                wX_ = negative integer
\#                all values are in hex
\# author: jtrapass (Joseph Paul Trapasso)
\# based on the mcm tool by Yevgen Voronenko
\# under the guidance of James Hoe and Markus Pueschel
\#------------------------------------------------------------------------------

./acm2values.pl 10 20 30 40 30 20 10 [-acmOutput file]

  constants:the constants to multiply by.
    these constants are passed to the mcm tool
    the mcm tool's program is then parsed and transformed into useful 
    names for future use

  -acmOutput: instead of calling acm on the commandline, parse the pregenerated output
     provided here


EOF

}

if(main()){
    printUsage();
    printf("\n\n./acm2values.pl Script Failed.\n");
    printf(" please report this to jtrapass\@andrew.cmu.edu\n\n");
    exit(-1);
}

1;
