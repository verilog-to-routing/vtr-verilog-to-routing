yosys -import
plugin -i parmys
yosys -import

# The slang SystemVerilog frontend (read_slang) is built into yosys when it
# is configured with -DSLANG_SYSTEMVERILOG=ON (the default).
if {$env(PARSER) == "slang" } {
	if {[llength [info commands read_slang]] == 0} {
		error "Error: this yosys was built without the slang frontend. Run make with CMake param -DSLANG_SYSTEMVERILOG=ON."
	}
	puts "Using Yosys' built-in slang frontend"
} elseif {$env(PARSER) == "default" } {
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

if {$env(PARSER) == "slang" } {
	# Create a file list containing the name(s) of file(s) \
	# to read together with read_slang
	source [file join [pwd] "slang_filelist.tcl"]
	set readfile [file join [pwd] "filelist.txt"]
	#Writing names of circuit files to file list
	build_filelist {XXX} $readfile
	puts "Using Yosys read_slang command"
	#Read vtr_primitives library and user design verilog in same command
	read_slang -v $env(PRIMITIVES) -C $readfile
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

hierarchy -check -auto-top -purge_lib

write_blif -true + vcc -false + gnd -undef + unconn -blackbox ZZZ
