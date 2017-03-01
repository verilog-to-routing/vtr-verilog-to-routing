set_min_delay 2     -from [get_clocks{clk}]         -to [get_clocks{output_clk}] #Clocks
set_min_delay 3.6   -from {asdf~/ff}                -to {wer/234/ff3 xcw/32|ff2} #Objects
set_min_delay .3    -from {asdf/ff qwert/asd/ff}    -to [get_clocks{output_clk}] #Mixed Clocks/Objects
set_min_delay 0.    -from [get_clocks{output_clk}]  -to {asdf/ff2} #Mixed Clocks/Objects
