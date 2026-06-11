# Create a master clock with a non-zero rising edge (rise=1.5 ns, fall=4.5 ns).
create_clock -period 6.0 -waveform {1.5 4.5} -name master_clk [get_pins {div_clk.clk[0]}]

# Create a generated clock using -edges {1 3 5}, equivalent to -divide_by 2 for this master.
# Edge 1 (odd, cycle 0) -> rise at 1.5 ns
# Edge 3 (odd, cycle 1) -> rise at 1.5+6=7.5 ns  (fall of generated clock)
# Edge 5 (odd, cycle 2) -> rise at 1.5+12=13.5 ns (next rise, so period = 12.0 ns)
# Generated clock: rise=1.5 ns, fall=7.5 ns, period=12 ns.
# This should produce the same result as clock_divider_nonzero_rise (-divide_by 2, same master).
create_generated_clock -source [get_pins {div_clk.clk[0]}] -edges {1 3 5} [get_pins {div_clk.Q[0]}]
