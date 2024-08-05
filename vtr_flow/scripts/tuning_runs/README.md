A script used to run tuning experiments with multiple parameters.

Steps to use:
=============
    1) edit the first section of the script by setting `params_dict` dictionary to the parameters that you want to sweep and the corresponding values that you want to try. If you want the resulting spreadheet to include specific metrics, set `keep_metrics_only` variable to `True` and the metrics that you care about in `parsed_metrics`. If you want the full parsed result sheet, set `keep_metrics_only` to `False`

    2) run the script as follows:
'''
python control_runs.py --generate <path_to_task_to_run>
'''

This will edit the `config.txt` file of this task adding several lines `script_params_list_add` for each of the combinations of the input params

    3) Launch the task using `run_vtr_task.py` script
    4) When the run is done, run the script to parse the results as follows:
'''
python control_runs.py --parse <path_to_task_to_parse>
'''

The script will generate 3 csv files in the runXXX idrectory of the task as follows:
    - `full_res.csv` that exactly matches parse_results.txt but in csv format
    - `avg_seed.csv` that averages the results of the each circuit with one set of parameters over the different seed values
    - `geomean_res.csv` that geometrically average the results of all the circuits over the same set of parameters
    - `summary.xlsx` that merges all the previously mentioned sheets in a single spreadsheet
