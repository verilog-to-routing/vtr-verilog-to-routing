# Create a master clock on the clk port.
create_clock -period 6.0 -name master_clk [get_ports clk]

# Create a generated clock that is triple the frequency of master_clk with a
# 30% duty cycle. -duty_cycle is only valid with -multiply_by.
# Generated clock period = 6.0 / 3 = 2.0 ns.
# With -duty_cycle 30: rise at 0 ns, fall at 0.6 ns.
create_generated_clock -source [get_ports clk] -multiply_by 3 -duty_cycle 30 [get_ports clk2]
