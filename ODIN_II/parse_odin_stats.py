#!/usr/bin/env python3
from __future__ import division
from datetime import datetime
import sys, os

current_line = ""
delim = ","
threshold_file = ""

def time_to_millis(time_in):
    time_in.strip()
    time_in.replace(' ', '')
    
    result = 'NaN'


    if 'ms'	in time_in:
        result = str(assert_float(time_in.replace('ms','')))

    elif 's' in time_in:
        result = str(assert_float(time_in.replace('s','')) * 1000.0)

    elif 'm' in time_in:
        result = str(assert_float(time_in.replace('m','')) * 60.0 * 1000.0)

    elif 'h' in time_in:
        result = str(assert_float(time_in.replace('h','')) * 60.0 * 60.0 * 1000.0)

    return result

def help_menu(code):
    if(code == 0):
        print("-------------------------------------")
        print("parse_odin_stats: Tool for parsing and analyzing the output of the --stat flag in Odin_II")
        print("USAGE:\n")
        print("MULTI MODE: -m")
        print("Reads multiple inputs and places their respective metrics in a line within the specified csv/tsv file")
        print("e.g.\t[./parse_odin_stats.py <-m> <output.csv/tsv> <input-1> ... <input-n>")
        print("-------------------------------------")
        print("MULTI-COMPARE MODE: -mc")
        print("Reads multiple inputs and for every two inputs in order prints the difference between them on a third line")
        print("e.g.\t[./exec <-mc> (optional)<--flags/-f> (opt)<--concise> <output.csv> <input-1> ... <input-n>]")
        print("options: --flags")
        print("-con or --concise: don't print diff summary")
        print("-------------------------------------")
        print("COMPARE MODE: -c")
        print("e.g.\t./parse_odin_stats.py <-c> <output.csv> <input.log> <input2.log>")
        print("Compare two stat_gen outputs, write both metrics and diff on csv, creates flag file for results that differ significantly from (optional) threshold file, print summary of diff to stdout")
        print("-------------------------------------")
        print("THRESHOLD GENERATOR MODE:")
        print("Specify +/- % of acceptable change and optionally threshold file (else it will be default filename) to generate file to be used in compareMode")
        print("\t-Will generate same ratio for certain core stats, otherwise must customize your own threshold file with metrics you want and ratio")
        print("\t-Format is metric_name: <ratio>\nmetric_name2: <ratio>")
        print("-------------------------------------")
        print("SPECIFY THRESHOLD FILE:")
        print("\tInclude [-t <threshold_filename>] to either compare mode before input/output to include threshold file to test against")
        print("-------------------------------------")
        print("SIMPLE_MODE:")
        print("Take output of stat_gen as input, parse it and place results as one line in csv/tsv")
        print("e.g.\t[./parse_odin_stats.py <input.log> <output.csv>]\n")
        
    elif(code == 1):
        print("Nothing to compare, use -mc mode")

    elif(code == 2):
        print("\tNo inputs and/or outputs specified, expecting [./exec <-m/mc> (optional)<--flags/-f> (opt)<--concise> <output> <input-1> ... <input-n>]")

    elif(code == 3):
        print("Incorrect threshold generator usage. Acceptable patterns below:")
        print("\n1) [./exec -gen] -> yields thresholds.txt with every value as 0.15 ('-g' also accepted)")
        print("2) [./exec -gen <ratio>] (e.g. -gen 0.25) -> specify ratio to generate thresholds.txt with custom ratio")
        print("3) [./exec -gen <ratio> <outputfilename.txt>] -> yields specified threshold file name with specified ratio (custom name requires custom ratio)\n")

    elif(code==4):
        print("No input specified")
    elif(code==5):
        print("Formatting error: check log file")
    elif(code==6):
        print("Wrong number of arguments, expecting [./exec -c <input.log> <input2.log> (opt)<output.csv>]")
    elif(code==7):
        # this is our only mechanism against the possibility of overwriting the log file, important that input files are .log
        print("\nImproper output format, output file must not begin with '-' or end in .log, use no args or -h/--help for proper format")


def size_to_MiB(size_in):
    size_in = format_token(size_in)

    base_size = 1.0
    if 'b' in size_in:
        base_size = 8.0

    size_in = size_in.replace('i','')
    size_in = size_in.replace('I','')
    size_in = size_in.replace('b','')
    size_in = size_in.replace('B','')

    if 'G' in size_in or 'g' in size_in:
        size_in = str(assert_float(size_in.replace('G','').replace('g','')) / 1024.0 / 1024.0 / base_size)

    elif 'M' in size_in or 'm' in size_in:
        size_in = str(assert_float(size_in.replace('M','').replace('m','')) / 1024.0 / base_size)

    elif 'K' in size_in or 'k' in size_in:
        size_in = str(assert_float(size_in.replace('K','').replace('k','')) / 1024.0 / base_size)

    #we are now in byte
    return str(assert_float(size_in) * 1024.0 * 1024.0)

def contains(line_in, on_tokens):
    for token in on_tokens:
        if token not in line_in:
            return False

    return True

def assert_float(input_str):
    try:
        return float(input_str)
    except ValueError:
        global current_line
        print("Error, line: " + current_line)
        exit(1);

    return str(input_str)

def format_token(input_str):
    return input_str.strip().replace(' ', '').replace('(','').replace(')','').replace('%','').replace(',','')

def format_token_dots(input_str):
    return input_str.strip().replace('.','_')

#strips path portion out of input if it exists
def format_path_token(input_str):
    x = input_str.split("/")
    return x[len(x)-1]

# 1 indexed
def get_token(line_in, index):
    return str(line_in.split(" ")[index-1])

# 0 indexed, can specify split symbol
def get_token_sym(line_in, index, symbol):
    try:
        ret_tok = str(line_in.split(symbol)[index])
    except Exception as e:
        help_menu(5)
        print("Problem Line (first 100 chars): "+line_in[0:100]) # printing the line is potentially risky if it's something extremely long
        print("index:",str(index),"error:", e)
        exit(5)
    return ret_tok

def stringify(value_map, key):
    if key not in value_map:
        return str(float("NaN"))
    else:
        return str(value_map[key])

def get_all_value(value_map, key_list, index):
    return_str = ""
    for key in key_list:
        if(stringify(value_map[index], key) != str(float("NaN"))):
            if(key == "input_file"):
                return_str += format_path_token(stringify(value_map[index], key)) + delim
            else:
                return_str += stringify(value_map[index], key) + delim
        else:
            for values in value_map:
                if key in values:
                    return_str += "NaN" + delim
                    break
    # lose the last comma
    return return_str

# only include keys that have at least one valid entry (but only first one for some reason)
def get_all_valid_key(value_map, key_list):
    return_str = ""
    for key in key_list:
        if(len(value_map) > 1): # I can't remember what this is for
            if key in value_map[0] or key in value_map[1]:
                return_str += key + delim
        else:
            return_str += (key + delim) if (key in value_map[0]) else "" # I don't know why this does this
    return return_str

# TODO this should replace the above function I just don't have time to break the above function, also make this more pythonic
def get_all_valid_key_multi(value_map, key_list):
    return_str = ""
    metric_absent_all = True
    for key in key_list:
        i = 0
        while(i < len(value_map)):
            if key in value_map[i]:
                metric_absent_all = False
            i += 1
        if(not metric_absent_all):
            return_str += (key + delim)
        metric_absent_all = True
    return return_str

# old function, remove or update later
def get_all_key(value_map, key_list):
    return_str = ""
    for key in key_list:
        return_str += key + "\t"

    return return_str

# experimental: something that search directory for threshold file, so this can work in a testing suite
# for now not going to deal with ratios or anything just raw +/- values
# TODO make thresh name not hardcoded, change name of function
def threshold_test():
    thresholds = {}
    try:
        file = threshold_file if(threshold_file != "") else "thresholds.txt"
        ft = open(file, "r")
        if ft == None:
            print("thresholds.txt file not included")
            return
        else:
            for wholeLine in ft:
                parse_line(thresholds, wholeLine)
    except:
        print("unable to open threshold file", file)
        return
    return thresholds

#   slightly redundant but will be useful in producing our final non-redundant output
#   checks if value was flagged as +/- outside of threshold or changed to or from 0
def is_flagged(inLimit, sym1, sym2):
    if(inLimit == "No"):
        return True
    elif((sym1 and not sym2) or (sym2 and not sym1)):
        return True
    else:
        return False

# many different ways to approach this, unclear which is right way, just proof of concept, for now it flags anything +/- threshold limit
# Change to not be abs value if only increases should be flagged
def within_threshold(thresholds, category, diff, percentChange):
    ret_str = ""
    limit = thresholds.get(category, "N/A") #this doesn't work as hoped
    if limit == "N/A":
        return limit
    if abs(percentChange) <= float(limit):
        # This is too messy and is not formatted for percentage
        #ret_str += "Yes\t("+str(abs(int(float(limit) - diff)))+" under limit)"
        ret_str += "Yes"
    else:
        #ret_str += "No \t("+str(abs(int(float(limit) - diff)))+" over limit)"
        ret_str += "No"
    return ret_str

def flag_line(header, inLimit, flagCount, diff, thresholdCoefficient, value2, percentChange):
    line = str(flagCount) + ")  " + header + " flagged because: "
    reason = ""
    if(inLimit == None or inLimit == "N/A"):
        reason += "[Change to/from Zero]: "
        if(diff>0):
            reason += "Value in input1 increased from 0 to "+str(diff)+" (relative to input2)"
        else:
            reason += "Value in input1 decreased from "+str(value2)+" to 0"+" (relative to input2)"
    else:
        # range of non-flagged values is +- tolerance
        tolerance = thresholdCoefficient * value2
        if(diff<0):
            reason += "[Significant Reduction]: Value in input1 ("+str(int(value2+diff))+") decreased below specified threshold ("+str(value2 - (tolerance))+") by "+str(abs(diff - tolerance))
            
        else:
            reason += "[Significant Increase]: Value in input1 ("+str(int(value2+abs(diff)))+") exceeded max threshold ("+str(value2 - (tolerance))+") by "+str(diff - tolerance)
        reason += " ("+str(round(float(percentChange) - thresholdCoefficient*100,2))+"%)" +" (relative to input2)"
    line += reason + "\n\n"
    return line

def output_flagged(flagLines, flagCount, in_file, generate_flag_files):
    timeTag = str(datetime.now()).replace(" ", "_").split(".")[0].replace(":","")
    # TODO  consider either trimming date or removing altogether as it makes the file name rly long
    filename = "flagged_"+format_token_dots(in_file)+"-"+timeTag+".txt" if(generate_flag_files) else "[FLAG OUTPUT DISABLED]"
    print(str(flagCount) + " flags raised, see "+filename+" for details")
    if(generate_flag_files):
        flagout = open(filename, "w+")
        # see specified output csv or diff results for further details
        out_str = "===========Flagged Results===========\n" + flagLines
        flagout.write(out_str)
    
#  line_in1 and 2 in order that they are inputted on cmd line
def get_diff(header_in, line_in1, line_in2, index, generate_flag_files, conciseOutput, printRatios):
    # this won't work if 'columns' aren't aligned, maybe inelegant but no reason for these to not be aligned
    print("======Preliminary QoR Results======\n(a=input1 b=input2)\t(see output csv for more data)\n")
    headerVal = ""
    ret_str = "Difference" + delim
    ratioLine = "Percent" + delim # only used if printRatio true
    if(conciseOutput):
        print("No summary (concise mode)\n")
    else:
        print("\tCategory\t\t\t\tDifference (b-a)\tRatio (b/a)\t\t% Change   (sign)\tWithin Threshold?\tFlagged")
        print("-------------------------------------------------------------------------------------------------------------------------------------------------------")
    headerVal = str(header_in.split(delim)[index])
    # TODO Replace float(symbol*) etc multiple conversions with one conversion (program is lightweight anyway so doesn't matter for perf, just messy)
    # TODO Make the spacing / tabbing situation more consistent and less awful
    # TODO Saying N/A for changes from/to zero is a bit vague, change to something else
    thresholds = threshold_test()
    flags = 0
    flagLine = ""
    input1_name = ""

    while headerVal != "\n":
        if(headerVal == ("input_file")):
            input1_name = get_token_sym(line_in1, index, delim) # we want the second input name to name our flag file after
            input2_name = get_token_sym(line_in2, index, delim)
            print("Testing "+input2_name+" (b) against "+input1_name+" (a)")
            index += 1
            headerVal = str(header_in.split(delim)[index])
            ret_str += "N/A"+delim
            ratioLine += "N/A"+delim
            continue
        symbolB = get_token_sym(line_in2, index, delim)
        symbolA = get_token_sym(line_in1, index, delim)
        # get diff of headerVal, write diff to ret line, check flags and store them as string to be outputted right before function return, prints summary to stdout
        if((symbolB and symbolA) and symbolB.lower() != "nan" and symbolA.lower() != "nan" and symbolA != "\n" and symbolB != "\n"):
            diff = int(float(symbolB) - float(symbolA))
            ret_str += str(diff) + delim
            #sign = "" if (diff == 0) else ("+" if diff > 0 else "-")
            sign = "" if diff <= 0 else "+"
            biggerSmaller = " No Change" if (diff == 0) else (" b > a" if diff > 0 else " a > b")
            spacing = "" if (len(headerVal) >= 15) else "\t"
            ratio ="0.0 (a=b=0)" if (not float(symbolB) and not float(symbolA)) else "undef (a=0)" if (float(symbolB) and not float(symbolA)) else str(round(float(symbolB) / float(symbolA),5))
            # this is a very janky formatting thing, cleanup later
            ratio += " "*(11-len(ratio))
            percentChange = "INF" if float(symbolB) and not float(symbolA) else "0" if diff == 0 else str(round((diff / float(symbolA) * 100),3))
            ratioLine += "INF" if(percentChange=="INF") else "0" if(diff==0) else "-100.0" if float(symbolA) and not float(symbolB) else str(round((diff / float(symbolA)),4)*100)
            ratioLine += delim
            if(thresholds and thresholds.get(headerVal) != None):
                if(float(symbolB) == 0 or float(symbolA) == 0):
                    withinLimit = "N/A"
                else:
                    withinLimit = within_threshold(thresholds, headerVal, diff, float(percentChange))
            else:
                withinLimit = "N/A"
            isFlagged = is_flagged(withinLimit, float(symbolB), float(symbolA))
            flagged = "No"
            if(isFlagged):
                flagged = "Yes"
                flags += 1
                thresholdCoefficient = 0.0
                if(thresholds and thresholds.get(headerVal) != None):
                    thresholdCoefficient = float(thresholds.get(headerVal)) / 100
                flagLine += flag_line(headerVal, withinLimit, flags, diff, thresholdCoefficient, float(symbolA), percentChange)
            percentChange +="%"+" "*(10-len(percentChange))
            if(not conciseOutput):
                print("\t"+headerVal+":\t\t\t"+spacing+sign+str(diff)+"\t\t\t"+str(ratio)+"\t\t"+percentChange+biggerSmaller+" \t"+withinLimit+"\t\t\t"+flagged)
        else:
            ret_str += "NaN"+delim
            ratioLine += "NaN"+delim if(printRatios) else ""
        index += 1
        headerVal = str(header_in.split(delim)[index])
    if(flags >= 1):
        output_flagged(flagLine, flags, input1_name, generate_flag_files)
    ret_str += ("\n"+ratioLine) if(printRatios) else ""
    return ret_str
    

def insert_string(value_map, key, input_str):
    value_map[key] = format_token(input_str)

def insert_decimal(value_map, key, input_str):
    value_map[key] = str(assert_float(format_token(input_str)))

# quickly generates thresholds for core stats, same value for each stat
def generate_thresholds(percentage, filename):
    ft = open(filename,"w+")
    wrt = ""
    coreStats = ["num_nodes"
        , "num_names"
        , "num_latches"
        , "num_edges_fo"
        , "num_input_nodes"
        , "num_top_output_nodes"
        , "max_dist_out"
        , "min_dist_out"
        , "num_already_visited"
        , "top_input_avg_dist_min"
        , "top_input_avg_dist_max"
        , "longest_minpath"
        , "shortest_maxpath"
        , "vcc_fanout_live"
        , "gnd_fanout_live"]
        #, "nodeOutputCount"]
    for header in coreStats:
        wrt += header + ": "+str(percentage)+"\n"
    ft.write(wrt)

# TODO the parser really doesn't like newline after word w/out space, which could cause problems for people creating their own threshold files
def parse_line(benchmarks, line):

    line.strip()
    line.strip("\n")
    line = " ".join(line.split())
    
    global current_line
    current_line = line

    if contains(line, {"Executing simulation with", "threads"}):
        insert_decimal(benchmarks, "max_thread_count", get_token(line,8))

    elif contains(line, {"Simulating", "vectors"}):
        insert_decimal(benchmarks, "number_of_vectors", get_token(line,2))
#file_names, most importantly input_file
    elif contains(line, {"input_file:"}):
        insert_string(benchmarks, "input_file", get_token(line,2))
# unclear we even want this?
    #elif contains(line, {"output_file:"}):
    #    insert_string(benchmarks, "output_file", get_token(line,2))

# PARSE STATS 
    elif contains(line, {"num_nodes:"}):
        insert_decimal(benchmarks, "num_nodes", get_token(line,2))

    elif contains(line, {"num_names:"}):
        insert_decimal(benchmarks, "num_names", get_token(line,2))

    elif contains(line, {"num_latches:"}):
        insert_decimal(benchmarks, "num_latches", get_token(line,2))

    elif contains(line, {"vcc_fanout_live:"}):
        insert_decimal(benchmarks, "vcc_fanout_live", get_token(line,2))

    elif contains(line, {"gnd_fanout_live:"}):
        insert_decimal(benchmarks, "gnd_fanout_live", get_token(line,2))

    elif contains(line, {"num_subckts:"}):
        insert_decimal(benchmarks, "num_subckts", get_token(line,2))
    
    elif contains(line, {"num_logic_nodes:"}):
        insert_decimal(benchmarks, "num_logic_nodes", get_token(line,2))

    elif contains(line, {"num_nets:"}):
        insert_decimal(benchmarks, "num_nets", get_token(line,2))
    
    elif contains(line, {"num_edges_fo:"}):
        insert_decimal(benchmarks, "num_edges_fo", get_token(line,2))

    elif contains(line, {"num_total_input_pins:"}):
        insert_decimal(benchmarks, "num_total_input_pins", get_token(line,2))

    elif contains(line, {"gnd_path_length:"}):
        insert_decimal(benchmarks, "gnd_path_length", get_token(line,2))

    elif contains(line, {"vcc_path_length:"}):
        insert_decimal(benchmarks, "vcc_path_length", get_token(line,2))

    elif contains(line, {"max_level:"}):
        insert_decimal(benchmarks, "max_level", get_token(line,2))
    

    #elif contains(line, {"longest_toplvl_path:"}):
    #    insert_decimal(benchmarks, "longest_toplvl_path", get_token(line,2))

    #elif contains(line, {"shortest_toplvl_path:"}):
    #    insert_decimal(benchmarks, "shortest_toplvl_path", get_token(line,2))

    elif contains(line, {"num_input_nodes:"}):
        insert_decimal(benchmarks, "num_input_nodes", get_token(line,2))

    elif contains(line, {"num_top_output_nodes:"}):
        insert_decimal(benchmarks, "num_top_output_nodes", get_token(line,2))

    elif contains(line, {"num_memories:"}):
        insert_decimal(benchmarks, "num_memories", get_token(line,2))

    elif contains(line, {"num_adders:"}):
        insert_decimal(benchmarks, "num_adders", get_token(line,2))
    
    elif contains(line, {"num_muxes:"}):
        insert_decimal(benchmarks, "num_muxes", get_token(line,2))

    elif contains(line, {"num_carry_func:"}):
        insert_decimal(benchmarks, "num_carry_func", get_token(line,2))

    elif contains(line, {"num_generic_nodes:"}):
        insert_decimal(benchmarks, "num_generic_nodes", get_token(line,2))

    elif contains(line, {"num_unconn:"}):
        insert_decimal(benchmarks, "num_unconn", get_token(line,2))
    
    elif contains(line, {"num_deadends:"}):
        insert_decimal(benchmarks, "num_deadends", get_token(line,2))

    elif contains(line, {"num_deadend_inputs:"}):
        insert_decimal(benchmarks, "num_deadend_inputs", get_token(line,2))

    elif contains(line, {"num_already_visited:"}):
        insert_decimal(benchmarks, "num_already_visited", get_token(line,2))

    elif contains(line, {"avg_fanin_per_node:"}):
        insert_decimal(benchmarks, "avg_fanin_per_node", get_token(line,2))

    elif contains(line, {"avg_fanout_per_net:"}):
        insert_decimal(benchmarks, "avg_fanout_per_net", get_token(line,2))

    elif contains(line, {"avg_output_pins_per_node:"}): # too long
        insert_decimal(benchmarks, "avg_outpins_per_node", get_token(line,2))

    elif contains(line, {"avg_top_lvl_fanout:"}):
        insert_decimal(benchmarks, "avg_top_lvl_fanout", get_token(line,2))

    elif contains(line, {"top_input_fanout:"}):
        insert_decimal(benchmarks, "top_input_fanout", get_token(line,2))

    elif contains(line, {"pad_node_fanout:"}):
        insert_decimal(benchmarks, "pad_node_fanout", get_token(line,2))

    elif contains(line, {"fanin_outputs_total:"}):
        insert_decimal(benchmarks, "fanin_outputs_total", get_token(line,2))

    elif contains(line, {"fanin_outputs_avg:"}):
        insert_decimal(benchmarks, "fanin_outputs_avg", get_token(line,2))

    elif contains(line, {"fanin_node_to_output:"}):
        insert_decimal(benchmarks, "fanin_node_to_output", get_token(line,2))

    elif contains(line, {"max_fanout_node:"}):
        insert_decimal(benchmarks, "max_fanout_node", get_token(line,2))

    elif contains(line, {"max_fanin_node:"}):
        insert_decimal(benchmarks, "max_fanin_node", get_token(line,2))

    elif contains(line, {"avg_toplvl_chain_max_length:"}):
        insert_decimal(benchmarks, "avg_toplvl_chain_max_length", get_token(line,2))

    # new stats
    elif contains(line, {"min_dist_out:"}):
        insert_decimal(benchmarks, "min_dist_out", get_token(line,2))

    elif contains(line, {"max_dist_out:"}):
        insert_decimal(benchmarks, "max_dist_out", get_token(line,2))

    elif contains(line, {"top_input_avg_dist_max:"}):
        insert_decimal(benchmarks, "top_input_avg_dist_max", get_token(line,2))

    elif contains(line, {"top_input_avg_dist_min:"}):
        insert_decimal(benchmarks, "top_input_avg_dist_min", get_token(line,2))

    elif contains(line, {"avg_min_dist_out:"}):
        insert_decimal(benchmarks, "avg_min_dist_out", get_token(line,2))

    elif contains(line, {"longest_minpath:"}):
        insert_decimal(benchmarks, "longest_minpath", get_token(line,2))

    elif contains(line, {"shortest_maxpath:"}):
        insert_decimal(benchmarks, "shortest_maxpath", get_token(line,2))

    elif contains(line, {"vcc_max_path:"}):
        insert_decimal(benchmarks, "vcc_max_path", get_token(line,2))

    elif contains(line, {"gnd_max_path:"}):
        insert_decimal(benchmarks, "gnd_max_path", get_token(line,2))

    elif contains(line, {"vcc_min_path:"}):
        insert_decimal(benchmarks, "vcc_min_path", get_token(line,2))

    elif contains(line, {"gnd_min_path:"}):
        insert_decimal(benchmarks, "gnd_min_path", get_token(line,2))

    elif contains(line, {"gnd_fanout_all:"}):
        insert_decimal(benchmarks, "gnd_fanout_all", get_token(line,2))

    elif contains(line, {"vcc_fanout_all:"}):
        insert_decimal(benchmarks, "vcc_fanout_all", get_token(line,2))

    elif contains(line, {"pad_fanout_all:"}):
        insert_decimal(benchmarks, "pad_fanout_all", get_token(line,2))

    elif contains(line, {"max_input_pathrange:"}):
        insert_decimal(benchmarks, "max_input_pathrange", get_token(line,2))

    elif contains(line, {"min_input_pathrange:"}):
        insert_decimal(benchmarks, "min_input_pathrange", get_token(line,2))

    elif contains(line, {"avg_pathrange:"}):
        insert_decimal(benchmarks, "avg_pathrange", get_token(line,2))

    elif contains(line, {"top_lvl_avg_max:"}):
        insert_decimal(benchmarks, "top_lvl_avg_max", get_token(line,2))

    #elif contains(line, {"avg_node_max_dist:"}):
    #   insert_decimal(benchmarks, "avg_node_max_dist", get_token(line,2))

##################################
    elif contains(line, {"num_output_nodes:"}):
        insert_decimal(benchmarks, "num_output_nodes", get_token(line,2))

# The name of this value is perhaps more clear than max_dist_out but less accurate
#    elif contains(line, {"Longest Chain:"}):
#        insert_decimal(benchmarks, "longest_chain", get_token(line,2))

    elif contains(line, {"Simulation time:"}):
        insert_decimal(benchmarks, "simulation_time", time_to_millis(get_token(line,3)))

    elif contains(line, {"Elapsed time:"}):
        insert_decimal(benchmarks, "elapsed_time", time_to_millis(get_token(line,3)))

    elif contains(line, {"Coverage:"}):
        insert_decimal(benchmarks, "percent_coverage", get_token(line,3))

    elif contains(line, {"Odin ran with exit status:"}):
        insert_decimal(benchmarks, "exit_code", get_token(line,6))

    elif contains(line, {"Odin II took", "seconds", "(max_rss"}):
        insert_decimal(benchmarks, "total_time", time_to_millis(get_token(line,4) + 's'))
        insert_decimal(benchmarks, "max_rss", size_to_MiB(get_token(line,7) + get_token(line,8)))

    elif contains(line, {"context-switches #"}):
        insert_decimal(benchmarks, "context_switches", get_token(line,1))

    elif contains(line, {"cpu-migrations #"}):
        insert_decimal(benchmarks, "cpu_migration", get_token(line,1))

    elif contains(line, {"page-faults #"}):
        insert_decimal(benchmarks, "page_faults", get_token(line,1))

    elif contains(line, {"stalled-cycles-frontend #"}):
        insert_decimal(benchmarks, "stalled_cycle_frontend", get_token(line,1))

    elif contains(line, {"stalled-cycles-backend #"}):
        insert_decimal(benchmarks, "stalled_cycle_backend", get_token(line,1))

    elif contains(line, {"cycles #"}):
        insert_decimal(benchmarks, "cycles", get_token(line,1))

    elif contains(line, {"branches #"}):
        insert_decimal(benchmarks, "branches", get_token(line,1))

    elif contains(line, {"branch-misses #"}):
        insert_decimal(benchmarks, "branch_misses", get_token(line,1))

    elif contains(line, {"LLC-loads #"}):
        insert_decimal(benchmarks, "llc_loads", get_token(line,1))

    elif contains(line, {"LLC-load-misses #"}):
        insert_decimal(benchmarks, "llc_load_miss", get_token(line,1))

    elif contains(line, {"CPU:"}):
        insert_decimal(benchmarks, "percent_cpu_usage", get_token(line,2))

    elif contains(line, {"Minor PF:"}):
        insert_decimal(benchmarks, "minor_page_faults", get_token(line,2))
    # Unimplemented   
    """elif contains(line, {"Connections:"}):
        insert_decimal(benchmarks, "number_of_connections", get_token(line,2))

    elif contains(line, {"Threads:"}):
        insert_decimal(benchmarks, "used_threads", get_token(line,2))	

    elif contains(line, {"Degree:"}):
        insert_decimal(benchmarks, "degree", get_token(line,2))

    elif contains(line, {"Stages:"}):
        insert_decimal(benchmarks, "number_of_stages", get_token(line,2))
    """


def main():
    if(len(sys.argv)<2 or sys.argv[1] == "--help" or sys.argv[1] == "-h"):
        help_menu(0)
        exit(0)
    
    benchmarks = [{}]
    key_list = [
        "max_thread_count"
        , "number_of_vectors"
        , "input_file"
        , "num_nodes"
        , "num_names"
        , "num_latches"
        , "num_input_nodes"
        , "num_top_output_nodes"
        , "max_depth"
        , "longest_chain"
        , "output_counter"
        , "num_subckts"
        , "num_unconn"
        , "num_nets"
        , "num_logic_nodes"
        , "num_memories"
        , "num_muxes"
        , "num_adders"
        , "num_total_input_pins"
        , "num_carry_func"
        , "num_generic_nodes"
        , "num_deadends"
        , "num_deadend_inputs"
        , "num_already_visited"
        , "avg_outpins_per_node"
        , "avg_fanin_per_node"
        , "avg_fanout_per_net"
        , "top_input_fanout"
        , "avg_top_lvl_fanout"
        , "pad_node_fanout"
        , "fanin_outputs_total"
        , "fanin_outputs_avg"
        , "num_nodes_to_output"
        , "max_fanout_node"
        , "max_fanin_node"
        , "max_level"
        , "gnd_path_length"
        , "vcc_path_length"
        , "vcc_max_path"
        , "vcc_min_path"
        , "max_dist_out"
        , "min_dist_out"
        , "top_input_avg_dist_min"
        , "top_input_avg_dist_max"
        #, "longest_toplvl_path"
        #, "shortest_toplvl_path"
        ,"shortest_maxpath"
        , "longest_minpath"
        , "top_lvl_avg_max"
        , "avg_node_max_dist"
        , "avg_toplvl_chain_max_length"
        , "longest_latch_chain"
        , "longest_consecutive_latch_chain"
        , "num_edges_fo"
        , "num_edges_fi"
        #######################
        , "vcc_fanout_live"
        , "gnd_fanout_live"
        , "pad_fanout_live"
        , "gnd_fanout_all"
        , "vcc_fanout_all"
        , "pad_fanout_all"
        , "max_input_pathrange"
        , "min_input_pathrange"
        , "output_file"
        ########################## from prev parser
        , "simulation_time"
        , "elapsed_time"
        , "percent_coverage"
        , "exit_code"
        , "total_time"
        , "max_rss"
        , "context_switches"
        , "cpu_migration"
        , "page_faults"
        , "stalled_cycle_frontend"
        , "stalled_cycle_backend"
        , "cycles"
        , "branches"
        , "branch_misses"
        , "llc_loads"
        , "llc_load_miss"
        , "percent_cpu_usage"
        , "minor_page_faults"
    ]
        #removed for now - unimplemented
    """ 
        , "number_of_connections"
        , "used_threads"
        , "degree"
        , "number_of_stages"
        """

    """
    # TODO add --help / -h flag and put all formatting options in sep function, call function on error or help
    # multimode, multiple inputs into csv and do nothing else 
    # TODO Streamline all this args stuff into something more elegant
    # Minimum of 3 inputs required
    # TODO (maybe) Could do diffs on these as well as long as they are bunched in twos but that is perhaps overkill
    # TODO Maybe add percent changes as a fourth line
    # this hinges on the x.abc.blif a.odin.blif format generated by run_vtr_flow and supplied in alphabetical order
    # This is kind of the inverse of how we presently use compare mode, which assumes the 'reduced' log will be compared against input1
    # NOTE This now by default does not produce flag files for -mc mode (but still flag warnings) unless "--flags explicitly specified 
    # Also --concise flag disables printing of summary to stdout
    # TODO Multicompare works fine with 2 inputs and so compareMode is somewhat redundant, only different in defaulting to flags on
    # Another way to do multicompare would be to test every line against every previous line, not sure if that is better or worse than using pairs"""
    global threshold_file
    i = 0
    global delim
    output_file = ""
    conciseOutput = False
    printRatios = False
    flag_offset = 0      # rename to something more general given its dual role
    flags = False   # to handle disabling the flag files which gets to be very unwieldy
    j = 2
    while(j < 8): # again this should maybe check against -mc but at least this is stable and doesn't require handling
        if(j < len(sys.argv)-2 ): # if you specify flags and no input then nothing should happen
            if(sys.argv[j] == "--flags" or sys.argv[j] == "-f"): # This should check if(-mc) but this way they can specify --flags and be ignored and not break everything
                if(sys.argv[1] != "-mc" and sys.argv[1] != "-c"):
                    help_menu(1)
                flags = True
                flag_offset += 1
            elif(sys.argv[j] == "--concise" or sys.argv[j] == "-con"):
                conciseOutput = True
                flag_offset += 1
            elif(sys.argv[j] == "--ratio" or sys.argv[j] == "-r" or sys.argv[j] == "-p"): # print ratios as well as diff, remove the r part it's not a ratio anymore
                printRatios = True
                flag_offset+=1
            elif(sys.argv[j] == "-t"): # if you really wanted to screw this up you could stick it in the middle of the outputs and mess up the offset
                if(sys.argv[j+1][0] != "-"):
                    threshold_file = sys.argv[j+1]
                    flag_offset += 2
                    print("Threshold File: "+threshold_file)
        else:
            break
        j+=1
    if(sys.argv[1] == "-m" or sys.argv[1] == "-mc"):
        if(len(sys.argv) <= 3):
            help_menu(2)
            exit(1)

        inputCount = (len(sys.argv)-(3 + flag_offset))
        if(inputCount==0):
            help_menu(4)
            exit(4)

        if(output_file[len(output_file)-4 : len(output_file)] != ".log"):
            output_file = sys.argv[2+flag_offset]
        else: # this almost is not worth the effort
            output_file = "default_out.csv"
            inputCount -= 1
            flag_offset -= 1
        if(output_file[0]=="-"):
            help_menu(7)
            exit(7)
        if(output_file[len(output_file)-4:len(output_file)] == ".tsv"): # this is used verbatim below, think of streamlining this
            delim = "\t" # default is ","

        values_in = [""]*inputCount
        log_file_to_parse = [""]*inputCount
        header_in = ""
        # output must always be at end of flags, which isn't very versatile might change, also light guard against not including an output file only works if log file explicitly is .log

        print("printing to output_file: "+output_file)
        
        # TODO Maybe guard against inputs starting with -
        while(i < inputCount):
            benchmarks.append({})
            log_file_to_parse[i] = sys.argv[i+(3+flag_offset)]
            fileContext = open(log_file_to_parse[i], "r")
            for wholeLine in fileContext:
                parse_line(benchmarks[i], wholeLine)
            i += 1
        i = 0
        while(i < inputCount):
            values_in[i] += "Input" + str(i+1) + delim + get_all_value(benchmarks, key_list, i) + "\n"
            i += 1
        # TODO Some kind of check in case one of the inputs is totally empty or invalid
        header_in += "Headers"+delim+get_all_valid_key_multi(benchmarks, key_list) + "\n"
        f = open(output_file,"w+")
        f.write(header_in)
        alternator = 0 # this is probably ... not the best way to do a toggle
        for index, vals in enumerate(values_in):
            f.write(vals) # write out line to csv output
            alternator += 1
            # compare every second line with preceding line
            if(sys.argv[1] == "-mc" and alternator % 2 == 0):
                # verify this print doesn't break anything with > 2 inputs
                print("Logfiles: Input1=\""+log_file_to_parse[alternator-2]+"\" | Input2=\""+log_file_to_parse[alternator-1]+"\"\n") # to reduce confusion when comparing same input file names but different logs
                line1 = values_in[index-1] # preceding line
                f.write(get_diff(header_in, line1, vals, 1, flags, conciseOutput, printRatios)+"\n")
        f.close()
        exit(0)
    # generate thresholds file
    if len(sys.argv) >= 2 and (sys.argv[1] == "-gen" or sys.argv[1] == "-g"):
        # if you specify file you must specify percent first
        try:
            percent = 0.15 if(len(sys.argv) < 3) else float(sys.argv[2])
        except:
            help_menu(3)
            exit(3)
        threshold_file = "thresholds.txt" if len(sys.argv) < 4 else sys.argv[3]
        # if you want to specify a file you have to put in -f 
        print("Generating "+threshold_file+" for core stats, +/- "+str(100*percent)+"%")
        generate_thresholds(percent, threshold_file)
        exit(0)

    # at this point we've fallen through to compare and/or simple mode
    log_file_to_parse = ["", ""]

    compareMode = 0 
    # -c == compareMode (TODO maybe this should not alter the original csv supplied to it)
    # TODO In making -mc mode the simple mode and -c were not similarly upgraded and/or tested so do that
    # TODO Make -t part of -mc
    if(sys.argv[1] == "-c"):
        compareMode = 1
        #there should be a better argument control structure, this really got out of hand
        if(len(sys.argv)<(4+flag_offset)):
            help_menu(6)
            exit(6)
        output_file = "default_out.csv" if (len(sys.argv) < 5 + flag_offset) else sys.argv[2+flag_offset]
        while i < len(sys.argv)-1: # This now exists only to pickup the (non-specified) use-case of using -t <thresh> after input2 in compare mode
            if(threshold_file != ""):
                break
            if(sys.argv[i] == "-t"):
                threshold_file = sys.argv[i+1]
                print("Threshold File: "+threshold_file)
            i += 1

        log_file_to_parse[0] = sys.argv[3+flag_offset]
        log_file_to_parse[1] = sys.argv[4+flag_offset]
    else: # simple mode, turn one input into an output csv/tsv
        log_file_to_parse[0] = sys.argv[1]
        output_file = "default_out.csv" if(len(sys.argv) < 3) else sys.argv[2+flag_offset] # offset should = 0 here but in case it doesn't just offset args and push thru

    if(output_file[0]=="-" or output_file[len(output_file)-4 : len(output_file)] == ".log"):
            help_menu(7)
            exit(7)
    if(output_file[len(output_file)-3:len(output_file)] == "tsv"):
        delim = "\t" # default is ","
    i = 0
    f = open(output_file,"w+")
    line1 = "" # The stats we want to evaluate
    line2 = "" # The stats we are comparing against
    header_in = ""

    # Compare or simple mode -- TODO This has become somewhat inelegant and redundant as -mc will handle 2 or more inputs and this handles 1 or 2
    values_in = [""]*(compareMode+1)
    print("Output CSV: "+output_file)
    if(compareMode): benchmarks.append({}) # benchmark is now list of two dictionaries
    while i <= compareMode:
        fileContext = open(log_file_to_parse[i], "r")
        for wholeLine in fileContext:
            parse_line(benchmarks[i], wholeLine)
        i += 1

    # no longer any need for line1 and line2 just use the list values themselves as strings
    values_in[0] += "Input" + str(1) + delim + get_all_value(benchmarks, key_list, 0) + "\n"
    line1 = values_in[0]
    # Going through entire keylist for just input files is a bit inefficient but acceptable for now
    # this is supposed to strip headers that are missing both values but it might do it if only second num is missing, can't tell
    header_in += "Headers"+delim+get_all_valid_key(benchmarks, key_list) + "\n"
    f.write(header_in)
    f.write(values_in[0])
    # these are reversed because it was backwards, 'a' is baseline
    if(compareMode):
        values_in[1] += "Input" + str(2) + delim + get_all_value(benchmarks, key_list, 1) + "\n"
        line2 = values_in[1] # TODO remove line1 and line2 probably
        f.write(values_in[1])
        print("====================")
        flag = 0
        print("Logfiles: Input1=\""+log_file_to_parse[0]+"\" | Input2=\""+log_file_to_parse[1]+"\"\n") # to reduce confusion when comparing same input file names but different logs
        f.write(get_diff(header_in, line1, line2, 1, flags, conciseOutput, printRatios) + "\n")
    f.close()
    exit(0)
    #TODO have some way to have a different exit code if flags or something


if __name__ == "__main__":
    main()