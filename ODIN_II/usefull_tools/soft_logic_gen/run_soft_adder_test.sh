#!/bin/bash
name=$1
TASK_DIR=`pwd`/configs/$name
VTR_TASK=../../ODIN_II/usefull_tools/soft_logic_gen/configs/$name
TRACKER_DIR=`pwd`/track_completed
cd ../../..
HOME_DIR=`pwd`
VTR_FLOW_DIR=`pwd`/vtr_flow

if [ ! -d $TASK_DIR ]
then
  echo "not a valid test!"
  exit 1
fi

mkdir -p $TRACKER_DIR

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
        cat $TRACKER_DIR/current_config.odin > $HOME_DIR/ODIN_II/odin.soft_config

        for WIDTH in $(eval echo {$4..128}); do
            echo "$1,$NEXT_SIZE" >> $HOME_DIR/ODIN_II/odin.soft_config
        done

        $2/scripts/run_vtr_task.pl $VTR_TASK -p $(nproc --all) > $TRACKER_DIR/run_pass
        if grep -q OK "$TRACKER_DIR/run_pass";
        then
            $2/scripts/parse_vtr_task.pl $VTR_TASK > /dev/null
            VAR=$(get_geomean $5/run001/parse_results.txt)

            if (( $(echo "$(cat $TRACKER_DIR/best_value ) >= $VAR" | bc ) ));
            then
                echo $1,$NEXT_SIZE > $TRACKER_DIR/best_name
                echo $VAR > $TRACKER_DIR/best_value
                echo "$1,$NEXT_SIZE ... PASS setting new best: $VAR"
            else
                echo "$1,$NEXT_SIZE ... PASS"
            fi
        else
            echo "$1,$NEXT_SIZE ... FAILED"
            mkdir -p $TRACKER_DIR/failure/$4
            cat $TRACKER_DIR/run_pass > $TRACKER_DIR/failure/$4/$NEXT_SIZE.log
        fi

    done
}

######### inital cleanup

rm -Rf $TASK_DIR/run*
rm -Rf $TRACKER_DIR
mkdir $TRACKER_DIR

INITIAL_BITSIZE=2

touch $TRACKER_DIR/current_config.odin

for BITWIDTH in $(eval echo {$INITIAL_BITSIZE..128}); do
    ######### run baseline
    echo "TESTING $BITWIDTH ##########"
    echo ""

    rm -Rf $TRACKER_DIR/verilog && mkdir -p $TRACKER_DIR/verilog
    ## create test file
    sed -e "s/%%LEVELS_OF_ADDER%%/2/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TRACKER_DIR/verilog/adder_tree_2L.v
    sed -e "s/%%LEVELS_OF_ADDER%%/3/g" -e "s/%%ADDER_BITS%%/$BITWIDTH/g" "$VTR_FLOW_DIR/benchmarks/arithmetic/adder_trees/verilog/adder_tree.v" > $TRACKER_DIR/verilog/adder_tree_3L.v

    rm -Rf $TASK_DIR/run*
    cat $TRACKER_DIR/current_config.odin > $HOME_DIR/ODIN_II/odin.soft_config

    for WIDTH in $(eval echo {$BITWIDTH..128}); do
        echo "rca,0" >> $HOME_DIR/ODIN_II/odin.soft_config
    done

    $VTR_FLOW_DIR/scripts/run_vtr_task.pl $VTR_TASK -p $(nproc --all) > $TRACKER_DIR/run_pass
    $VTR_FLOW_DIR/scripts/parse_vtr_task.pl $VTR_TASK > /dev/null
    BASELINE=$(get_geomean $TASK_DIR/run001/parse_results.txt)

    echo "rca,0" > $TRACKER_DIR/best_name
    echo $BASELINE > $TRACKER_DIR/best_value
    echo "rca,0 ... PASS INITIAL: $BASELINE"

    cat $TRACKER_DIR/best_value >> $TRACKER_DIR/current_config_baseline.odin

    run_task  "csla" $VTR_FLOW_DIR $VTR_TASK $BITWIDTH $TASK_DIR
    run_task  "bec_csla" $VTR_FLOW_DIR $VTR_TASK $BITWIDTH $TASK_DIR

    BEST_RESULT=$(cat $TRACKER_DIR/best_value)
    RATIO=$(echo "$BEST_RESULT / $BASELINE" | bc -l)
    echo "$BEST_RESULT __ $RATIO" >> $TRACKER_DIR/current_config_values.odin
    cat $TRACKER_DIR/best_name >> $TRACKER_DIR/current_config.odin


done
