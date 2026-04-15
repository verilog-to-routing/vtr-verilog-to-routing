# Create two named clocks to the target pins.
# This should create one clock that fans-out to FFA and FFC,
# and another clock that fans-out to FFD and FFB.
create_clock -period 10.0 -name clk {FFA.clk[0] FFC.clk[0]}
create_clock -period 10.0 -name clk2 {FFD.clk[0] FFB.clk[0]}