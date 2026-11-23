create_clock * -period 0

set all_design_clocks [all_clocks]

if {[llength $all_design_clocks] > 1} {
    set cmd "set_clock_groups -asynchronous"

    foreach clk_obj $all_design_clocks {
        set clk_name [get_property $clk_obj name]
        append cmd " -group {$clk_name}"
    }

    eval $cmd
}