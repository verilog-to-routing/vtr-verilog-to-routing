# Two physically-exclusive clocks on the same source port, using create_clock -add.
# Both clk_a and clk_b originate at port clk, so the generated clock must use
# -master_clock to select which one it is derived from.
create_clock -period 5.0 -name clk_a [get_ports {clk}]
create_clock -period 10.0 -name clk_b -add [get_ports {clk}]
set_clock_groups -physically_exclusive -group {clk_a} -group {clk_b}

# Generated clock on clk2 derived from clk_a only.
# Without -master_clock, VPR would reject this because the source pin hosts
# multiple clock domains.
create_generated_clock -source [get_ports {clk}] -master_clock clk_a -divide_by 2 [get_ports {clk2}]
