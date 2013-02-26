* Simple Inverter

.subckt nfet drain gate source body size=1 

M1 drain gate source body nmos \
	L='tech' W='size*tech' AS='size*tech*2.5*tech' PS='2*2.5*tech+size*tech' AD='size*tech*2.5*tech' PD='2*2.5*tech+size*tech'
	
.ends

.subckt pfet drain gate source body size=1 

M1 drain gate source body pmos \
	L='tech' W='size*tech' AS='size*tech*2.5*tech' PS='2*2.5*tech+size*tech' AD='size*tech*2.5*tech' PD='2*2.5*tech+size*tech'
	
.ends

.subckt pfetz drain gate source body wsize=1 lsize=1

M1 drain gate source body pmos \
	L='lsize*tech' W='wsize*tech' AS='wsize*tech*2.5*lsize*tech' PS='2*2.5*lsize*tech+wsize*tech' AD='wsize*tech*2.5*lsize*tech' PD='2*2.5*lsize*tech+wsize*tech'
	
.ends