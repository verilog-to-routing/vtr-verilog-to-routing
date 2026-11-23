# this file is the mosaic policy for mosaic/tests/fixtures/min_exotic_integer_mul.xml.
# the fixture has no classic multiply model, so exoticRoles binds inferred $mul
# onto my_mul through the stock integer_mul role and keepCellTypes retains that cell.

set bramRomCost    0.5
set bramSpCost     30
set bramDpCost     100
set cmpLutWidth    6
set lutCost        "6:1"
set hardAdderThreshold 3
set dspMinWidth    2
set sweepMaxIters  64
set abcOptScript   ""
set abcMapScript   ""
set keepCellTypes  "t:single_port_ram t:dual_port_ram t:my_mul"
set exoticRoles {{my_mul integer_mul}}
