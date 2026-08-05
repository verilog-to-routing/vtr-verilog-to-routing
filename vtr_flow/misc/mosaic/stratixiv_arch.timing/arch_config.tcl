# policy for vtr_flow/arch/titan/stratixiv_arch.timing.xml
# titan has no classic multiply/adder/single_port_ram/dual_port_ram models.
# soft-map memories; stub all scanned hardblocks for rtl passthrough.
# rules/vtr_hardblock_lib.v.tmpl overlays the shared lib without classic ram stubs.
# inferred $mul/$add stay soft until titan-specific roles/templates exist.

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
# siv lutK=6 but lutK1=0 (not fracturable k6-style); plain -luts unless tuned
set abcOptScript   ""
set abcMapScript   ""
set keepCellTypes  ""
set exoticRoles {}
set exoticTemplatePairs {}
