# Create a master clock on the D flip-flop's clock pin.
create_clock -period 6.0 -name master_clk [get_pins {div_clk.clk[0]}]

# Create a generated clock that is half the frequency of master_clk and inverted.
# The -invert flag swaps the rising and falling edges of the generated clock,
# so it is high during the second half of its period instead of the first.
create_generated_clock -source [get_pins {div_clk.clk[0]}] -divide_by 2 -invert [get_pins {div_clk.Q[0]}]
