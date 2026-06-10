# Master clock: 6 ns period, rise at 0 ns, fall at 3 ns.
create_clock -period 6.0 -name master_clk [get_ports {clk}]

# Generated clock: divide by 2 (period 12 ns), then phase-shifted by 180 degrees.
# Phase shift is 180 / 360 * 6 ns (source period) = 3 ns.
# Base waveform: rise at 0 ns, fall at 6 ns (50% duty cycle of 12 ns).
# After shift: rise at 3 ns, fall at 9 ns, period 12 ns.
create_generated_clock -source [get_ports {clk}] -divide_by 2 -phase 180 [get_ports {clk2}]
