set_clock_uncertainty -setup -from clka -to clkb 0.3

set_clock_uncertainty -hold -from clka -to clkb 0.2

set_clock_uncertainty -hold -from {clka clkc} -to clkb 0.2

set_clock_uncertainty -hold -from {clka clkc} -to {clkb clkd} 0.2

set_clock_uncertainty -setup -from [get_clocks {clka clkc}] -to {clkb clkd} 0.2
