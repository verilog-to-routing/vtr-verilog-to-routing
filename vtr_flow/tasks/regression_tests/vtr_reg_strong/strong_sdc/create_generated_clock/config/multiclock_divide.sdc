# Create a master clock to generate the clocks relative to.
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a clock that is half the frequency of 'master_clk'
create_generated_clock -source [get_ports clk] -divide_by 2 [get_ports clk2]
