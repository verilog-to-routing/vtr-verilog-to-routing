./netlist_parse.py -route_in=/home/vm/VTR-Tools/workspace/scripts/blink_manual/blink.route  -route_out=/home/vm/VTR-Tools/workspace/scripts/blink_manual/switchbox_routing.csv

./netlist_parse.py -route_in=/home/vm/VTR-Tools/workspace/scripts/blink_run_flow/blink.route  -route_out=/home/vm/VTR-Tools/workspace/scripts/blink_run_flow/switchbox_routing.csv




./switch_position.py -route /home/vm/VTR-Tools/workspace/scripts/blink_manual/switchbox_routing.csv -sbu /home/vm/VTR-Tools/workspace/scripts/blink_manual/SB-U_Positions.csv

./switch_position.py -route /home/vm/VTR-Tools/workspace/scripts/blink_run_flow/switchbox_routing.csv -sbu /home/vm/VTR-Tools/workspace/scripts/blink_run_flow/SB-U_Positions.csv