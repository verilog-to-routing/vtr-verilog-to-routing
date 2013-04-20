create_clock -period 0 *
set_input_delay -clock * -max 0 [get_ports{*}]
set_output_delay -clock * -max 0 [get_ports{*}]