for benchmark in $(ls /project/work/timing_analysis/skew/*/vpr_timing_graph.echo) 
do
    echo "==============================================================="
    echo $benchmark

    $1 $benchmark
done
