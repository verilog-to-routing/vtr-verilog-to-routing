# Create a master clock to generate the clocks relative to.
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock at triple the frequency of 'master_clk', inverted.
# Generated period = 6.0 / 3 = 2.0 ns.
# -invert shifts the rising edge by half the generated period (1.0 ns), so the
# generated clock rises at 1.0 ns while the source rises at 0.0 ns. This
# offset allows setup time to be met on cross-domain paths.
create_generated_clock -source [get_ports clk] -multiply_by 3 -invert [get_ports clk2]
