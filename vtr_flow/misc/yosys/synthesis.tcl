yosys -import
plugin -i parmys

# yosys-slang plugin error handling
if {$env(PARSER) == "slang" } {
	if {![info exists ::env(yosys_slang_path)]} {
		puts "Error: $err"
		puts "yosys_slang_path is not set"
	} elseif {![file exists $::env(yosys_slang_path)]} {
		error "Error: cannot find plugin at '$::env(yosys_slang_path)'. Run make with CMake param -DSLANG_SYSTEMVERILOG=ON to enable yosys-slang plugin."
	} else {
		plugin -i slang
		yosys -import
		puts "Using yosys-slang as yosys frontend"
	}
} elseif {$env(PARSER) == "default" } {
	yosys -import
	puts "Using Yosys read_verilog as yosys frontend"
} else {
	error "Invalid PARSER"
}

# arch file: QQQ
# input files: [XXX]
# other args: [YYY]
# config file: CCC
# output file: ZZZ

parmys_arch -a QQQ

variable HDL_tops

# Create a file list containing the name(s) of file(s) \
# to read together with read_slang
source [file join [pwd] "slang_filelist.tcl"]
set readfile [file join [pwd] "filelist.txt"]
#Writing names of circuit files to file list
set HDL_tops [::slang::build_filelist {XXX} $readfile $env(TOPS)]
lassign $HDL_tops top_mods_slang top_mods_verilog

if {$env(PARSER) == "slang" } {
	puts "Using Yosys read_slang command"
	#Read vtr_primitives library and user design verilog in same command
	read_slang -v $env(PRIMITIVES) {*}$top_mods_slang -C $readfile
} elseif {$env(PARSER) == "default" } {
	puts "Using Yosys read_verilog command"
	read_verilog -nomem2reg +/parmys/vtr_primitives.v
	setattr -mod -set keep_hierarchy 1 single_port_ram
 	setattr -mod -set keep_hierarchy 1 dual_port_ram
	read_verilog -sv -nolatches XXX
} else {
	error "Invalid PARSER"
}

# Check that there are no combinational loops
scc -select
select -assert-none %
select -clear

#hierarchy -check -auto-top -purge_lib
hierarchy -check {*}$top_mods_verilog -purge_lib

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

# Separate options for Parmys execution (Verilog or SystemVerilog)
if {$env(PARSER) == "default" || $env(PARSER) == "slang"} {
    # For Verilog, use -nopass for a simpler, faster flow
    parmys -a QQQ -nopass -c CCC YYY
} 

opt -full

techmap 
opt -fast

dffunmap
opt -fast -noff
#autoname

stat

#hierarchy -check -auto-top -purge_lib
hierarchy -check {*}$top_mods_verilog -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ
