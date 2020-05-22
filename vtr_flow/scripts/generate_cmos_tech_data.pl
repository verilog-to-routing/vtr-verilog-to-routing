#!/usr/bin/env perl

use strict;
use POSIX;
use File::Basename;
use File::Spec;
use Cwd 'abs_path';

sub get_capacitances;
sub get_pn_ratio;
sub get_riset_fallt_diff;
sub get_Vth;
sub add_subckts;
sub get_leakage;
sub get_gate_leakage;
sub get_buffer_sc;
sub transistors;
sub components;
sub muxes;
sub nmos_leakages;

my $hspice      = "hspice";
my $script_path = ( fileparse( abs_path($0) ) )[1];
my $spice_path  = File::Spec->join( $script_path, "spice" );

my $quick_test = 0;

# Simulation Time
my $simt = "5n";

# P/N Ratio
my $pn_interval           = 0.1;
my $pn_search_lower_bound = 0.5;
my $pn_search_upper_bound = 4.0;

# Transistor Capacitances
my $max_size      = 500;
my $size_interval = 1.05;

# NMOS Pass transistor sizes
my @nmos_pass_sizes;
my $nmos_pass_interval = 1.25;
my $nmos_pass_max_size = 25;

# Multiplexer Voltages
my $max_mux_size  = 30;
my $vin_intervals = 10;

# Vds Leakage
my $vds_intervals = 25;

if ($quick_test) {
	$pn_interval           = 1.0;
	$pn_search_lower_bound = 1.0;
	$pn_search_upper_bound = 3.0;
	$max_size              = 2;
	$size_interval         = 1.5;
	$max_mux_size          = 2;
	$vin_intervals         = 3;
	$vds_intervals         = 5;
	$nmos_pass_interval    = 2.0;
    $nmos_pass_max_size    = 3.0;
}

#my $max_buffer_size = 100;
#my $buffer_interval = 1.5;
#my $mux_interval    = 4;

my $number_arguments = @ARGV;
if ( $number_arguments < 4 ) {
	print(
		"usage: generate_cmos_tech_data.pl <tech_file> <tech_size> <Vdd> <temp>\n"
	);
	exit(-1);
}

my $tech_file      = abs_path( shift(@ARGV) );
my $tech_size      = shift(@ARGV);
my $Vdd            = shift(@ARGV);
my $temp           = shift(@ARGV);
my $tech_file_name = basename($tech_file);

-r $tech_file or die "Cannot open tech file ($tech_file)";

my $C_g;
my $C_s;
my $C_d;

my @sizes;
my @long_size        = ( 1,      2 );
my @transistor_types = ( "nmos", "pmos" );

my $optimal_p_to_n = get_pn_ratio();

my $size = 1.0;
while ( $size < $max_size ) {
	my $size_rounded = ( sprintf "%.2f", $size );
	push( @sizes, [ $size_rounded, 1 ] );
	$size = $size * $size_interval;
}

$size = 1.0;
while ( $size < $nmos_pass_max_size ) {
	my $size_rounded = ( sprintf "%.2f", $size );
	push( @nmos_pass_sizes, $size_rounded );
	$size = $size * $nmos_pass_interval;
}

#print join(", ", @nmos_pass_sizes);

print "<technology file=\"$tech_file_name\" size=\"$tech_size\">\n";

print "\t<operating_point temperature=\"$temp\" Vdd=\"$Vdd\"/>\n";
print "\t<p_to_n ratio=\"" . $optimal_p_to_n . "\"/>\n";

transistors();
muxes();
nmos_leakages();
components();

print "</technology>\n";

sub transistors() {
	foreach my $type (@transistor_types) {

		#my $Vth = get_Vth();

		print "\t<transistor type=\"$type\">\n";

		( $C_g, $C_s, $C_d ) =
		  get_capacitances( $type, $long_size[0], $long_size[1] );
		print "\t\t<long_size W=\""
		  . $long_size[0]
		  . "\" L=\""
		  . $long_size[1] . "\">\n";
		print "\t\t\t<leakage_current subthreshold=\""
		  . get_leakage_long( $type, $long_size[0], $long_size[1] )
		  . "\"/>\n";
		print "\t\t\t<capacitance C_g=\"$C_g\" C_s=\"$C_s\" C_d=\"$C_d\"/>\n";
		print "\t\t</long_size>\n";

		foreach my $size_ref (@sizes) {
			my @size = @$size_ref;
			( $C_g, $C_s, $C_d ) =
			  get_capacitances( $type, $size[0], $size[1] );
			print "\t\t<size W=\"" . $size[0] . "\" L=\"" . $size[1] . "\">\n";
			print "\t\t\t<leakage_current subthreshold=\""
			  . get_leakage( $type, $size[0], $optimal_p_to_n )
			  . "\" gate=\""
			  . get_gate_leakage( $type, $size[0], $optimal_p_to_n )
			  . "\"/>\n";
			print
			  "\t\t\t<capacitance C_g=\"$C_g\" C_s=\"$C_s\" C_d=\"$C_d\"/>\n";
			print "\t\t</size>\n";
		}
		print "\t</transistor>\n";
	}
}

sub muxes() {
	print "\t<multiplexers>\n";
	foreach my $size (@nmos_pass_sizes) {
		print "\t\t<nmos size=\"" . $size . "\">\n";
		for ( my $i = 1 ; $i <= $max_mux_size ; $i++ ) {
			print "\t\t\t<multiplexer size=\"" . $i . "\">\n";
			for ( my $j = 0 ; $j <= $vin_intervals ; $j++ ) {
				my $Vin = $Vdd * ( 0.5 + 0.5 * ( $j / $vin_intervals ) );
				my ( $min, $max ) = get_mux_out_voltage( $size, $i, $Vin );
				print "\t\t\t\t<voltages in=\"" . $Vin
				  . "\" out_min=\""
				  . $min
				  . "\" out_max=\""
				  . $max
				  . "\"/>\n";
			}
			print "\t\t\t</multiplexer>\n";
		}
		print "\t\t</nmos>\n";
	}
	print "\t</multiplexers>\n";
}

sub nmos_leakages() {
	print "\t<nmos_leakages>\n";
	foreach my $size (@nmos_pass_sizes) {
		print "\t\t<nmos size=\"" . $size . "\">\n";
		for ( my $i = 0 ; $i <= $vds_intervals ; $i++ ) {
			my $Vds = $Vdd * ( 0.5 + 0.5 * ( $i / $vds_intervals ) );
			my $leakage_current = get_nmos_leakage_for_Vds( $size, $Vds );
			print "\t\t\t<nmos_leakage Vds=\"" . $Vds
			  . "\" Ids=\""
			  . $leakage_current
			  . "\"/>\n";
		}
		print "\t\t</nmos>\n";
	}
	print "\t </nmos_leakages>\n";
}

sub components() {
	my $spice_script =
	  File::Spec->join( $script_path, "spice", "run_spice.py" );
	my $temp_file =
	  File::Spec->join( $script_path, "spice", "temp", "temp.txt" );

	my $tech_size_nm = $tech_size * 1e9;

	# 	Component_name,	@Inputs, @Sizes, type
	my @components = (
		[ "buf",      [1], [ 1, 4, 16, 64 ], 0 ],
		[ "buf_levr", [1], [ 1, 4, 16, 64 ], 0 ],
		[ "mux", [ 4, 9, 16, 25 ], \@nmos_pass_sizes, 1 ],
		[ "lut", [ 2, 4, 6 ], \@nmos_pass_sizes, 1 ],
		[ "dff", [1], [ 1, 2, 4, 8 ], 0 ]
	);

	print "\t<components>\n";

	foreach my $component_ref (@components) {
		my @component = @$component_ref;

		my $component_name = @component[0];
		my $inputs         = @component[1];
		my $sizes          = @component[2];
		my $type           = @component[3];

		print "\t\t<$component_name>\n";

		foreach my $num_inputs (@$inputs) {
			print "\t\t\t<inputs num_inputs=\"" . $num_inputs . "\">\n";
			foreach my $size (@$sizes) {
				my $cmd;
				if ( $type == 0 ) {
					$cmd =
					  "$spice_script $tech_file $tech_size_nm $Vdd $optimal_p_to_n $temp h $component_name $size";
				}
				else {
					$cmd =
					  "$spice_script $tech_file $tech_size_nm $Vdd $optimal_p_to_n $temp h $component_name $num_inputs $size";
				}

				#				print $cmd;
				my $result = `$cmd`;
				chomp($result);
				print "\t\t\t\t<size transistor_size=\"" . $size
				  . "\" power=\""
				  . $result
				  . "\"/>\n";
			}
			print "\t\t\t</inputs>\n";
		}
		print "\t\t</$component_name>\n";
	}

	print "\t</components>\n";
}

sub get_pn_ratio {
	my @sizes;
	my $opt_size;
	my $opt_percent;

	for (
		my $size = $pn_search_lower_bound ;
		$size < $pn_search_upper_bound ;
		$size += $pn_interval
	  )
	{
		push( @sizes, $size );
	}

	$opt_percent = 10000;    # large number
	foreach my $size (@sizes) {
		my $diff = get_riset_fallt_diff($size);

		#print "$size $diff\n";
		if ( $diff < $opt_percent ) {
			$opt_size    = $size;
			$opt_percent = $diff;
		}
	}

	return $opt_size;
}

sub get_nmos_leakage_for_Vds {
	my $size = shift(@_);
	my $Vds  = shift(@_);

	my $s = spice_header();
	$s = $s . "Vin in 0 " . $Vds . "\n";
	$s = $s . "X0 in 0 0 0 nfet size='" . $size . "'\n";
	$s = $s . spice_sim(10);
	$s = $s . ".measure tran leakage avg I(Vin)\n";
	$s = $s . spice_end();

	my @results = spice_run( $s, ["leakage"] );
	return $results[0];
}

sub get_mux_out_voltage_min {
	my $s = spice_header();

	$s = $s . "X0 Vdd Vdd out 0 nfet size='1'\n";

	for ( my $i = 1 ; $i < $max_mux_size ; $i++ ) {
		$s = $s . "X" . $i . " 0 0 out 0 nfet size='1'\n";
	}

	$s = $s . spice_sim(10);

	$s = $s . ".measure tran vout avg V(out)\n";

	$s = $s . spice_end();

	my @results = spice_run( $s, ["vout"] );

	return $results[0];
}

sub get_mux_out_voltage {
	my $size     = shift(@_);
	my $mux_size = shift(@_);
	my $Vin      = shift(@_);

	my $s = spice_header();
	$s = $s . "Vin in 0 " . $Vin . "\n";
	$s = $s . "X0a in Vdd outa 0 nfet size='" . $size . "'\n";
	for ( my $i = 1 ; $i < $mux_size ; $i++ ) {
		$s = $s . "X" . $i . "a in 0 outa 0 nfet size='" . $size . "'\n";
	}
	$s = $s . "X0b in Vdd outb 0 nfet size='" . $size . "'\n";
	for ( my $i = 1 ; $i < $mux_size ; $i++ ) {
		$s = $s . "X" . $i . "b 0 0 outb 0 nfet size='" . $size . "'\n";
	}
	$s = $s . spice_sim(10);
	$s = $s . ".measure tran vout_min avg V(outb)\n";
	$s = $s . ".measure tran vout_max avg V(outa)\n";
	$s = $s . spice_end();
	my @results = spice_run( $s, [ "vout_min", "vout_max" ] );

	return @results;
}

sub calc_buffer_stage_effort {
	my $N = shift(@_);
	my $S = shift(@_);

	if ( $N > 1 ) {
		return $S**( 1.0 / ( $N - 1 ) );
	}
}

sub calc_buffer_num_stages {
	my $S = shift(@_);

	if ( $S <= 1.0 ) {
		return 1;
	}
	elsif ( $S <= 4.0 ) {
		return 2;
	}
	else {
		my $N = int( log($S) / log(4.0) + 1 );
		if (
			abs( calc_buffer_stage_effort( $N + 1, $S ) - 4 ) <
			abs( calc_buffer_stage_effort( $N, $S ) - 4 ) )
		{
			$N = $N + 1;
		}
		return $N;
	}
}

#sub print_buffer_sc {
#	my $pn_ratio = shift(@_);
#
#	print "\t<buffer_sc>\n";
#
#	for ( my $N = 1.0 ; $N <= 5 ; $N = $N + 1 ) {
#
#		print "\t\t<stages num_stages='" . $N . "'>\n";
#
#		for ( my $g = 1 ; $g <= 6 ; $g = $g + 1 ) {
#
#			my $sc_nolevr = get_buffer_sc( $N, $g, 0, 0, $pn_ratio );
#
#			print "\t\t\t<strength gain='" . $g
#			  . "' sc_nolevr='"
#			  . $sc_nolevr . "'>\n";
#
#			for (
#				my $i = 1 ;
#				$i <= $max_mux_size + $mux_interval ;
#				$i = $i + $mux_interval
#			  )
#			{
#				my $sc_levr = get_buffer_sc( $N, $g, 1, $i, $pn_ratio );
#				print "\t\t\t\t<input_cap mux_size='" . $i
#				  . "' sc_levr='"
#				  . $sc_levr . "'/>\n";
#			}
#
#			print "\t\t\t</strength>\n";
#
#			if ( $N == 1 ) {
#				last;
#			}
#		}
#
#		print "\t\t</stages>\n";
#	}
#
#	print "\t</buffer_sc>\n";
#}

#sub get_buffer_sc {
#	my $N              = shift(@_);
#	my $g              = shift(@_);
#	my $level_restorer = shift(@_);
#	my $mux_in_size    = shift(@_);
#	my $pn_ratio       = shift(@_);
#
#	my $s = "";
#
#	$s = spice_header();
#
#	# Voltage Sources per stage
#	for ( my $i = 0 ; $i < $N ; $i = $i + 1 ) {
#		$s = $s . "Vup" . $i . " Vdd VupL" . $i . " 0\n";
#		$s = $s . "Vdown" . $i . " VdownH" . $i . " 0 0\n";
#		$s = $s . "Vgate" . $i . " VgateH" . $i . " out" . ( $i + 1 ) . " 0\n";
#		$s =
#		    $s . "VgP"
#		  . ( $i + 1 ) . " out"
#		  . ( $i + 1 ) . " VgLP"
#		  . ( $i + 1 ) . " 0\n";
#		$s =
#		    $s . "VgN"
#		  . ( $i + 1 ) . " out"
#		  . ( $i + 1 ) . " VgLN"
#		  . ( $i + 1 ) . " 0\n";
#	}
#
#	# Input Pulse
#	$s = $s
#	  . "Vin in 0 PWL (0 0 'simt/4' 0 'simt/4+rise' Vol '3*simt/4' Vol '3*simt/4+rise' 0)\n";
#
#	if ($level_restorer) {
#		$s = $s . "Xmux0pre in Vdd inA 0 nfet size='1'\n";
#		$s = $s . "Xmux0 inA Vdd out0 0 nfet size='1'\n";
#		for ( my $i = 1 ; $i < $mux_in_size ; $i = $i + 1 ) {
#			if ( $i % 2 == 0 ) {
#				$s = $s . "Xmux" . $i . " 0 0 out0 0 nfet size='1'\n";
#			}
#			else {
#				$s = $s . "Xmux" . $i . " Vdd 0 out0 0 nfet size='1'\n";
#			}
#
#		}
#		$s = $s . "X0 out0 VgateH0 VupL0 VdownH0 levr\n";
#	}
#	else {
#		$s = $s . "XinA in inA Vdd 0 inv nsize='1' psize='" . $pn_ratio . "'\n";
#		$s =
#		  $s . "XinB inA out0 Vdd 0 inv nsize='1' psize='" . $pn_ratio . "'\n";
#		$s = $s
#		  . "X0 out0 VgateH0 VupL0 VdownH0 inv nsize='1' psize='"
#		  . $pn_ratio . "'\n";
#	}
#
#	my $stage_size = $g;
#	for ( my $i = 1 ; $i < $N ; $i = $i + 1 ) {
#
#		$s =
#		    $s . "X" . $i . " VgLP" . $i . " VgLN" . $i
#		  . " VgateH"
#		  . $i . " VupL"
#		  . $i
#		  . " VdownH"
#		  . $i
#		  . " invd nsize='"
#		  . $stage_size
#		  . "' psize='"
#		  . ( $stage_size * $pn_ratio ) . "'\n";
#		$stage_size = $stage_size * $g;
#	}
#
#	$s = $s . spice_sim(10000);
#
#	my $up = 1;
#	for ( my $i = 0 ; $i < $N ; $i = $i + 1 ) {
#		if ($up) {
#			$s = $s
#			  . ".measure rs"
#			  . $i
#			  . " when V(out"
#			  . $i
#			  . ")='0.1*Vol' CROSS=1\n";
#			$s = $s
#			  . ".measure re"
#			  . $i
#			  . " when V(out"
#			  . ( $i + 1 )
#			  . ")='0.1*Vol' CROSS=1\n";
#			$s = $s
#			  . ".measure fs"
#			  . $i
#			  . " when V(out"
#			  . $i
#			  . ")='0.9*Vol' CROSS=2\n";
#			$s = $s
#			  . ".measure fe"
#			  . $i
#			  . " when V(out"
#			  . ( $i + 1 )
#			  . ")='0.9*Vol' CROSS=2\n";
#		}
#		else {
#			$s = $s
#			  . ".measure fs"
#			  . $i
#			  . " when V(out"
#			  . $i
#			  . ")='0.9*Vol' CROSS=1\n";
#			$s = $s
#			  . ".measure fe"
#			  . $i
#			  . " when V(out"
#			  . ( $i + 1 )
#			  . ")='0.9*Vol' CROSS=1\n";
#			$s = $s
#			  . ".measure rs"
#			  . $i
#			  . " when V(out"
#			  . $i
#			  . ")='0.1*Vol' CROSS=2\n";
#			$s = $s
#			  . ".measure re"
#			  . $i
#			  . " when V(out"
#			  . ( $i + 1 )
#			  . ")='0.1*Vol' CROSS=2\n";
#		}
#		$s = $s . ".measure rd" . $i . " param=('re" . $i . "-rs" . $i . "')\n";
#		$s = $s . ".measure fd" . $i . " param=('fe" . $i . "-fs" . $i . "')\n";
#		$up = !$up;
#
#		$s = $s
#		  . ".measure tran ITr"
#		  . $i
#		  . " integ Par('0.5*(I(Vdown"
#		  . $i
#		  . ") + abs(I(Vdown"
#		  . $i
#		  . ")))') FROM '(rs"
#		  . $i
#		  . " - 0.25 * rd"
#		  . $i
#		  . ")' TO '(re"
#		  . $i
#		  . " + 0.25 * rd"
#		  . $i . ")'\n";
#		$s = $s
#		  . ".measure tran ISCr"
#		  . $i
#		  . " integ Par('0.5*(I(Vup"
#		  . $i
#		  . ") + abs(I(Vup"
#		  . $i
#		  . ")))') FROM '(rs"
#		  . $i
#		  . " - 0.25 * rd"
#		  . $i
#		  . ")' TO '(re"
#		  . $i
#		  . " + 0.25 * rd"
#		  . $i . ")'\n";
#
#		#$s = $s . ".measure tran IGr" . $i . " integ I(Vgate" . $i . ") FROM 'rs" . $i . "' TO 're" . $i . "'\n";
#		#$s = $s . ".measure tran IPr" . $i . " integ I(VgP" . ($i+1) . ") FROM 'rs" . $i . "' TO 're" . $i . "'\n";
#		#$s = $s . ".measure tran INr" . $i . " integ I(VgN" . ($i+1) . ") FROM 'rs" . $i . "' TO 're" . $i . "'\n";
#
#		$s = $s
#		  . ".measure tran ITf"
#		  . $i
#		  . " integ Par('0.5*(I(Vup"
#		  . $i
#		  . ") + abs(I(Vup"
#		  . $i
#		  . ")))') FROM '(fs"
#		  . $i
#		  . " - 0.25 * fd"
#		  . $i
#		  . ")' TO '(fe"
#		  . $i
#		  . " + 0.25 * fd"
#		  . $i . ")'\n";
#		$s = $s
#		  . ".measure tran ISCf"
#		  . $i
#		  . " integ Par('0.5*(I(Vdown"
#		  . $i
#		  . ") + abs(I(Vdown"
#		  . $i
#		  . ")))') FROM '(fs"
#		  . $i
#		  . " - 0.25 * fd"
#		  . $i
#		  . ")' TO '(fe"
#		  . $i
#		  . " + 0.25 * fd"
#		  . $i . ")'\n";
#
#		#$s = $s . ".measure tran IGf" . $i . " integ I(Vgate" . $i . ") FROM 'fs" . $i . "' TO 'fe" . $i . "'\n";
#		#$s = $s . ".measure tran IPf" . $i . " integ I(VgP" . ($i+1) . ") FROM 'fs" . $i . "' TO 'fe" . $i . "'\n";
#		#$s = $s . ".measure tran INf" . $i . " integ I(VgN" . ($i+1) . ") FROM 'fs" . $i . "' TO 'fe" . $i . "'\n";
#
#		$s = $s
#		  . ".measure tran fSC"
#		  . $i
#		  . " Param=('(ISCr"
#		  . $i . "+ISCf"
#		  . $i
#		  . ")/(ITr"
#		  . $i
#		  . " + ITf"
#		  . $i
#		  . " - ISCr"
#		  . $i
#		  . " - ISCf"
#		  . $i . ")')\n";
#	}
#
#	my $SCp;
#	my $SCm = "";
#	my $T;
#	for ( my $i = 0 ; $i < $N ; $i = $i + 1 ) {
#
#		if ( $i == 0 ) {
#			$SCp = "ISCr0 + ISCf0";
#			$T   = "ITr0 + ITf0";
#		}
#		else {
#			$SCp = $SCp . " + ISCr" . $i . " + ISCf" . $i;
#			$T   = $T . " + ITr" . $i . " + ITf" . $i;
#		}
#		$SCm = $SCm . " - ISCr" . $i . " - ISCf" . $i;
#	}
#	$s = $s
#	  . ".measure tran fSC Param=('("
#	  . $SCp
#	  . ")/(0.5*("
#	  . $T
#	  . $SCm
#	  . "))')\n";
#
#	$s = $s . spice_end();
#
#	my @results = spice_run( $s, [ "fsc", "fsc0" ] );
#
#	return $results[0];
#}

sub get_riset_fallt_diff {
	my $size = shift(@_);

	my $s = "";
	my $rise;
	my $fall;

	$s = spice_header();

	# Input Voltage
	$s = $s . "Vin in 0 PULSE(0 Vol 'simt/4' 'rise' 'fall' 'simt/2' 'simt')\n";

	# Inverter
	$s = $s . "X0 in out Vdd 0 inv nsize='2.0' psize='2.0*" . $size . "'\n";

	$s = $s . spice_sim(1000);

	$s = $s
	  . ".measure tran fallt TRIG V(in) VAL = '0.5*Vol' TD = 0 RISE = 1 TARG V(out) VAL = '0.5*Vol' FALL = 1\n";
	$s = $s
	  . ".measure tran riset TRIG V(in) VAL = '0.5*Vol' TD = 'simt/2' FALL = 1 TARG V(out) VAL = '0.5*Vol' RISE = 1\n";

	$s = $s . spice_end();

	my @results = spice_run( $s, [ "fallt", "riset" ] );

	return abs( $results[0] - $results[1] ) / $results[1];
}

sub spice_header {
	my $s = "";

	$s = $s . "Automated spice simuation: " . localtime() . "\n";
	$s = $s . ".include \"$tech_file\"\n";
	$s = $s . ".param tech = $tech_size\n";
	$s = $s . ".param Vol = $Vdd\n";

	$s = $s . ".param simt = " . $simt . "\n";
	$s = $s . ".param rise = 'simt/500'\n";
	$s = $s . ".param fall = 'simt/500'\n";

	$s = $s . ".include \"${spice_path}/subckt/nmos_pmos.sp\"\n";
	$s = $s . ".include \"${spice_path}/subckt/inv.sp\"\n";
	$s = $s . ".include \"${spice_path}/subckt/level_restorer.sp\"\n";

	$s = $s . "Vdd Vdd 0 'Vol'\n";

	return $s;
}

sub spice_sim {
	my $accuracy = shift(@_);
	my $s        = "";

	$s = $s . ".TEMP $temp\n";
	$s = $s . ".OP\n";
	$s = $s . ".OPTIONS LIST NODE POST CAPTAB\n";

	$s = $s . ".tran 'simt/" . $accuracy . "' simt\n";
	return $s;
}

sub spice_end {
	my $s = "";

	$s = $s . ".end\n\n";

	return $s;
}

sub spice_run {
	my $cmd          = shift(@_);
	my $result_names = shift(@_);

	my @results;

	my $run_dir    = File::Spec->join( $script_path, "spice", "temp" );
	my $spice_file = File::Spec->join( $run_dir,     "vtr_auto_spice.sp" );
	my $spice_out  = File::Spec->join( $run_dir,     "vtr_auto_spice.out" );

	open( SP_FILE, "> $spice_file" );
	print SP_FILE $cmd;
	close(SP_FILE);

	system("cd $run_dir; $hspice $spice_file 1> $spice_out 2> /dev/null");

	my $spice_data;
	{
		local $/ = undef;
		open( SP_FILE, "$spice_out" );
		$spice_data = <SP_FILE>;
		close SP_FILE;
	}

	foreach my $result_name ( @{$result_names} ) {
		if ( $spice_data =~
			/transient analysis.*?$result_name\s*=\s*[+-]*(\S+)/s )
		{
			push( @results, $1 );
		}
		else {
			die "Could not find $result_name in spice output.\n";
		}
	}

	return @results;
}

sub get_leakage_long {
	my $type = shift(@_);
	my $w    = shift(@_);
	my $l    = shift(@_);

	my $s = spice_header();

	$s = $s . "Vleak Vleak 0 Vol\n";
	if ( $type eq "nmos" ) {
		$s = $s . "X0 Vleak 0 0 0 nfetz wsize=" . $w . " lsize=" . $l . "\n";
	}
	else {
		$s =
		  $s . "X0 0 Vdd Vleak Vdd pfetz wsize=" . $w . " lsize=" . $l . "\n";
	}

	$s = $s . spice_sim(100);
	$s = $s . ".measure tran leakage avg I(Vleak)\n";
	$s = $s . spice_end();
	my @results = spice_run( $s, ["leakage"] );

	return $results[0];
}

sub get_gate_leakage {
	my $type = shift(@_);
	my $size = shift(@_);
	my $pn   = shift(@_);

	my $s = spice_header();

	if ( $type eq "nmos" ) {
		$s = $s . "Vleak Vdd VleakL 0\n";
		$s = $s
		  . "X0 VleakL out Vdd 0 inv nsize='"
		  . $size
		  . "' psize='"
		  . ( $size * $pn ) . "'\n";
	}
	else {
		$s = $s . "Vleak VleakH 0 0\n";
		$s = $s
		  . "X0 VleakH out Vdd 0 inv nsize='"
		  . ( $size / $pn )
		  . "' psize='"
		  . $size . "'\n";
	}

	$s = $s . spice_sim(100);
	$s = $s . ".measure tran leakage avg I(Vleak)\n";
	$s = $s . spice_end();
	my @results = spice_run( $s, ["leakage"] );

	return $results[0];

}

sub get_leakage {
	my $type = shift(@_);
	my $size = shift(@_);
	my $pn   = shift(@_);

	my $s = spice_header();

	if ( $type eq "nmos" ) {
		$s = $s . "Vleak Vleak 0 0\n";
		$s = $s
		  . "X0 0 out Vdd Vleak inv nsize='"
		  . $size
		  . "' psize='"
		  . ( $size * $pn ) . "'\n";
	}
	else {
		$s = $s . "Vleak Vdd VleakL 0\n";
		$s = $s
		  . "X0 Vdd out VleakL 0 inv nsize='"
		  . ( $size / $pn )
		  . "' psize='"
		  . $size . "'\n";
	}

	$s = $s . spice_sim(100);
	$s = $s . ".measure tran leakage avg I(Vleak)\n";
	$s = $s . spice_end();
	my @results = spice_run( $s, ["leakage"] );

	return $results[0];

}

sub get_capacitances {
	my $type   = shift(@_);
	my $width  = shift(@_);
	my $length = shift(@_);

	my $s = "";
	my $C_g;
	my $C_s;
	my $C_d;

	$s = spice_header();
	$s = $s . ".param tick = 'simt/6'\n";

	# Gate, Drain, Source Inputs
	# Time	NMOS			PMOS
	# 		G 	D 	S		G 	D 	S
	# 0		0 	0 	0		0 	0 	0
	# 1		0 	0 	1     	0 	1 	1
	# 2		0 	1 	0     	1 	0 	0
	# 3		0 	1 	1     	1 	0 	1
	# 4		1 	0 	0     	1 	1 	0
	# 5		1 	1 	1	    1 	1 	1

	$s = $s . ".param t0s = '0*tick+2*rise'\n";
	$s = $s . ".param t0e = '1*tick-rise'\n";
	$s = $s . ".param t1s = '1*tick+2*rise'\n";
	$s = $s . ".param t1e = '2*tick-rise'\n";
	$s = $s . ".param t2s = '2*tick+2*rise'\n";
	$s = $s . ".param t2e = '3*tick-rise'\n";
	$s = $s . ".param t3s = '3*tick+2*rise'\n";
	$s = $s . ".param t3e = '4*tick-rise'\n";
	$s = $s . ".param t4s = '4*tick+2*rise'\n";
	$s = $s . ".param t4e = '5*tick-rise'\n";
	$s = $s . ".param t5s = '5*tick+2*rise'\n";
	$s = $s . ".param t5e = '6*tick-rise'\n";

	if ( $type eq "nmos" ) {
		$s = $s . "Vgate gate 0 PWL(	0 0 	'4*tick' 0 '4*tick+rise' Vol)\n";
		$s = $s
		  . "Vdrain drain 0 PWL(	0 0 	'2*tick' 0 '2*tick+rise' Vol 	'4*tick' Vol '4*tick+rise' 0 	'5*tick' 0 '5*tick+rise' Vol)\n";
		$s = $s
		  . "Vsource source 0 PWL(0 0 	'1*tick' 0 '1*tick+rise' Vol 	'2*tick' Vol '2*tick+rise' 0 	'3*tick' 0 '3*tick+rise' Vol 	'4*tick' Vol '4*tick+rise' 0 	'5*tick' 0 '5*tick+rise' Vol)\n";

		$s = $s
		  . "X1 drain gate source 0 nfetz wsize="
		  . $width
		  . " lsize="
		  . $length . "\n";
	}
	else {
		$s = $s . "Vgate gate 0 PWL(	0 0 	'2*tick' 0 '2*tick+rise' Vol)\n";
		$s = $s
		  . "Vdrain drain 0 PWL(	0 0 	'1*tick' 0 '1*tick+rise' Vol 	'2*tick' Vol '2*tick+rise' 0 	'4*tick' 0 '4*tick+rise' Vol)\n";
		$s = $s
		  . "Vsource source 0 PWL(0 0 	'1*tick' 0 '1*tick+rise' Vol 	'2*tick' Vol '2*tick+rise' 0 	'3*tick' 0 '3*tick+rise' Vol 	'4*tick' Vol '4*tick+rise' 0 	'5*tick' 0 '5*tick+rise' Vol)\n";

		$s = $s
		  . "X1 drain gate source Vdd pfetz wsize="
		  . $width
		  . " lsize="
		  . $length . "\n";
	}

	$s = $s . spice_sim(100);

	#$s = $s . ".print tran cap(gate)\n";
	#$s = $s . ".print tran cap(drain)\n";
	#$s = $s . ".print tran cap(source)\n";

	if ( $type eq "nmos" ) {
		$s = $s . ".measure tran c_g avg cap(gate) FROM = t4s TO = t5e\n";
		$s = $s . ".measure tran c_d1 avg cap(drain) FROM = t2s TO = t3e\n";
		$s = $s . ".measure tran c_d2 avg cap(drain) FROM = t5s TO = t5e\n";
		$s = $s . ".measure tran c_d Param=('(2*c_d1 + c_d2)/3')\n";
		$s = $s . ".measure tran c_s1 avg cap(source) FROM = t1s TO = t1e\n";
		$s = $s . ".measure tran c_s2 avg cap(source) FROM = t3s TO = t3e\n";
		$s = $s . ".measure tran c_s3 avg cap(source) FROM = t5s TO = t5e\n";
		$s = $s . ".measure tran c_s Param=('(c_s1 + c_s2 + c_s3)/3')\n";
	}
	else {
		$s = $s . ".measure tran c_g avg cap(gate) FROM = t2s TO = t5e\n";
		$s = $s . ".measure tran c_d1 avg cap(drain) FROM = t1s TO = t1e\n";
		$s = $s . ".measure tran c_d2 avg cap(drain) FROM = t4s TO = t5e\n";
		$s = $s . ".measure tran c_d Param=('(c_d1 + 2*c_d2)/3')\n";
		$s = $s . ".measure tran c_s1 avg cap(source) FROM = t1s TO = t1e\n";
		$s = $s . ".measure tran c_s2 avg cap(source) FROM = t3s TO = t3e\n";
		$s = $s . ".measure tran c_s3 avg cap(source) FROM = t5s TO = t5e\n";
		$s = $s . ".measure tran c_s Param=('(c_s1 + c_s2 + c_s3)/3')\n";
	}

	$s = $s . spice_end();
	my @results = spice_run( $s, [ "c_g", "c_s", "c_d" ] );

	return @results;
}
