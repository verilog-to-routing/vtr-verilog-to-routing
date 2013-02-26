* 3 input mux

*.subckt nfet drain gate source size=1 

.subckt mux3 in0 in1 in2 out sel0 sel1 sel2 size=1 

X0 in0 sel0 out 0 nfet size='size'
X1 in1 sel1 out 0 nfet size='size'
X2 in2 sel2 out 0 nfet size='size'
	
.ends
