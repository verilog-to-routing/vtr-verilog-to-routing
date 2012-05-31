#! /usr/bin/python
#Iterate through a directory listing treating each individual verilog file as a single project and synthesize using VL2MV


import os 
import subprocess
import sys
import odin_script_util as odin
from os.path import abspath


if len(sys.argv) is not 2:
	print "usage: " + sys.argv[0] + " <dir>"
	
path = abspath(sys.argv[1]) + "/"
os.system("mkdir -p \"" + path + "VL2MV_Blifs/\"")
slog = open(path + "VL2MV_success.lst", "w")
filelist = filter(odin.isVerilog, os.listdir(path))

for file in filelist:
	cmd = "vl2mv -o \"" + path + "VL2MV_Blifs/" + odin.trimDotV(file) + ".mv\" \"" + path + file + "\""
	process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
	(out, err) = process.communicate()
	
	#Log our failures and successes
	if process.returncode != 0 :
		flog = open(path + "VL2MV_Blifs/" + odin.trimDotV(file) + ".log", "w")
		flog.write("Verilog File: " + file + "\n")
		flog.write("VL2MV Command: " + cmd + "\n")
		flog.write("VL2MV Output:\n" + out + "\n\n")
		flog.close()
	else:
		slog.write(file + "\n")
