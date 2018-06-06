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

#$Parse_result_file    critical_weight   size_weight power_weight
#$1                   $2                $3          $4
function get_geomean() {

    #this should equal 1 !!
    TOLERANCE="0.001"
    TOT_WEIGHT_TOLERANCE=$(echo " 1.0 - $2 + $3 + $4 + $TOLERANCE " | bc -l)
    if [ $(echo " $TOT_WEIGHT_TOLERANCE < 0 " | bc -l) || $(echo " $TOT_WEIGHT_TOLERANCE > ($TOLERANCE *2) " | bc -l) ]
    then
      exit 1;
    fi

    OUT_CRITICAL_PATH=0
    OUT_SIZE=0
    OUT_POWER=0

    NB_OF_TEST=0

    while $IFS read arch circuit critical_path_delay,logic_block_area_total,total_power; do
        case $critical_path_delay in
        ''|*[!0-9.]*) ;;
        *)
          OUT_CRITICAL_PATH=$(echo " l( $critical_path_delay )+ $OUT " | bc -l)
          OUT_SIZE=$(echo " l( $logic_block_area_total )+ $OUT " | bc -l)
          OUT_POWER=$(echo " l( $total_power )+ $OUT " | bc -l)
          NB_OF_TEST=$((NB_OF_TEST+1))
       ;;
        esac
    done <$1

    OUT_CRITICAL_PATH=$(echo "e( $OUT_CRITICAL_PATH / $NB_OF_TEST )" | bc -l)
    OUT_SIZE=$(echo "e( $OUT_SIZE / $NB_OF_TEST )" | bc -l)
    OUT_POWER=$(echo "e( $OUT_POWER / $NB_OF_TEST )" | bc -l)

    echo $(echo " $OUT_CRITICAL_PATH * $2 + $OUT_SIZE * $3 + $OUT_POWER * $4 " | bc -l)
}

#file_to_compare  new_value
#$1              $2
function compare_and_swap_results () {
  if (( $(echo "$(cat $TRACKER_DIR/best_value ) >= $VAR" | bc ) ));
  then
      echo $1,$NEXT_SIZE > $TRACKER_DIR/best_name
      echo $VAR > $TRACKER_DIR/best_value
      echo "+,soft,$1,$NEXT_SIZE ... PASS setting new best: $VAR"
  else
      echo "+,soft,$1,$NEXT_SIZE ... PASS"
  fi
}

#$ADDER_TYPE $VTR_FLOW_DIR   $TASK   $BIT_WIDTH   $TASK_DIR
#$1          $2              $3      $4           $5
function run_task() {
    MAX_SIZE=$(( $4 ))
    for NEXT_SIZE in $(eval echo {$MAX_SIZE..2}); do

        rm -Rf $5/run*
        cat $TRACKER_DIR/current_config.odin > $HOME_DIR/ODIN_II/odin.soft_config

        $2/scripts/run_vtr_task.pl $VTR_TASK -p $(nproc --all) > $TRACKER_DIR/run_pass
        if grep -q OK "$TRACKER_DIR/run_pass";
        then
            $2/scripts/parse_vtr_task.pl $VTR_TASK > /dev/null
            VAR=$(get_geomean $5/run001/parse_results.txt)

            if (( $(echo "$(cat $TRACKER_DIR/best_value ) >= $VAR" | bc ) ));
            then
                echo $1,$NEXT_SIZE > $TRACKER_DIR/best_name
                echo $VAR > $TRACKER_DIR/best_value
                echo "+,soft,$1,$NEXT_SIZE ... PASS setting new best: $VAR"
            else
                echo "+,soft,$1,$NEXT_SIZE ... PASS"
            fi
        else
            echo "+,soft,$1,$NEXT_SIZE ... FAILED"
            mkdir -p $TRACKER_DIR/failure/$4
            cat $TRACKER_DIR/run_pass > $TRACKER_DIR/failure/$4/$NEXT_SIZE.log
        fi

    done
}

######### inital cleanup

rm -Rf $TASK_DIR/run*
rm -Rf $TRACKER_DIR
mkdir $TRACKER_DIR

INITIAL_BITSIZE=0

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

    $VTR_FLOW_DIR/scripts/run_vtr_task.pl $VTR_TASK -p $(nproc --all) > $TRACKER_DIR/run_pass
    $VTR_FLOW_DIR/scripts/parse_vtr_task.pl $VTR_TASK > /dev/null
    BASELINE=$(get_geomean $TASK_DIR/run001/parse_results.txt)

    echo "+,soft,default,$BITWIDTH" > $TRACKER_DIR/best_name
    echo $BASELINE > $TRACKER_DIR/best_value
    echo "+,soft,default,$BITWIDTH ... PASS INITIAL: $BASELINE"

    cat $TRACKER_DIR/best_value >> $TRACKER_DIR/current_config_baseline.odin

    run_task  "csla" $VTR_FLOW_DIR $VTR_TASK $BITWIDTH $TASK_DIR
    run_task  "bec_csla" $VTR_FLOW_DIR $VTR_TASK $BITWIDTH $TASK_DIR

    BEST_RESULT=$(cat $TRACKER_DIR/best_value)
    RATIO=$(echo "$BEST_RESULT / $BASELINE" | bc -l)
    echo "$BEST_RESULT __ $RATIO" >> $TRACKER_DIR/current_config_values.odin
    cat $TRACKER_DIR/best_name >> $TRACKER_DIR/current_config.odin


done
