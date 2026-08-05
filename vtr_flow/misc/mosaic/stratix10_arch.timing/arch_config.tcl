# this file is the mosaic policy for vtr_flow/arch/titan/stratix10_arch.timing.xml.
# titan declares no classic multiply, adder, single_port_ram, or dual_port_ram
# models and so memories are soft mapped because classic ram models are absent.
# every scanned hardblock is stubbed so rtl can passthrough by cell name, and
# rules/vtr_hardblock_lib.v.tmpl overlays the shared lib without classic ram stubs.
# inferred $mul and $add stay soft until titan specific roles or templates exist.

set softOnlyMemory 1
set stubAllHardblocks 1
set primitiveProfile passthrough_exotics

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
set keepCellTypes  ""
set exoticRoles {}
set exoticTemplatePairs {}
