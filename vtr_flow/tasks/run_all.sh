# find all tasks with a config
tasks=$(find . -name "regression_titan*" -prune -o -name "config" -print)

for task in $tasks
do
    striped="${task%/config}"
    striped="${striped#./}"
    ~/vtr/vtr_flow/scripts/run_vtr_task.py $striped
done
