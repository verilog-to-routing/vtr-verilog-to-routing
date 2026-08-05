# this file is the mosaic policy for mosaic/tests/fixtures/min_aliased_ram.xml.
# the fixture names its rams my_spram and my_dpram, so the aliases remap the
# classic ram roles onto those models and the emitted blif keeps matching names.

set aliasSinglePortRam my_spram
set aliasDualPortRam   my_dpram
set bramRomCost        0.5
set bramSpCost         30
set bramDpCost         100
set cmpLutWidth        6
set lutCost            "6:1"
set hardAdderThreshold 3
set dspMinWidth        2
set sweepMaxIters      64
set abcOptScript       ""
set abcMapScript       ""
