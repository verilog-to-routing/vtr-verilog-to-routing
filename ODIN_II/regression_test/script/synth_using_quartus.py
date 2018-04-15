#! /usr/bin/python
#Iterate through a directory listing treating each individual verilog file as a single project and synthesize using quartus

#import standard modules
import os
import re
import string
import subprocess
import sys
from os.path import abspath

#import functions custom modules
import odin_script_util as odin

if __name__ == '__main__':
	# args are in sys.argv
	# 0 - Script Name
	# 1 - Directory with .v sources

	#print sys.argv
	if len(sys.argv) != 2:
		print "Take our Verilog benchmarks and synthesize them using QIS (Quartus)"
		print "Usage: GenQuartusBlifs.py <Verilog Source Dir>"
		sys.exit(0)

	projNames = map(odin.trimDotV, filter(odin.isVerilog, os.listdir(sys.argv[1])))

	path = abspath(sys.argv[1]) + "/"
	slog = open(sys.argv[1] + "/quartus_success.lst", "w")

	for proj in projNames:
		cmd = "quartus_map " + sys.argv[1] + proj + " --set=INI_VARS=\"no_add_ops=on;opt_dont_use_mac=on;dump_blif_before_optimize=on\""
		process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
		(out, err) = process.communicate()
		
		#Log our failures and successes
		if process.returncode != 0 :
			flog = open(path + "/Quartus_Blifs/" + proj + ".log", "w")
			flog.write("Verilog File: " + proj + ".v\n")
			flog.write("Quartus Command: " + cmd + "\n")
			flog.write("Quartus Output:\n" +out + "\n\n")
			flog.close()
		else:
			slog.write(proj + ".blif\n")

	clean = "rm -rf " + sys.argv[1] + "*.rpt " + sys.argv[1] + "*.summary  " + sys.argv[1] + "*.q?f  " + sys.argv[1] + "db/  " + sys.argv[1] + "incremental_db/"
	create = "mkdir -p " + sys.argv[1] + "Quartus_Blifs/"
	move = "mv  " + sys.argv[1] + "*.blif " + sys.argv[1] + "/Quartus_Blifs/"
	
	process = subprocess.Popen(clean, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
	(out, err) = process.communicate()
	print out + "\n"
	
	process = subprocess.Popen(create, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
	(out, err) = process.communicate()
	print out + "\n"
	
	process = subprocess.Popen(move, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
	(out, err) = process.communicate()