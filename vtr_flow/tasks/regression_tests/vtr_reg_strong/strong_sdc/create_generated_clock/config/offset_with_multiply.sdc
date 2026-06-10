# Master clock: 6 ns period, rise at 0 ns, fall at 3 ns.
create_clock -period 6.0 -name master_clk [get_ports {clk}]

# Generated clock: multiply by 2 (period 3 ns), then fixed time offset of 1 ns.
# Base waveform: rise at 0 ns, fall at 1.5 ns (50% duty cycle of 3 ns).
# After offset: rise at 1 ns, fall at 2.5 ns, period 3 ns.
create_generated_clock -source [get_ports {clk}] -multiply_by 2 -offset 1.0 [get_ports {clk2}]
