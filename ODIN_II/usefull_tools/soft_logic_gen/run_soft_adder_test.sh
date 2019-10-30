#!/bin/bash

##############################################
##### TUNING PARAMETER

#weigth of different variables



VTR_TASK_TO_HOME=../..
TRACKER_DIR=ODIN_II/usefull_tools/soft_logic_gen/track_completed
cd ../../..
VTR_HOME_DIR=`pwd`
export VTR_HOME_DIR=$VTR_HOME_DIR
INITIAL_BITSIZE=2
END_BITSIZE=70
CURRENT_BITSIZE=0

OUTPUT_SEPARATOR="~"


######### inital cleanup ##########

rm -Rf $VTR_HOME_DIR/$TRACKER_DIR
mkdir -p $VTR_HOME_DIR/$TRACKER_DIR/failures

[ ! -e $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin ] && touch $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin


######### build necessary variables ##########
list_of_arch="\
arch/timing/k6_N10_mem32K_40nm.xml \
arch/timing/k6_frac_N10_mem32K_40nm.xml \
"

#what we are parsing
MEASUREMENT_NAME=(
"critical_path_delay"
"logic_block_area_total"
"total_power"
"total_dyn"
)

critical_path_delay=0
logic_block_area_total=1
total_power=2
total_dyn=3
MEASUREMENT_ARRAY_LEN=4

#build parse file
printf "${MEASUREMENT_NAME[$critical_path_delay]};vpr.crit_path.out;Final critical path: (.*) ns\n\
${MEASUREMENT_NAME[$logic_block_area_total]};vpr.out;Total logic block area .*: (.*)\n\
${MEASUREMENT_NAME[$total_power]};*.power;^Total\s+(.*?)\s+\n\
${MEASUREMENT_NAME[$total_dyn]};*.power;^Total\s+\S+\s+\S+\s+(.*?)\s+\n"> $VTR_HOME_DIR/$TRACKER_DIR/parse.txt

#init the weigth list
declare -a WEIGTH_LIST
#set some manualy
WEIGTH_LIST[$critical_path_delay]="1.0"
WEIGTH_LIST[$logic_block_area_total]="1.0"
WEIGTH_LIST[$total_power]="1.0"
WEIGTH_LIST[$total_dyn]="100.0"


#################################################


DIV_W=$(echo ${WEIGTH_LIST[@]} | awk '{sum=0; for(var=1;var<=NF;var++) sum = sum+$var; print sum}' )
WEIGTH_LIST=( $(echo $DIV_W ${WEIGTH_LIST[@]} | awk '{ for(var=2;var<=NF;var++) print $var/$1; }' ) )
echo ${MEASUREMENT_NAME[@]} ${WEIGTH_LIST[@]} | awk '
{ 
	st1=1; 
	st2=NF/2; 
	print "RUNNING exploration using following weigths:"
	for(var=st1;var<=st2;var++) 
		printf "%s\t:: %f %\n",$(var),$(var+st2)*100.0; 
}'

#result file header
printf "op,target_bit_size,soft_hard,circuit,bit_width,selected,overall_result" >> $VTR_HOME_DIR/$TRACKER_DIR/result.csv
for (( i=0; i<${MEASUREMENT_ARRAY_LEN}; i++ )); do
	printf ",${MEASUREMENT_NAME[$i]}" >> $VTR_HOME_DIR/$TRACKER_DIR/result.csv
done
printf "\n" >> $VTR_HOME_DIR/$TRACKER_DIR/result.csv


function get_geomean() {
	ADDERS=$1

	declare -a RESULT_OUT=( $(for (( i=0; i<${MEASUREMENT_ARRAY_LEN}; i++ )); do echo "1.0"; done) )
	while IFS=, read -a VALUE_LIST; do
		RESULT_OUT=( $(echo ${VALUE_LIST[@]} ${RESULT_OUT[@]} | awk '
		{ 
			st1=1;
			st2=NF/2;
			for(var=st1;var<=st2;var++) 
				print $(var)*$(var+st2); 
		}' ) )
	done < $ADDERS
	
	RESULT_OUT=( $(echo ${RESULT_OUT[@]} | awk '
		{ 
			for(var=1;var<=NF;var++) 
			{
				calc=$(var) ** (1.0/NF);
				printf "%f\n",calc
			}
		}') 
	)
	
	WEIGHTED_SUM=$(echo ${RESULT_OUT[@]} ${WEIGTH_LIST[@]} | awk '
	{
		st1=1;
		st2=NF/2;
		sum=0.0; 
		for(var=st1;var<=st2;var++) 
			sum += $(var)*$(var+st2); 
		
		print sum;
	}' )

	basename=${ADDERS%.results}
	basename=${basename##*/}
	test_name=$(echo $basename | cut -d ${OUTPUT_SEPARATOR} -f1)
	test_size=$(echo $basename | cut -d ${OUTPUT_SEPARATOR} -f2)
	
	
	
	#print to file for keeping
	printf "+,${CURRENT_BITSIZE},soft,${test_name},${test_size}${OUTPUT_SEPARATOR}${WEIGHTED_SUM}" >> $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv
	for (( i=0; i<${MEASUREMENT_ARRAY_LEN}; i++ )); do
		printf ",${RESULT_OUT[$i]}" >> $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv
	done
	printf "\n" >> $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv
}

function normalize() {
	for TEST_DIR in $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/*; do
		for results_out in $TEST_DIR/*.results; do
			echo $(cat $results_out) $(cat $TEST_DIR/default${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.results) | awk '
			{ 
				for(var=1;var<=NF/2;var++) 
				{
					result=$(var)/$(var+(NF/2));
					end_token=(var != NF/2)? ",": "\n";
					printf "%f%s",result, end_token;
				}
			}' >> $TEST_DIR/../${results_out##*/}
		done
	done
}

#$ADDER_TYPE MIN_SIZE
#$1          $2
function build_task() {
    ADDER_TYPE=$1
    FILL_DEF_TO=$END_BITSIZE
    MIN=$2

    for NEXT_SIZE in $(eval echo {$MIN..$CURRENT_BITSIZE}); do
        v_counter=0
        for verilogs in $VTR_HOME_DIR/$TRACKER_DIR/test_$CURRENT_BITSIZE/verilogs/*; do
          a_counter=0
          for xmls in $list_of_arch; do
            TEST_DIR=$VTR_HOME_DIR/$TRACKER_DIR/test_$CURRENT_BITSIZE/${v_counter}_${a_counter}/${ADDER_TYPE}${OUTPUT_SEPARATOR}${NEXT_SIZE}
            mkdir -p $TEST_DIR

            #get arch and verilog
            cp $VTR_HOME_DIR/vtr_flow/$xmls $TEST_DIR/arch.xml
            cp $verilogs $TEST_DIR/circuit.v
            cp $VTR_HOME_DIR/$TRACKER_DIR/parse.txt $TEST_DIR/parse.txt

            #build odin config
            cat $TRACKER_DIR/current_config.odin > $TEST_DIR/odin.soft_config
            for bit_size in $(eval echo {$CURRENT_BITSIZE..$FILL_DEF_TO}); do
              echo "+,$bit_size,soft,$ADDER_TYPE,$NEXT_SIZE" >> $TEST_DIR/odin.soft_config
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

  find $VTR_HOME_DIR/$TRACKER_DIR/test_$1/ -maxdepth 2 -mindepth 2 | grep default${OUTPUT_SEPARATOR}${CURRENT_BITSIZE} | xargs -P$(nproc) -I test_dir /bin/bash -c \
  '
  TEST_DIR=test_dir
  mkdir -p $TEST_DIR/../OUTPUT
  echo "$VTR_HOME_DIR/ODIN_II/odin_II \
    -a $TEST_DIR/arch.xml \
    -V $TEST_DIR/circuit.v \
    -o $TEST_DIR/odin.blif \
    -g 100 \
    -sim_dir $TEST_DIR/../OUTPUT/" > $TEST_DIR/run_result

  $VTR_HOME_DIR/ODIN_II/odin_II \
    -a $TEST_DIR/arch.xml \
    -V $TEST_DIR/circuit.v \
    -o $TEST_DIR/odin.blif \
    -g 100 \
    -sim_dir $TEST_DIR/../OUTPUT/ &>> $TEST_DIR/run_result

  /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/run_vtr_flow.pl \
  $TEST_DIR/odin.blif $TEST_DIR/arch.xml \
  -power \
  -cmos_tech $VTR_HOME_DIR/vtr_flow/tech/PTM_45nm/45nm.xml \
  -temp_dir $TEST_DIR/run \
  -starting_stage abc &>> $TEST_DIR/run_result

  /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/parse_vtr_flow.pl \
  $TEST_DIR/run $TEST_DIR/parse.txt \
  &> $TEST_DIR/run/parse_results.txt

  tail -n +2 $TEST_DIR/run/parse_results.txt > $TEST_DIR/../${TEST_DIR##*/}.results
  echo "INITIALIZED"
  '

  # #parralel this START
  find $VTR_HOME_DIR/$TRACKER_DIR/test_$1/ -maxdepth 2 -mindepth 2 | grep -v default${OUTPUT_SEPARATOR}${CURRENT_BITSIZE} | grep -v OUTPUT | xargs -P$(nproc) -I test_dir /bin/bash -c \
  '
  TEST_DIR=test_dir
  echo "$VTR_HOME_DIR/ODIN_II/odin_II \
    --adder_type $TEST_DIR/odin.soft_config \
    -a $TEST_DIR/arch.xml \
    -V $TEST_DIR/circuit.v \
    -o $TEST_DIR/odin.blif \
    -t $TEST_DIR/../OUTPUT/input_vectors \
    -T $TEST_DIR/../OUTPUT/output_vectors
    -sim_dir $TEST_DIR/ \
  && /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/run_vtr_flow.pl \
      $TEST_DIR/odin.blif $TEST_DIR/arch.xml \
      -power \
      -cmos_tech $VTR_HOME_DIR/vtr_flow/tech/PTM_45nm/45nm.xml \
      -temp_dir $TEST_DIR/run \
      -starting_stage abc" > $TEST_DIR/run_result

  $VTR_HOME_DIR/ODIN_II/odin_II \
      --adder_type $TEST_DIR/odin.soft_config \
      -a $TEST_DIR/arch.xml \
      -V $TEST_DIR/circuit.v \
      -o $TEST_DIR/odin.blif \
      -t $TEST_DIR/../OUTPUT/input_vectors \
      -T $TEST_DIR/../OUTPUT/output_vectors \
      -sim_dir $TEST_DIR/ &>> $TEST_DIR/run_result \
  && /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/run_vtr_flow.pl \
      $TEST_DIR/odin.blif $TEST_DIR/arch.xml \
      -power \
      -cmos_tech $VTR_HOME_DIR/vtr_flow/tech/PTM_45nm/45nm.xml \
      -temp_dir $TEST_DIR/run \
      -starting_stage abc &>> $TEST_DIR/run_result

  my_dir=$(readlink -f $TEST_DIR)
  parent_dir=$(readlink -f $TEST_DIR/..)
  test_bit=$(readlink -f $TEST_DIR/../..)

  if grep -q OK "$TEST_DIR/run_result";then
      /bin/perl $VTR_HOME_DIR/vtr_flow/scripts/parse_vtr_flow.pl \
      $TEST_DIR/run $TEST_DIR/parse.txt \
      &> $TEST_DIR/run/parse_results.txt

      tail -n +2 $TEST_DIR/run/parse_results.txt > $TEST_DIR/../${TEST_DIR##*/}.results
      printf "<${test_bit##*/}-${parent_dir##*/}>\t<${my_dir##*/}> \t\t... PASS\n"
  else
      my_dir=$(readlink -f $TEST_DIR)
      parent_dir=$(readlink -f $TEST_DIR/..)
      test_bit=$(readlink -f $TEST_DIR/../..)
      mv $TEST_DIR/run_result $TEST_DIR/../../../failures/${test_bit##*/}${parent_dir##*/}_${my_dir##*/}
      printf "<${test_bit##*/}_${parent_dir##*/}>\t<${my_dir##*/}> \t\t... ERROR\n"
  fi
  '
  rm -Rf $(find $VTR_HOME_DIR/$TRACKER_DIR/test_$1/ -maxdepth 2 -mindepth 2 | grep OUTPUT)
  #END
}


#bitwidth
#$1
function build_verilogs() {
  filename="$VTR_HOME_DIR/ODIN_II/usefull_tools/soft_logic_gen/verilogs/adder_tree.v"
  OUTPUT_DIR="$VTR_HOME_DIR/$TRACKER_DIR/test_$1/verilogs"
  sed -e "s/%%OPERATOR%%/+/g" -e "s/%%LEVELS_OF_ADDER%%/2/g" -e "s/%%ADDER_BITS%%/$1/g" "$filename" > $OUTPUT_DIR/2L_add_tree.v
  sed -e "s/%%OPERATOR%%/+/g" -e "s/%%LEVELS_OF_ADDER%%/3/g" -e "s/%%ADDER_BITS%%/$1/g" "$filename" > $OUTPUT_DIR/3L_add_tree.v
}


START=1
END=$((INITIAL_BITSIZE-1))

for BITWIDTH in $(eval echo {$START..$END}); do
  echo "+,$BITWIDTH,soft,default,$BITWIDTH" >> $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin
done

for BITWIDTH in $(eval echo {$INITIAL_BITSIZE..$END_BITSIZE}); do
  echo ""
  echo "TESTING $BITWIDTH ##########"
  echo ""
  
  CURRENT_BITSIZE=$((BITWIDTH))

  mkdir -p $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/verilogs

  build_verilogs $BITWIDTH

  build_task  "default"   $BITWIDTH
  build_task  "csla"      "1"
  #fix in the adder definition... other than full width, this won't work... IDK!!! :(
  build_task  "bec_csla"  $BITWIDTH

  rm -Rf $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/verilogs

  run_parralel_tasks $BITWIDTH

  #get dif for each arch_verilog pair vs default
  
	normalize
	rm -Rf $TEST_DIR


	touch $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${BITWIDTH}.csv

	
  for ADDERS in $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH/*.results; do
    get_geomean $ADDERS
  done
  
	#get best
	OUTPUT_VALUES="123456789.0"
	OUTPUT_NAME="ERROR"
	
	while IFS=${OUTPUT_SEPARATOR} read output_naming_schema output_result_schema ; do
		TEMP_VALUES=$(echo $output_result_schema | cut -d ',' -f1)
		
		if [ "$(echo $TEMP_VALUES $OUTPUT_VALUES | awk '{ print ($1 < $2); }')" -eq "1" ]; then
			OUTPUT_VALUES=$TEMP_VALUES
			OUTPUT_NAME=$output_naming_schema
		fi

	done < $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv

	echo $OUTPUT_NAME >> $VTR_HOME_DIR/$TRACKER_DIR/current_config.odin
	rm -Rf $VTR_HOME_DIR/$TRACKER_DIR/test_$BITWIDTH
	sed -e "s/${OUTPUT_SEPARATOR}/,,/g" "$VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv" > $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.reformatted.csv
	sed -e "s/${OUTPUT_NAME},,/${OUTPUT_NAME},X,/g" "$VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.reformatted.csv" >> $VTR_HOME_DIR/$TRACKER_DIR/result.csv

	rm -f $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.reformatted.csv
	rm -f $VTR_HOME_DIR/$TRACKER_DIR/result${OUTPUT_SEPARATOR}${CURRENT_BITSIZE}.csv
done
