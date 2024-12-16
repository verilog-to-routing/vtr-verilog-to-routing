"""
Module for flattening the SV design files.
"""

import os
import re

def find_verilog_files():
    """Find all Verilog (.sv, .v) files in the current directory."""
    return [f for f in os.listdir('.') if f.endswith(('.sv', '.v'))]

def identify_top_module(file_list):
    """Identify the file containing the top module definition."""
    top_module_regex = re.compile(r"module\s+top\s*\(")
    for file in file_list:
        with open(file, 'r') as f:
            for line in f:
                if top_module_regex.search(line):
                    return file
    return None

def create_flattened_file(top_file, file_list):
    """Create a flattened Verilog file with all file contents."""
    current_dir = os.path.basename(os.getcwd())
    output_file_name = f"flattened_{current_dir}.sv"

    with open(output_file_name, 'w') as output_file:
        if top_file:
            # Write the top module first
            with open(top_file, 'r') as top_module:
                output_file.write(f"// Content from {top_file}\n")
                output_file.write(top_module.read())
                output_file.write("\n\n")

        # Write the rest of the files
        for file in file_list:
            if file != top_file:
                with open(file, 'r') as verilog_file:
                    output_file.write(f"// Content from {file}\n")
                    output_file.write(verilog_file.read())
                    output_file.write("\n\n")

    print(f"Flattened file created: {output_file_name}")

def main():
    """Main function to generate the flattened Verilog file."""
    print("Searching for Verilog files...")
    verilog_files = find_verilog_files()

    if not verilog_files:
        print("No Verilog files found in the current directory.")
        return

    print("Identifying the top module...")
    top_file = identify_top_module(verilog_files)

    if top_file:
        print(f"Top module found in: {top_file}")
    else:
        print("No top module found. Files will be combined in arbitrary order.")

    print("Creating flattened file...")
    create_flattened_file(top_file, verilog_files)

if __name__ == "__main__":
    main()

