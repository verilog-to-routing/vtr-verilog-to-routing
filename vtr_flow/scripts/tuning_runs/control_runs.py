#!/usr/bin/env python3

""" This module controls and parses the large runs that includes
sweeping multiple parameters. """
import itertools
import os
import sys
import csv
import pandas as pd
import numpy as np
from scipy import stats

# Define the global dictionary
PARAMS_DICT = {
    "--seed": [1, 2],
    "--place_algorithm": ["criticality_timing"],
    "--place_agent_epsilon": [0.3],
}

# Set to True if you only care about specific metrics
KEEP_METRICS_ONLY = True
PARSED_METRICS = ["num_io", "num_LAB"]


def safe_gmean(series):
    """Calculate the geomeans of a series in a safe way even for large numbers"""
    series = series.replace({0: np.nan})
    return stats.gmean(series.dropna())


def generate_combinations():
    """Generates all the parameter combinations between the input parameters values."""
    keys = list(PARAMS_DICT.keys())
    values = list(PARAMS_DICT.values())
    combinations = list(itertools.product(*values))

    lines = []
    for combination in combinations:
        params_str = " ".join(f"{key} {value}" for key, value in zip(keys, combination))
        lines.append(f"script_params_list_add={params_str}\n")
    return lines


def parse_results(input_path):
    """
    Parse the output results
    """
    # Find the runXXX directory with the largest XXX
    run_dirs = [
        d for d in os.listdir(input_path) if d.startswith("run") and d[3:].isdigit()
    ]
    if not run_dirs:
        print("No runXXX directories found in the specified input path.")
        sys.exit(1)

    largest_run_path = os.path.join(input_path, max(run_dirs, key=lambda d: int(d[3:])))

    # Path to parse_results.txt and full_res.csv
    full_res_csv_path = os.path.join(largest_run_path, "full_res.csv")

    if not os.path.exists(os.path.join(largest_run_path, "parse_results.txt")):
        print("File parse_results.txt not found.")
        sys.exit(1)

    # Read the parse_results.txt file and write to full_res.csv
    with open(
        os.path.join(largest_run_path, "parse_results.txt"), "r"
    ) as txt_file, open(full_res_csv_path, "w", newline="") as csv_file:
        reader = csv.reader(txt_file, delimiter="\t")
        writer = csv.writer(csv_file)

        headers = next(reader)
        script_params_index = headers.index("script_params")

        # Create new headers with PARAMS_DICT keys
        new_headers = (
            headers[:script_params_index]
            + list(PARAMS_DICT.keys())
            + headers[script_params_index + 1 :]
        )
        writer.writerow(new_headers)

        for row in reader:
            script_params_value = row[script_params_index]
            script_params_dict = parse_script_params(script_params_value)
            new_row = (
                row[:script_params_index]
                + [script_params_dict.get(key, "") for key in PARAMS_DICT]
                + row[script_params_index + 1 :]
            )
            writer.writerow(new_row)

    print(f"Converted parse_results.txt to {full_res_csv_path}")

    # Generate avg_seed.csv if --seed column exists
    generate_avg_seed_csv(full_res_csv_path, largest_run_path)
    print("Generated average seed results")

    # Generate gmean_res.csv
    generate_geomean_res_csv(
        os.path.join(largest_run_path, "avg_seed.csv"), largest_run_path
    )
    print("Generated geometric average results over all the circuits")

    generate_xlsx(largest_run_path)
    print("Generated xlsx that merges all the result csv files")


def generate_xlsx(largest_run_path):
    """Generate a xlsx file that includes the full results, average results over the seed
    and the geometrically averaged results over all the benchmarks."""

    csv_files = [
        os.path.join(largest_run_path, "full_res.csv"),
        os.path.join(largest_run_path, "avg_seed.csv"),
        os.path.join(largest_run_path, "geomean_res.csv"),
    ]
    sheet_names = ["Full res", "Avg. seeds", "Summary"]
    output_excel_file = os.path.join(largest_run_path, "summary.xlsx")
    # Create an Excel writer object
    # pylint: disable=abstract-class-instantiated
    with pd.ExcelWriter(output_excel_file, engine="xlsxwriter") as writer:
        for csv_file, sheet_name in zip(csv_files, sheet_names):
            # Read each CSV file
            df = pd.read_csv(csv_file)

            # Write each DataFrame to a different sheet
            df.to_excel(writer, sheet_name=sheet_name, index=False)


def parse_script_params(script_params):
    """Helper function to parse the script params values from earch row in
    the parse_results.txt"""

    parsed_params = {key: "" for key in PARAMS_DICT}

    parts = script_params.split("_")
    i = 0

    while i < len(parts):
        for key in PARAMS_DICT:
            key_parts = key.split("_")
            key_length = len(key_parts)

            if parts[i : i + key_length] == key_parts:
                value_parts = []
                j = i + key_length

                while j < len(parts) and not any(
                    parts[j : j + len(k.split("_"))] == k.split("_")
                    for k in PARAMS_DICT
                ):
                    value_parts.append(parts[j])
                    j += 1

                parsed_params[key] = "_".join(value_parts)
                i = j - 1
                break

        i += 1

    return parsed_params


def generate_avg_seed_csv(full_res_csv_path, output_dir):
    """Generate the average results over the seeds"""
    df = pd.read_csv(full_res_csv_path)

    if KEEP_METRICS_ONLY:
        col_to_keep = ["circuit", "arch"]
        col_to_keep.extend(list(PARAMS_DICT.keys()))
        col_to_keep.extend(PARSED_METRICS)
        df = df.drop(columns=[col for col in df.columns if col not in col_to_keep])

    # Check if '--seed' column is present
    if "--seed" in df.columns:
        # Determine the grouping keys: ['circuit', 'arch'] + keys from PARAMS_DICT that
        # are present in the dataframe
        grouping_keys = ["circuit", "arch"] + [
            key for key in PARAMS_DICT if key in df.columns and key != "--seed"
        ]

        # Group by specified keys and compute the mean for numeric columns
        df_grouped = df.groupby(grouping_keys).mean(numeric_only=True).reset_index()

        # Drop the '--seed' column if it exists
        if "--seed" in df_grouped.columns:
            df_grouped.drop(columns=["--seed"], inplace=True)
    else:
        df_grouped = df

    # Save the resulting dataframe to a CSV file
    avg_seed_csv_path = os.path.join(output_dir, "avg_seed.csv")
    df_grouped.to_csv(avg_seed_csv_path, index=False)


def generate_geomean_res_csv(full_res_csv_path, output_dir):
    """Generate the geometric average results over the different circuits"""

    df = pd.read_csv(full_res_csv_path)

    param_columns = [key for key in PARAMS_DICT if key != "--seed"]
    non_param_columns = [col for col in df.columns if col not in param_columns]

    geomean_df = (
        df.groupby(param_columns)
        .agg(
            {
                col: (lambda x: "" if x.dtype == "object" else safe_gmean(x))
                for col in non_param_columns
            }
        )
        .reset_index()
    )

    geomean_df.drop(columns=["circuit"], inplace=True)
    geomean_df.drop(columns=["arch"], inplace=True)

    geomean_res_csv_path = os.path.join(output_dir, "geomean_res.csv")
    geomean_df.to_csv(geomean_res_csv_path, index=False)


def main():
    """Main function"""

    if len(sys.argv) < 3:
        print("Usage: script.py <option> <path_to_directory>")
        sys.exit(1)

    option = sys.argv[1]
    directory_path = sys.argv[2]

    if option == "--generate":
        # Generate the combinations
        lines = generate_combinations()

        # Define the path to the config file
        config_path = os.path.join(directory_path, "config", "config.txt")

        # Ensure the config directory exists
        os.makedirs(os.path.dirname(config_path), exist_ok=True)

        # Append the lines to the config file
        with open(config_path, "a") as file:
            file.writelines(lines)

        print(f"Appended lines to {config_path}")

    elif option == "--parse":
        parse_results(directory_path)

    else:
        print("Invalid option. Use --generate or --parse")
        sys.exit(1)


if __name__ == "__main__":
    main()
