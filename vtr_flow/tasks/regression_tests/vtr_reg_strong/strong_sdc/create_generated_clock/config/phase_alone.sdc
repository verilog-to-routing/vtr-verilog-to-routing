# Master clock: 12 ns period, rise at 0 ns, fall at 6 ns.
create_clock -period 12.0 -name master_clk [get_ports {clk}]

# Generated clock: same period (12 ns), phase-shifted by 90 degrees.
# 90 degrees of 12 ns = 3 ns shift -> rise at 3 ns, fall at 9 ns, period 12 ns.
# Period and fall edge are both inherited from the master, then shifted.
create_generated_clock -source [get_ports {clk}] -phase 90 [get_ports {clk2}]
