* Simple Inverter

.subckt inv in out Vdd Gnd nsize=1 psize=2

X0 out in Gnd Gnd nfet size='nsize'
X1 out in Vdd Vdd pfet size='psize'

.ends

.subckt invd inp inn out Vdd Gnd nsize=1 psize=2

X0 out inn Gnd Gnd nfet size='nsize'
X1 out inp Vdd Vdd pfet size='psize'

.ends