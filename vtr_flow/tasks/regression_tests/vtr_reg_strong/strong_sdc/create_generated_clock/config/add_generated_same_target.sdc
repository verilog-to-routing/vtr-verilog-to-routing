# Two physically-exclusive clocks on the internal divider latch's clock pin.
# Using create_clock -add to place both clk_a and clk_b on the same source pin.
create_clock -period 5.0 -name clk_a [get_pins {div_clk.clk[0]}]
create_clock -period 10.0 -name clk_b -add [get_pins {div_clk.clk[0]}]
set_clock_groups -physically_exclusive -group {clk_a} -group {clk_b}

# Two generated clocks on the same target pin (div_clk.Q[0]), one per master.
# The first create_generated_clock uses -master_clock to select clk_a.
# The second uses -add to add a second generated clock to the same target pin,
# with -master_clock selecting clk_b.
create_generated_clock -source [get_pins {div_clk.clk[0]}] -master_clock clk_a -divide_by 2 -name gen_clk_a [get_pins {div_clk.Q[0]}]
create_generated_clock -source [get_pins {div_clk.clk[0]}] -master_clock clk_b -divide_by 2 -name gen_clk_b -add [get_pins {div_clk.Q[0]}]
set_clock_groups -physically_exclusive -group {gen_clk_a} -group {gen_clk_b}
