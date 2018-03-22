#!/bin/sh
TASK=$1

HOME_DIR="/workspace/vtr-verilog-to-routing"
VTR_FLOW_DIR="$HOME_DIR/vtr_flow"
TASK_DIR="$VTR_FLOW_DIR/tasks/$TASK"

#$Parse_result_file
#$1 
function get_geomean() {
    OUT=1
    while $IFS read arch circuit critical_path_delay; do
        case $critical_path_delay in
        ''|*[!0-9.]*) ;;
        *) TEMP=$(echo "$critical_path_delay*$OUT" | bc -l)
        OUT=$TEMP 
        ;;
        esac
    done <$1
    
    echo $(echo "e( l( $OUT )/4 )" | bc -l)
}

#$ADDER_TYPE $VTR_FLOW_DIR   $TASK   $BIT_WIDTH   $TASK_DIR
#$1          $2              $3      $4           $5
function run_task() {
    MAX_SIZE=$(( $4-1 ))
    for NEXT_SIZE in $(eval echo {0..$MAX_SIZE}); do
        
        rm -Rf $5/run*
        cat $5/track_completed/current_config.odin > $5/track_completed/odin.config
        
        for WIDTH in $(eval echo {$4..128}); do
            echo "$1,$NEXT_SIZE" >> $5/track_completed/odin.config
        done
        
        $2/scripts/run_vtr_task.pl $3 -p $(nproc --all) > $5/track_completed/run_pass
        if grep -q OK "$5/track_completed/run_pass";
        then
            $2/scripts/parse_vtr_task.pl $3 > /dev/null
            VAR=$(get_geomean $5/run001/parse_results.txt)
            
            if (( $(echo "$(cat $5/track_completed/best_value ) >= $VAR" | bc ) ));
            then
                echo $1,$NEXT_SIZE > $5/track_completed/best_name
                echo $VAR > $5/track_completed/best_value
                echo "$1,$NEXT_SIZE ... PASS setting new best: $VAR"
            else
                echo "$1,$NEXT_SIZE ... PASS"
            fi
        else
            echo "$1,$NEXT_SIZE ... FAILED"
        fi

    done
}

######### inital cleanup

rm -Rf $TASK_DIR/run*
rm -Rf $TASK_DIR/track_completed
mkdir $TASK_DIR/track_completed

INITIAL_BITSIZE=1

touch $TASK_DIR/track_completed/current_config.odin

for BITWIDTH in $(eval echo {$INITIAL_BITSIZE..128}); do
    ######### run baseline
    echo "TESTING $BITWIDTH ##########"
    echo ""
    
    rm -Rf $TASK_DIR/verilog && mkdir $TASK_DIR/verilog
    ## create test file
    sed -e "s/%%LEVELS_OF_ADDER%%/2/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TASK_DIR/verilog/adder_tree_2L.v
    sed -e "s/%%LEVELS_OF_ADDER%%/3/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TASK_DIR/verilog/adder_tree_3L.v
    
    rm -Rf $TASK_DIR/run*
    cat $TASK_DIR/track_completed/current_config.odin > $TASK_DIR/track_completed/odin.config
    
    for WIDTH in $(eval echo {$BITWIDTH..128}); do
        echo "ripple,0" >> $TASK_DIR/track_completed/odin.config
    done
    
    $VTR_FLOW_DIR/scripts/run_vtr_task.pl $TASK -p $(nproc --all) > $TASK_DIR/track_completed/run_pass
    $VTR_FLOW_DIR/scripts/parse_vtr_task.pl $TASK > /dev/null
    BASELINE=$(get_geomean $TASK_DIR/run001/parse_results.txt)

    echo "ripple,0" > $TASK_DIR/track_completed/best_name
    echo $BASELINE > $TASK_DIR/track_completed/best_value
    echo "ripple,0 ... PASS INITIAL: $BASELINE"
    
    cat $TASK_DIR/track_completed/best_value >> $TASK_DIR/track_completed/current_config_baseline.odin
    
    run_task  "fixed" $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR

    BEST_RESULT=$(cat $TASK_DIR/track_completed/best_value)
    RATIO=$(echo "$BEST_RESULT / $BASELINE" | bc -l)
    echo "$BEST_RESULT __ $RATIO" >> $TASK_DIR/track_completed/current_config_values.odin
    cat $TASK_DIR/track_completed/best_name >> $TASK_DIR/track_completed/current_config.odin
    
    
done