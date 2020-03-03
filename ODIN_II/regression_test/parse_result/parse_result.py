#!/usr/bin/env python3

import os
import sys
# we use regex to parse
import re

# we use config parse and json to read a subset of toml
import configparser
import json

import csv

c_red='\033[31m'
c_grn='\033[32m'
c_org='\033[33m'
c_rst='\033[0m'

COLORIZE=True

def colored(input_str, color):
    if COLORIZE:
        if color == 'red':
            return c_red + input_str + c_rst
        elif color == 'green':
            return c_grn + input_str + c_rst
        elif color == 'orange':
            return c_org + input_str + c_rst
        
    return input_str

#############################################
# for TOML
def preproc_toml(file):
    current_dir=os.getcwd()
    directory = os.path.dirname(file)
    if directory != '':
        os.chdir(directory)

    current_file_as_str = ""
    with open(file) as current_file:
        for line in current_file:
            if line.startswith('#include '):
                # import the next file in line
                next_file = line.strip('#include').strip();
                current_file_as_str += preproc_toml(next_file)
            else:
                current_file_as_str += line


    os.chdir(current_dir)
    return current_file_as_str

def strip_str(value_str):
    value_str.strip()
    if (value_str.startswith("'") and value_str.endswith("'")) \
    or (value_str.startswith('"') and value_str.endswith('"')):
        value_str = value_str[1:-1]

    return value_str.strip()

def parse_toml(toml_str):
    # raw configparse
    parser = configparser.RawConfigParser(
        defaults=None,
        dict_type=dict,
        allow_no_value=False,
        delimiters=('='),
        comment_prefixes=('#'),
        strict=False,
        empty_lines_in_values=False
    )
    parser.read_string(toml_str)
    toml_dict = {}
    for _header in parser.sections():

        header = strip_str(_header)
        if header not in toml_dict:
            toml_dict[header] = {}

        for _key, _value in parser.items(_header):
            key = _key.strip()
            value = _value.strip()
            v_len = len(value)
            value = strip_str(value)
            if len(value) == v_len:
                # print( "Decode str :" + key + " = " + value)
                value = json.loads(value)
            # else:
            #     print( "RAW str :" + key + " = " + value)

            toml_dict[header][key] = value

    return toml_dict

def compile_regex(toml_dict):
    for header in toml_dict:
        if 'regex' in toml_dict[header]:
            toml_dict[header]['regex'] = re.compile(toml_dict[header]['regex'])

def load_toml(toml_file_name):
    toml_str = preproc_toml(os.path.abspath(toml_file_name))
    toml_dict = parse_toml(toml_str)
    compile_regex(toml_dict)
    return toml_dict

###################################
# build initial table from toml
def create_tbl(toml_dict):
    # set the defaults
    input_values = { }
    for header in toml_dict:
        if 'default' in toml_dict[header]:
            input_values[header] = toml_dict[header]['default']

        elif 'default' in toml_dict and toml_dict['default']['default']:
            input_values[header] = toml_dict['default']['default']

        else:
            input_values[header] = 'n/a'

    return input_values

def hash_item(toml_dict, input_line):
    hashing_str = ""
    for header in toml_dict:
        if 'key' in toml_dict[header] and toml_dict[header]['key'] == True:
            hashing_str += input_line[header]

    return " ".join(hashing_str.split())


def print_as_csv(toml_dict, output_dict):
    # dump csv to stdout
    header_line = []
    result_lines = []
    for keys in output_dict:
        result_lines.append( list() )

    for header in toml_dict:
        # figure out the pad
        pad = len(str(header))
        for keys in output_dict:
            if header in output_dict[keys]:
                pad = max(pad, len(str(output_dict[keys][header])))

        header_line.append('{0:<{1}}'.format(header, pad))
        index = 0
        for keys in output_dict:
            result_lines[index].append('{0:<{1}}'.format(output_dict[keys][header], pad))
            index += 1

    # now write everything to the file:
    print(', '.join(header_line))
    for row in result_lines:
        print(', '.join(row))

def load_csv_into_tbl(toml_dict, csv_file_name):
    header = {}
    file_dict = {}
    with open(csv_file_name, newline='') as csvfile:
        is_header = True
        csv_reader = csv.reader(csvfile)
        for row in csv_reader:
            if row is not None and len(row) > 0:
                input_row = create_tbl(toml_dict)
                index = 0
                for element in row:
                    element = " ".join(element.split())
                    if is_header:
                        header[index] = element
                    elif index in header:
                        input_row[header[index]] = element
                    index += 1    

                if not is_header:
                    # insert in table
                    key = hash_item(toml_dict, input_row)
                    file_dict.update({key:input_row}) 

                is_header = False

    return file_dict
_FILLER_LINE=". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ."
def error_line(key):
    output = "  Failed " + _FILLER_LINE
    return colored(output[:38], 'red') + " " + key

def success_line(key):

    output = "  Ok " + _FILLER_LINE
    return colored(output[:38], 'green') + " " + key

def mismatch(header, expected, got):
    header = '{0:<{1}}'.format("- " + header, 38)
    expected = colored("[-" + str(expected) + "-]", 'red')
    got = colored("{+" + str(got) + "+}", 'green')
    return "    " + header + expected + got


def sanity_check(header, toml_dict, golden_tbl, tbl):
    if header not in golden_tbl:
        print("Golden result is missing " + header)
    elif header not in tbl:
        print("Result is missing " + header)  
    elif header not in toml_dict:
        print("Toml is missing " + header)  
    else:
        return True

    return False

def range(header, toml_dict, golden_tbl, tbl):
    if len(toml_dict[header]['range']) != 2:
        print("expected a min and a max for range = [ min, max ]")
    elif golden_tbl[header] != tbl[header]:
        gold_value = float(golden_tbl[header])
        min_ratio = float(toml_dict[header]['range'][0])
        max_ratio = float(toml_dict[header]['range'][1])
        value = float(tbl[header])

        min_range = ( min_ratio * gold_value )
        max_range = ( max_ratio * gold_value )  
        if value < min_range  or value > max_range:
            return mismatch(header, gold_value, value)

    return ''

def abs(header, toml_dict, golden_tbl, tbl):
    if golden_tbl[header] != tbl[header]:
        return mismatch(header, golden_tbl[header], tbl[header])
    
    return ''


def compare_tbl(toml_dict, golden_tbl, tbl):
    error_str=""
    for header in toml_dict:
        output_str = ""
        if sanity_check(header, toml_dict, golden_tbl, tbl):
            # register your function here for different way to compare
            if 'range' in toml_dict[header]:
                output_str = range(header, toml_dict, golden_tbl, tbl)
            else: # absolute
                output_str = abs(header, toml_dict, golden_tbl, tbl)

        if output_str is not None and output_str != "":
            error_str += output_str + "\n"

    # drop the last extra newline
    return error_str[:-1]

def _parse(toml_file_name, log_file_name):
    # load toml
    toml_dict = load_toml(toml_file_name)

    # setup our output dict, print as csv expects a hashed table
    parsed_dict = {}

    #load log file and parse
    with open(log_file_name) as log:

        # set the defaults
        input_values = create_tbl(toml_dict)

        for line in log:
            line = " ".join(line.split())
            for header in toml_dict:
                if 'regex' in toml_dict[header]:
                    entry = re.search(toml_dict[header]['regex'], line)
                    if entry is not None:
                        input_values[header] = str(entry.group(1)).strip()

        key = hash_item(toml_dict, input_values)
        parsed_dict[key] = input_values

    print_as_csv(toml_dict, parsed_dict)

def _join(toml_file_name, file_list):
    # load toml
    toml_dict = load_toml(toml_file_name)
    parsed_files = {}

    for files in file_list:
        parsed_files.update(load_csv_into_tbl(toml_dict, files))

    print_as_csv(toml_dict, parsed_files)

def _compare(toml_file_name, golden_result_file_name, result_file_name):
    # load toml
    failure_count = 0
    failed_key = []
    toml_dict = load_toml(toml_file_name)
    
    golden_parsed_file = load_csv_into_tbl(toml_dict, golden_result_file_name)
    parsed_file = load_csv_into_tbl(toml_dict, result_file_name)

    for key in golden_parsed_file:
        error_str = ""
        if key in parsed_file:
            error_str = compare_tbl(toml_dict, golden_parsed_file[key], parsed_file[key])

        if error_str != "":
            print(error_line(key))
            print(error_str)
            # print to std error
            print(key,file=sys.stderr)
        else:
            print(success_line(key))
    
    return len(failed_key)

def parse(arg):
    if len(arg) == 2:
        _parse(arg[0], arg[1])
    else:
        print("Expected 2 arguments",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def join(arg):
    if len(arg) >= 2:
        return _join(arg[0], arg[1:])
    else:
        print("Expected at least 2 files to join",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def compare(arg):
    if len(arg) == 3:
        return _compare(arg[0], arg[1], arg[2])
    else:
        print("Expected 3 files to do the comparison",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def parse_shared_args(args):
    #pars eout shared arguments
    index = 0
    for arg in args:
        arg = arg.strip()
        args[index] = arg
        if arg is None or arg == "":
            del args[index]
        elif arg == '--no_color':
            global COLORIZE
            COLORIZE=False
            del args[index]

        index += 1

    return args

def main():

    this_exec = sys.argv[0]
    command = sys.argv[1]
    arguments = parse_shared_args(sys.argv[2:])

    exit({
        "parse":    parse,
        "join":     join,
        "compare":  compare,
    }.get(command, lambda: "Invalid Command")\
    (arguments))

if __name__ == "__main__":
    main()
