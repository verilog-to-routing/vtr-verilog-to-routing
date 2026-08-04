yosys -import
plugin -i wildebeest

# ============================================================================
# mosaic vtr synthesis template
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

set dspMaxWidth    0
set dspMinWidth    2
# half cost so zero/undef roms prefer hard bram over soft luts.
# non-zero init is refused by bram_memory_map (init zero) and soft-maps.
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
# when 1, vtr_arch_rules emits generic stubs for every exotic hardblock model
# and writes hardblock_keep_types.txt so rtl-instantiated cells survive synth
set stubAllHardblocks 0

set archConfigFile ""
if { $archSupportDir ne "" } {
    set archConfigFile "$archSupportDir/arch_config.tcl"
}
if { $archConfigFile ne "" && [file exists $archConfigFile] } {
    source $archConfigFile
}

# ----------------------------------------------------------------------------
# rule files  all derived from the arch xml by vtr_arch_rules at runtime.
# the xml is required  there are no static fallback maps
# ----------------------------------------------------------------------------
if { $archXmlPath eq "" } {
    error "mosaic synthesis requires an arch xml (VVV)"
}
if { $stubAllHardblocks } {
    vtr_arch_rules -xml $archXmlPath -outdir $archRulesDir -tpldir $templateDir/templates -sp-cost $bramSpCost -dp-cost $bramDpCost -hard-adder-threshold $hardAdderThreshold -stub-all-hardblocks
} else {
    vtr_arch_rules -xml $archXmlPath -outdir $archRulesDir -tpldir $templateDir/templates -sp-cost $bramSpCost -dp-cost $bramDpCost -hard-adder-threshold $hardAdderThreshold
}
set bramMapFile      "$archRulesDir/bram_memory_map.txt"
set techBramFile     "$archRulesDir/tech_bram.v"
set hardblockLibFile "$archRulesDir/vtr_hardblock_lib.v"
set multMapFile      "$archRulesDir/mult_map.v"
set mul2dspMapFile   "$archRulesDir/mul2dsp_map.v"
set addSubMapFile    "$archRulesDir/add_sub_map.v"

# arch-derived facts from the xml (dsp widths, ram abits, hardblock presence).
# defaults below are overwritten when arch_facts.tcl exists.
set archName         ""
set vtrRamAbits      0
set multiplyPresent  0
set adderPresent     0
set multiplyModes    ""
set archFactsFile "$archRulesDir/arch_facts.tcl"
if { [file exists $archFactsFile] } {
    source $archFactsFile
}

# exotic keep types from stub-all (opt-in via arch_config); append so the
# arch_config keepCellTypes builtins stay first
set keepTypesFile "$archRulesDir/hardblock_keep_types.txt"
if { [file exists $keepTypesFile] } {
    set keepTypesFd [open $keepTypesFile r]
    set keepTypesExtra [string trim [read $keepTypesFd]]
    close $keepTypesFd
    if { $keepTypesExtra ne "" } {
        append keepCellTypes " $keepTypesExtra"
    }
}

# ----------------------------------------------------------------------------
# hardblock sweep / keep / densify helpers
# ----------------------------------------------------------------------------
# ordering rules:
#   1. mosaicHardblockSweep only while keep is unset so unused cascade
#      tips can still die (murray IV-A).
#   2. never call setundef inside the sweep. setundef -undriven + opt_clean
#      deletes live multiply/adder/ram outputs that look undriven.
#   3. mosaicKeepHardblocks must run before any densify setundef/opt_clean
#      and again after hierarchy -purge_lib which can drop attributes.
# ----------------------------------------------------------------------------
proc mosaicHardblockSweep {{maxIters 64}} {
    # keep must stay unset here; only opt_merge/opt_clean so unused tips fall
    opt_merge
    for {set i 0} {$i < $maxIters} {incr i} {
        # becomes a no-op once the cascade is gone so overshooting is fine
        opt_clean
    }
}

proc mosaicKeepHardblocks {} {
    global keepCellTypes
    # freeze live hardblocks so later setundef/opt_clean cannot drop them
    setattr -set keep 1 {*}$keepCellTypes
}

proc mosaicDensifyLutInputs {{withOptLut 0}} {
    # vpr crashes when write_blif emits sparse unconn $lut pins. only zero
    # undriven nets that feed $lut inputs; do not blanket-zero every undriven
    # net (that hides rtl bugs and can disturb hardblock edges).
    mosaicKeepHardblocks
    select {t:$lut} %ci
    setundef -zero -undriven
    select -clear
    opt_lut_ins
    if {$withOptLut} {
        opt_lut
    }
    # keep is still set so opt_clean will not delete live hardblock outputs
    opt_clean
}

proc mosaicWarnAsyncFf {} {
    # k6/vpr has no async ff primitive; adff2dff is required but changes timing
    set asyncFfDump "mosaic_async_ff.select"
    tee -q -o $asyncFfDump select -list {t:$adff} {t:$adffe} {t:$aldff} {t:$aldffe}
    set asyncFfPresent 0
    if {[file exists $asyncFfDump]} {
        set asyncFfFd [open $asyncFfDump r]
        set asyncFfText [read $asyncFfFd]
        close $asyncFfFd
        file delete -force $asyncFfDump
        foreach asyncFfLine [split $asyncFfText "\n"] {
            if {[string trim $asyncFfLine] ne ""} {
                set asyncFfPresent 1
                break
            }
        }
    }
    select -clear
    if {$asyncFfPresent} {
        log -warning "mosaic: \$adff/\$adffe/\$aldff/\$aldffe present; k6/vpr has no async ff so adff2dff maps them to sync dff (async reset becomes clocked)."
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

# omit -nolatches so inferred latches stay latches; forcing them away can
# diverge from rtl that relies on latch inference. write_blif and
# fix_blif_for_vpr already handle .latch emission for vpr/abc.
read_verilog -sv XXX

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
read_verilog -overwrite -D VTR_RAM_ABITS=$vtrRamAbits $templateDir/vtr_ram_whitebox.v
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

# async assert becomes clocked; required because k6/vpr has no async ff
mosaicWarnAsyncFf
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
# memory_libmap uses init zero / rdwr old from bram_memory_map.txt.
# non-zero-initialized memories are left as $mem here then soft-mapped.
memory_libmap -lib $bramMapFile -logic-cost-rom $bramRomCost

# leftovers (incl. non-zero init / unfit sizes) become flip-flop arrays
memory_map

techmap -map $techBramFile

# write_blif needs the arch model names not the internal bit-cell names
delete single_port_ram dual_port_ram
chtype -map vtr_sp_ram_bit single_port_ram
chtype -map vtr_dp_ram_bit dual_port_ram
delete vtr_sp_ram_bit vtr_dp_ram_bit
read_verilog -lib -D VTR_RAM_ABITS=$vtrRamAbits $templateDir/vtr_ram_bit_lib.v

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
# sweep while keep is unset; do not densify/setundef until after final sweep
mosaicHardblockSweep $sweepMaxIters

# ----------------------------------------------------------------------------
# clock-domain cut points
# YYY is -vtr_arch <xml> on the mosaic leg and empty on vanilla
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

# snapshot the pre-abc per-type counts (and io pins via -top) for the
# compare s/a split. the trailing file arg writes stat there instead of
# needing a tee pass.
if { "TTT" ne "" } {
    stat -top TTT mosaic_synth.stat
} else {
    stat mosaic_synth.stat
}

# ----------------------------------------------------------------------------
# abc  runs in-yosys now that vtr's yosys is built with the in-yosys abc pass
# (ENABLE_ABC=1). dress leaves PI and PO as pi* and po* which yosys then
# cannot wire up so the gia opt script ends with move_names instead. two
# passes when the scripts are set otherwise one plain -luts pass.
# ----------------------------------------------------------------------------
if { $abcOptScript ne "" && $abcMapScript ne "" } {
    abc -script $abcOptScript
    abc -luts $lutCost -script $abcMapScript
} else {
    abc -luts $lutCost
}

# abc will not delete blackboxes so sweep anything it left unused.
# last sweep before keep; densify below must setattr keep first.
mosaicHardblockSweep $sweepMaxIters

# snapshot the post-abc per-type counts
if { "TTT" ne "" } {
    stat -top TTT mosaic_abc.stat
} else {
    stat mosaic_abc.stat
}

# ----------------------------------------------------------------------------
# densify then write_blif
# ----------------------------------------------------------------------------
# scoped densify: keep hardblocks, zero only undriven $lut input nets, then
# opt_lut_ins so write_blif does not emit sparse unconn lut pins.
mosaicDensifyLutInputs 1

stat
if { "TTT" ne "" } {
    hierarchy -check -top TTT -purge_lib
} else {
    hierarchy -check -auto-top -purge_lib
}

# hierarchy -purge_lib can drop keep and leave fresh undriven lut inputs
mosaicDensifyLutInputs 0

# the minus form  with a plus yosys emits a second .names gnd and vpr dies
# on inconsistent block data sizes
write_blif -true + vcc -false + gnd -undef - gnd -blackbox ZZZ
