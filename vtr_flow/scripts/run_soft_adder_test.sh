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

#$ADDER_TYPE    $SKIP_TYPE $VTR_FLOW_DIR   $TASK   $BIT_WIDTH   $TASK_DIR
#$1             $2         $3              $4      $5           $6
function run_task() {
    
    MAX_INIT=$(echo "$5 - 1" | bc)
    
    for INITIAL_SIZE in $(eval echo {1..$MAX_INIT}); do
        MAX_SKIP=$(echo "$5 - $INITIAL_SIZE" | bc)
        
        for SKIP_SIZE in $(eval echo {1..$MAX_SKIP}); do
            rm -Rf $6/run*
            
            cat $6/track_completed/current_config.odin > $6/track_completed/odin.config
            
            for WIDTH in $(eval echo {$5..256}); do
                echo -e $1,$2,$INITIAL_SIZE,$SKIP_SIZE >> $6/track_completed/odin.config
            done
            
            $3/scripts/run_vtr_task.pl $4 -p $(nproc --all)
            
            $3/scripts/parse_vtr_task.pl $4
            
            VAR=$(get_geomean $6/run001/parse_results.txt)
            
            if (( $(echo "$(cat $6/track_completed/best_value ) > $VAR" | bc ) ));
            then
                echo $1,$2,$INITIAL_SIZE,$SKIP_SIZE > $6/track_completed/best_name
                echo $VAR > $6/track_completed/best_value
            fi
        done
    done
}

######### inital cleanup
if [ ! -d $TASK_DIR/track_completed ]; then
    mkdir $TASK_DIR/track_completed
fi
rm -Rf $TASK_DIR/run*

#default config
ADDER_TYPE="ripple"
SKIP_TYPE="fixed"
INITIAL_SIZE=0
SKIP_SIZE=0

INITIAL_BITSIZE=2

#check if file is already populated and get the current bitwidth
if [ ! -f $TASK_DIR/track_completed/current_config.odin ]; then
    #bit width 0
    echo -e $ADDER_TYPE,$SKIP_TYPE,$INITIAL_SIZE,$SKIP_SIZE > $TASK_DIR/track_completed/current_config.odin
    #bit width 1
    echo -e $ADDER_TYPE,$SKIP_TYPE,$INITIAL_SIZE,$SKIP_SIZE >> $TASK_DIR/track_completed/current_config.odin
else
    INITIAL_BITSIZE=$(wc -l < $TASK_DIR/track_completed/current_config.odin) 
fi


for BITWIDTH in $(eval echo {$INITIAL_BITSIZE..256}); do
    ######### run baseline
    
    ADDER_TYPE="ripple"
    SKIP_TYPE="fixed"
    INITIAL_SIZE=0
    SKIP_SIZE=0

    if [ ! -d $TASK_DIR/verilog ]; then
        mkdir $TASK_DIR/verilog
    else
        if [ ! -f $TASK_DIR/verilog/adder_tree_2L.v ]; then
            rm -f $TASK_DIR/verilog/adder_tree_2L.v
        fi
        
        if [ ! -f $TASK_DIR/verilog/adder_tree_3L.v ]; then
            rm -f $TASK_DIR/verilog/adder_tree_3L.v
        fi
    fi

    ## create test file
    sed -e "s/%%LEVELS_OF_ADDER%%/2/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TASK_DIR/verilog/adder_tree_2L.v
    sed -e "s/%%LEVELS_OF_ADDER%%/3/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TASK_DIR/verilog/adder_tree_3L.v

    cat $TASK_DIR/track_completed/current_config.odin > $TASK_DIR/track_completed/odin.config
    
    for i in $(eval echo {$BITWIDTH..256}); do
        echo $ADDER_TYPE,$SKIP_TYPE,$INITIAL_SIZE,$SKIP_SIZE >> $TASK_DIR/track_completed/odin.config
    done
    
    $VTR_FLOW_DIR/scripts/run_vtr_task.pl $TASK -p $(nproc --all)
    $VTR_FLOW_DIR/scripts/parse_vtr_task.pl $TASK
    
    echo $ADDER_TYPE,$SKIP_TYPE,$INITIAL_SIZE,$SKIP_SIZE > $TASK_DIR/track_completed/best_name
    echo $(get_geomean $TASK_DIR/run001/parse_results.txt) > $TASK_DIR/track_completed/best_value 
     
     
     
    #start the full sweep 
    ADDER_TYPE="parallel"
    
    SKIP_TYPE="fixed"
    run_task  $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    SKIP_TYPE="log"
    run_task $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    SKIP_TYPE="increasing"
    run_task $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    
    ADDER_TYPE="skip"
    
    SKIP_TYPE="fixed"
    run_task $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    SKIP_TYPE="log"
    run_task $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    SKIP_TYPE="increasing"
    run_task $ADDER_TYPE $SKIP_TYPE $VTR_FLOW_DIR $TASK $BITWIDTH $TASK_DIR
    
    
    cat $TASK_DIR/track_completed/best_name >> $TASK_DIR/track_completed/current_config.odin
done