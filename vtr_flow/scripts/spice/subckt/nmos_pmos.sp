* NMOS/PMOS

* Diffusion sizes:
* Wdiff = size * 2.5 * tech
* Ldiff = 2.25 * tech

.subckt nfet drain gate source body size=1 
	M1 drain gate source body nmos L='tech' W='2.5*size*tech' AS='size*2.5*tech*2.25*tech' AD='size*2.5*tech*2.25*tech' PS='2*2.25*tech+size*2.5*tech'  PD='2*2.25*tech+size*2.5*tech'	
.ends

.subckt pfet drain gate source body size=1 
	M1 drain gate source body pmos L='tech' W='2.5*size*tech' AS='size*2.5*tech*2.25*tech' AD='size*2.5*tech*2.25*tech' PS='2*2.25*tech+size*2.5*tech'  PD='2*2.25*tech+size*2.5*tech'		
.ends

.subckt pfetz drain gate source body wsize=1 lsize=1
	M1 drain gate source body pmos L='lsize*tech' W='2.5*wsize*tech' AS='wsize*2.5*tech*2.25*lsize*tech' AD='wsize*2.5*tech*2.25*lsize*tech' PS='2*2.25*lsize*tech+wsize*2.5*tech'  PD='2*2.25*lsize*tech+wsize*2.5*tech'	
.ends

.subckt nfetz drain gate source body wsize=1 lsize=1
	M1 drain gate source body nmos L='lsize*tech' W='2.5*wsize*tech' AS='wsize*2.5*tech*2.25*lsize*tech' AD='wsize*2.5*tech*2.25*lsize*tech' PS='2*2.25*lsize*tech+wsize*2.5*tech'  PD='2*2.25*lsize*tech+wsize*2.5*tech'	
.ends