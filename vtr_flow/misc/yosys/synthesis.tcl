yosys -import

plugin -i parmys
yosys -import

read_verilog -nomem2reg +/parmys/vtr_primitives.v
setattr -mod -set keep_hierarchy 1 single_port_ram
setattr -mod -set keep_hierarchy 1 dual_port_ram

# synlig path error handling
if {[catch {set synlig $::env(synlig_exe_path)} err]} {
	puts "Error: $err"
	puts "synlig_exe_path is not set"
} else {
	set synlig $::env(synlig_exe_path)
	puts "Using parmys as partial mapper"
}


# arch file: QQQ
# input files: [XXX]
# other args: [YYY]
# config file: CCC
# output file: ZZZ

parmys_arch -a QQQ

if {$env(PARSER) == "surelog" } {
	puts "Using Synlig read_uhdm command"
	
	exec $synlig -p "read_uhdm XXX"
	
} elseif {$env(PARSER) == "system-verilog" } {
	puts "Using Synlig read_systemverilog "
	exec $synlig -p "read_systemverilog XXX"
	
} elseif {$env(PARSER) == "default" } {
	puts "Using Yosys read_verilog command"
	read_verilog -sv -nolatches XXX
} else {
	error "Invalid PARSER"
}

read_verilog -lib /home/soheil/vpr_repos/vtr_flow/benchmarks/verilog/kmeans/noc_router.v

# Declare the noc_router_module as a black box
setattr -mod -set blackbox 1 noc_router_module

# Check that there are no combinational loops
scc -select
select -assert-none %
select -clear


hierarchy -check -auto-top

opt_expr
#opt_clean
check
opt -nodffe -nosdff
procs -norom
fsm
opt
wreduce
peepopt
#opt_clean
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

# Separate options for Parmys execution (Verilog or SystemVerilog)
if {$env(PARSER) == "default"} {
    # For Verilog, use -nopass for a simpler, faster flow
    parmys -a QQQ -nopass -c CCC YYY
} elseif {$env(PARSER) == "system-verilog" || $env(PARSER) == "surelog"} {
    # For Synlig SystemVerilog, run additional passes to handle complexity
    parmys -a QQQ -c CCC YYY
}

opt -full

techmap 
opt -fast

dffunmap
opt -fast -noff
#autoname

stat

hierarchy -check -auto-top -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ