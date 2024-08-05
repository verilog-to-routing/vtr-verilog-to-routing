import itertools
import os
import sys
import csv
import pandas as pd
import numpy as np
from scipy import stats

# Define the global dictionary
params_dict = {
    "--seed": [1, 2],
    "--place_algorithm": ["criticality_timing"],
    "--place_agent_epsilon": [0.3]
}

# Set to True if you only care about specific metrics
keep_metrics_only = True
parsed_metrics = ["num_io", "num_LAB"]


def safe_gmean(series):
    series = series.replace({0: np.nan})
    return stats.gmean(series.dropna())

def generate_combinations(params_dict):
    keys = list(params_dict.keys())
    values = list(params_dict.values())
    combinations = list(itertools.product(*values))
    
    lines = []
    for combination in combinations:
        params_str = ' '.join(f"{key} {value}" for key, value in zip(keys, combination))
        lines.append(f"script_params_list_add={params_str}\n")
    return lines

def parse_results(input_path, params_dict):
    # Find the runXXX directory with the largest XXX
    run_dirs = [d for d in os.listdir(input_path) if d.startswith("run") and d[3:].isdigit()]
    if not run_dirs:
        print("No runXXX directories found in the specified input path.")
        sys.exit(1)
    
    largest_run_dir = max(run_dirs, key=lambda d: int(d[3:]))
    largest_run_path = os.path.join(input_path, largest_run_dir)
    
    # Path to parse_results.txt and full_res.csv
    parse_results_path = os.path.join(largest_run_path, "parse_results.txt")
    full_res_csv_path = os.path.join(largest_run_path, "full_res.csv")
    
    if not os.path.exists(parse_results_path):
        print(f"{parse_results_path} not found.")
        sys.exit(1)
    
    # Read the parse_results.txt file and write to full_res.csv
    with open(parse_results_path, "r") as txt_file, open(full_res_csv_path, "w", newline='') as csv_file:
        reader = csv.reader(txt_file, delimiter='\t')
        writer = csv.writer(csv_file)
        
        headers = next(reader)
        script_params_index = headers.index("script_params")
        
        # Create new headers with params_dict keys
        new_headers = headers[:script_params_index] + list(params_dict.keys()) + headers[script_params_index + 1:]
        writer.writerow(new_headers)
        
        for row in reader:
            script_params_value = row[script_params_index]
            script_params_dict = parse_script_params(script_params_value, params_dict)
            new_row = row[:script_params_index] + [script_params_dict.get(key, '') for key in params_dict.keys()] + row[script_params_index + 1:]
            writer.writerow(new_row)
    
    print(f"Converted {parse_results_path} to {full_res_csv_path}")
    
    # Generate avg_seed.csv if --seed column exists
    generate_avg_seed_csv(full_res_csv_path, largest_run_path)
    print(f"Generated average seed results")

    # Generate gmean_res.csv 
    generate_geomean_res_csv(os.path.join(largest_run_path, "avg_seed.csv"), largest_run_path, params_dict)
    print(f"Generated geometric average results over all the circuits")

    generate_xlsx(largest_run_path)
    print(f"Generated xlsx that merges all the result csv files")

def generate_xlsx(largest_run_path):
    csv_files = [os.path.join(largest_run_path, "full_res.csv"),
                 os.path.join(largest_run_path, "avg_seed.csv"),
                 os.path.join(largest_run_path, "geomean_res.csv")]
    sheet_names = ["Full res", "Avg. seeds", "Summary"]
    output_excel_file = os.path.join(largest_run_path, "summary.xlsx") 
    # Create an Excel writer object
    with pd.ExcelWriter(output_excel_file) as writer:
        for csv_file, sheet_name in zip(csv_files, sheet_names):
            # Read each CSV file
            df = pd.read_csv(csv_file)
    
            # Write each DataFrame to a different sheet
            df.to_excel(writer, sheet_name=sheet_name, index=False)
   
def parse_script_params(script_params, params_dict):
    parsed_params = {key: '' for key in params_dict.keys()}
    
    parts = script_params.split('_')
    i = 0
    
    while i < len(parts):
        for key in params_dict.keys():
            key_parts = key.split('_')
            key_length = len(key_parts)
            
            if parts[i:i+key_length] == key_parts:
                value_parts = []
                j = i + key_length
                
                while j < len(parts) and not any(parts[j:j+len(k.split('_'))] == k.split('_') for k in params_dict.keys()):
                    value_parts.append(parts[j])
                    j += 1
                
                parsed_params[key] = '_'.join(value_parts)
                i = j - 1
                break
        
        i += 1

    return parsed_params

def generate_avg_seed_csv(full_res_csv_path, output_dir):
    
    df = pd.read_csv(full_res_csv_path)

    if keep_metrics_only:
        col_to_keep = ['circuit', 'arch']
        col_to_keep.extend(list(params_dict.keys()))
        col_to_keep.extend(parsed_metrics)
        df = df.drop(columns=[col for col in df.columns if col not in col_to_keep])

    # Check if '--seed' column is present
    if '--seed' in df.columns:
        # Determine the grouping keys: ['circuit', 'arch'] + keys from params_dict that are present in the dataframe
        grouping_keys = ['circuit', 'arch'] + [key for key in params_dict.keys() if key in df.columns and key != "--seed"]
        
        # Group by specified keys and compute the mean for numeric columns
        df_grouped = df.groupby(grouping_keys).mean(numeric_only=True).reset_index()
        
        # Drop the '--seed' column if it exists
        if '--seed' in df_grouped.columns:
            df_grouped.drop(columns=['--seed'], inplace=True)
    else:
        df_grouped = df

    # Save the resulting dataframe to a CSV file
    avg_seed_csv_path = os.path.join(output_dir, "avg_seed.csv")
    df_grouped.to_csv(avg_seed_csv_path, index=False)

def generate_geomean_res_csv(full_res_csv_path, output_dir, params_dict):
    df = pd.read_csv(full_res_csv_path)

    param_columns = [key for key in params_dict.keys() if key != '--seed']
    non_param_columns = [col for col in df.columns if col not in param_columns]

    geomean_df = df.groupby(param_columns).agg(
        {col: (lambda x: '' if x.dtype == 'object' else safe_gmean(x)) for col in non_param_columns}
    ).reset_index()

    geomean_df.drop(columns=['circuit'], inplace=True)
    geomean_df.drop(columns=['arch'], inplace=True)

    geomean_res_csv_path = os.path.join(output_dir, "geomean_res.csv")
    geomean_df.to_csv(geomean_res_csv_path, index=False)

def main():
    if len(sys.argv) < 3:
        print("Usage: script.py <option> <path_to_directory>")
        sys.exit(1)
    
    option = sys.argv[1]
    directory_path = sys.argv[2]
    
    if option == "--generate":
        # Generate the combinations
        lines = generate_combinations(params_dict)
        
        # Define the path to the config file
        config_path = os.path.join(directory_path, "config", "config.txt")
        
        # Ensure the config directory exists
        os.makedirs(os.path.dirname(config_path), exist_ok=True)
        
        # Append the lines to the config file
        with open(config_path, "a") as file:
            file.writelines(lines)
        
        print(f"Appended lines to {config_path}")
    
    elif option == "--parse":
        parse_results(directory_path, params_dict)
    
    else:
        print("Invalid option. Use --generate or --parse")
        sys.exit(1)

if __name__ == "__main__":
    main()

