#Netlist Clocks
create_clock -period 3 -waveform {1.25 2.75} clk0 #Integer period, float waveform
create_clock -period 3 -waveform {1 2} clk1 #Integer period, integer waveform
create_clock -period 2.3 clk2 #Float period
create_clock -period 2 {clk3 clk4} #Multiple targets


#Virtual Clocks
create_clock -period 1 -name input_clk
create_clock -period 0 -name output_clk #Zero period

#Clock Groups
set_clock_groups -exclusive -group input_clk -group {clk2} -group [get_clocks {clk3}] #Single element
set_clock_groups -exclusive -group {input_clk input_clock2} -group {clk2} -group [get_clocks {asdf qwerty}] #Multiple string elements

#False Path
set_false_path -from [get_clocks{clk}]          -to [get_clocks{output_clk}] #Clocks
set_false_path -from {asdf~/ff}                 -to {wer/234/ff3 xcw/32|ff2} #Objects
set_false_path -from {asdf/ff qwert/asd/ff}     -to [get_clocks{output_clk}] #Mixed Clocks/Objects
set_false_path -from [get_clocks{output_clk}]   -to {asdf/ff2} #Mixed Clocks/Objects

#Max Delay
set_max_delay 2     -from [get_clocks{clk}]         -to [get_clocks{output_clk}] #Clocks
set_max_delay 3.6   -from {asdf~/ff}                -to {wer/234/ff3 xcw/32|ff2} #Objects
set_max_delay .3    -from {asdf/ff qwert/asd/ff}    -to [get_clocks{output_clk}] #Mixed Clocks/Objects
set_max_delay 0.    -from [get_clocks{output_clk}]  -to {asdf/ff2} #Mixed Clocks/Objects

#MCP
set_multicycle_path 2 -setup -from [get_clocks{clk}]        -to [get_clocks{output_clk}] #Clocks
set_multicycle_path 3 -setup -from {asdf~/ff}               -to {wer/234/ff3 xcw/32|ff2} #Objects
set_multicycle_path 3 -setup -from {asdf/ff qwert/asd/ff}   -to [get_clocks{output_clk}] #Mixed Clocks/Objects
set_multicycle_path 0 -setup -from [get_clocks{output_clk}] -to {asdf/ff2} #Mixed Clocks/Objects
set_multicycle_path 3 -hold -from {asdf/ff qwert/asd/ff}   -to [get_clocks{output_clk}] #hold

#I/O Delay
set_input_delay -clock input_clk -max 0.5 [get_ports{in1 in2 in3}]
set_output_delay -clock output_clk -max 1 [get_ports{out*}]

#Line continuation
create_clock -period \
2 {clk3 \
clk4} \
#asdf

#Spaced Comments

#Non-empty line at end of file
set_output_delay -clock eof_test -max 1.7293 [ get_ports {eof_test_port*} ]
