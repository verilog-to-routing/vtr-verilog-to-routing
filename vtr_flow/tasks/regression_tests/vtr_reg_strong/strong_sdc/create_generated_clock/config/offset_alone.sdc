# Master clock: 10 ns period, rise at 0 ns, fall at 5 ns.
create_clock -period 10.0 -name master_clk [get_ports {clk}]

# Generated clock: same period (10 ns), fixed time offset of 2 ns.
# Rise at 2 ns, fall at 7 ns, period 10 ns.
# Period and fall edge are both inherited from the master, then shifted.
create_generated_clock -source [get_ports {clk}] -offset 2.0 [get_ports {clk2}]
