#! /usr/bin/python
#Iterate through a directory listing and synthesize each  project using ODIN_II

import os 
import subprocess
import sys
import odin_script_util as odin
from os.path import abspath


if len(sys.argv) is not 2:
	print "usage: " + sys.argv[0] + " <dir>"
	
path = abspath(sys.argv[1]) + "/"
slog = open(path + "ODIN_success.lst", "w")
filelist = filter(odin.isXML, os.listdir(path))

for file in filelist:
	cmd = "../../odin_II.exe -c " + path + file
	process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, shell=True)
	(out, err) = process.communicate()
	
	#Log our failures and successes
	if process.returncode != 0 :
		flog = open(path + file + ".log", "w")
		flog.write("CONFIG File: " + file + "\n")
		flog.write("ODIN_II Command: " + cmd + "\n")
		flog.write("ODIN_II Output:\n" +out + "\n\n")
		flog.close()
	else:
		slog.write(file + "\n")
