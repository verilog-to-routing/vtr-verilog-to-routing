import sys
import re
import os

openlist = []

do_power = True

# Architecture Properties
Fc_in = 0.15
Fc_out = 0.1

R_minW_nmos = 6065.520020
R_minW_pmos = 18138.500000
ipin_mux_trans_size = 1.222260

C_ipin_cblock = 0.000000e+00
T_ipin_cblock = 7.247000e-11

grid_logic_tile_area = 14813.392

switch_T_del = 6.837e-11
switch_mux_trans_size = 2.630740
switch_buf_size = 27.645901

seg_Cmetal_per_m = 2.52e-10
CLB_interc_Cmetal_per_m = 2.52e-10
clock_Cmetal_per_m = 2.52e-10 

delay_CLB_I_to_ble = 8.044000e-11
delay_ble_to_ble = 7.354000e-11

delay_LUT = 2.690e-10

T_setup = 2.448e-10
T_clock_to_Q = 7.732e-11

def xprint(s, newline=False):
    for i in range(tabs):
        f.write('\t')
    f.write(str(s))
    
    if (newline):
        f.write('\n')        

def xbegin(s):
    global tabs
    
    xprint("<" + s)
    openlist.append(s)
    tabs = tabs + 1

def xclose():
    f.write(">\n")

def xcloseend():
    global tabs
    
    f.write("/>\n")
    openlist.pop()
    tabs = tabs - 1

def xend():
    global tabs
    
    s = openlist.pop()    
    tabs = tabs - 1
    xprint("</" + s + ">\n")    

def xcopy(fsrc):
    src = open(fsrc, 'r')
    for line in src:
        xprint(line)
    src.close()
    f.write("\n")
    
def xprop(s, v):
    f.write(" " + s + "=\"" + str(v) + "\"")
    
def xport(type, name, p,  e = "", c = ""):
    assert (type == "input" or type == "output" or type == "clock")
        
    xbegin(type)
    xprop("name", name)
    xprop("num_pins", p)
    if (e <> ""):
        xprop("equivalent", e)
        
    if (c <> ""):
        xprop("port_class", c)
    xcloseend()
    
def xcomment(s):
    f.write("<!-- ")
    f.write(s)
    f.write(" -->\n")
    
def xLUT(LUT_size, num_LUT):
    xbegin("pb_type")
    xprop("name", "lut" + str(LUT_size))
    xprop("blif_model", ".names")
    xprop("num_pb", num_LUT)
    xprop("class", "lut")
    xclose()
    xport("input", "in", LUT_size, c = "lut_in")
    xport("output", "out", 1, c = "lut_out")
    
    xbegin("delay_matrix")
    xprop("type", "max")
    xprop("in_port", "lut" + str(LUT_size) + ".in")
    xprop("out_port", "lut" + str(LUT_size) + ".out")
    xclose()
    for j in range(LUT_size):
        xprint(delay_LUT, True)
    xend() #delay_matrix
    
    xend() #pb_type lut
    
def xCLB(k_LUT, N_BLE, I_CLB, fracture_level, num_FF):    
    O_LUT = 2 ** fracture_level
    O_soft = O_LUT
    O_ble = O_soft
    O_CLB = N_BLE * O_ble
    
    assert((O_LUT % num_FF) == 0)
    
    xbegin("pb_type")
    xprop("name", "clb")
    xclose()
    
    xport("input", "I", I_CLB, "true")
    xport("output", "O", O_CLB, "false")
    xport("clock", "clk", 1 )
    
    xbegin("interconnect")
    xclose()
    
    ble_array = "ble[" + str((N_BLE - 1)) + ":0]"
    
    xbegin("complete")
    xprop("name", "crossbar")
    xprop("input", "clb.I " + ble_array + ".out")
    xprop("output", ble_array + ".in")
    xclose()
    
    xbegin("delay_constant")
    xprop("max", delay_CLB_I_to_ble)
    xprop("in_port", "clb.I")
    xprop("out_port", ble_array + ".in")
    xcloseend()
    
    xbegin("delay_constant")
    xprop("max", delay_ble_to_ble)
    xprop("in_port", ble_array + ".out")
    xprop("out_port", ble_array + ".in")
    xcloseend()
    xend() #complete
    
    xbegin("complete")
    xprop("name", "clks")
    xprop("input", "clb.clk")
    xprop("output", ble_array + ".clk")
    xcloseend()
    
    xbegin("direct")
    xprop("name", "clbouts")
    xprop("input", ble_array + ".out")
    xprop("output", "clb.O")
    xcloseend()
    
    xend() #interconnect
    
    xbegin("pb_type")
    xprop("name", "ble")
    xprop("num_pb", N_BLE)
    xclose()
    xport("input", "in", k_LUT)
    xport("output", "out", O_ble)
    xport("clock", "clk", 1)
    
    xbegin("interconnect")
    xclose()
    i = 0
    
    # Connection from ble to soft logic
    xbegin("direct")
    xprop("name", "direct" + str(i))
    xprop("input", "ble.in")
    xprop("output", "soft_logic.in")
    xcloseend()
    i = i + 1    
    
    # Clock connections to FFs 
    for j in range(num_FF):
        xbegin("direct")
        xprop("name", "direct" + str(i))
        xprop("input", "ble.clk")
        xprop("output", "ff[" + str(j) + ":" + str(j) + "].clk")
        xcloseend()
        i = i + 1
    
    # Connections from Soft Logic to FFs
    soft_per_ff = O_soft / num_FF
    for j in range(num_FF):
        xbegin("mux")
        xprop("name", "mux" + str(i))
        inputs = ""
        for k in range(soft_per_ff):
            inputs = inputs + "soft_logic.out[" + str(j * soft_per_ff + k) + ":" + str(j * soft_per_ff + k) + "] "
        inputs = inputs.strip()
        xprop("input", inputs)
        xprop("output", "ff[" + str(j) + ":" + str(j) + "].D")
        xcloseend()
        i = i + 1
    
    # Connections from Soft/FF to BLE output    
    for j in range(O_ble):
        xbegin("mux")
        xprop("name", "mux" + str(i))
        xprop("input", "soft_logic.out[" + str(j) + ":" + str(j) + "] ff[" + str(j / soft_per_ff) + ":" + str(j / soft_per_ff) + "].Q")
        xprop("output", "ble.out[" + str(j) + ":" + str(j) + "]")
        xcloseend()
        i = i + 1        
    

    
    xend() #interconnect
    
    xbegin("pb_type")
    xprop("name", "soft_logic")
    xprop("num_pb", 1)
    xclose()
    
    xport("input", "in", k_LUT)
    xport("output", "out", O_soft)
    
    if (fracture_level == 2):
        frac_stages = 4
    else:
        frac_stages = fracture_level + 1
        
    for frac_stage in range(frac_stages):
        if (frac_stage == 3):
            # This is a special configuration with 1x(k-1) and 2x(k-2) LUTs
            special_stage = True
            num_LUT = 2
            LUT_size = k_LUT - 2
            
            LUT_size_special = k_LUT - 1
            idx_special = "[" + str(LUT_size_special - 1) + ":0]"
        else:
            special_stage = False
            
            num_LUT = 2 ** frac_stage
            LUT_size = k_LUT - frac_stage
            idx = "[" + str(LUT_size - 1) + ":0]"     
         
        xbegin("mode")
        if (special_stage):
            xprop("name", "n3" + "_lut" + str(LUT_size) + "-" + str(LUT_size_special))
        else:
            xprop("name", "n" + str(num_LUT) + "_lut" + str(LUT_size))
        xclose()
        
        xbegin("interconnect")
        xclose()
        i = 0    
        
        # Soft Logic to LUTs
        for j in range(num_LUT):
            xbegin("direct")
            xprop("name", "direct" + str(i))        
            xprop("input", "soft_logic.in" + idx)
            xprop("output", "lut" + str(LUT_size) + "[" + str(j) + ":" + str(j) + "].in" + idx)
            xcloseend()
            i = i + 1
        
        # Soft Logic to special LUT
        if (special_stage):            
            xbegin("direct")
            xprop("name", "direct" + str(i))        
            xprop("input", "soft_logic.in" + idx_special)
            xprop("output", "lut" + str(LUT_size_special) + "[0:0].in" + idx_special)
            xcloseend()
            i = i + 1
        
        # LUTs to soft logic
        for j in range(num_LUT):
            xbegin("direct")
            xprop("name", "direct" + str(i))
            xprop("input", "lut" + str(LUT_size) + "[" + str(j) + ":" + str(j) + "].out")
            xprop("output", "soft_logic.out[" + str(j) + ":" + str(j) + "]")
            xcloseend()
            i = i + 1
            
        # Special LUT to soft Logic
        if (special_stage):
            xbegin("direct")
            xprop("name", "direct" + str(i))
            xprop("input", "lut" + str(LUT_size_special) + "[0:0].out")
            xprop("output", "soft_logic.out[" + str(num_LUT) + ":" + str(num_LUT) + "]")
            xcloseend()
            i = i + 1     
        
        xend() #interconnect
        
        xLUT(LUT_size, num_LUT);
        
        if (special_stage):
            xLUT(LUT_size_special, 1)
        
        xend() #mode
        
    
    xend() #pb_type soft_logic
    
    xbegin("pb_type")
    xprop("name", "ff")
    xprop("blif_model", ".latch")
    xprop("num_pb", num_FF)
    xprop("class", "flipflop")
    xclose()
    xport("input", "D", 1, c = "D")
    xport("output", "Q", 1, c = "Q")
    xport("clock", "clk", 1, c = "clock")
    
    xbegin("T_setup")
    xprop("value", T_setup)
    xprop("port", "ff.D")
    xprop("clock", "clk")
    xcloseend()
    
    xbegin("T_clock_to_Q")
    xprop("max", T_clock_to_Q)
    xprop("port", "ff.Q")
    xprop("clock", "clk")
    xcloseend()
        
    xend() #pb_type ff
    
    xend() #pb_type ble
    
    xbegin("fc_in")
    xprop("type", "frac")
    xclose()
    xprint(Fc_in, True)
    xend() #fc_in
    
    xbegin("fc_out")
    xprop("type", "frac")
    xclose()
    xprint(Fc_out, True)
    xend() #fc_out
    
    xbegin("pinlocations")
    xprop("pattern", "spread")
    xcloseend()
    
    xbegin("gridlocations")
    xclose()
    xbegin("loc")
    xprop("type", "fill")
    xprop("priority", 1)
    xcloseend()
    xend()
    
    xend() #pb_type clb
    
def gen_arch(dir, k_LUT, N_BLE, I_CLB, fracture_level, num_FF, seg_length):
    global f
    global tabs

    assert(fracture_level == 0 or fracture_level == 1 or fracture_level == 2)
    

    filename = "k" + str(k_LUT) + "_N" + str(N_BLE) + "_I" + str(I_CLB) + "_L" + str(seg_length) + "_frac" + str(fracture_level) + "_ff" + str(num_FF) + ".xml"    
     
    tabs = 0
    


    m = re.search("(.*vtr_flow)", sys.path[0])
    vtr_flow_dir = m.group(1)
    script_dir = os.path.join(vtr_flow_dir, "scripts", "arch_gen")
    
    
    models = ["multiply", "single_port_ram", "dual_port_ram"]
    cbs = ["io", "clb", "memory", "mult_36"]
    
    f = open(os.path.join(dir, filename),'w')
    
    xcomment("k = " + str(k_LUT))
    xcomment("N = " + str(N_BLE))
    xcomment("I = " + str(I_CLB))
    xcomment("frac_level = " + str(fracture_level))
    xcomment("ff = " + str(num_FF))
    
    xbegin("architecture")
    xclose()
    
    xbegin("models")
    xclose()
    
    for model in models: 
        xcopy(os.path.join(script_dir, "models",  model + ".xml"))
    
    xend() #Models
    
    xbegin("layout")
    xprop("auto", "1.0")
    xcloseend()
    
    xbegin("device")
    xclose()
    
    xbegin("sizing")
    xprop("R_minW_nmos", R_minW_nmos)
    xprop("R_minW_pmos", R_minW_pmos)
    xprop("ipin_mux_trans_size", ipin_mux_trans_size)
    xcloseend()
    
    xbegin("timing")
    xprop("C_ipin_cblock", C_ipin_cblock)
    xprop("T_ipin_cblock", T_ipin_cblock)
    xcloseend()
    
    xbegin("area")
    xprop("grid_logic_tile_area", grid_logic_tile_area)
    xcloseend()
    
    xbegin("chan_width_distr")
    xclose()
    
    xbegin("io")
    xprop("width", 1.0)
    xcloseend()
    
    xbegin("x")
    xprop("distr", "uniform")
    xprop("peak", "1.0")
    xcloseend()
    
    xbegin("y")
    xprop("distr", "uniform")
    xprop("peak", "1.0")
    xcloseend()
    
    xend() # chan_width_distr
    
    xbegin("switch_block")
    xprop("type", "wilton")
    xprop("fs", 3)
    xcloseend()
    
    xend() #Device
    
    xbegin("switchlist")
    xclose()
    xbegin("switch")
    xprop("type", "mux")
    xprop("name", "0")
    xprop("R", 0.0)
    xprop("Cin", 0.0)
    xprop("Cout", 0.0)
    xprop("Tdel", switch_T_del)
    xprop("mux_trans_size", switch_mux_trans_size)
    xprop("buf_size", switch_buf_size)
    if (do_power):
        xprop("buf_last_stage_size", "auto")
    xcloseend()
    xend() #switchlist
    
    xbegin("segmentlist")
    xclose()
    xbegin("segment")
    xprop("freq", 1.0)
    xprop("length", seg_length)
    xprop("type", "unidir")
    xprop("Rmetal", 0.0)
    xprop("Cmetal", 0.0)
    if (do_power):
        xprop("Cmetal_per_m", seg_Cmetal_per_m)
    xclose()
    
    xbegin("mux")
    xprop("name", "0")
    xcloseend()
    
    xbegin("sb")
    xprop("type","pattern")
    xclose()
    s = ""
    for i in range(seg_length + 1):
        s = s + "1 "
    s = s.strip()
    xprint(s + "\n", False)
    xend() #sb
    
       
    xbegin("cb")
    xprop("type","pattern")
    xclose()
    
    s = ""
    for i in range(seg_length):
        s = s + "1 "
    s = s.strip()
    xprint(s + "\n", False)
    xend() #cb
    
    xend() #segment
    xend() #segmentlist
    
    xbegin("complexblocklist")
    xclose()
    

    
    
    for cb in cbs: 
        if (cb == "clb"):
            xCLB(k_LUT, N_BLE, I_CLB, fracture_level, num_FF)
        else:
            xcopy(os.path.join(script_dir, "complexblocks",  cb + ".xml"))
     
    xend() #complexblocklist
    
    if (do_power):
        xbegin("power")
        xclose()
        
        xbegin("short_circuit_power")
        xprop("percentage", 0.0)
        xcloseend()
        
        xbegin("local_interconnect")
        xprop("C_wire", CLB_interc_Cmetal_per_m)
        xcloseend()
        
        xend() #Power
        
        xbegin("clocks")
        xclose()
        xbegin("clock")
        xprop("buffer_size", "auto")
        xprop("C_wire", clock_Cmetal_per_m)
        xcloseend()
        xend() #clocks
    
    xend() #Architecture
    
    f.close()


# Architecture Options
arches = []
#              k    N       I_CLB   frac    num_FF  seg_length
arches.append([6,   10,     33,     0,      1,      4])
arches.append([6,   10,     33,     1,      1,      4])
arches.append([6,   10,     33,     1,      2,      4])
arches.append([6,   10,     33,     2,      1,      4])
arches.append([6,   10,     33,     2,      2,      4])
arches.append([6,   10,     33,     2,      4,      4])
arches.append([6,   10,     33,     0,      1,      3])
arches.append([6,   10,     33,     1,      1,      3])
arches.append([6,   10,     33,     1,      2,      3])
arches.append([6,   10,     33,     2,      1,      3])
arches.append([6,   10,     33,     2,      2,      3])
arches.append([6,   10,     33,     2,      4,      3])
arches.append([6,   10,     33,     0,      1,      5])
arches.append([6,   10,     33,     1,      1,      5])
arches.append([6,   10,     33,     1,      2,      5])
arches.append([6,   10,     33,     2,      1,      5])
arches.append([6,   10,     33,     2,      2,      5])
arches.append([6,   10,     33,     2,      4,      5])
arches.append([6,   10,     33,     0,      1,      6])
arches.append([6,   10,     33,     1,      1,      6])
arches.append([6,   10,     33,     1,      2,      6])
arches.append([6,   10,     33,     2,      1,      6])
arches.append([6,   10,     33,     2,      2,      6])
arches.append([6,   10,     33,     2,      4,      6])






dir = "C:\\Users\\Jeff\\Dropbox\\linux_home\\vtr\\vtr_flow\\arch\\timing_gen"

for arch in arches:
    gen_arch(dir, arch[0], arch[1], arch[2], arch[3], arch[4], arch[5])
print "Done\n"


