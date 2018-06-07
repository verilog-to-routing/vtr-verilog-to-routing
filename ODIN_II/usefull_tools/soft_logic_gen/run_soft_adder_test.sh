#!/bin/bash
VTR_TASK_TO_HOME=../..
TRACKER_DIR=ODIN_II/usefull_tools/soft_logic_gen/track_completed
cd ../../..
VTR_HOME_DIR=`pwd`
export VTR_HOME_DIR=$VTR_HOME_DIR
INITIAL_BITSIZE=2
END_BITSIZE=128

list_of_arch="\
arch/timing/k6_N8_gate_boost_0.2V_22nm.xml \
arch/timing/fraclut_carrychain/k6_frac_N8_22nm.xml \
"
function build_parse_file() {
  #build parse file
  printf "\
    critical_path_delay;vpr.crit_path.out;Final critical path: (.*) ns \n\
    logic_block_area_total;vpr.out;Total logic block area .*: (.*) \
  " > $VTR_HOME_DIR/$TRACKER_DIR/parse.txt
}

#destination start end adder
#$1           $2    $3  $4
function fill_def_file() {
  if [ ! -e $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin ]; then
    touch $1
  fi

  for BITWIDTH in $(eval echo {$2..$3}); do
    echo "+,$BITWIDTH,soft,$4,$BITWIDTH" >> $1
  done
}

function get_geomean() {
  ADDERS=$1
  CRIT_W="1.0"
  SIZE_W="1.0"
  DIV_W=$(echo $CRIT_W $SIZE_W | awk '{ print $0+$1 }')
  OUT_CRIT="1.0"
  OUT_SIZE="1.0"
  count=0

  while $IFS read critical_path_delay logic_block_area_total; do
      OUT_CRIT=$(echo $critical_path_delay $OUT_CRIT | awk '{ print $0*$1 }')
      OUT_SIZE=$(echo $logic_block_area_total $OUT_SIZE | awk '{ print $0*$1 }')
      count=$((count+=1))
  done < $ADDERS
  rm $ADDERS
  OUT_CRIT=$(echo $OUT_CRIT $count | awk '{ print $0 ** (1.0/$1) }')
  OUT_SIZE=$(echo $OUT_SIZE $count | awk '{ print $0 ** (1.0/$1) }')

  WEIGHTED_SUM=$(echo $OUT_CRIT $CRIT_W $OUT_SIZE $SIZE_W $DIV_W | awk '{ print ($0*($1/$5))+($2*($3/$5)) }')
  basename=${ADDERS%.results}
  test_name=$(echo ${basename##*/} | tr "-" ',')

  echo "$test_name,$OUT_CRIT,$OUT_SIZE,$WEIGHTED_SUM" >> $VTR_HOME_DIR/$TRACKER_DIR/result-$BITWIDTH.csv
  OUT="$test_name
  $WEIGHTED_SUM"
  echo $test_name $WEIGHTED_SUM
}

#$ADDER_TYPE $BIT_WIDTH MIN_SIZE
#$1          $2         $3
function build_task() {
    ADDER_TYPE=$1
    MIN_SIZE=$(( $2 ))
    MAX_SIZE=$(( $3 ))

    for NEXT_SIZE in $(eval echo {$MIN_SIZE..$MAX_SIZE}); do
        v_counter=0
        for verilogs in $VTR_HOME_DIR/$TRACKER_DIR/test_$MAX_SIZE/verilogs/*; do
          a_counter=0
          for xmls in $list_of_arch; do
            TEST_DIR=$VTR_HOME_DIR/$TRACKER_DIR/test_$MAX_SIZE/${v_counter}_${a_counter}/${ADDER_TYPE}-${NEXT_SIZE}
            mkdir -p $TEST_DIR

            #get arch and verilog
            cp $VTR_HOME_DIR/vtr_flow/$xmls $TEST_DIR/arch.xml
            cp $verilogs $TEST_DIR/circuit.v
            cp $VTR_HOME_DIR/$TRACKER_DIR/parse.txt $TEST_DIR/parse.txt

            #build odin config
            cat $TRACKER_DIR/current_config.odin > $TEST_DIR/odin.soft_config
            for SIZE_IN in $(eval echo {$MAX_SIZE..$END_BITSIZE}); do
              echo "+,$SIZE_IN,soft,$ADDER_TYPE,$NEXT_SIZE" >> $TEST_DIR/odin.soft_config
            done
            a_counter=$((a_counter+1))
          done
          v_counter=$((v_counter+1))
        done
    done
}
#bitwidth
#$1
function run_parralel_tasks() {
  #parralel this START
  find $VTR_HOME_DIR/$TRACKER_DIR/test_$1/ -maxdepth 2 -mindepth 2 | xargs -P$((`nproc`*2+1)) -I test_dir /bin/bash -c \
  '
  TEST_DIR=test_dir
  echo "$VTR_HOME_DIR/ODIN_II/odin_II \
    --adder_type $TEST_DIR/odin.soft_config \
    -a $TEST_DIR/arch.xml \
    -V $TEST_DIR/circuit.v \
    -o $TEST_DIR/odin.blif" > $TEST_DIR/odin_result

  $VTR_HOME_DIR/ODIN_II/odin_II \
  --adder_type $TEST_DIR/odin.soft_config \
  -a $TEST_DIR/arch.xml \
  -V $TEST_DIR/circuit.v \
  -o $TEST_DIR/odin.blif &>> $TEST_DIR/odin_result

  /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/run_vtr_flow.pl \
  $TEST_DIR/odin.blif $TEST_DIR/arch.xml \
  -temp_dir $TEST_DIR/run \
  -starting_stage abc > $TEST_DIR/run_result

  if grep -q OK "$TEST_DIR/run_result";then
      /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/parse_vtr_flow.pl \
      $TEST_DIR/run $TEST_DIR/parse.txt \
      > $TEST_DIR/run/parse_results.txt
      test_name=${TEST_DIR##*/}
      tail -n +2 $TEST_DIR/run/parse_results.txt | tr -s [:blank:] > $TEST_DIR/../${test_name}.results
      rm -Rf $TEST_DIR
      echo "$TEST_DIR ... PASS"
  else
      mkdir -p $VTR_HOME_DIR/ODIN_II/odin_II/usefull_tools/soft_logic_gen/track_completed/failures
      cp -Rf $TEST_DIR $VTR_HOME_DIR/ODIN_II/odin_II/usefull_tools/soft_logic_gen/track_completed/failures
      echo "$TEST_DIR ... FAILED"
  fi
  '
  #END
}

#bitwidth
#$1
function build_verilogs() {
  sed -e "s/%%ADDER_BITS%%/$1/g" "$VTR_HOME_DIR/ODIN_II/usefull_tools/soft_logic_gen/verilogs/2L_tree_sync.v" > $VTR_HOME_DIR/$TRACKER_DIR/test_$1/verilogs/2L_add_tree_sync.v
  sed -e "s/%%ADDER_BITS%%/$1/g" "$VTR_HOME_DIR/ODIN_II/usefull_tools/soft_logic_gen/verilogs/2L_tree_async.v" > $VTR_HOME_DIR/$TRACKER_DIR/test_$1/verilogs/2L_add_tree_async.v
  sed -e "s/%%ADDER_BITS%%/$1/g" "$VTR_HOME_DIR/ODIN_II/usefull_tools/soft_logic_gen/verilogs/3L_tree_sync.v" > $VTR_HOME_DIR/$TRACKER_DIR/test_$1/verilogs/3L_add_tree_sync.v
  sed -e "s/%%ADDER_BITS%%/$1/g" "$VTR_HOME_DIR/ODIN_II/usefull_tools/soft_logic_gen/verilogs/3L_tree_async.v" > $VTR_HOME_DIR/$TRACKER_DIR/test_$1/verilogs/3L_add_tree_async.v
}


######### inital cleanup ##########

rm -Rf $VTR_HOME_DIR/$TRACKER_DIR
mkdir -p $VTR_HOME_DIR/$TRACKER_DIR

#build files
build_parse_file

[ ! -e $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin ] && touch $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin

for BITWIDTH in $(eval echo {1..$((INITIAL_BITSIZE-1))}); do
  echo "+,$BITWIDTH,soft,default,$BITWIDTH" >> $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin
done

for BITWIDTH in $(eval echo {$INITIAL_BITSIZE..$END_BITSIZE}); do
  echo ""
  echo "TESTING $BITWIDTH ##########"
  echo ""

  mkdir -p $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/verilogs

  build_verilogs $BITWIDTH

  build_task  "default"   $BITWIDTH $BITWIDTH
  build_task  "csla"      1 $BITWIDTH
  build_task  "bec_csla"  1 $BITWIDTH

  rm -Rf $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/verilogs

  run_parralel_tasks $BITWIDTH

  #get dif for each arch_verilog pair vs default
  for TEST_DIR in $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/*; do
    #find default_values
    d_t=$(cat $TEST_DIR/default-${BITWIDTH}.results | cut -d " " -f1)
    d_s=$(cat $TEST_DIR/default-${BITWIDTH}.results | cut -d " " -f2)
    #+1 for geomean
    echo "1.0 1.0" >> $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/default-${BITWIDTH}.results

    for results_out in $TEST_DIR/*.results; do
      test_name=${results_out##*/}
      c_t=$(cat $results_out | cut -d " " -f1)
      c_s=$(cat $results_out | cut -d " " -f2)
      if [ $(echo $c_t $c_s | awk '{ print ($0>0 && $1>0)?1:0 }') ]; then
        echo $(echo $c_t $d_t | awk '{ print ($0/$1) }') $(echo $c_s $d_s | awk '{ print ($0/$1) }') >> $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/$test_name
      else
        echo "1.0 1.0" >> $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/$test_name
      fi
    done
    rm -Rf $TEST_DIR
  done

  #get geomean of each adder_type and output to single file
  #keep best adder in memory to output to odin_config
  values=$(get_geomean $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/default-${BITWIDTH}.results)
  BEST_N=$(echo $values | cut -d " " -f1 )
  BEST_V=$(echo $values | cut -d " " -f2 )
  TOLERANCE="0.0001"
  for ADDERS in $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/*.results; do
    values=$(get_geomean $ADDERS)
    TEMP_N=$(echo $values | cut -d " " -f1 )
    TEMP_V=$(echo $values | cut -d " " -f2 )
    if [ $(echo $TEMP_V $BEST_V $TOLERANCE | awk '{ print (($0+$2)<$1)?1:0 }') ]; then
      BEST_N=$TEMP_N
      BEST_V=$TEMP_V
    fi
    rm -Rf $ADDERS
  done
  echo "+,$BITWIDTH,soft,$BEST_N" >> $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin

done
