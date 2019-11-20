#Usage:
#      perl vcd2act.pl <vcd file> <blif file for circuit> > <Output act file name>

open vcd , $ARGV[0] or die $!;
open blif , $ARGV[1] or die $!;

$reached = 0;

while(<vcd>)
{
    #print "1\n";
    @tokens = split(/ +/);
    #print "$tokens[0]\n";
    if($tokens[0] eq "\$var")
    {
	$signal_name{$tokens[3]} = $tokens[4];
	$TC{($signal_name{$tokens[3]})}=0;
	#print "$signal_code{$tokens[4]}\n";
	push(@codes , $tokens[3]);
    }
    elsif($tokens[0] eq "\$dumpvars\n")
    {
	$reached = 1;
	while(<vcd>)
	{
	    if($_ ne "\$end\n")
	    {
		chomp($_);
		@chars = split(//);
		$initial_val = $chars[0];
		for($i=1 ; $i<=$#chars ; $i++)
		{
		    #print "pushing: $chars[$i] - ";
		    push(@code , $chars[$i]);
		}
		$sig_code = join('' , @code[0..$#code]);
		print "initial => $sig_code -> $initial_val\n";
		$current_val{$sig_code} = $initial_val;
		$newstring = "";
		@code = "";
	    }
	    else{
		last;
	    }
	}
	for($i=0 ; $i<=$#codes ; $i++)
	{
	    $following_transition{$codes[$i]} = $current_val{$codes[$i]};
	    $T1{(  $signal_name{$codes[$i]}  )} = 0;
	    $logic_1_time_count_start{$codes[$i]} = 0;
	}
	$sim_time = 0;
	$prev_sim_time = 0;
    }
    else{
	chomp($_);
	if($reached == 1){
	    @check = split(//);
	    if($check[0] ne "#" && $check[0] ne "\$")
	    {
		@chars = split(//);
		$val = $chars[0];
		for($i=1 ; $i<=$#chars ; $i++)
		{
		    #print "pushing: $chars[$i] - ";
		    push(@code , $chars[$i]);
		}
		$sig_code = join('' , @code[0..$#code]);
		#print "$sig_code -> $val\n";
		if($is_first_toggle_in_timestep{$sig_code} == 1)
		{
		    $state_at_start{$sig_code} = $val;
		    #$TC{($signal_name{$sig_code})}++;
		    print "$signal_name{$sig_code} TC: $TC{($signal_name{$sig_code})}\n";
		    $following_transition{$sig_code} = $val;
		    $is_first_toggle_in_timestep{$sig_code} = 0;
		}
		elsif($is_first_toggle_in_timestep{$sig_code} == 0)
		{
		    $following_transition{$sig_code} = $val;
		}
		
		$newstring = "";
		@code = "";
	    }
	    elsif($check[0] eq "#")
	    {
		
		for($i=0 ; $i<=$#codes ; $i++)
                {
                    #print "\t\t$signal_name{$codes[$i]} : $following_transition{$codes[$i]} vs. $current_val{$codes[$i]}\n";
                    if($following_transition{$codes[$i]} ne $current_val{$codes[$i]})
                    {
                        $TC{ (  $signal_name{$codes[$i]}  ) }++;
                        #print "\t\t\t$signal_name{$codes[$i]} TC: $TC{($signal_name{$codes[$i]})}\n";
                        if($following_transition{$codes[$i]} eq "1")
                        {
                            #print "\t\t\t\t$signal_name{$codes[$i]} : going to logic 1 at $sim_time\n";
                            $logic_1_time_count_start{$codes[$i]} = $sim_time;
                        }
                        elsif($current_val{$codes[$i]} eq "1")
                        {
                            $T1{(  $signal_name{$codes[$i]}  )} += ($sim_time - $logic_1_time_count_start{$codes[$i]});
                            #print "\t\t\t\t$signal_name{$codes[$i]} : moving from logic 1 at $sim_time\n";
                        }
                        #print "\t\t\t$signal_name{$codes[$i]} T1: $T1{(  $signal_name{$codes[$i]}  )}\n";
			$current_val{$codes[$i]} = $following_transition{$codes[$i]};
		    }
		}
		    
		    
		$temp_time = $prev_sim_time;
		$sig_time = "";
		@time_lapse = "";
		for($i=1 ; $i<=$#check ; $i++)
		{
		    push(@time_lapse , $check[$i]);
		}
		$prev_sim_time = $sim_time;
		$sim_time = join('' , @time_lapse[0..$#time_lapse]);
		for($i=0 ; $i<=$#codes ; $i++)
		{
		    $is_first_toggle_in_timestep{$codes[$i]} = 1;
		}
		
	    }
	}
    }
}

for($i=0 ; $i<=$#codes ; $i++)
{
    if($current_val{$codes[$i]} eq "1")
    {
	$T1{(  $signal_name{$codes[$i]}  )} += ($sim_time - $logic_1_time_count_start{$codes[$i]});
	#print "\n$signal_name{$codes[$i]} T1 final:  $T1{(  $signal_name{$codes[$i]}  )}\n";
    }
}


while(<blif>)
{
    @tokens = split(/ +/);
    if($tokens[0] eq ".inputs")
    {
        chop($tokens[$#tokens]);
        for($i=1 ; $i<=$#tokens ; $i++)
	{
	    $fixed_name = $tokens[$i];
	    $fixed_name =~ s/\^/\_/g;
	    $fixed_name =~ s/\+/\_/g;
	    $fixed_name =~ s/\~/\_/g;
	    $fixed_name =~ s/\[/\_/g;
	    $fixed_name =~ s/\]/\_/g;
	    
	    $stat_prob = $T1{$fixed_name} / $sim_time ;
	    $activity = $TC{$fixed_name} / ($sim_time / 20000) ;

	    print "$tokens[$i] $stat_prob $activity\n";
	}
    }
    elsif($tokens[0] eq ".names")
    {
	chop($tokens[$#tokens]);

	$fixed_name = $tokens[$#tokens];
	$fixed_name =~ s/\^/\_/g;
	$fixed_name =~ s/\+/\_/g;
	$fixed_name =~ s/\~/\_/g;
	$fixed_name =~ s/\[/\_/g;
	$fixed_name =~ s/\]/\_/g;
	$vcd_name= $fixed_name . "_output_0_0";
	#print "$fixed_name $vcd_name $T1{$vcd_name} $TC{$vcd_name} \n";
	
	$stat_prob = $T1{$vcd_name} / $sim_time ;
	$activity =$TC{$vcd_name} / ($sim_time / 20000);
	
	print "$tokens[$#tokens] $stat_prob $activity\n";
    }
    elsif($tokens[0] eq ".latch")
    {
	$fixed_name = $tokens[2];
        $fixed_name =~ s/\^/\_/g;
        $fixed_name =~ s/\+/\_/g;
        $fixed_name =~ s/\~/\_/g;
        $fixed_name =~ s/\[/\_/g;
        $fixed_name =~ s/\]/\_/g;
        $vcd_name= $fixed_name . "_output_0_0";
        #print "$fixed_name $vcd_name $T1{$vcd_name} $TC{$vcd_name} \n";
 
        $stat_prob = $T1{$vcd_name} / $sim_time ;
        $activity =$TC{$vcd_name} / ($sim_time / 20000);

        print "$tokens[2] $stat_prob $activity\n";

    }
}
