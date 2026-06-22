# Create a master clock with a non-zero rise edge (rise at 6 ns, fall at 11 ns).
create_clock -period 10.0 -waveform {6 11} -name master_clk [get_ports clk]

# Create a generated clock that is double the frequency of master_clk.
# Generated clock period = 10.0 / 2 = 5.0 ns.
# The rise edge is inherited from the source: 6 ns, which is outside the
# generated clock's period of 5 ns. This exercises the modulo normalization
# of the rise edge in the setup/hold constraint calculation.
create_generated_clock -source [get_ports clk] -multiply_by 2 [get_ports clk2]
