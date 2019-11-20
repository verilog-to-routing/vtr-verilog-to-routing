* Level Restorer

.subckt levr in out Vdd Gnd

X0 in out Vdd Gnd inv nsize=2 psize=1
X11 in out Vdd Vdd pfetz wsize=1 lsize=2

.ends