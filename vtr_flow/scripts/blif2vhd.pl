#!/usr/bin/perl
use strict;
use Switch;

my $file = $ARGV[$#ARGV-1];
#my $userfile = $ARGV[$#ARGV];
if ($ARGV[$#ARGV] eq "-h" || "$#ARGV" eq "-1") {
    print "\n\nThe command line syntax for running this script is\n\n";
    print "perl blif2vhd.pl <blif file> \n\n"; 
    print "Here blif file is the name of your blif file which you want to parse with the .blif extension\n\n";
    print "For example, if you say\n\n";
    print "perl script abc.blif \n\n";
    print "Then the script would parse abc.blif and generate abc.vhd\n"; 
}
else {
    my $blifString;
    die("$file does not .blif extension.\n") unless ($file =~ m/\.blif/);
    $file =~ m/(.*?)\.blif/;
    my $userfile = $1;
    {
        print "Parsing $file\n";
        print "Please wait...\n";
        $userfile =~ s/\..*//;
        open (OUTFILE, ">$userfile.vhd") || die "Cannot create .vhd :$!";
        open(INFILE, "<$file")|| die "Cannot open blif: $!";
        local $/ = undef ;
        $blifString=<INFILE>;
        close(INFILE);
    }
    my @buffer=();
    #$blifString=~ s/(\[)|(\])/x/gm; #get rid of comments and extra lines
    $blifString=~ s/([^\w])\[(\d+)\]/$1wire$2/gs; #replace abc berkeley wires
    $blifString=~ s/([^\s]+)_+([\s])/$1$2/gs; #replace variables that end in _'s e.g. PKSi_191_ => PKSi_191
    $blifString=~ s/(\w)\.([a-zA-Z])/$1_$2/gs; #replace abc.test with abc_test    
    $blifString=~ s/([a-zA-Z])\.(\w)/$1_$2/gs; #replace abc.test with abc_test    
    $blifString=~ s/\[(\d+)\]/xy$1xy/gs; #replace vectors [123] with xy123xy
    $blifString=~ s/\<(\d+)\>/yx$1yx/gs; #replace vectors <123> with yx123yx
    $blifString=~ s/\*|\$|\//zz/gs; #replace * with zz        
    $blifString=~ s/\.wire_load_slope 0\.00\s*//gs;#remove .wire_load_slope 0.00 
    $blifString=~ s/([a-zA-z]\d+)\.(\d)/$1_$2/gs;#replace x1.2 with x1_2
    #still need to fix headers
    
    $blifString=~ s/\\\s*\n//gm;
    $blifString=~ s/[ ]+/ /gm;
    my @blifBlocks=split(/\./,$blifString);
    shift(@blifBlocks);
    
    my $ending_value = scalar(@blifBlocks) ;
    my %primaryVariableHash=();
   
    my @inputList;
    my @outputList;
    my @nodeList;
    my $circuitName;
    
    # @Array returns a scalar in this context
    for(my $counter=0 ; $counter < $ending_value ; $counter++)
    {
        my @currentLines=split(/\n/,$blifBlocks[$counter]);
        my @currentNames=split(/\s/,$currentLines[0]); #split the first line of the block
        my $numNames = scalar(@currentNames) ;
        switch($currentNames[0]){
        case "inputs"
        {
            shift(@currentNames);
            foreach my $nodeName (@currentNames){
                #$primaryVariableHash{$nodeName}='pin';
                if (not $nodeName eq "") {
                    push @inputList,$nodeName ;
                }
            }            
        }
        case "outputs"
        {
            shift(@currentNames);
            foreach my $nodeName (@currentNames){
                $primaryVariableHash{$nodeName}='output';
                if (not $nodeName eq "") {
                    push @outputList,$nodeName ;
                }
            }            
        }   
        case "names"
        {
            if(not $primaryVariableHash{$currentNames[$numNames-1]})
            {
                #if(not $variableHash{$currentNames[$numNames-1]}){
                    push @nodeList,$currentNames[$numNames-1] ;
                #}
            }
            shift(@currentLines);
            my $pos;
            foreach my $line (@currentLines){
                my @eqn=();                
                if($line =~ m/\s1/){
                    $line=~s/\s1//;
                    $pos=1;
                }elsif($line =~ m/\s0/){
                    $line=~s/\s0//;
                    $pos=0;
                }
                else{
                    $pos=-1;
                }
                for(my $NameCounter=1 ; $NameCounter < $numNames-1 ; $NameCounter++){                    
                    if($line=~m/^1/){ #found a 1
                        $line=~s/^1(.*)/$1/;
                        push @eqn," $currentNames[$NameCounter] " ;
                    }
                    elsif ($line=~m/^0/){ #found a 0
                        $line=~s/^0(.*)/$1/;
                        push @eqn," (not $currentNames[$NameCounter]) " ;
                    }
                    else{
                        $line =~ s/-//;
                    }
                }
                $line ='(' . join(" and ",@eqn) . ')';
                #if($pos==0){
                #    $line="(not $line)";
                #}
            }
            if(scalar(@currentLines) eq 0){ #prob set eq to logic low
                push (@buffer, "$currentNames[$numNames-1] <=((not ". $inputList[0].") and ". $inputList[0].");\n");
            }elsif($pos==-1){
                push (@buffer, "$currentNames[$numNames-1] <=((not ". $inputList[0].") or ". $inputList[0].");\n");
            }elsif($pos==0){            
                push (@buffer, "$currentNames[$numNames-1] <=(not(". join(" or \n",@currentLines) . "));\n");
            }else{
                push (@buffer, "$currentNames[$numNames-1] <=". join(" or \n",@currentLines) . ";\n");
            }
        }
        case "latch" 
        {
            my $input  = $currentNames[1];
            my $output = $currentNames[2];            
            if(not $primaryVariableHash{$output}) #is it an output
            {
                push (@nodeList,$output) ;                
            }
            my $init_value;
            die("Not supported latch format:".$currentLines[0]) unless (($numNames eq 6) or # 
                                                                        ($numNames eq 5) or
                                                                        ($numNames eq 4) or
                                                                        ($numNames eq 3) );
            if( ($numNames - 3) % 2 ) # 1 or 3 arguments => has init value
            {
                $init_value = $currentNames[$numNames-1];
                die("Don't know how to initialize HVDL without a reset signal (unkown reset value=".$init_value.")") unless (($init_value eq 2) or #don't care
                                                                                                                             ($init_value eq 3)or #unkown
                                                                                                                             ($init_value eq 0)); #unkown
            }
            
            push @buffer, "process($input";
            my $type ="";
            my %types;
            if( ($numNames - 3) >= 2 ) # more than 2 arguments => has has type and control
            {   
                my $clock = $currentNames[4];
                if(not $clock eq "NIL")
                {
                    push (@buffer, ",$clock");
                }
                
                $types{"re"}="rising_edge($clock)";
                $types{"fe"}="falling_edge($clock)";
                $types{"ah"}="$clock='1'";
                $types{"al"}="$clock='0'";
                $types{"as"}=$input."'event";
                
                $type    = $currentNames[3];
                die("Type not define:$type") if(not $types{$type});
                
            }
            push @buffer, ")\nbegin\n";
            if(not $type eq "")
            {
                push (@buffer, "\tif($types{$type}) then\n\t\t$output <= $input;\n\tend if;\n");
            }
            else
            {
                push (@buffer, "\t$output <= $input;\n");
            }
            push (@buffer, "end process;\n");
        }
        case "end" 
        {
            #do nothing;
        }
        case "model" 
        {
            $circuitName=$currentNames[1];
        }
        case "clock" 
        {
            push (@inputList,$currentNames[1]) ;
            
        }
        else
        {
            die("Cannot handle to following directive: ".$currentNames[0]);
        }
        }#end switch
    }
    print OUTFILE
        "---Output from blif2vhdl.pl (written by Kirill Minkovich - cory_m\@cs.ucla.edu)\n",
        "---From model $circuitName\n",
        "Library IEEE;\n",
        "\tuse IEEE.std_logic_1164.all;\n",
        "entity $userfile is\n",
        "\tPort (\n",
        "\t",join(",",@inputList), ": in std_logic;\n",
        "\t",join(",",@outputList), ": buffer std_logic\n",        
        ");\n",
        "end $userfile;\n\n",
        "architecture $userfile\_behav of $userfile is\n",
        "signal ",join(",",@nodeList), ": std_logic;\n",
        "begin\n\n",    
        "@buffer\n\nend $userfile\_behav;\n";        
    
    close(OUTFILE);
    print("Done\n");
}