* DFF

*.subckt mux2trans in0 in1 out clk clkn Vdd Gnd size=1 pnratio=2
*.subckt inv in out Vdd Gnd nsize=1 psize=2


.subckt dff D Q clk clkn Vdd Gnd pnratio=2 size=1

X0 D mux1in mux1out clk clkn Vdd Gnd mux2trans size='size' pnratio='pnratio'
X1 mux1out mux2in Vdd Gnd inv nsize=1 psize='pnratio'
X2 mux2in mux1in Vdd Gnd inv nsize=1 psize='pnratio'

X3 Qn mux2in mux2out clk clkn Vdd Gnd mux2trans size='size' pnratio='pnratio'
X4 mux2out Q Vdd Gnd inv nsize=1 psize='pnratio'
X5 Q Qn Vdd Gnd inv nsize=1 psize='pnratio'

.ends