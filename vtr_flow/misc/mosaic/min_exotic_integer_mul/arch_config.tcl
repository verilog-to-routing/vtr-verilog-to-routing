# policy for mosaic/tests/fixtures/min_exotic_integer_mul.xml
# bind inferred $mul to exotic my_mul (no classic multiply).

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
