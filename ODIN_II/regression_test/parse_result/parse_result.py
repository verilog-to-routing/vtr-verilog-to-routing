#!/usr/bin/env python3

import os
import sys
from collections import OrderedDict 
# we use regex to parse
import re

# we use config parse and json to read a subset of toml
import configparser
import json

import csv
import math

c_red='\033[31m'
c_grn='\033[32m'
c_org='\033[33m'
c_rst='\033[0m'

COLORIZE=True
SUBSET=False

_FILLER_LINE=". . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . ."
_LEN=38

def colored(input_str, color):
    if COLORIZE:
        if color == 'red':
            return c_red + input_str + c_rst
        elif color == 'green':
            return c_grn + input_str + c_rst
        elif color == 'orange':
            return c_org + input_str + c_rst
        
    return input_str

def error_line(key):
    output = "  Failed " + _FILLER_LINE
    return colored(output[:_LEN], 'red') + " " + key

def new_entry_line(key):
    output = "  Added  " + _FILLER_LINE
    return colored(output[:_LEN], 'red') + " " + key

def new_entry_str(header, got):
    header = '{0:<{1}}'.format("- " + header, _LEN)
    got = colored("{+" + str(got) + "+}", 'green')
    return "    " + header + got


def success_line(key):
    output = "  Ok " + _FILLER_LINE
    return colored(output[:_LEN], 'green') + " " + key

def mismatch_str(header, expected, got):
    header = '{0:<{1}}'.format("- " + header, _LEN)
    expected = colored("[-" + str(expected) + "-]", 'red')
    got = colored("{+" + str(got) + "+}", 'green')
    return "    " + header + expected + got

def is_json_str(value):
    value = value.strip()

    return (value.startswith("'") and value.endswith("'")) or \
             (value.startswith('"') and value.endswith('"'))

#############################################
# for TOML
_DFLT_HDR='DEFAULT'
_DFLT_VALUE='n/a'

_K_DFLT='default'
_K_KEY='key'
_K_REGEX='regex'
_K_RANGE='range'
_K_CUTOFF='cutoff'

def preproc_toml(file):
    current_dir = os.getcwd()
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
    while (value_str.startswith("'") and value_str.endswith("'")) \
    or (value_str.startswith('"') and value_str.endswith('"')):
        value_str = value_str[1:-1]
        value_str.strip()

    return value_str

def parse_toml(toml_str):
    # raw configparse
    parser = configparser.RawConfigParser(
        defaults=None,
        dict_type=OrderedDict,
        allow_no_value=False,
        delimiters=('='),
        comment_prefixes=('#'),
        strict=False,
        empty_lines_in_values=False
    )
    parser.read_string(toml_str)
    # default is always there
    toml_dict = OrderedDict()
    toml_dict[_DFLT_HDR] = { _K_DFLT: _DFLT_VALUE}
    for _header in parser.sections():

        header = strip_str(_header)
        if header not in toml_dict:
            toml_dict[header] = OrderedDict()

        for _key, _value in parser.items(_header):
            # drop extra whitespace
            key = strip_str(_key)

            if not is_json_str(_value):
                toml_dict[header][key] = json.loads(_value)
            else:
                toml_dict[header][key] = strip_str(_value)

    return toml_dict

def compile_regex(toml_dict):
    for header in toml_dict:
        if header != _DFLT_HDR:
            if _K_REGEX in toml_dict[header]:
                toml_dict[header][_K_REGEX] = re.compile(toml_dict[header][_K_REGEX])

def load_toml(toml_file_name):
    toml_str = preproc_toml(os.path.abspath(toml_file_name))
    toml_dict = parse_toml(toml_str)
    compile_regex(toml_dict)
    return toml_dict

###################################
# build initial table from toml
def create_tbl(toml_dict):
    # set the defaults
    input_values = OrderedDict()
    for header in toml_dict:
        if header != _DFLT_HDR:
            # initiate with fallback
            value = _DFLT_VALUE

            if _K_DFLT in toml_dict[header]:
                value = toml_dict[header][_K_DFLT]

            elif _K_DFLT in toml_dict[_DFLT_HDR]:
                value = toml_dict[_DFLT_HDR][_K_DFLT]

            input_values[header] = value

    return input_values

def hash_item(toml_dict, input_line):
    hashing_str = ""
    for header in toml_dict:
        if header != _DFLT_HDR:
            if _K_KEY in toml_dict[header] and toml_dict[header][_K_KEY] == True:
                hashing_str += input_line[header]

    return " ".join(hashing_str.split())


def print_as_csv(toml_dict, output_dict, file=sys.stdout):
    # dump csv to stdout
    header_line = []
    result_lines = []
    for keys in output_dict:
        result_lines.append( list() )

    for header in toml_dict:
        if header != _DFLT_HDR:
            # figure out the pad
            pad = len(str(header).strip())
            for keys in output_dict:
                if header in output_dict[keys]:
                    pad = max(pad, len(str(output_dict[keys][header]).strip()))

            header_line.append('{0:<{1}}'.format(header, pad))
            index = 0
            for keys in output_dict:
                if header in output_dict[keys]:
                    result_lines[index].append('{0:<{1}}'.format(output_dict[keys][header], pad))
                    index += 1

    # now write everything to the file:
    print(', '.join(header_line), file=file)
    for row in result_lines:
        print(', '.join(row), file=file)

def load_csv_into_tbl(toml_dict, csv_file_name):
    header = OrderedDict()
    file_dict = OrderedDict()
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
    mismatch = False
    if len(toml_dict[header][_K_RANGE]) != 2:
        print("expected a min and a max for range = [ min, max ]")
        exit(1)
    elif golden_tbl[header] != tbl[header]:

        low_cutoff = None
        if _K_CUTOFF in toml_dict[header]:
            low_cutoff = toml_dict[header][_K_CUTOFF]

        value = float(tbl[header])
        gold_value = float(golden_tbl[header])
        if low_cutoff is not None:
            if value < low_cutoff:
                value = low_cutoff
            if gold_value < low_cutoff:
                gold_value = low_cutoff

        min_ratio = float(toml_dict[header][_K_RANGE][0])
        max_ratio = float(toml_dict[header][_K_RANGE][1])
        min_range = ( min_ratio * gold_value )
        max_range = ( max_ratio * gold_value )  
        if value < min_range  or value > max_range:
            mismatch = True

    return mismatch

def abs(header, toml_dict, golden_tbl, tbl):
    return golden_tbl[header] != tbl[header];

def _parse(toml_file_name, log_file_name):
    # load toml
    toml_dict = load_toml(toml_file_name)

    # setup our output dict, print as csv expects a hashed table
    parsed_dict = OrderedDict()

    #load log file and parse
    with open(log_file_name) as log:
        # set the defaults
        input_values = create_tbl(toml_dict)

        for line in log:
            line = " ".join(line.split())
            for header in toml_dict:
                if header != _DFLT_HDR:
                    if _K_REGEX in toml_dict[header]:
                        entry = re.search(toml_dict[header][_K_REGEX], line)
                        if entry is not None:
                            input_values[header] = str(entry.group(1)).strip()

        key = hash_item(toml_dict, input_values)
        parsed_dict[key] = input_values

    print_as_csv(toml_dict, parsed_dict)

def _join(toml_file_name, file_list):
    # load toml
    toml_dict = load_toml(toml_file_name)
    parsed_files = OrderedDict()

    for files in file_list:
        parsed_files.update(load_csv_into_tbl(toml_dict, files))

    print_as_csv(toml_dict, parsed_files)

def _compare(toml_file_name, golden_result_file_name, result_file_name, diff_file_name):
    # load toml
    failed_key = []
    toml_dict = load_toml(toml_file_name)
    golden_tbl = load_csv_into_tbl(toml_dict, golden_result_file_name)
    tbl = load_csv_into_tbl(toml_dict, result_file_name)
    diff = OrderedDict()

    for key in golden_tbl:
        diff[key] = OrderedDict()

        error_str = []
        if key not in tbl:
            if SUBSET:
                for header in toml_dict:
                    if header != _DFLT_HDR:
                        diff[key][header] = golden_tbl[key][header]
            else:
                print(error_line(key))
                print(key,file=sys.stderr)
        else:
            for header in toml_dict:
                if header != _DFLT_HDR:
                    mismatch = True
                    if sanity_check(header, toml_dict, golden_tbl[key], tbl[key]):
                        # register your function here for different way to compare
                        if _K_RANGE in toml_dict[header]:
                            mismatch = range(header, toml_dict, golden_tbl[key], tbl[key])
                        else: # absolute
                            mismatch = abs(header, toml_dict, golden_tbl[key], tbl[key])

                    if mismatch:
                        error_str.append(mismatch_str(header, golden_tbl[key][header], tbl[key][header]))
                        diff[key][header] = tbl[key][header]
                    else:
                        diff[key][header] = golden_tbl[key][header]

            if len(error_str):
                print(error_line(key))
                print("\n".join(error_str))
                # print to std error
                print(key,file=sys.stderr)
            else:
                print(success_line(key))

    # add the new entries
    for key in tbl:
        if key not in golden_tbl:

            diff[key] = OrderedDict()
            error_str = []
            for header in toml_dict:
                if header != _DFLT_HDR:
                    error_str.append(new_entry_str(header, tbl[key][header]))
                    diff[key][header] = tbl[key][header]

            if len(error_str):
                print(new_entry_line(key))
                print("\n".join(error_str))

    with open(diff_file_name, 'w+') as diff_file:
        print_as_csv(toml_dict, diff, file=diff_file)

    return len(failed_key)

def parse(arg):
    if len(arg) == 2:
        _parse(arg[0], arg[1])
    else:
        print("Expected 2 arguments <conf> <file to parse>",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def join(arg):
    if len(arg) >= 2:
        return _join(arg[0], arg[1:])
    else:
        print("Expected at least 2 files to join <conf> <files ...>",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def compare(arg):
    if len(arg) == 4:
        return _compare(arg[0], arg[1], arg[2], arg[3])
    else:
        print("Expected 4 files to do the comparison <conf> <golden> <result> <diff>",file=sys.stderr)
        print(arg,file=sys.stderr)
        return -1

def parse_shared_args(args):
    #pars eout shared arguments
    clean_args = []
    for arg in args:
        if arg is not None and arg != "":
            arg = arg.strip()
            if arg == '--no_color':
                global COLORIZE
                COLORIZE=False
            elif arg == '--subset':
                global SUBSET
                SUBSET=True
            else:
                clean_args.append(arg)

    return clean_args

def main():

    this_exec = sys.argv[0]
    if len(sys.argv) < 2:
        print("expected: parse, join or compare")
        exit(255)
    else:
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
