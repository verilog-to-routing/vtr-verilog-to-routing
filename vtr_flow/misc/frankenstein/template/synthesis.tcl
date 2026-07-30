yosys -import
plugin -i wildebeest

# ============================================================================
# frankenstein vtr synthesis template
# ============================================================================
#
# synth_fpga only works with a zeroasic partname so for vtr archs we drive
# the individual passes ourselves. wildebeest is loaded for max_level and
# vtr_arch_rules.
#
# put arch_config.tcl in the arch support dir named by K6D; it is the only
# genuinely per-arch artifact the script needs
# shared arch-independent support lives in the template dir TDIR
#   vtr_ram_whitebox.v vtr_ram_bit_lib.v and whatever abc scripts the
#   config points at
#
# harness fills these tokens before yosys sees the script
#   XXX circuit verilog
#   TTT top module  empty means -auto-top
#   ZZZ output blif
#   K6D arch support dir
#   TDIR this templates parent dir  its templates/ subdir is -tpldir
#   VVV arch xml
#   YYY max_level flag  empty for the vanilla wildebeest leg

# ----------------------------------------------------------------------------
# knobs  generic defaults then per-arch overrides from arch_config.tcl
# ----------------------------------------------------------------------------
set archSupportDir "K6D"
set archXmlPath    "VVV"
set archRulesDir   "."
set templateDir    "TDIR"

set dspMaxWidth    18
set dspMinWidth    2
# half cost so roms prefer hard bram over soft luts
set bramRomCost    0.5
set bramSpCost     128
set bramDpCost     128
set cmpLutWidth    6
set lutCost        "6:1"
# $add/$sub at or below this width stay soft so abc can still optimize
# across them. substituted into the generated add_sub_map.v because the
# map file is the only place that sees $add widths at techmap time
set hardAdderThreshold 3
# long enough to walk an msb-to-lsb carry cascade in one go
set sweepMaxIters  64
# empty means skip the two-pass abc and do one plain -luts pass
set abcOptScript   ""
set abcMapScript   ""
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"

set archConfigFile "$archSupportDir/arch_config.tcl"
if { [file exists $archConfigFile] } {
    source $archConfigFile
}

# ----------------------------------------------------------------------------
# rule files  all derived from the arch xml by vtr_arch_rules at runtime.
# the xml is required  there are no static fallback maps
# ----------------------------------------------------------------------------
if { $archXmlPath eq "" } {
    error "frankenstein synthesis requires an arch xml (VVV)"
}
vtr_arch_rules -xml $archXmlPath -outdir $archRulesDir -tpldir $templateDir/templates -sp-cost $bramSpCost -dp-cost $bramDpCost -hard-adder-threshold $hardAdderThreshold
set bramMapFile      "$archRulesDir/bram_memory_map.txt"
set techBramFile     "$archRulesDir/tech_bram.v"
set hardblockLibFile "$archRulesDir/vtr_hardblock_lib.v"
set multMapFile      "$archRulesDir/mult_map.v"
set mul2dspMapFile   "$archRulesDir/mul2dsp_map.v"
set addSubMapFile    "$archRulesDir/add_sub_map.v"

# ----------------------------------------------------------------------------
# hardblock sweep  murray IV-A
# opt_merge once then walk opt_clean repeatedly because a carry chain only
# frees one bit tip per pass. has to run before setattr keep freezes the
# hardblocks. do not pair this with setundef -undriven  that combo deletes
# live blackbox datapaths.
# ----------------------------------------------------------------------------
proc frankensteinHardblockSweep {{maxIters 64}} {
    opt_merge
    for {set i 0} {$i < $maxIters} {incr i} {
        # becomes a no-op once the cascade is gone so overshooting is fine
        opt_clean
    }
}

# ----------------------------------------------------------------------------
# read
# ----------------------------------------------------------------------------
# the max-width stubs not +/parmys/vtr_primitives.v because -lib freezes the
# parameter defaults and the primitives default to WIDTH=1 which makes
# write_blif emit 1-bit blackboxes. rams are left out of this file for the
# same reason  -lib DATA_WIDTH=1 truncates the real rtl instance widths.
read_verilog -lib $hardblockLibFile

read_verilog -sv -nolatches XXX

# lock the top before the whitebox lands. with -auto-top a whitebox module
# can win and the actual circuit gets purged. the -lib ram stubs above are
# wide enough that rtl instances resolve without truncating their buses.
if { "TTT" ne "" } {
    hierarchy -check -top TTT -purge_lib
} else {
    hierarchy -check -auto-top -purge_lib
}

# no -lib here so the generate loops expand at the real DATA_WIDTH. top is
# already set so a plain hierarchy re-elaborates without -auto-top.
read_verilog -overwrite $templateDir/vtr_ram_whitebox.v
hierarchy -check -purge_lib
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

# keep $mem as $mem so memory_libmap can see them
memory -nomap
flatten

opt -full

techmap -map +/parmys/adff2dff.v
techmap -map +/parmys/adffe2dff.v
techmap -map +/parmys/aldff2dff.v
techmap -map +/parmys/aldffe2dff.v
opt -full

# ----------------------------------------------------------------------------
# coarse synth while arith is still soft  same order as synth_fpga
# ----------------------------------------------------------------------------
opt_expr
opt_clean
wreduce
peepopt
opt_clean
share
techmap -map +/cmp2lut.v -D LUT_WIDTH=$cmpLutWidth
opt_expr
opt_clean
opt -full
pmux2shiftx
opt -full
share
wreduce
opt_clean

# first hierarchy -purge_lib dropped the stubs  rams are already the
# whitebox bit cells so only multiply and adder need restoring
read_verilog -lib $hardblockLibFile

# ----------------------------------------------------------------------------
# bram
# ----------------------------------------------------------------------------
memory_libmap -lib $bramMapFile -logic-cost-rom $bramRomCost

# leftovers become flip-flop arrays
memory_map

techmap -map $techBramFile

# write_blif needs the arch model names not the internal bit-cell names
delete single_port_ram dual_port_ram
chtype -map vtr_sp_ram_bit single_port_ram
chtype -map vtr_dp_ram_bit dual_port_ram
delete vtr_sp_ram_bit vtr_dp_ram_bit
read_verilog -lib $templateDir/vtr_ram_bit_lib.v

# last chance to shrink soft cones before hard mapping freezes the edges
opt -full
share
wreduce
opt_clean

# ----------------------------------------------------------------------------
# dsp  before alumacc turns $mul into $macc
# ----------------------------------------------------------------------------
# mul2dsp chops wide $mul into chunks that fit one dsp block. mult_map.v
# then picks up anything left that still fits an arch multiply mode.
memory_dff
techmap -map +/mul2dsp.v -map $mul2dspMapFile -D DSP_A_MAXWIDTH=$dspMaxWidth -D DSP_B_MAXWIDTH=$dspMaxWidth -D DSP_A_MINWIDTH=$dspMinWidth -D DSP_B_MINWIDTH=$dspMinWidth -D DSP_NAME=_dsp_block_
select a:mul2dsp
setattr -unset mul2dsp
opt_expr
wreduce
select -clear
# tcl only suppresses $ when the word starts with a brace
chtype -set {$mul} {t:$__soft_mul}
techmap -map $multMapFile

# ----------------------------------------------------------------------------
# adder / carry chain
# ----------------------------------------------------------------------------
# before alumacc or the $add chains get folded into $macc and the carry
# chain never sees them. compares stay soft on purpose.
techmap -map $addSubMapFile

alumacc

# no $alu to adder map  comparing through a hard adder is a qor loss
frankensteinHardblockSweep $sweepMaxIters

# ----------------------------------------------------------------------------
# clock-domain cut points
# YYY is -vtr_arch <xml> on the frankenstein leg and empty on vanilla
# ----------------------------------------------------------------------------
max_level -clk2clk YYY

# ----------------------------------------------------------------------------
# lower to gates for abc
# ----------------------------------------------------------------------------
# -noff on every opt past this point  opt_dff cannot see through the
# multiply and adder blackboxes and would otherwise leave them alone while
# still chewing the soft cones around them in weird ways
demuxmap
simplemap
techmap
opt -fast -noff
dffunmap
opt -fast -noff
opt -noff
dffunmap
opt -fast -noff

insbuf
opt -purge -noff

# ----------------------------------------------------------------------------
# abc
# ----------------------------------------------------------------------------
# dress leaves PI and PO as pi* and po* which yosys then cannot wire up so
# the gia opt script ends with move_names instead. two passes when the
# scripts are set otherwise one plain -luts pass.
if { $abcOptScript ne "" && $abcMapScript ne "" } {
    abc -script $abcOptScript
    abc -luts $lutCost -script $abcMapScript
} else {
    abc -luts $lutCost
}

# abc will not delete blackboxes so sweep anything it left unused
frankensteinHardblockSweep $sweepMaxIters

# ----------------------------------------------------------------------------
# densify then write_blif
# ----------------------------------------------------------------------------
# an undriven $lut input becomes an unconn pin in the blif. vpr deletes that
# net and the remaining sparse pin indices crash the delay calculator.
# setundef -zero turns them into constants so opt_lut_ins can drop and
# renumber.
#
# setattr keep first otherwise opt_clean treats blackbox outputs as
# undriven and deletes live datapaths.
setattr -set keep 1 {*}$keepCellTypes
setundef -zero -undriven
opt_lut_ins
opt_lut
opt_clean

stat
if { "TTT" ne "" } {
    hierarchy -check -top TTT -purge_lib
} else {
    hierarchy -check -auto-top -purge_lib
}

# hierarchy -purge_lib can leave fresh undriven lut inputs so densify again
setattr -set keep 1 {*}$keepCellTypes
setundef -zero -undriven
opt_lut_ins
opt_clean

# the minus form  with a plus yosys emits a second .names gnd and vpr dies
# on inconsistent block data sizes
write_blif -true + vcc -false + gnd -undef - gnd -blackbox ZZZ
