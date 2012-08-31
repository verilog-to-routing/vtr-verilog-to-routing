$_ = <>; #read a line
@tokens = split(/ +/); #split based on spaces
chop($tokens[1]); #remove \n from first token
chop($tokens[1]); #remove ( from first token
#print "$tokens[1]\n";

$clockName = $ARGV[0]; #If the circuit has a clock input, the user must enter it's name as a command line option.

$moduleName = $tokens[1];

#( defined $ARGV[0] and (-e $ARGV[0]) ) or die "Error: $ARGV[0] is expected to be existed in CED.\n";
#open F1 $ARGV[0];
#open F2 $ARGV[1];
#my @file_1 = <F1>;
#my @file_2 = <F2>;
#for (index i) {
#    chomp($file_1[0]);
#}
while (<>) { # keep reading until end of file
    chop;
#    chomp ($_);
    if (m/input /) { # see if line contains input
	@tokens = split(/input /); # split the line based on the word 'input'
	#print "$tokens[1]\n"; # print the name of the input to the screen
	push(@inputNames, $tokens[1]);
	push(@inoutNames , $tokens[1]);
    }
    if(m/output /){ #see if this line contains input
	@tokens = split(/output /);# split the line based on the word 'output'
	push(@outputNames, $tokens[1]);
	#print "$tokens[1]\n";
	push(@inoutNames , $tokens[1]);
    }
}

#now that we found the names of the inputs,
#print out an always block that supplies the random inputs
print "`timescale 1ns/1ns\n";
print "module $moduleName";
print "_testbench();\n\n";

for ($i = 0; $i <= $#inputNames; $i++) {

    print "reg $inputNames[$i];\n";

}

for ($i = 0; $i <= $#outputNames; $i++) {

    print "wire $outputNames[$i];\n";

}

print "\n$moduleName inst(\n";
for ($i = 0; $i <= $#inoutNames; $i++) {
    print "\t$inoutNames[$i]";
    if($i < $#inoutNames)
    {
	print ",\n";
    }
    else{
	print "\n";
    }
}
print ");\n\n";

if($clockName)#If there is a clock input to the circuit then this is a sequential circuit
{
#setting up the clock signal @ 100MHz
    print "initial\n";
    print "\t$clockName = 1'b0;\n\n";
    print "always@($clockName)\n";
    print "begin\n";
    print "\t#10\n";
    print "\t$clockName <= !$clockName;\n";
    print "end\n\n";

    print "always@(posedge $clockName)\n";
    print "begin\n\t#2\n";
    for ($i = 0; $i <= $#inputNames; $i++) {  #  $#inputNames is the # of array elements in inputNames
	if($inputNames[$i] !~ $clockName)
	{
	    print "\t$inputNames[$i] <= ((\$random & 1) == 1) ? 1'b1 : 1'b0;\n";
	}
    }
    print "end\n\n";
}
else{
#Then this is a combinational circuit, i.e. there is no clock
#setting up a clock signal for the random number generator @ 100MHz

    print "reg clock;\n\n";
    print "initial\n";
    print "\tclock = 1'b0;\n\n";
    print "always@(clock)\n";
    print "begin\n";
    print "\t#10\n";
    print "\tclock <= !clock;\n";
    print "end\n\n";

    print "always@(posedge clock)\n";
    print "begin\n\t#2\n";
    for ($i = 0; $i <= $#inputNames; $i++) {  #  $#inputNames is the # of array elements in inputNames  
	print "\t$inputNames[$i] <= ((\$random & 1) == 1) ? 1'b1 : 1'b0;\n";
    }
    print "end\n\n";

}
print "endmodule\n";
