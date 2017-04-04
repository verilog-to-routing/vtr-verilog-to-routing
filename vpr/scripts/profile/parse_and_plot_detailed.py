#!/usr/bin/python
# parse congestion and time per iteration for a given benchmark and output file (defaults to vpr.out)
from __future__ import print_function, division
import sys
from subprocess import call
import os
import re
import shlex
import argparse
import textwrap
import shutil
import csv
from datetime import datetime
# generate images wihtout having a window appear
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
sys.path.append(os.path.expanduser("~/benchtracker/"))
from util import (get_trailing_num, immediate_subdir, natural_sort)


NO_PARSE = 1
NO_OUTPUT = 2

def main():
    params = Params()
    parse_args(params)

    (param_names, param_options) = read_parse_file(params)

    results = parse_output(param_names, params)

    plot_results(param_names, param_options, results, params)


def plot_results(param_names, param_options, results, params):
	"""Create a directory based on key parameters and date and plot results vs iteration

	Each of the parameter in results will receive its own plot, drawn by matplotlib"""

	# circuit/run_num where run_num is one before the existing one
	directory = params.circuit
	if not os.path.isdir(directory):
		os.mkdir(directory)
	runs = immediate_subdir(directory)
	latest_run = 0
	if runs:
		natural_sort(runs)
		latest_run = get_trailing_num(runs[-1])
	directory = os.path.join(directory, "run" + str(latest_run+1))

	print(directory)
	if not os.path.isdir(directory):
		os.mkdir(directory)

	with Chdir(directory):

		export_results_to_csv(param_names, results, params)

		x = results.keys()
		y = []
		next_figure = True

		p = 0
		plt.figure()
		while p < len(param_names):
			print(param_names[p])

			if param_options[p]:
				nf = True
				for option in param_options[p].split():
					# stopping has veto power (must all be True to pass)
					nf = nf and plot_options(option)
				next_figure = nf

			if not next_figure:
				# y becomes list of lists (for use with stackable plots)
				y.append([result[p] for result in results.values()])
				p += 1
				continue
			elif not y:
				y = [result[p] for result in results.values()]

			lx = x[-1]
			ly = y[-1]
			plot_method(x,y)
			plt.xlabel('iteration')
			plt.xlim(xmin=0)
			
			plt.ylabel(param_names[p])

			# annotate the last value
			annotate_last(lx,ly)

			if next_figure:
				plt.savefig(param_names[p])
				plt.figure()

			p += 1
		# in case the last figure hasn't been shuffled onto file yet
		if not next_figure:
			plot_method(x,y)
			plt.savefig(param_names[-1])

# can be changed by parameter options (always dump y after plotting)
def default_plot(x,y):
	plt.plot(x,y)
	del y[:]

plot_method = default_plot

def stack_plot(x,y):
	global plot_method
	plt.stackplot(x,y)
	del y[:]
	plot_method = default_plot


# each plot option returns true or false on whether its ready for the next figure
def plot_stack():
	global plot_method
	plot_method = stack_plot
	return False

def plot_log():
	plt.semilogy(nonposy='mask')
	return True

def plot_percent():
	plt.ylim(0,100)
	return True

plot_opts = {
	'stackplot':plot_stack,
	'log':plot_log,
	'percent':plot_percent
}

def plot_options(option):
	print(option)
	option_handler = plot_opts.get(option, None)
	if option_handler:
		return option_handler()
	return True

def annotate_last(lx, ly):
	if type(ly) == list:
		return
	plt.plot([lx,lx], [0, ly], linestyle="--")
	plt.scatter([lx,],[ly,], 50, color='black')

	plt.annotate('({},{})'.format(lx, ly),
		xy=(lx, ly), xytext=(0,15), textcoords='offset points')

def export_results_to_csv(param_names, results, params):
	param_values = {}
	param_values["circuit"] = params.circuit
	param_values["channel_width"] = params.channel_width
	with open(params.output_file, 'r') as out:
		for regex in params.param_regex:
			match = re.search(regex, out.read())
			if match:
				param_values[regex.split(':')[0]] = match.group(1)
				print("matched:", match.group(1))

	for param,val in param_values.items():
		print("{}: {}".format(param, val))
	# also move the output file, place file, and pack file
	shutil.copy(params.output_file, os.path.basename(params.output_file))
	shutil.copy(params.pack_file, os.path.basename(params.pack_file))
	shutil.copy(params.place_file, os.path.basename(params.place_file))

	with open('results.csv', 'wb') as csvfile:
		writer = csv.writer(csvfile)
		# header line
		writer.writerow(['iteration'] + param_names + param_values.keys())
		for iteration, result in results.items():
			writer.writerow([iteration] + result + param_values.values())

def parse_output(param_names, params):
	"""Return a dictionary mapping iteration -> [params...] 
	
	The index of the params list corresponds to the index of param_names
	assuming consecutively matched groups"""
	if not os.path.isfile(params.output_file):
		print("output file does not exist! ({})".format(params.output_file))
		sys.exit(NO_OUTPUT)

	results = {}

	with open(params.output_file, 'r') as out:
		iteration_matches = re.findall(params.iteration_regex, out.read())
		for iteration_match in iteration_matches:
			iteration = int(iteration_match[0])
			results[iteration] = [float(iteration_match[m]) for m in range(1,len(iteration_match))]

	return results


def read_parse_file(params):
	"""Read the parameters to match and their options"""
	param_names = []
	param_options = []
	if not os.path.isfile(params.parse_file):
		print("parse file does not exist! ({})".format(params.parse_file))
		sys.exit(NO_PARSE)
	with open(params.parse_file, 'r') as pf:
		# first line should be iteration regex
		setattr(params, 'iteration_regex', re.compile(pf.readline().strip()))
		for line in pf:
			param_desc = line.split(';')
			param_names.append(param_desc[0])
			param_options.append(param_desc[1])

	return param_names,param_options


# automatic return and exception safe
class Chdir:
	def __init__(self, new_path):
		self.prev_path = os.getcwd()
		self.new_path = new_path

	def __enter__(self):
		os.chdir(self.new_path)

	def __exit__(self, type, value, traceback):
		os.chdir(self.prev_path)


class Params(object):
    def __iter__(self):
        for attr, value in self.__dict__.iteritems():
            yield attr, value

def parse_args(ns=None):
    """parse arguments from command line and return as namespace object"""
    parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=textwrap.dedent("""\
            parse parameters per iteration for a given benchmark and plots them
            
        Benchmark:
            This script is designed for VPR, so circuit and channel_width of a benchmark are required.
            The circuit name can be a summary or short name rather than the full name.

        Parse file:
        	The first line should describe how to match iteration, such as:
        		'(\d+)  (.*) sec (.*) ns'
        	and the first group will be the matched value of iteration.

        	Each following line in this file should describe 1 parameter to parse as:
        	<parameter_name>;<options>
        	for example,
        		'time (s);log'
        	on the second line, then with the previous iteration regex, it would match the value before 'sec'
        	The parameter order should match the group order that they are supposed to match

        Parameter Options:
        	log - make the y-axis log scale for this parameter
        	"""),
            usage="%(prog)s <output_file> <circuit> <channel_width> [OPTIONS]")

    # arguments should either end with _dir or _file for use by other functions
    parser.add_argument("output_file", 
            default="vpr.out",
            help="output file to parse; default: %(default)s")
    parser.add_argument("-p", "--parse_file",
            default="nocongestion_parse.txt",
            help="config file where each line describes 1 parameter to parse as:\
            	<parameter_name>;<regex_to_match>\
                default: %(default)s")
    parser.add_argument("circuit",
    		help="titan circuit to be parsed, similar to task")
    parser.add_argument("channel_width",
    		type=int,
    		help="channel used to route circuit")
    parser.add_argument("--architecture",
    		default="stratixiv_arch_timing",
    		help="architecture used to map circuit to; default: %(default)s")
    parser.add_argument("--resfile_dir",
    		default="titan",
    		help="directory (relative or absolute) of where result files (.pack, .place, .route) are kept")
    parser.add_argument("-r","--param_regex",            
    		nargs='+',
            default=[],
            help="additional regular expressions to match on the result file")
    params = parser.parse_args(namespace=ns)
    params.output_file = os.path.abspath(params.output_file)

    resfile_name = os.path.join(params.resfile_dir, '_'.join((params.circuit, params.architecture)))
    print("result file base name: ", resfile_name);

    setattr(params, 'pack_file', os.path.abspath(resfile_name + '.net'))
    setattr(params, 'place_file', os.path.abspath(resfile_name + '.place'))
    return params;


if __name__ == "__main__":
    main()
