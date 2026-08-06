yosys -import
plugin -i wildebeest

# this script is the mosaic vtr synthesis template.
#
# synth_fpga only works with a zeroasic partname, so for vtr arches we drive the
# individual passes ourselves. the mosaic plugin loads as wildebeest and provides
# max_level plus vtr_arch_rules.
#
# arch_config.tcl lives in the arch support dir named by ARCH_SUPPORT_DIR and is
# the primary per-arch policy artifact. optional ARCH_SUPPORT_DIR/rules/*.tmpl
# overlays the shared template rules so only listed files override the defaults.
# shared arch-independent support lives in the template dir TDIR (abc/delay_*.scr,
# lut_models/, and rules/*.tmpl passed as -tpldir).
#
# the harness fills these tokens before yosys sees the script:
#   XXX circuit verilog
#   TTT top module (empty means -auto-top)
#   ZZZ output blif
#   ARCH_SUPPORT_DIR arch support dir
#   TDIR this template dir (rules/ is -tpldir)
#   VVV arch xml
#   YYY max_level flag (empty when -vtr_arch is omitted)

# these knobs start as generic defaults and arch_config.tcl overrides them per arch.
set archSupportDir "ARCH_SUPPORT_DIR"
set archXmlPath    "VVV"
set archRulesDir   "."
set templateDir    "TDIR"

set dspMaxWidth    0
set dspMinWidth    2
# bramRomCost is half the default so zero/undef roms prefer hard bram over soft
# luts, while non-zero init is refused by bram_memory_map (init zero) and so
# soft-maps instead.
set bramRomCost    0.5
set bramSpCost     128
set bramDpCost     128
# these are sentinel defaults; when policy leaves them untouched, arch_facts
# derives cmpLutWidth and lutCost from lutK/lutK1 after the xml scan.
set cmpLutWidthDefault 6
set lutCostDefault     "6:1"
set cmpLutWidth    $cmpLutWidthDefault
set lutCost        $lutCostDefault
# $add/$sub at or below hardAdderThreshold stay soft so abc can still optimize
# across them, and the value is substituted into generated add_sub_map.v because
# that map is the only place that sees $add widths at techmap time.
set hardAdderThreshold 3
# minHardMulWidth keeps $mul soft when both operand widths are at or below this
# threshold (0 disables the limit).
set minHardMulWidth 0
# memories whose deepest scanned mode has fewer than minHardMemAbits address bits
# stay soft (0 disables the filter).
set minHardMemAbits 0
# when softOnlyMemory is 1, missing classic ram modes soft-map memories as
# titan-style arches require.
set softOnlyMemory 0
# sweepMaxIters is long enough to walk an msb-to-lsb carry cascade in one pass.
set sweepMaxIters  64
# empty abcOptScript and abcMapScript skip the two-pass abc flow and use one
# plain -luts pass instead.
set abcOptScript   ""
set abcMapScript   ""
# multiplyModel, adderModel, spRamModel, and dpRamModel are overwritten after
# arch_facts.tcl when aliases remap classic model names.
set multiplyModel  "multiply"
set adderModel     "adder"
set spRamModel     "single_port_ram"
set dpRamModel     "dual_port_ram"
set keepCellTypes  "t:multiply t:adder t:single_port_ram t:dual_port_ram"
# when stubAllHardblocks is 1, vtr_arch_rules emits generic stubs for every
# exotic hardblock model and writes hardblock_keep_types.txt so rtl-instantiated
# cells survive synthesis.
set stubAllHardblocks 0
# classic model aliases when arch xml uses non-standard model names
set aliasMultiply ""
set aliasAdder ""
set aliasSinglePortRam ""
set aliasDualPortRam ""
# exoticTemplatePairs lists {model template_path} pairs that become -exotic and
# -exotic-template on vtr_arch_rules.
set exoticTemplatePairs {}
# exoticRoles lists {model role} pairs that become -exotic-role using stock
# tpldir/roles/<role>_map.v.tmpl files.
set exoticRoles {}

# USE: warn when inferred $mul cannot bind to a hard multiply.
# rule gen already warns, and this surfaces the same contract in the synth log
# so designers see why behavioral multiply stayed soft.
proc mosaicCheckClassicMulContract {} {
    global multiplyPresent stubAllHardblocks exoticRoles exoticTemplatePairs
    if { $multiplyPresent } {
        return
    }
    if { [llength $exoticRoles] > 0 || [llength $exoticTemplatePairs] > 0 } {
        return
    }
    if { $stubAllHardblocks } {
        log -warning "mosaic: no classic multiply; inferred \$mul stays soft. rtl-instantiated exotic cells passthrough when stubAllHardblocks is on; bind \$mul with exoticRoles or exoticTemplatePairs."
        return
    }
    log -warning "mosaic: no classic multiply and no exotic mul binding; inferred \$mul stays soft."
}

set archConfigFile ""
if { $archSupportDir ne "" } {
    set archConfigFile "$archSupportDir/arch_config.tcl"
}
if { $archConfigFile ne "" && [file exists $archConfigFile] } {
    source $archConfigFile
}
# remember the policy keep list from arch_config so extras (for example
# role-bound models) survive the post-facts rebuild of classic builtins.
set keepCellTypesFromConfig $keepCellTypes
# mul2dspMinWidth is policy; arch_facts may overwrite dspMinWidth with the
# smallest multiply mode when arch_facts.tcl is sourced later.
set mul2dspMinWidth $dspMinWidth

# rule files are all derived from the arch xml by vtr_arch_rules at runtime, and
# the xml is required because there are no static fallback maps.
if { $archXmlPath eq "" } {
    error "mosaic synthesis requires an arch xml (VVV)"
}
set archRulesCmd "vtr_arch_rules -xml $archXmlPath -outdir $archRulesDir -tpldir $templateDir/rules -sp-cost $bramSpCost -dp-cost $bramDpCost -hard-adder-threshold $hardAdderThreshold -min-hard-mul $minHardMulWidth -min-hard-mem-abits $minHardMemAbits"
if { $softOnlyMemory } {
    append archRulesCmd " -soft-only-memory"
}
if { $archSupportDir ne "" && [file isdirectory "$archSupportDir/rules"] } {
    append archRulesCmd " -overlay-tpldir $archSupportDir/rules"
}
if { $aliasMultiply ne "" } {
    append archRulesCmd " -alias multiply=$aliasMultiply"
}
if { $aliasAdder ne "" } {
    append archRulesCmd " -alias adder=$aliasAdder"
}
if { $aliasSinglePortRam ne "" } {
    append archRulesCmd " -alias single_port_ram=$aliasSinglePortRam"
}
if { $aliasDualPortRam ne "" } {
    append archRulesCmd " -alias dual_port_ram=$aliasDualPortRam"
}
if { $stubAllHardblocks } {
    append archRulesCmd " -stub-all-hardblocks"
}
foreach exoticPair $exoticTemplatePairs {
    append archRulesCmd " -exotic [lindex $exoticPair 0] -exotic-template [lindex $exoticPair 1]"
}
foreach rolePair $exoticRoles {
    append archRulesCmd " -exotic-role [lindex $rolePair 0] [lindex $rolePair 1]"
}
eval $archRulesCmd
set bramMapFile      "$archRulesDir/bram_memory_map.txt"
set techBramFile     "$archRulesDir/tech_bram.v"
set hardblockLibFile "$archRulesDir/vtr_hardblock_lib.v"
set multMapFile      "$archRulesDir/mult_map.v"
set mul2dspMapFile   "$archRulesDir/mul2dsp_map.v"
set addSubMapFile    "$archRulesDir/add_sub_map.v"

# arch_facts.tcl carries arch-derived facts from the xml (dsp widths, ram abits,
# hardblock presence), and the defaults below are overwritten when that file exists.
set archName         ""
set vtrRamAbits      0
set multiplyPresent  0
set adderPresent     0
set adderCarryChain  0
set lutK             0
set lutK1            0
set multiplyModes    ""
set archFactsFile "$archRulesDir/arch_facts.tcl"
if { [file exists $archFactsFile] } {
    source $archFactsFile
    # policy dspMinWidth from arch_config wins over the facts min mode width.
    set dspMinWidth $mul2dspMinWidth
    # derive lut and abc knobs from scanned lutK when policy left the sentinels.
    if { $lutK > 0 && $lutCost eq $lutCostDefault } {
        if { $lutK1 > 0 && $lutK1 < $lutK } {
            set lutCost "${lutK1}:1,${lutK}:1"
        } else {
            set lutCost "${lutK}:1"
        }
    }
    if { $lutK > 0 && $cmpLutWidth == $cmpLutWidthDefault } {
        set cmpLutWidth $lutK
    }
    # fracturable k6-like arches auto-pick shared delay scripts when policy left
    # both abc script knobs empty.
    if { $abcOptScript eq "" && $abcMapScript eq "" && $lutK == 6 && $lutK1 > 0 && $lutK1 < $lutK } {
        set abcOptScript "$templateDir/abc/delay_gia_opt.scr"
        set abcMapScript "$templateDir/abc/delay_map.scr"
        log "mosaic: auto-selected delay abc scripts for fracturable lutK=$lutK lutK1=$lutK1"
    }
    if { $adderPresent && !$adderCarryChain } {
        log "mosaic: adder present but not carry-chain (cin/cout/sumout); \$add/\$sub stay soft"
    }
    mosaicCheckClassicMulContract
}
# keepCellTypes must name the models that appear in the blif, and aliases win
# over the classic defaults.
set keepCellTypes "t:$multiplyModel t:$adderModel t:$spRamModel t:$dpRamModel"
foreach tok $keepCellTypesFromConfig {
    if { $tok ne "" && [string first $tok $keepCellTypes] < 0 } {
        append keepCellTypes " $tok"
    }
}

# append exotic keep types from stub-all when arch_config enabled it so
# arch_config builtins stay first in the list.
set keepTypesFile "$archRulesDir/hardblock_keep_types.txt"
if { [file exists $keepTypesFile] } {
    set keepTypesFd [open $keepTypesFile r]
    set keepTypesExtra [string trim [read $keepTypesFd]]
    close $keepTypesFd
    if { $keepTypesExtra ne "" } {
        append keepCellTypes " $keepTypesExtra"
    }
}

# hardblock sweep, keep, and densify helpers run in a fixed order because keep
# versus densify interact.
#   1. mosaicHardblockSweep runs only while keep is unset so unused cascade tips
#      can still die (murray IV-A).
#   2. never call setundef inside the sweep because setundef -undriven plus
#      opt_clean deletes live multiply/adder/ram outputs that look undriven.
#   3. mosaicKeepHardblocks must run before any densify setundef/opt_clean and
#      again after hierarchy -purge_lib which can drop attributes.
# USE: walk unused hardblock cascade tips while keep is still unset.
proc mosaicHardblockSweep {{maxIters 64}} {
    # keep must stay unset here so only opt_merge/opt_clean run and unused tips fall.
    opt_merge
    for {set i 0} {$i < $maxIters} {incr i} {
        # opt_clean becomes a no-op once the cascade is gone, so overshooting is fine.
        opt_clean
    }
}

# USE: freeze live hardblocks so later setundef/opt_clean cannot drop them.
proc mosaicKeepHardblocks {} {
    global keepCellTypes
    setattr -set keep 1 {*}$keepCellTypes
}

# USE: densify sparse $lut inputs without blanket-zeroing every undriven net
# because vpr crashes when write_blif emits sparse unconn $lut pins. only zero
# undriven nets that feed $lut inputs since blanket-zeroing hides rtl bugs and
# can disturb hardblock edges.
proc mosaicDensifyLutInputs {{withOptLut 0}} {
    mosaicKeepHardblocks
    select {t:$lut} %ci
    setundef -zero -undriven
    select -clear
    opt_lut_ins
    if {$withOptLut} {
        opt_lut
    }
    # keep is still set so opt_clean will not delete live hardblock outputs.
    opt_clean
}

# USE: warn when async ff cells are present before adff2dff rewrites them
# because vpr has no async ff primitive and adff2dff is required but changes
# timing.
proc mosaicWarnAsyncFf {} {
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
        log -warning "mosaic: \$adff/\$adffe/\$aldff/\$aldffe present; vpr has no async ff so adff2dff maps them to sync dff (async reset becomes clocked)."
    }
}

# read the design plus sized hardblock stubs.
# the max-width stubs replace +/parmys/vtr_primitives.v because -lib freezes
# parameter defaults and those primitives default to WIDTH=1, which makes
# write_blif emit 1-bit blackboxes. rams are omitted from this file for the
# same reason because -lib DATA_WIDTH=1 truncates real rtl instance widths.
read_verilog -lib $hardblockLibFile

# read_verilog omits -nolatches so inferred latches stay latches because forcing
# them away can diverge from rtl that relies on latch inference, and
# fix_blif_for_vpr already handles .latch emission for vpr/abc.
read_verilog -sv XXX

# lock the top before the whitebox lands because with -auto-top a whitebox module
# can win and purge the actual circuit. the -lib ram stubs above are wide enough
# that rtl instances resolve without truncating their buses.
if { "TTT" ne "" } {
    hierarchy -check -top TTT -purge_lib
} else {
    hierarchy -check -auto-top -purge_lib
}

# classic bram whitebox is skipped when softOnlyMemory is active or classic ram
# modes are absent.
set softOnlyMemoryMarker "$archRulesDir/soft_only_memory.txt"
set softOnlyMemoryActive [file exists $softOnlyMemoryMarker]
if { !$softOnlyMemoryActive } {
    # read without -lib so generate loops expand at the real DATA_WIDTH because
    # top is already set and hierarchy re-elaborates without -auto-top. whitebox
    # module names follow spRamModel/dpRamModel (classic names or aliases).
    read_verilog -overwrite $archRulesDir/vtr_ram_whitebox.v
    hierarchy -check -purge_lib
}
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

# keep $mem as $mem so memory_libmap can see inferred memories.
memory -nomap
flatten

opt -full

# async assert becomes clocked because vpr has no async ff support.
mosaicWarnAsyncFf
techmap -map +/parmys/adff2dff.v
techmap -map +/parmys/adffe2dff.v
techmap -map +/parmys/aldff2dff.v
techmap -map +/parmys/aldffe2dff.v
opt -full

# coarse synth while arithmetic is still soft, matching synth_fpga ordering.
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

# hierarchy -purge_lib dropped the stubs, and rams are already whitebox bit cells
# so only multiply and adder need restoring here.
read_verilog -lib $hardblockLibFile

# identity passthrough for rtl-instantiated exotic hardblocks when stub-all ran.
set exoticIdentityMapFile "$archRulesDir/exotic_identity_maps.v"
if { [file exists $exoticIdentityMapFile] } {
    techmap -map $exoticIdentityMapFile
}

# bram
if { $softOnlyMemoryActive } {
    log "mosaic: softOnlyMemory active; skipping hard bram libmap"
    memory_map
} else {
    # memory_libmap uses init zero and rdwr old from bram_memory_map.txt, so
    # non-zero-initialized memories stay as $mem here and soft-map via memory_map.
    memory_libmap -lib $bramMapFile -logic-cost-rom $bramRomCost

    # leftovers (including non-zero init and unfit sizes) become flip-flop arrays.
    memory_map

    techmap -map $techBramFile

    # write_blif needs arch model names rather than internal bit-cell names.
    delete $spRamModel $dpRamModel
    chtype -map vtr_sp_ram_bit $spRamModel
    chtype -map vtr_dp_ram_bit $dpRamModel
    delete vtr_sp_ram_bit vtr_dp_ram_bit
    read_verilog -lib $archRulesDir/vtr_ram_bit_lib.v
}

# last chance to shrink soft cones before hard mapping freezes the edges.
opt -full
share
wreduce
opt_clean

# dsp mapping runs before alumacc turns $mul into $macc.
# mul2dsp chops wide $mul into chunks that fit one dsp block, and mult_map.v
# then binds anything left that still fits an arch multiply mode.
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

# USE: techmap role maps listed by vtr_arch_rules.
# role maps that bind $mul (integer_mul) run after classic mult_map, and
# integer_mul is skipped at rule-gen time when classic multiply is present.
proc mosaicApplyRoleMaps {} {
    global archRulesDir
    set roleMapListFile "$archRulesDir/role_map_files.txt"
    if { ![file exists $roleMapListFile] } {
        return
    }
    set roleMapFd [open $roleMapListFile r]
    foreach roleMapPath [split [string trim [read $roleMapFd]] "\n"] {
        if { $roleMapPath ne "" } {
            techmap -map $roleMapPath
        }
    }
    close $roleMapFd
}
mosaicApplyRoleMaps

# adder and carry chain mapping runs before alumacc because otherwise $add chains
# fold into $macc and the carry chain never sees them, while compares stay soft
# on purpose.
techmap -map $addSubMapFile

alumacc

# role maps that bind $macc (integer_mac) need alumacc to run first.
mosaicApplyRoleMaps

# there is no $alu to adder map because comparing through a hard adder is a qor
# loss. sweep while keep is unset and do not densify/setundef until after the
# final sweep.
mosaicHardblockSweep $sweepMaxIters

# clock-domain cut points for max_level; YYY is -vtr_arch <xml> on the mosaic leg
# and empty on the vanilla parmys leg.
max_level -clk2clk YYY

# lower to gates for abc with -noff on every opt past this point because opt_dff
# cannot see through multiply and adder blackboxes and would otherwise leave them
# alone while still chewing the soft cones around them in odd ways.
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

# snapshot pre-abc per-type counts (and io pins via -top) for the compare s/a
# split. the trailing file arg writes stat there instead of needing a tee pass.
if { "TTT" ne "" } {
    stat -top TTT mosaic_synth.stat
} else {
    stat mosaic_synth.stat
}

# abc runs in-yosys now that vtr's yosys is built with ENABLE_ABC=1. dress leaves
# pi and po as pi*/po* which yosys cannot wire up, so the gia opt script ends with
# move_names instead. use two passes when both scripts are set, otherwise one plain
# -luts pass.
if { $abcOptScript ne "" && $abcMapScript ne "" } {
    abc -script $abcOptScript
    abc -luts $lutCost -script $abcMapScript
} else {
    abc -luts $lutCost
}

# abc will not delete blackboxes, so sweep anything it left unused. this is the
# last sweep before keep, and densify below must setattr keep first.
mosaicHardblockSweep $sweepMaxIters

# snapshot post-abc per-type counts for the compare split.
if { "TTT" ne "" } {
    stat -top TTT mosaic_abc.stat
} else {
    stat mosaic_abc.stat
}

# densify then write_blif using scoped densify: keep hardblocks, zero only
# undriven $lut input nets, then opt_lut_ins so write_blif does not emit sparse
# unconn lut pins.
mosaicDensifyLutInputs 1

stat
if { "TTT" ne "" } {
    hierarchy -check -top TTT -purge_lib
} else {
    hierarchy -check -auto-top -purge_lib
}

# hierarchy -purge_lib can drop keep and leave fresh undriven lut inputs behind.
mosaicDensifyLutInputs 0

# use the minus form for const nets because a plus form makes yosys emit a
# second .names gnd and vpr dies on inconsistent block data sizes.
write_blif -true + vcc -false + gnd -undef - gnd -blackbox ZZZ
