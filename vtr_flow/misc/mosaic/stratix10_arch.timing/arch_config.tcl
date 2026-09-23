# this file is the mosaic policy for vtr_flow/arch/titan/stratix10_arch.timing.xml.
# every scanned hardblock is stubbed so rtl can passthrough by cell name, and
# rules/vtr_hardblock_lib.v.tmpl overlays the shared lib without classic ram stubs.
# inferred $mul and $add stay soft until titan specific roles or templates exist.

# softOnlyMemory stays on because those models use titan port names and opmode
# qualifiers rather than classic addr or data pins, so memories still soft map
# until a titan bram emitter exists.
set softOnlyMemory 1
set stubAllHardblocks 1

# titan names its rams and mac as fourteennm_ram_block and fourteennm_mac rather
# than classic multiply or single_port_ram or dual_port_ram, so the aliases below
# point classic roles at those models.

# both single_port and dual_port are opmodes of the same fourteennm_ram_block model.
set aliasSinglePortRam fourteennm_ram_block
set aliasDualPortRam   fourteennm_ram_block
# comb m18x18_full is the closest multiply shaped hardblock on this arch.
set aliasMultiply      "fourteennm_mac.opmode{m18x18_full}.input_type{comb}.output_type{comb}"

set bramRomCost    0.5
set bramSpCost     128
set bramDpCost     128
set cmpLutWidth    6
set lutCost        "6:1"
set hardAdderThreshold 3
set minHardMulWidth 0
set dspMinWidth    2
set sweepMaxIters  64
# empty abc scripts keep synthesis on a plain -luts pass until a tuned pair exists.
set abcOptScript   ""
set abcMapScript   ""
set keepCellTypes  "t:fourteennm_ram_block t:fourteennm_mac.opmode{m18x18_full}.input_type{comb}.output_type{comb}"
set exoticRoles {}
set exoticTemplatePairs {}
