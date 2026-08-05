# this file is the mosaic policy for vtr_flow/arch/titan/stratixiv_arch.timing.xml.
# every scanned hardblock is stubbed so rtl can passthrough by cell name, and
# rules/vtr_hardblock_lib.v.tmpl overlays the shared lib without classic ram stubs.
# inferred $mul and $add stay soft until titan specific roles or templates exist.

# softOnlyMemory stays on because those models use titan port names and opmode
# qualifiers rather than classic addr or data pins, so memories still soft map
# until a titan bram emitter exists.
set softOnlyMemory 1
set stubAllHardblocks 1
set primitiveProfile passthrough_exotics

# titan names its rams and mac as stratixiv_ram_block and stratixiv_mac_mult rather
# than classic multiply or single_port_ram or dual_port_ram, so the aliases below
# point classic roles at those models.

# both single_port and dual_port are opmodes of the same stratixiv_ram_block model.
set aliasSinglePortRam stratixiv_ram_block
set aliasDualPortRam   stratixiv_ram_block
# comb mac_mult is the closest multiply shaped hardblock on this arch.
set aliasMultiply      "stratixiv_mac_mult.input_type{comb}"

set bramRomCost    0.5
set bramSpCost     128
set bramDpCost     128
set cmpLutWidth    6
set lutCost        "6:1"
set hardAdderThreshold 3
set minHardMulWidth 0
set dspMinWidth    2
set sweepMaxIters  64
# this arch reports lutK=6 with lutK1=0 so it is not fracturable k6 style, and
# empty abc scripts keep synthesis on a plain -luts pass until a tuned pair exists.
set abcOptScript   ""
set abcMapScript   ""
set keepCellTypes  "t:stratixiv_ram_block t:stratixiv_mac_mult.input_type{comb}"
set exoticRoles {}
set exoticTemplatePairs {}
