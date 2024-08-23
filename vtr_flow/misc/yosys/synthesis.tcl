yosys -import

plugin -i parmys
yosys -import

read_verilog -nomem2reg +/parmys/vtr_primitives.v
setattr -mod -set keep_hierarchy 1 single_port_ram
setattr -mod -set keep_hierarchy 1 dual_port_ram

puts "Using parmys as partial mapper"

# arch file: QQQ
# input files: [XXX]
# other args: [YYY]
# config file: CCC
# output file: ZZZ

parmys_arch -a QQQ

if {$env(PARSER) == "surelog" } {
	puts "Using Yosys read_uhdm command"
	plugin -i systemverilog
	yosys -import
	read_uhdm XXX
} elseif {$env(PARSER) == "system-verilog" } {
	puts "Using Yosys read_systemverilog command"
	plugin -i systemverilog
	yosys -import
	read_systemverilog -I"SEARCHPATH" -debug XXX
} elseif {$env(PARSER) == "default" } {
	puts "Using Yosys read_verilog command"
	read_verilog -I"SEARCHPATH" -sv -nolatches XXX
} else {
	error "Invalid PARSER"
}

# Check that there are no combinational loops
scc -select
select -assert-none %
select -clear

hierarchy -check TOPMODULE -purge_lib

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

parmys -a QQQ -nopass -c CCC YYY

opt -full

techmap 
opt -fast

dffunmap
opt -fast -noff

#autoname

stat

hierarchy -check TOPMODULE -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ
