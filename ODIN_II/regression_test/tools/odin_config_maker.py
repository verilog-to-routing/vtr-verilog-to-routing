#!/usr/bin/python

#import standard modules
import os
import sys

#import custom modules
import ODIN_CONFIG as odin

#import functions from standard modules
from optparse import OptionGroup
from optparse import OptionParser
from os.path import abspath
from string import rstrip


#import functions from custom modules
from odin_script_util import isVerilog

def main(argv=None):

	if argv is None:
		argv = sys.argv

	parser = configOptParse()
	(options, args) = parser.parse_args(argv)

	#Check our options for input errors
	if not options.arch:
		print "\tDid not get an architecture file; use odin_config_maker.py -h for help."
		return -1
	if options.individual is None and options.directory is None and options.files is None:
		print "\tDid not get any input options; use odin_config_maker.py -h for help."
		return -1
	if options.individual is not None and options.directory is not None:
		print "\tThe -i and -d options are mutually exclusive; use odin_config_maker.py -h for help."
		return -1

	#Create our Config Files
	if options.individual:								#Create a collection of configs
		path = abspath(args.pop()) + "/"
		base = None
		file_list = map(lambda file: abspath(file), options.files) if options.files else []
		if options.individual:
			file_list.extend(filter(isVerilog, map(lambda file: abspath(options.individual) + "/" + file, os.listdir(options.individual))))

		for file in file_list:
			print file[file.rfind("/") + 1:file.rfind(".v")]
			base = file[file.rfind("/") + 1:file.rfind(".v")]
			create_odin_projects(options, [file], base, path)

	elif options.directory or options.files and len(args) == 3:	#Create a single project based on a directory or file list
		base = args.pop()
		path = abspath(args.pop()) + "/"

		file_list = map(lambda file: abspath(file), options.files) if options.files else []
		if options.directory:
			file_list.extend(filter(isVerilog, map(lambda file: abspath(options.directory) + "/" + file, os.listdir(options.directory))))

		create_odin_projects(options, file_list, base, path)
	else:
		print "Something Failed!"
		return -1

def create_odin_projects(options, file_list, base, path):
	soft = base + ".soft." if options.soft else None
	hard_mem = base + ".hard_mem." if options.mem or options.both else None
	hard_mult = base + ".hard_mult." if options.mult or options.both else None
	hard = base + ".hard." if options.both else None
	output = abspath(options.output) + "/"

	config = odin.config(verilog_files=create_common_verilog(file_list), debug_outputs= create_common_debug(options))
	if soft:
		config.set_output(create_common_output(options, output + soft))
		config.export(open(path + soft + "xml", "w"), 0)

	if hard_mem or hard_mult or hard:
		if hard:
			config.set_output(create_common_output(options, output + hard))
			config.set_optimizations(create_common_optimizations(options))
			config.export(open(path + hard + "xml", "w"), 0)
		if hard_mem:
			config.set_output(create_common_output(options, output + hard_mem))
			config.set_optimizations(create_common_optimizations(options))
			config.optimizations.set_multiply(None)
			config.export(open(path + hard_mem + "xml", "w"), 0)
		if hard_mult:
			config.set_output(create_common_output(options, output + hard_mult))
			config.set_optimizations(create_common_optimizations(options))
			config.optimizations.set_memory(None)
			config.export(open(path + hard_mult + "xml", "w"), 0)

def create_common_verilog(file_list):
	verilog_files = odin.verilog_files()
	verilog_files.set_verilog_file(file_list)
	return verilog_files

def create_common_output(options, outfile):
	output = odin.output()
	target = odin.target()
	target.set_arch_file(abspath(options.arch))
	output.set_output_type("blif")
	output.set_output_path_and_name(outfile + output.get_output_type())
	output.set_target(target)
	return output

def create_common_optimizations(options):
	optimizations = odin.optimizations()
	if options.mem  or options.both:
		mem = odin.memory()
		if options.split_width or options.split_depth:
			mem.set_split_memory_width(1 if options.split_width or options.split_depth else 0)
			mem.set_split_memory_depth(1 if options.split_depth else 0)
		else:
			mem = None
		optimizations.set_memory(mem)


	if options.mult or options.both:
		mult = odin.multiply()
		if options.size:
			mult.set_size(int(options.size))
			mult.set_fracture(1 if options.fracture else 0)
			mult.set_fixed(1 if options.fixed else 0)
		else:
			mult = None
		optimizations.set_multiply(mult)

	return optimizations

def create_common_debug(options):
	debug = odin.debug_outputs()
	debug.set_debug_output_path(abspath(options.debug_path))
	debug.set_output_ast_graphs(1 if options.ast_graph else 0)
	debug.set_output_netlist_graphs(1 if options.netlist_graph else 0)
	return debug

def configOptParse():
	parser = OptionParser(usage="odin_config_maker.py [options] [output_path] [base_filename]",
						  prog="odin_config_maker.py",
						  description="[output_path] specifies where configuration files will be written. [base_filename] is used to create the appropriate configuration files, [base_filename].soft.xml, [base_filename].hard_mem.xml, [base_filename].hard_mult.xml, [base_filename].hard.xml")

	#Input options
	input = OptionGroup(parser, "Input Options", "These options specify what input options are available. At least one of these must be specified.")
	input.add_option("-a", help="The ODIN_II architecture file to use", action="store", dest="arch", metavar="ARCH_FILE")
	input.add_option("-i", help="Treat each verilog file in the given directory as a seperate ODIN_II project. The -i and -d options cannot be used together.", action="store", dest="individual", metavar="DIRECTORY")
	input.add_option("-d", help="Treat the given directory as a single ODIN_II project and include all verilog files within it. The -i and -d options cannot be used together.", action="store", dest="directory")
	input.add_option("-v", help="Add the given file to an ODIN_II project. This option can be used to create a project with files in different locations or in conjunction with -i and -d",
					  action="append", dest="files", metavar="FILE")
	parser.add_option_group(input)

	#Output Options
	output = OptionGroup(parser, "Output Options", "These options specify the different output options available, if more than one is specified multiple configs will be created.")
	output.add_option("-o", help="The blif output path to be used in the configuration file. Defaults to ./", action="store", dest="output", metavar="OUTPUT_PATH")
	output.add_option("--soft", help="Generate a blif using all soft logic.", action="store_true")
	output.add_option("--both", help="Generate a blif that uses both hard block multiliers and memories.", action="store_true")
	output.add_option("--mult", help="Generate a blif that uses hard block multiliers.", action="store_true")
	output.add_option("--mem", help="Generate a blif that uses hard block memories.", action="store_true")
	parser.add_option_group(output)
	parser.set_defaults(output="./", soft=True, both=False, mult=False, mem=False)


	#Muliplier Hard Block Options
	mult = OptionGroup(parser, "Hard Block Multiplier Options", "Configure how ODIN will use hard block multipliers.")
	mult.add_option("--size", help="Specify the minimum size of a multiplier.", action="store", dest="size")
	mult.add_option("--fixed", help="Pad any extra pins on the multiplier.", action="store_true", dest="fixed")
	mult.add_option("--no_fixed", help="Remove any extra pins on the multiplier.", action="store_false", dest="fixed")
	mult.add_option("--fracture", help="split the multiplier if neccesary.", action="store_true", dest="fracture")
	mult.add_option("--no_fracture", help="split the multiplier if neccesary.", action="store_true", dest="fracture")
	parser.add_option_group(mult)
	parser.set_defaults(fixed=True, fracture=False, mult=False)


	#Memory Hard Block Options
	mem = OptionGroup(parser, "Hard Block Memory Options", "Configure how ODIN will use hard block memories.")
	mem.add_option("--split_width", help="Split memories on width.", action="store_true")
	mem.add_option("--split_depth", help="Split memories on width and depth.", action="store_true")
	parser.add_option_group(mem)
	parser.set_defaults(split_width=False, split_depth=False)


	#Debug Options
	debug = OptionGroup(parser, "ODIN_II Debug Options", "Configure what type of bebug information ODIN_II will produce and where to put it.")
	debug.add_option("--debug_path", help="Set the path where debug info will be put.", action="store")
	debug.add_option("--no_ast_graph", help="Do not output an AST graph", action="store_false", dest="ast_graph")
	debug.add_option("--no_netlist_graph", help="Do not output a netlist graph", action="store_false", dest="netlist_graph")
	parser.add_option_group(debug)
	parser.set_defaults(debug_path="./", ast_graph=True, netlist_graph=True)

	return parser

if __name__ == "__main__":
	sys.exit(main())
