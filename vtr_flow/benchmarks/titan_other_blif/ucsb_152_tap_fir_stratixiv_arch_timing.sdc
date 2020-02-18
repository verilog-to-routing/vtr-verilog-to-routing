#VPR compatible SDC file for benchmark circuit 'ucsb_152_tap_fir'

# Creates an external virtual clock 'virtual_io_clock', and non-virtual clocks for each netlist clock (each with 1ns target clock period).
# Paths between netlist clock domains are not analyzed, but paths to/from the 'virtual_io_clock' and netlist clocks are analyzed.
# Small clocks which drive less than 0.1% of blocks are ignored and not created

#**************************************************************
# Unit Information
#**************************************************************
#VPR assumes time unit is nanoseconds

#**************************************************************
# Create Clock
#**************************************************************
create_clock -period 1.000 -name virtual_io_clock
create_clock -period 1.000 {clk}

#**************************************************************
# Create Generated Clock
#**************************************************************
#None

#**************************************************************
# Set Clock Latency
#**************************************************************
set_clock_latency -source 0.000 [get_clocks {clk}]

#**************************************************************
# Set Clock Uncertainty
#**************************************************************
set_clock_uncertainty -from [get_clocks {clk}] -to [get_clocks {clk}] 0.000
set_clock_uncertainty -from [get_clocks {clk}] -to [get_clocks {virtual_io_clock}] 0.000
set_clock_uncertainty -from [get_clocks {virtual_io_clock}] -to [get_clocks {clk}] 0.000
set_clock_uncertainty -from [get_clocks {virtual_io_clock}] -to [get_clocks {virtual_io_clock}] 0.000

#**************************************************************
# Set Input Delay
#**************************************************************
set_input_delay -clock virtual_io_clock 0.0 [get_ports *]

#**************************************************************
# Set Output Delay
#**************************************************************
set_output_delay -clock virtual_io_clock 0.0 [get_ports *]

#**************************************************************
# Set Clock Groups
#**************************************************************
#None

#**************************************************************
# Set False Path
#**************************************************************
#None

#**************************************************************
# Set Multicycle Path
#**************************************************************
#None

#**************************************************************
# Set Maximum Delay
#**************************************************************
#None

#**************************************************************
# Set Minimum Delay
#**************************************************************
#None

#EOF
