#!/usr/bin/python

import sys
import subprocess
import os
import os.path
import re
from subprocess import Popen, PIPE, STDOUT

my_dir = ""

if len(sys.argv) < 9:
    print ("Usage: spice.py <tech_file> <tech_size> <Vdd> <P/N> <temp> <activity [hz]> <component_type> <size> <nmos_size>")
    sys.exit();
    
my_dir = os.path.dirname(os.path.realpath(__file__));
    
tech_file = sys.argv[1]
tech_file = os.path.abspath(tech_file)
tech_size = sys.argv[2]
Vdd = sys.argv[3]
pn = sys.argv[4]
temp = sys.argv[5]
activity = sys.argv[6]
type = sys.argv[7]
size = sys.argv[8]

do_nmos_size = False
if len(sys.argv) > 9:
    do_nmos_size = True
    nmos_size = sys.argv[9]

if (activity == "h"):
    na = 0
elif (activity == "z"):
    na = 1
else:
    print("Invalid activity type\n");
    sys.exit()
    
base_dir = os.path.join(my_dir)
temp_dir = os.path.join(base_dir, "temp")


if not os.path.exists(temp_dir):
    os.mkdir(temp_dir)

file_name = type + "_" + str(size) 

if na:
    file_path = os.path.join(temp_dir, file_name + "_na.sp")
else:
    file_path = os.path.join(temp_dir, file_name + ".sp")

f = open(file_path, 'w')

f.write ("Auto Spice\n")
f.write (".include \"" + tech_file + "\n")

f.write (".param tech = " + str(tech_size) + "e-9\n")
f.write(".param tempr = " + temp + "\n")
f.write(".param simt = 5n\n")


f.write(".param Vol=" + Vdd + "\n")
f.write(".param pnratio=" + pn + "\n")

if do_nmos_size:
    f.write(".param nmos_size=" + nmos_size + "\n")

subckt_path = os.path.join(base_dir, "subckt")
f.write(".include " + subckt_path + "/nmos_pmos.sp\n")
f.write(".include " + subckt_path + "/mux2.sp\n")
f.write(".include " + subckt_path + "/mux2trans.sp\n")
f.write(".include " + subckt_path + "/mux3.sp\n")
f.write(".include " + subckt_path + "/mux4.sp\n")
f.write(".include " + subckt_path + "/mux5.sp\n")
f.write(".include " + subckt_path + "/inv.sp\n")
f.write(".include " + subckt_path + "/dff.sp\n")
f.write(".include " + subckt_path + "/level_restorer.sp\n")

if na:
    read_file_path = os.path.join(base_dir, type, size + "_na.spz")
else:
    read_file_path = os.path.join(base_dir, type, size + ".spz")



fr = open(read_file_path)
f.write(fr.read())
fr.close()
f.close()

cmd = "hspice " + file_path
# print cmd 

p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT, cwd=temp_dir)
stdout, stderr = p.communicate()

if re.search("error", stdout):
    print "Error" 

else:
    m = re.search("^\s*power=\s*(\S*).*$", stdout, re.MULTILINE)
    if (m):
        print m.group(1)


# f = open("~/spice_modeling/" + sys.argv[1] + ".spx")





# hspice ~/spice_modeling/$1 2>&1 | grep -P "^\s+e=|\s+tech_size="
