
##############################################################
# Helper functions for verilog generation and other parsing
# author: jtrapass (Joseph Paul Trapasso)
# under the guidance of James Hoe and Markus Pueschel
# at Carnegie Mellon University
##############################################################

sub getHex{
	my $val = shift;
	if($val < 10){
		return "$val";
	}
	return "A" if($val == 10);
	return "B" if($val == 11);
	return "C" if($val == 12);
	return "D" if($val == 13);
	return "E" if($val == 14);
	return "F" if($val == 15);
	
	return "?";
}

sub toHex{
	my ($x, $bits) = @_;
	my $returnVal = "$bits\'h";
	while($bits >= 4){
		$returnVal .= getHex($x & 0xF);
		$bits -= 4;
		$x = $x >> 4;
	}

	if($bits){
		$returnVal .= getHex($x & ((1 << $bits) - 1));
	}
	
	return $returnVal;
}

sub max{
	my $maxX = shift;
	foreach my $x (@_){
		if($x > $maxX){
			$maxX = $x;
		}
	}
	return $maxX;
}

sub min{
	my $minX = shift;
	foreach my $x (@_){
		if($x < $minX){
			$minX = $x;
		}
	}
	return $minX;
}

#-----------------------------------------------------------------------
# @brief removes negative values of 0
# @param value value to compare
# @return value if not (-0), 0 if it is
#-----------------------------------------------------------------------
sub fix_zero {
	my ($value) = @_;
	if($value == 0){
		return 0;
	}
	return $value;
} 

#-----------------------------------------------------------------------
# @brief replaces integer value with representative integer string
# @param x value to convert to string
# @return abs of x, with an _ if x is negative, eg:
#     6 => 6
#    -2 => 2_
#     0 => 0
#-----------------------------------------------------------------------
sub checkPos{ 
	my ($x) = @_;
	#necessary to weed out -0
	$x = fix_zero($x);
	if($x < 0){
		my $new_val = -1 * $x;
		return "${new_val}_";
	}
	else{
		return "$x";
	}
}

#-----------------------------------------------------------------------
# @brief prints the necessary code to cleanly register 1 input
# @param fh file handle to print to
# @param regs list of register values references: (module input wire, register output wire, bitwidth)
# @param clk wire name for the clock wire
# @return undefined
#-----------------------------------------------------------------------
sub registerIn{
	my ($fh, $regList, $clk, $reset, $reset_edge) = @_;
	
	print $fh "  //registerIn\n";

	my $resetSenses = "";
	my $resetIndent = "";

	if(defined($reset)){
		$resetSenses = " or " . $reset_edge . " " . $reset;
		$resetIndent = "  ";
	}
	
	my $always_block = "\n  always@(posedge ${clk}${resetSenses}) begin\n";
	
	$always_block .= "    if(~$reset) begin\n" if(defined($reset) && ($reset_edge eq "negedge"));
	$always_block .= "    if($reset) begin\n" if(defined($reset) && ($reset_edge eq "posedge"));
	
	my $non_resets = "";
	my $resets = "";

	my $regSize = 0;

	foreach my $reg(@$regList){
		my ($inWire, $inReg, $bitWidth) = @$reg;
		if(!(defined($inWire) && defined($inReg) && defined($bitWidth))){
			print STDERR "Bad parameters passed to registerIn($inWire, $inReg, $bitWidth)";
			exit(-2);
		}
		$regSize += flopArea($bitWidth, defined($reset));
		print $fh "  reg [" . ($bitWidth - 1) . ":0] $inReg;\n";

		$resets .= $resetIndent . "    $inReg <= " . toHex(0, $bitWidth) . ";\n";
		$non_resets .= $resetIndent . "    $inReg <= $inWire;\n";
	}
	
	if(defined($reset)){
		$always_block .= 
			$resets .
			"    end  else begin\n" .
			$non_resets .
			"    end\n";
	}    
	else{
		$always_block .= $non_resets;
	}
	$always_block .= "  end\n\n";
	
	print $fh $always_block;
	
	return $regSize;
}

#-----------------------------------------------------------------------
# @brief prints the necessary code to cleanly register 1 output
# @param fh file handle to print to
# @param regs list of register values references: (module output wire, register input wire, bitwidth)
# @param clk wire name for the clock wire
# @return undefined
#-----------------------------------------------------------------------
sub registerOut{
	my ($fh, $regList, $clk, $reset, $reset_edge) = @_;

	print $fh "  //registerOut\n";
	
	my $resetSenses = "";
	my $resetIndent = "";

	if(defined($reset)){
		$resetSenses = " or " . $reset_edge . " " . $reset;
		$resetIndent = "  ";
	}
	
	my $always_block = "\n  always@(posedge ${clk}${resetSenses}) begin\n";
	
	$always_block .= "    if(~$reset) begin\n" if(defined($reset) && ($reset_edge eq "negedge"));
	$always_block .= "    if($reset) begin\n" if(defined($reset) && ($reset_edge eq "posedge"));
	
	my $resets = "";
	my $non_resets = "";
	
	my $regSize = 0;

	foreach my $reg(@$regList){
		my ($outReg, $inWire, $bitWidth) = @$reg;
		if(!(defined($inWire) && defined($outReg) && defined($bitWidth))){
			print STDERR "Bad parameters passed to registerOut($inWire, $outReg, $bitWidth)";
			exit(-2);
		}

		$regSize += flopArea($bitWidth, defined($reset));
		
		print $fh "  reg [" . ($bitWidth - 1) . ":0] $outReg;\n";
		#this may not have been instantiated yet
		print $fh "  wire [" . ($bitWidth - 1) . ":0] $inWire;\n";
		
		
		$resets = $resetIndent . "    $outReg <= " . toHex(0, $bitWidth) . ";\n" ;
		$non_resets .= $resetIndent . "    $outReg <= $inWire;\n";
		
		
	}
	
	if(defined($reset)){
		$always_block .= 
			$resets . 
			"    end  else begin\n" .
			$non_resets .
			"    end\n";
	}
	else{
		$always_block .= $non_resets;
	}
	$always_block .= "  end\n\n";
	
	print $fh $always_block;
	return $regSize;
}

#-----------------------------------------------------------------------
# @brief obtain the numeric value of the string, decimal or integer
#  borrowed from: http://www.unix.org.ua/orelly/perl/cookbook/ch02_02.htm
# @param str string to extract value from
# @return numeric value or undefined if no value extractable
#-----------------------------------------------------------------------
sub getnum {
	use POSIX qw(strtod);
	my $str = shift;
	$str =~ s/^\s+//;
	$str =~ s/\s+$//;
	$! = 0;
	my($num, $unparsed) = strtod($str);
	if (($str eq '') || ($unparsed != 0) || $!) {
		return;
	} else {
		return $num;
	} 
}

#-----------------------------------------------------------------------
# @brief checks if the string is numeric
#  borrowed from: http://www.unix.org.ua/orelly/perl/cookbook/ch02_02.htm
# @param str string to check
# @return true if numeric, false otherwise
#-----------------------------------------------------------------------
sub is_numeric { defined scalar &getnum } 


#-----------------------------------------------------------------------
# @brief prints header and port declations in the form of:
#          module $moduleName(
#            
# @param fh File handle to print to, pass by: \*FH
# @param moduleName name of the module being generating
# @param portList array of list refs of:
#                 (portNames, portTypes, portBitwidths, portSizes)
#                  bitwidths and sizes are optional
#                  bitwidth should be in the form of [15:0]
# @return undefined
#-----------------------------------------------------------------------
sub genHeader {
	my ($fh, $moduleName, $portList) = @_;
	
	#Header information explaining file
	print $fh "\nmodule $moduleName (";
	
	my $portModeDecl = "";

	my $wiresNeeded = "";
	my $assignStatements = "";
	
	my $portListSize = scalar(@$portList);
	$portModeDecl .= "  // Port mode declarations:\n" if($portListSize);
	for(my $i = 0; $i < $portListSize; $i++){
		my $port = @$portList[$i];
		my ($portType, $portName, $portBitwidth, $portSize) = @$port;
		
		if(!(defined($portName) && defined($portType))){
			print STDERR "Bad parameters passed to genHeader($portName, $portType)";
			exit(-2);
		}
		
		$portModeDecl .=  "  $portType ";
		$portModeDecl .= "[" . ($portBitwidth-1) . ":0]" if(defined($portBitwidth) && ($portBitwidth > 1));
		
		if(defined($portSize)) {
			#necessary to add commas
			print $fh "\n    ${portName}0";
			# $wiresNeeded .= 
			# "  wire [" . ($portBitwidth-1) . ":0] ${portName} [0:" . ($portSize-1) . "];\n";
			if($portType =~ /input/i){
				$assignStatements .= 
					"  assign ${portName} = {\n" .
					"    ${portName}0";
			}
			elsif($portType =~ /output/i){
				# $assignStatements .= 
				#     "  assign ${portName}0 = ${portName}\[0\];\n";
			}
			else{
				print STDERR "port type: $portType does not support a size beyond 1\n";
				exit(-2);
			}
			$portModeDecl .= "\n    ${portName}0";
			for(my $j = 1; $j < $portSize; $j++){
				my $str = ",\n    ${portName}" . ($j);
				if($portType =~ /input/i){
					$assignStatements .= $str;
				}
				elsif($portType =~ /output/i){
				 #    $assignStatements .= 
						# "  assign ${portName}" . (${j}) ." = ${portName}\[${j}\];\n";
				}
				print $fh $str;
				$portModeDecl .= $str;
			}
			
			if($portType =~ /input/i){
				$assignStatements .= "};\n";
			}

		}
		else{
			print $fh "\n    ${portName}0";
			$portModeDecl .= " ${portName}0";
		}
		
		$portModeDecl .=  ";\n";

		if ($i == $portListSize - 1){
			#last port, no comma
			print $fh "\n";
		}
		else{
			print $fh ",";
		}
	}

	print $fh ");\n";

	print  $fh "\n" . $portModeDecl . "\n";
	print  $fh $wiresNeeded . "\n"if($wiresNeeded);
	print  $fh $assignStatements . "\n" if($assignStatements);
}

#-----------------------------------------------------------------------
# @brief ends the module, with a useful comment to describe it
# @param fh file handle to print to, pass to like \*FH
# @param moduleName module to close
# @return undefined
#-----------------------------------------------------------------------
sub genTail {
	my ($fh, $moduleName, $areaEstimate) = @_;

	if(defined($areaEstimate)){
		print $fh "  //${moduleName} area estimate = ${areaEstimate};\n";
	}
	print $fh "endmodule //" . $moduleName . "\n\n";
}

#area estimates
sub adderArea {
	my ($bits) = @_;
	if($bits > 0){
		#cmulib18:return $bits * 14.5 - 11.5;
		return 69.8544022485294 * $bits - 39.9168140441175;
	}
	return 0;
}

sub subArea {
	my ($bits) = @_;
	if($bits > 0){
		# cmulib18: return $bits * 14 - 12.5;
		return 76.5071964941177 * $bits - 39.9167650514706;
	}
	return 0;
}

sub negArea {
	my ($bits) = @_;
	if($bits > 0){
		#cmulib18:return $bits * 7 - 8.5;
		return 46.5696014852941 * $bits - 53.2224088161765;
	}
	return 0;
}

sub flopArea {
	my ($bits, $reset) = @_;

	if($bits > 0){
		if(defined($reset) && $reset){
			#cmulib18 version: return $bits * 15;
			return 76.5072021588235 * $bits;
		}else{
			#cmulib18 version:return $bits * 10;
			return 56.5488022735294 * $bits;
		}
	}
	return 0;
}

#return true
1;