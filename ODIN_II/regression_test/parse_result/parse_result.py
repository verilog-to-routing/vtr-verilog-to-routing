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
OUTPUT_CSV=False

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

_K_DFLT='default'
_K_KEY='key'
_K_REGEX='regex'
_K_RANGE='range'
_K_CUTOFF='cutoff'
_K_LISTING='listing'

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

def sanitize_value(value):
	# convert to a number if possible
	if isinstance(value, (int, float)):
		pass
	elif isinstance(value, str):
		# tidy the input
		value = ' '.join(value.split())
		try:
			#try to convert but gracefully fail otherwise
			value = float(value)
			if value % 1.0 == 0.0:
				value = int(value)
		except ValueError:
			pass
	else:
		print("Invalid data type for comparison")
		exit(255)

	return value

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

def setup_2drange_type_var(toml_dict, header, var):
	if var in toml_dict[header]:
		try:
			if isinstance(toml_dict[header][var], (str, float, int) ):
				toml_dict[header][var] = [ float(toml_dict[header][var]) ]
			elif len(toml_dict[header][var]) == 0:
				toml_dict[header][var] = []
			elif len(toml_dict[header][var]) == 1:
				toml_dict[header][var] = [ float(toml_dict[header][var][0]) ]
			else:
				toml_dict[header][var] = [ float(toml_dict[header][var][0]), float(toml_dict[header][var][1]) ]
		except ValueError:
			print("Invalid range in TOML[" + header + "][" + var + "]")
			exit(255)

def sanitize_toml(toml_dict):
	keyed = False
	defaults = None
	if _DFLT_HDR in toml_dict:
		defaults = toml_dict[_DFLT_HDR]
		del toml_dict[_DFLT_HDR]

	for header in toml_dict:
		if _K_REGEX not in toml_dict[header]:
			print("Invalid entry in toml: " + header + ", requires at least a regex = ?")
			exit(255)

		# compile the regex entry
		toml_dict[header][_K_REGEX] = re.compile(toml_dict[header][_K_REGEX])

		# initialize all the key->values to the defaults provided
		if defaults is not None:
			for key in defaults:
				if key not in toml_dict[header]:
					toml_dict[header][key] = sanitize_value(defaults[key])

		if _K_LISTING not in toml_dict[header]:
			toml_dict[header][_K_LISTING] = False

		if _K_DFLT not in toml_dict[header]:
			if toml_dict[header][_K_LISTING]:
				toml_dict[header][_K_DFLT] = []
			else:
				toml_dict[header][_K_DFLT] = "n/a"

		# deal with cutoffs and adjust format to standardize the results
		setup_2drange_type_var(toml_dict, header, _K_CUTOFF)

		# deal with ranges if we have some
		setup_2drange_type_var(toml_dict, header, _K_RANGE)

		if _K_KEY not in toml_dict[header]:
			toml_dict[header][_K_KEY] = False

		if toml_dict[header][_K_KEY] :
			keyed = True

	if not keyed:
		print("Your toml has no keys, please add a key = ? to allow this tool to sort the items")
		exit(255)

def load_toml(toml_file_name):
	toml_str = preproc_toml(os.path.abspath(toml_file_name))
	toml_dict = parse_toml(toml_str)
	sanitize_toml(toml_dict)
	return toml_dict

###################################
# build initial table from toml
def create_tbl(toml_dict):
	# set the defaults
	input_values = OrderedDict()
	for header in toml_dict:
		input_values[header] = toml_dict[header][_K_DFLT]

	return input_values

def hash_item(toml_dict, input_line):
	keyed = []
	for header in toml_dict:
		if toml_dict[header][_K_KEY] and header in input_line:
			if toml_dict[header][_K_LISTING]:
				keyed += input_line[header]
			else:
				keyed.append(input_line[header])

	if len(keyed) == 0:
		print("FATAL_ERROR == Unable to key " + header)
		exit(255)

	return " ".join(keyed)


def print_as_csv(toml_dict, output_dict, file=sys.stdout):
	# dump csv to stdout
	header_line = []
	result_lines = []
	for keys in output_dict:
		result_lines.append( list() )

	for header in toml_dict:
		# skip listings
		if toml_dict[header][_K_LISTING]:
			continue

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

def pretty_print_tbl(toml_dict, output_dict, file=sys.stdout):
	if OUTPUT_CSV:
		print_as_csv(toml_dict, output_dict, file=file)
	else:
		print(json.dumps(output_dict, indent = 4), file=file)

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
						input_row[header[index]] = sanitize_value(element)
					index += 1

				if not is_header:
					# insert in table
					key = hash_item(toml_dict, input_row)
					file_dict.update({key:input_row})

				is_header = False

	return file_dict

def load_json_into_tbl(toml_dict, file_name):
	file_dict = OrderedDict()
	with open(file_name, newline='') as json_file:
		file_dict = json.load(json_file,object_pairs_hook=OrderedDict)
	return file_dict

def load_into_tbl(toml_dict, file_name):
	if file_name.endswith('.csv'):
		return load_csv_into_tbl(toml_dict, file_name)
	elif file_name.endswith('.json'):
		return load_json_into_tbl(toml_dict, file_name)
	else:
		print(
			"Unsupported file extension for file " + file_name + "\n" +
			"please use .csv or .json"
		)
		return None

def range(header, toml_dict, value, golden_value):
	# if there is no range, then defaut is that it
	is_within_range = True

	if _K_CUTOFF in toml_dict[header]:
		if len(toml_dict[header][_K_CUTOFF]) > 0:
			low_cutoff = float(toml_dict[header][_K_CUTOFF][0])
			value =      max(low_cutoff, value)
			golden_value = max(low_cutoff, golden_value)

		if len(toml_dict[header][_K_CUTOFF]) > 1:
			high_cutoff = float(toml_dict[header][_K_CUTOFF][1])
			value =      max(high_cutoff, value)
			golden_value = max(high_cutoff, golden_value)

	# minimum range
	if is_within_range and len(toml_dict[header][_K_RANGE]) > 0:
		min_ratio = float(toml_dict[header][_K_RANGE][0])
		if golden_value < 0:
			is_within_range = ( value <= ( min_ratio * golden_value ) )
		else:
			is_within_range = ( value >= ( min_ratio * golden_value ) )

	# maximum range
	if is_within_range and len(toml_dict[header][_K_RANGE]) > 1:
		max_ratio = float(toml_dict[header][_K_RANGE][1])
		if golden_value < 0:
			is_within_range = ( value >= ( max_ratio * golden_value ) )
		else:
			is_within_range = ( value <= ( max_ratio * golden_value ) )

	return is_within_range


def abs(header, toml_dict, value, golden_value):
	# should we loosen this rule with more leanient match? i.e. case sentitivity
	return value == golden_value

def compare_values(header, toml_dict, value, golden_value):
	# convert to a number if possible
	value = sanitize_value(value)
	golden_value = sanitize_value(golden_value)

	# register your function here for different way to compare
	if _K_RANGE in toml_dict[header]:
		if isinstance(value, (float, int)):
			return range(header, toml_dict, value, golden_value)		
		else:
			return False
	else: # absolute
		return abs(header, toml_dict, value, golden_value)

def compare_instances(header, toml_dict, tbl_entry, golden_tbl_entry):
	if header not in golden_tbl_entry and header not in tbl_entry:
		return True
	elif toml_dict[header][_K_LISTING]:
		# make sure we have the same number of items
		if ( len(tbl_entry[header]) != len(golden_tbl_entry[header]) ):
			return False

		for (value, golden_value) in zip(tbl_entry[header], golden_tbl_entry[header]):
			if not compare_values(header, toml_dict, value, golden_value):
				return False
		
		return True
	else:
		return compare_values(header, toml_dict, tbl_entry[header], golden_tbl_entry[header])


def regex_line(toml_dict, header, line):
	entry = re.match(toml_dict[header][_K_REGEX], line)
	entry_list = []
	entry_str = ""

	if entry is not None:
		entry_list = entry.groups()

	if len(entry_list) > 0:
		# collapse list into a single string
		entry_str = ' '.join(entry_list)

	# sanitize whitespace
	return sanitize_value(entry_str)

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
				value = regex_line(toml_dict, header, line)
				if value != "":
					# append all the finds to the list
					if toml_dict[header][_K_LISTING]:
						input_values[header].append(value)
					# keep overriding
					else:
						input_values[header] = value


		# fill null entries with default values
		for header in input_values:
			if input_values is None:
				input_values[header] = toml_dict[header][_K_DFLT]

		# make a key from the user desired key items
		key = hash_item(toml_dict, input_values)
		parsed_dict[key] = input_values

	pretty_print_tbl(toml_dict, parsed_dict)

def _join(toml_file_name, file_list):
	# load toml
	toml_dict = load_toml(toml_file_name)
	parsed_files = OrderedDict()

	for files in file_list:
		parsed_files.update(load_into_tbl(toml_dict, files))

	pretty_print_tbl(toml_dict, parsed_files)

def _compare(toml_file_name, golden_result_file_name, result_file_name, diff_file_name):
	# load toml
	failed_key = []
	toml_dict = load_toml(toml_file_name)
	golden_tbl = load_into_tbl(toml_dict, golden_result_file_name)
	tbl = load_into_tbl(toml_dict, result_file_name)
	diff = OrderedDict()

	for key in golden_tbl:
		error_str = []
		for header in toml_dict:

			if key not in tbl:
				if header in golden_tbl[key]:
					# if we are only running partial tests, key will not be existing
					if SUBSET:
						if key not in diff:
							diff[key] = OrderedDict()
						diff[key][header] = golden_tbl[key][header]
					# otherwise trigger all warning
					else:
						error_str.append(mismatch_str(header, golden_tbl[key][header], "null"))
						# don't create the entry
			else:
				if key not in diff:
					diff[key] = OrderedDict()

				if header not in golden_tbl[key] and header not in tbl[key]:
					# if both don't have the entry, then its a match
					pass

				elif header not in golden_tbl[key] and header in tbl[key]:
					error_str.append(mismatch_str(header, "null", tbl[key][header]))
					diff[key][header] = tbl[key][header]

				elif header in golden_tbl[key]  and header not in tbl[key]:
					error_str.append(mismatch_str(header, golden_tbl[key][header], "null"))
					# don't create the entry

				else:

					if compare_instances(header, toml_dict, tbl[key],golden_tbl[key]):
						# use the golden value since it is within range and we don't wanna trigger a diff
						diff[key][header] = golden_tbl[key][header]
					else:
						error_str.append(mismatch_str(header, golden_tbl[key][header], tbl[key][header]))
						diff[key][header] = tbl[key][header]

		if len(error_str):
			print(error_line(key))
			print("\n".join(error_str))
			# print to std error
			print(key,file=sys.stderr)
			failed_key.append(key)

		else:
			# don't display unran subtest
			if key not in tbl and SUBSET:
				pass
			else:
				print(success_line(key))

	# add the new entries
	for key in tbl:
		if key not in diff:
			diff[key] = OrderedDict()
			error_str = []
			for header in toml_dict:
				if header in tbl[key]:
					error_str.append(new_entry_str(header, tbl[key][header]))
					diff[key][header] = tbl[key][header]

			if len(error_str):
				print(new_entry_line(key))
				print("\n".join(error_str))
				# print to std error
				print(key,file=sys.stderr)
				failed_key.append(key)

	with open(diff_file_name, 'w+') as diff_file:
		pretty_print_tbl(toml_dict, diff, file=diff_file)

	return len(failed_key)

def display(arg):
	if len(arg) == 2:
		return _join(arg[0], arg[1:])
	else:
		print("Expected 2 arguments <conf> <file to display>",file=sys.stderr)
		print(arg,file=sys.stderr)
		return -1

def parse(arg):
	if len(arg) == 2:
		return _parse(arg[0], arg[1])
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
			elif arg == '--csv':
				global OUTPUT_CSV
				OUTPUT_CSV=True
			else:
				clean_args.append(arg)

	return clean_args

def main():

	this_exec = sys.argv[0]
	if len(sys.argv) < 2:
		print("expected: display, parse, join or compare")
		exit(255)
	else:
		command = sys.argv[1]
		arguments = parse_shared_args(sys.argv[2:])

		exit({
			"display":  display,
			"parse":    parse,
			"join":     join,
			"compare":  compare,
		}.get(command, lambda: "Invalid Command")\
		(arguments))

if __name__ == "__main__":
	main()
