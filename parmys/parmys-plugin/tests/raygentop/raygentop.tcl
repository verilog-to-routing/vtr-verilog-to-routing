yosys -import

plugin -i parmys

yosys -import

read_verilog -nomem2reg +/parmys/vtr_primitives.v

setattr -mod -set keep_hierarchy 1 single_port_ram

setattr -mod -set keep_hierarchy 1 dual_port_ram


puts "Using parmys as partial mapper"


parmys_arch -a k6_frac_N10_frac_chain_mem32K_40nm.xml


read_verilog -sv -nolatches $::env(DESIGN_TOP).v


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

parmys -a k6_frac_N10_frac_chain_mem32K_40nm.xml -nopass -c odin_config.xml -viz

opt -full

techmap 

opt -fast

dffunmap

opt -fast -noff


tee -o /dev/stdout stat

hierarchy -check -auto-top -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox $::env(DESIGN_TOP).yosys.blif

