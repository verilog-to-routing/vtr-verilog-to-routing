import sys
import re
import os

openlist = []

do_power = True

# Architecture Properties
Fc_in = 0.15
Fc_out = 0.1

power_short_circuit_percentage = 0.0
clb_area = 53894

R_minW_nmos = 8926
R_minW_pmos = 16067
ipin_mux_trans_size = 1.222260

C_ipin_cblock = 1.47e-15
T_ipin_cblock = 7.247e-11

grid_logic_tile_area = 0

switch_R = 551
switch_Cin = 0.77e-15
switch_Cout = 4e-15
switch_T_del = 58e-12
switch_mux_trans_size = 2.630740
switch_buf_size = 27.645901

segment_Rmetal = 101
segment_Cmetal = 22.5e-15

delay_crossbar_CLB_in_to_BLE_in = 95e-12
delay_crossbar_BLE_out_to_BLE_in = 75e-12

delay_FF_T_setup = 66e-12
delay_FF_T_clk_Q = 124e-12

delay_LUT_out_to_BLE_out = 25e-12
delay_FF_out_to_BLE_out = 45e-12

wire_C_per_m = 250e-12

def wire_C_scaling(tech):
    if (tech == 45):
        return 1.0
    elif (tech == 130):
        return (285.0/250.0)
    elif (tech == 22):
        return (218.0/250.0)

def delay_LUT(size):
    if (size == 6):
        return 261e-12
    elif (size == 5):
        return 235e-12
    else:
        assert(0);


def xprint(s, newline=False):
    for i in range(tabs):
        f.write('  ')
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
    
def xport(type, name, p, e="", c=""):
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

def outputs_by_frac_stage(frac_stage):
    if (frac_stage == 0):
        return 1
    elif (frac_stage == 1):
        return 2
    elif (frac_stage == 2 or frac_stage == 3):
        return 4

def xLUT(LUT_size, num_LUT):
    xbegin("pb_type")
    xprop("name", "lut" + str(LUT_size))
    xprop("blif_model", ".names")
    xprop("num_pb", num_LUT)
    xprop("class", "lut")
    xclose()
    xport("input", "in", LUT_size, c="lut_in")
    xport("output", "out", 1, c="lut_out")
    
    xbegin("delay_matrix")
    xprop("type", "max")
    xprop("in_port", "lut" + str(LUT_size) + ".in")
    xprop("out_port", "lut" + str(LUT_size) + ".out")
    xclose()
    for j in range(LUT_size):
        xprint(delay_LUT(LUT_size), True)
    xend()  # delay_matrix
    
    xend()  # pb_type lut
    
def xCLB(k_LUT, N_BLE, I_CLB, I_BLE, fracture_level, num_FF, crossbar_str):    
    O_LUT = 2 ** fracture_level
    O_soft = O_LUT
    O_ble = O_soft
    O_CLB = N_BLE * O_ble
    I_soft = I_BLE
    
    assert((O_LUT % num_FF) == 0)
    
    xbegin("pb_type")
    xprop("name", "clb")
    xprop("area", clb_area)
    xclose()
    
    if crossbar_str == "c":        
        xport("input", "I", I_CLB, "true")
    else:
        xport("input", "I", I_CLB, "false")
        
    xport("output", "O", O_CLB, "false")
    xport("clock", "clk", 1)
    
    xbegin("interconnect")
    xclose()
    
    ble_array = "ble[" + str((N_BLE - 1)) + ":0]"
    
    # Crossbar
    cb = 0
    if crossbar_str == "c":
        xbegin("complete")
        xprop("name", "crossbar")
        xprop("input", "clb.I " + ble_array + ".out")
        xprop("output", ble_array + ".in")
        xclose()    
        
        xbegin("delay_constant")
        xprop("max", delay_crossbar_CLB_in_to_BLE_in)
        xprop("in_port", "clb.I")
        xprop("out_port", ble_array + ".in")
        xcloseend()
        
        xbegin("delay_constant")
        xprop("max", delay_crossbar_BLE_out_to_BLE_in)
        xprop("in_port", ble_array + ".out")
        xprop("out_port", ble_array + ".in")
        xcloseend()
        xend()
    else:
        cb = int(crossbar_str)
         
 
        num_inputs = I_CLB + O_CLB
        
        for i_ble in range(N_BLE):
            input_idx = 0
            
            
            incr = i_ble + 1;
            # incr = 1
            
            next_start = 0
                        
            for lut_input in range(k_LUT):
                
                xbegin("mux")
                xprop("name", "crossbar-" + str(i_ble) + "-" + str(lut_input))
                
                # Inputs                           
                inputs = []
                for i_cb in range(1, cb + 1):
                       
                        
                    input_str = ""
                    if input_idx < I_CLB:
                        input_str = "clb.I[" + str(input_idx) + "]"
                    else:
                        if fracture_level == 0:
                            input_str = "ble[" + str(input_idx - I_CLB) + "].out"
                        elif fracture_level == 1:
                            input_str = "ble[" + str((input_idx - I_CLB) / 2) + "].out[" + str(input_idx % 2) + "]"
                        else:
                            assert(0)                    
                    inputs.append(input_str)
                    
                    input_idx = (input_idx + incr)
                    if input_idx >= num_inputs:
                        next_start = (next_start + 1) % incr
                        input_idx = next_start                    
       
                                    
                xprop("input", ' '.join(inputs))
                
                # Output
                output = "ble[" + str(i_ble) + "].in[" + str(lut_input) + "]"
                xprop("output", output)
                xclose()
                
                
                for input in inputs:
                    xbegin("delay_constant")
                    if (input[:3] == "clb") :
                        xprop("max", delay_crossbar_CLB_in_to_BLE_in)
                    else:
                        xprop("max", delay_crossbar_BLE_out_to_BLE_in)
                    xprop("in_port", input)
                    xprop("out_port", output)
                    xcloseend()
                
                xend()
        # End crossbar   
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
    
    xend()  # interconnect
    
    ### BLE ###
    xbegin("pb_type")
    xprop("name", "ble")
    xprop("num_pb", N_BLE)
    xclose()
    xport("input", "in", I_BLE)
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
    
    if (fracture_level == 2):
        frac_stages = 4
    else:
        frac_stages = fracture_level + 1
    
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
        xclose()
        
        if (j == 0):
            xbegin("pack_pattern");
            xprop("name", "bleF0")
            xprop("in_port", "soft_logic.out[0:0]")
            xprop("out_port", "ff[0:0].D")
            xcloseend()
        
        if (frac_stages > 1):  
            if (num_FF == 1):
                xbegin("pack_pattern");                
                xprop("name", "bleF1A")
                xprop("in_port", "soft_logic.out[0:0]")
                xprop("out_port", "ff[0:0].D")
                xcloseend()
                xbegin("pack_pattern");                
                xprop("name", "bleF1B")
                xprop("in_port", "soft_logic.out[1:1]")
                xprop("out_port", "ff[0:0].D")
                xcloseend() 
            else:
                xbegin("pack_pattern");                
                xprop("name", "bleF1" + chr(65 + j))
                xprop("in_port", "soft_logic.out[" + str(j) + ":" + str(j) + "]")
                xprop("out_port", "ff[" + str(j) + ":" + str(j) + "].D")
                xcloseend()
        xend()
        i = i + 1
    
    # Connections from Soft/FF to BLE output    
    for j in range(O_ble):
        soft_logic_out = "soft_logic.out[" + str(j) + ":" + str(j) + "]"
        FF_out = "ff[" + str(j / soft_per_ff) + ":" + str(j / soft_per_ff) + "].Q"
        BLE_out = "ble.out[" + str(j) + ":" + str(j) + "]"
        
        xbegin("mux")
        xprop("name", "mux" + str(i))
        xprop("input", soft_logic_out + " " + FF_out)
        xprop("output", BLE_out)
        xclose()
        
        xbegin("delay_constant")
        xprop("max", delay_LUT_out_to_BLE_out)
        xprop("in_port", soft_logic_out)
        xprop("out_port", BLE_out)
        xcloseend()
        
        xbegin("delay_constant")
        xprop("max", delay_FF_out_to_BLE_out)
        xprop("in_port", FF_out)
        xprop("out_port", BLE_out)
        xcloseend()
        
        xend()
        i = i + 1        
    

    
    xend()  # interconnect
    
    ### SOFT LOGIC ###
    xbegin("pb_type")
    xprop("name", "soft_logic")
    xprop("num_pb", 1)
    xclose()
    
    xport("input", "in", I_soft)
    xport("output", "out", O_soft)
    

        
    for frac_stage in reversed(range(frac_stages)):
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
        
        max_shift = I_soft - LUT_size;
        
        shift_interval = int(num_LUT)
        
        # Soft Logic to LUTs
        for j in range(num_LUT):
            if (num_LUT == 1):
                shift = 0;
            elif (special_stage):
                shift = int(float(j) / (num_LUT * 2 - 1) * max_shift)
            else:
                shift = int(float(j) / (num_LUT - 1) * max_shift)
                
            soft_idx = I_soft - 1 - shift;
            
            xbegin("direct")
            xprop("name", "direct" + str(i))        
            xprop("input", "soft_logic.in[" + str(LUT_size - 1 + shift) + ":" + str(shift) + "]")
            xprop("output", "lut" + str(LUT_size) + "[" + str(j) + ":" + str(j) + "].in" + idx)
            xcloseend()
            i = i + 1
        
        # Soft Logic to special LUT
        if (special_stage):            
            xbegin("direct")
            xprop("name", "direct" + str(i))        
            xprop("input", "soft_logic.in[" + str(I_soft - 1) + ":" + str(I_soft - LUT_size_special) + "]")
            xprop("output", "lut" + str(LUT_size_special) + "[0:0].in" + idx_special)
            xcloseend()
            i = i + 1
        
        # LUTs to soft logic
        for j in range(num_LUT):
            xbegin("direct")
            xprop("name", "direct" + str(i))
            in_str = "lut" + str(LUT_size) + "[" + str(j) + ":" + str(j) + "].out"
            out_str = "soft_logic.out[" + str(j) + ":" + str(j) + "]"
            xprop("input", in_str)
            xprop("output", out_str)
            xclose()
            
            if (outputs_by_frac_stage(frac_stage) > 1):
                char = chr(65 + j)
            else:
                char = ""
            xbegin("pack_pattern")
            xprop("name", "bleF" + str(frac_stage) + char)
            xprop("in_port", in_str)
            xprop("out_port", out_str)
            xcloseend()
            xend()
            i = i + 1
            
        # Special LUT to soft Logic
        if (special_stage):
            xbegin("direct")
            xprop("name", "direct" + str(i))
            xprop("input", "lut" + str(LUT_size_special) + "[0:0].out")
            xprop("output", "soft_logic.out[" + str(num_LUT) + ":" + str(num_LUT) + "]")
            xcloseend()
            i = i + 1     
        
        xend()  # interconnect
        
        xLUT(LUT_size, num_LUT);
        
        if (special_stage):
            xLUT(LUT_size_special, 1)
        
        xend()  # mode
        
    
    xend()  # pb_type soft_logic
    
    xbegin("pb_type")
    xprop("name", "ff")
    xprop("blif_model", ".latch")
    xprop("num_pb", num_FF)
    xprop("class", "flipflop")
    xclose()
    xport("input", "D", 1, c="D")
    xport("output", "Q", 1, c="Q")
    xport("clock", "clk", 1, c="clock")
    
    xbegin("T_setup")
    xprop("value", delay_FF_T_setup)
    xprop("port", "ff.D")
    xprop("clock", "clk")
    xcloseend()
    
    xbegin("T_clock_to_Q")
    xprop("max", delay_FF_T_clk_Q)
    xprop("port", "ff.Q")
    xprop("clock", "clk")
    xcloseend()
        
    xend()  # pb_type ff
    
    xend()  # pb_type ble
    
    xbegin("fc")
    xprop("default_in_type", "frac")
    xprop("default_in_val", Fc_in)
    xprop("default_out_type", "frac")
    xprop("default_out_val", Fc_out)
    xcloseend()
    
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
    
    xend()  # pb_type clb
    
def gen_arch(dir, k_LUT, N_BLE, I_CLB, I_BLE, fracture_level, num_FF, seg_length, tech_nm, crossbar_str):
    global f
    global tabs


    assert(fracture_level == 0 or fracture_level == 1 or fracture_level == 2)
    
    if crossbar_str != "c":
        filename = "k" + str(k_LUT) + "_N" + str(N_BLE) + "_I" + str(I_CLB) + "_Fi" + str(I_BLE) + "_L" + str(seg_length) + "_frac" + str(fracture_level) + "_ff" + str(num_FF) + "_C" + crossbar_str + "_" + str(tech_nm) + "nm.xml"    
    else :
        filename = "k" + str(k_LUT) + "_N" + str(N_BLE) + "_I" + str(I_CLB) + "_Fi" + str(I_BLE) + "_L" + str(seg_length) + "_frac" + str(fracture_level) + "_ff" + str(num_FF) + "_" + str(tech_nm) + "nm.xml"
     
    tabs = 0
    


    m = re.search("(.*vtr_flow)", sys.path[0])
    vtr_flow_dir = m.group(1)
    script_dir = os.path.join(vtr_flow_dir, "scripts", "arch_gen")
    
    
    models = ["multiply", "single_port_ram", "dual_port_ram"]
    cbs = ["io", "clb", "mult_36", "memory"]
    
    f = open(os.path.join(dir, filename), 'w')
    
    xcomment("k = " + str(k_LUT))
    xcomment("N = " + str(N_BLE))
    xcomment("I = " + str(I_CLB))
    xcomment("Fi = " + str(I_BLE))
    xcomment("frac_level = " + str(fracture_level))
    xcomment("ff = " + str(num_FF))
    
    xbegin("architecture")
    xclose()
    
    xbegin("models")
    xclose()
    
    for model in models: 
        xcopy(os.path.join(script_dir, "models", model + ".xml"))
    
    xend()  # Models
    
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
    
    xend()  # chan_width_distr
    
    xbegin("switch_block")
    xprop("type", "wilton")
    xprop("fs", 3)
    xcloseend()
    
    xend()  # Device
    
    xbegin("switchlist")
    xclose()
    xbegin("switch")
    xprop("type", "mux")
    xprop("name", "0")
    xprop("R", switch_R)
    xprop("Cin", switch_Cin)
    xprop("Cout", switch_Cout * wire_C_scaling(tech_nm))
    xprop("Tdel", switch_T_del)
    xprop("mux_trans_size", switch_mux_trans_size)
    xprop("buf_size", switch_buf_size)
    xcloseend()
    xend()  # switchlist
    
    xbegin("segmentlist")
    xclose()
    xbegin("segment")
    xprop("freq", 1.0)
    xprop("length", seg_length)
    xprop("type", "unidir")
    xprop("Rmetal", segment_Rmetal)
    xprop("Cmetal", segment_Cmetal * wire_C_scaling(tech_nm))
    # if (do_power):
    #    xprop("Cmetal_per_m", C_wire_glb(tech_nm))
    xclose()
    
    xbegin("mux")
    xprop("name", "0")
    xcloseend()
    
    xbegin("sb")
    xprop("type", "pattern")
    xclose()
    s = ""
    for i in range(seg_length + 1):
        s = s + "1 "
    s = s.strip()
    xprint(s + "\n", False)
    xend()  # sb
    
       
    xbegin("cb")
    xprop("type", "pattern")
    xclose()
    
    s = ""
    for i in range(seg_length):
        s = s + "1 "
    s = s.strip()
    xprint(s + "\n", False)
    xend()  # cb
    
    xend()  # segment
    xend()  # segmentlist
    
    xbegin("complexblocklist")
    xclose()
    

    
    
    for cb in cbs: 
        if (cb == "clb"):
            xCLB(k_LUT, N_BLE, I_CLB, I_BLE, fracture_level, num_FF, crossbar_str)
        else:
            xcopy(os.path.join(script_dir, "complexblocks", str(tech_nm) + "nm", cb + ".xml"))
     
    xend()  # complexblocklist
    
    if (do_power):
        xbegin("power")
        xclose()
        
        # xbegin("short_circuit_power")
        # xprop("percentage", power_short_circuit_percentage)
        # xcloseend()
        
        xbegin("local_interconnect")
        xprop("C_wire", wire_C_per_m * wire_C_scaling(tech_nm))
        xcloseend()
        
        xbegin("mux_transistor_size")
        xprop("mux_transistor_size", 3)
        xcloseend()
        
        xbegin("FF_size")
        xprop("FF_size", 4)
        xcloseend()
        
        xbegin("LUT_transistor_size")
        xprop("LUT_transistor_size", 4)
        xcloseend()
        
        xend()  # Power
        
        xbegin("clocks")
        xclose()
        xbegin("clock")
        xprop("buffer_size", "auto")
        xprop("C_wire", wire_C_per_m * wire_C_scaling(tech_nm))
        xcloseend()
        xend()  # clocks
    
    xend()  # Architecture
    
    f.close()


# Architecture Options
arches = []
#              k    N       I_CLB   I_BLE   frac    num_FF  seg_length

sweep = 0

if (sweep):
    dir = "C:/Users/Jeff/Dropbox/linux_home/vtr/vtr_flow/arch/power/sweep"
    a_k = [4, 6]
    a_N = [6, 8, 10]
    a_seg = [1, 2, 3, 4]
    a_CLB_frac = [0.6, 0.8]
    
    
    for l_k in (a_k):
        for l_N in (a_N):
            for l_I_BLE in range(l_k, l_k + 3):
                for l_CLB_frac in (a_CLB_frac): 
                    for l_frac in range(0, 2):
                        for l_num_FF in range(1, l_frac + 2):
                            for l_seg_length in a_seg:
                                arches.append([l_k, l_N, int(l_N * l_I_BLE * l_CLB_frac), l_I_BLE, l_frac, l_num_FF, l_seg_length, 45])
    
else:

    # K - N - I - Fi - Frac -  FF - L - 45
    
    # Non-Fractured
    arches.append([6, 10, 40, 6, 0, 1, 4, 45, "c"])
    arches.append([6, 10, 40, 6, 0, 1, 4, 130, "c"])    
    arches.append([6, 10, 40, 6, 0, 1, 4, 22, "c"])
    
    # Fractured LUT
    arches.append([6, 10, 40, 6, 1, 1, 4, 45, "c"])
    arches.append([6, 10, 40, 6, 1, 2, 4, 45, "c"])
    
    # Fi Variation
    arches_fi = []
    arches_fi.append([6, 10, 40, 7, 1, 1, 4, 45, "c"])
    arches_fi.append([6, 10, 40, 7, 1, 2, 4, 45, "c"])
    arches_fi.append([6, 10, 40, 8, 1, 1, 4, 45, "c"])
    arches_fi.append([6, 10, 40, 8, 1, 2, 4, 45, "c"])
    
    # I Variation
    arches_i = []
    arches_i.append([6, 10, 47, 7, 1, 1, 4, 45, "c"])
    arches_i.append([6, 10, 47, 7, 1, 2, 4, 45, "c"])
    arches_i.append([6, 10, 53, 8, 1, 1, 4, 45, "c"])
    arches_i.append([6, 10, 53, 8, 1, 2, 4, 45, "c"])
    
    # L Variation
    arches_l = []
    arches_l.append([6, 10, 40, 6, 0, 1, 1, 45, "c"])
    arches_l.append([6, 10, 40, 6, 0, 1, 2, 45, "c"])
    arches_l.append([6, 10, 40, 6, 0, 1, 3, 45, "c"])
    arches_l.append([6, 10, 40, 6, 0, 1, 5, 45, "c"])
    arches_l.append([6, 10, 40, 6, 0, 1, 6, 45, "c"])
    
    # Crossbar Variation
    arches_c = []
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "5"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "10"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "15"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "20"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "25"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "30"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "35"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "40"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "45"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "50"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "55"])
    arches_c.append([6, 10, 40, 6, 1, 2, 4, 45, "60"])
    
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "5"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "10"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "15"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "20"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "25"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "30"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "35"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "40"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "45"])
    arches_c.append([6, 10, 40, 6, 0, 1, 4, 45, "50"])
    
    
    dir = "C:/Users/Jeff/Dropbox/linux_home/vtr/vtr_flow/arch/power/"
    
    os.path.join(dir, "I")
    
for arch in arches:
    gen_arch(dir, *arch)
    
for arch in arches_i:
    gen_arch(dir, *arch)
    
for arch in arches_l:
    gen_arch(dir, *arch)
  
for arch in arches_fi:
    gen_arch(dir, *arch)  
    
for arch in arches_c:
    gen_arch(dir, *arch)
    
print "Done\n"


