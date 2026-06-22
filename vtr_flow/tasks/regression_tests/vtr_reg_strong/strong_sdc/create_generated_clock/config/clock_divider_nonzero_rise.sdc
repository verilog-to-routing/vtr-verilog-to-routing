# Create a master clock with a non-zero rising edge (rise at 1.5 ns, fall at 4.5 ns).
create_clock -period 6.0 -waveform {1.5 4.5} -name master_clk [get_pins {div_clk.clk[0]}]

# Create a generated clock that is half the frequency of master_clk.
# The generated clock inherits the shifted waveform: rise at 1.5 ns, fall at 7.5 ns,
# over a period of 12.0 ns.
create_generated_clock -source [get_pins {div_clk.clk[0]}] -divide_by 2 [get_pins {div_clk.Q[0]}]
