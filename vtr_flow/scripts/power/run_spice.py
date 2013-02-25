#!/usr/bin/python

import sys
import subprocess
import os
import os.path
import re
from subprocess import Popen, PIPE, STDOUT

def spice_it(tech, type, size, na):
    base_dir = os.path.join(os.path.expanduser("~/spice_modeling"))
    temp_dir = os.path.join(base_dir, "temp", type)
    
    
    if not os.path.exists(temp_dir):
        os.mkdir(temp_dir)
    
    file_name = type + "_" + str(size) 
    
    if na:
        file_path = os.path.join(temp_dir, file_name + "_" + str(tech) + "_na.sp")
    else:
        file_path = os.path.join(temp_dir, file_name + "_" + str(tech) + ".sp")
    
    f = open(file_path, 'w')
    
    f.write ("Auto Spice\n")
    f.write (".include \"~/vtr_flow/tech/PTM_" + str(tech) + "nm/" + str(tech) + "nm.pm\"\n")
    
    f.write (".param tech = " + str(tech) + "e-9\n")
    f.write(".param tempr = 85\n")
    f.write(".param simt = 5n\n")
    
    if (tech == 22):
        f.write(".param Vol=0.8\n")
        f.write(".param pnratio=1.7\n")
    elif (tech == 45):
        f.write(".param Vol=1.0\n")
        f.write(".param pnratio=1.75\n")
    elif (tech == 130):
        f.write(".param Vol=1.3\n")
        f.write(".param pnratio=2.5\n")
        
    f.write(".include ~/spice_modeling/subckt/nmos_pmos.sp\n")
    f.write(".include ~/spice_modeling/subckt/mux2.sp\n")
    f.write(".include ~/spice_modeling/subckt/mux2trans.sp\n")
    f.write(".include ~/spice_modeling/subckt/mux3.sp\n")
    f.write(".include ~/spice_modeling/subckt/mux4.sp\n")
    f.write(".include ~/spice_modeling/subckt/mux5.sp\n")
    f.write(".include ~/spice_modeling/subckt/inv.sp\n")
    f.write(".include ~/spice_modeling/subckt/dff.sp\n")
    f.write(".include ~/spice_modeling/subckt/level_restorer.sp\n")
    
    if na:
        read_file_path = os.path.join(base_dir, type, file_name + "_na.spz")
    else:
        read_file_path = os.path.join(base_dir, type, file_name + ".spz")
    
    
    
    fr = open(read_file_path)
    f.write(fr.read())
    fr.close()
    f.close()
    
    cmd = "hspice " + file_path
    #print cmd 
    
    p = Popen(["hspice " + file_path], shell=True, stdout=PIPE, stderr=STDOUT, cwd=temp_dir)
    stdout, stderr = p.communicate()
    
    
    
    
    if re.search("error", stdout):
        print "Error" 

    else:
        m = re.search("^\s*e=\s*(\S*).*$",stdout, re.MULTILINE)
        if (m):
            print m.group(1)


#f = open("~/spice_modeling/" + sys.argv[1] + ".spx")

type = sys.argv[1]
size = sys.argv[2]

print ("high activity:")
spice_it(22, type, size, 0)
spice_it(45, type, size, 0)
spice_it(130, type, size, 0)
print ("no activity:")
spice_it(22, type, size, 1)
spice_it(45, type, size, 1)
spice_it(130, type, size, 1)



#hspice ~/spice_modeling/$1 2>&1 | grep -P "^\s+e=|\s+tech_size="
