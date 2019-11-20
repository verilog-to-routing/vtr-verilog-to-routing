* 2-Mux w/ Transmission Gates

.subckt mux2trans in0 in1 out clk clkn Vdd Gnd size=1 pnratio=2

X0 in0 clkn out Gnd nfet size='size'
X1 out clk in0 Vdd pfet size='size*pnratio'

X2 in1 clk out Gnd nfet size='size'
X3 out clkn in1 Vdd pfet size='size*pnratio'

.ends