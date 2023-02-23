yosys -import

plugin -i parmys

yosys -import

read_verilog -nomem2reg +/parmys/vtr_primitives.v

setattr -mod -set keep_hierarchy 1 single_port_ram

setattr -mod -set keep_hierarchy 1 dual_port_ram

puts "Using parmys as partial mapper"

parmys_arch -a k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml

read_verilog -sv -nolatches hard_block_include.v $::env(DESIGN_TOP).v


# Check that there are no combinational loops

scc -select

select -assert-none %

select -clear

hierarchy -check -auto-top -purge_lib

opt_expr

opt_clean

check

opt -nodffe -nosdff

procs -norom

fsm

opt

wreduce

peepopt

opt_clean

share

opt -full

memory -nomap

flatten

opt -full

techmap -map +/parmys/adff2dff.v

techmap -map +/parmys/adffe2dff.v

techmap -map +/parmys/aldff2dff.v

techmap -map +/parmys/aldffe2dff.v

opt -full

parmys -a k6FracN10LB_mem20K_complexDSP_customSB_22nm.xml -nopass -c odin_config.xml

opt -full

techmap 

opt -fast

dffunmap

opt -fast -noff

tee -o /dev/stdout stat

hierarchy -check -auto-top -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox $::env(DESIGN_TOP).yosys.blif

