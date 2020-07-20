import sys
import shutil
import subprocess
import time
import glob
import vtr
from pathlib import Path
from collections import OrderedDict
from vtr import CommandError

VTR_STAGE = vtr.make_enum("odin", "abc", 'ace', "vpr")
vtr_stages = VTR_STAGE.reverse_mapping.values()

def run(architecture_file, circuit_file, 
                 power_tech_file=None,
                 start_stage=VTR_STAGE.odin, end_stage=VTR_STAGE.vpr, 
                 command_runner=vtr.CommandRunner(), 
                 temp_dir="./temp", 
                 odin_args=None,
                 abc_args=None,
                 vpr_args=None,
                 keep_intermediate_files=True,
                 keep_result_files=True,
                 min_hard_mult_size=3,
                 min_hard_adder_size=1,
                 check_equivalent=False,
                 check_incremental_sta_consistency = False,
                 use_old_abc_script=False,
                 relax_W_factor=1.3):
    """
    Runs the VTR CAD flow to map the specified circuit_file onto the target architecture_file

    .. note :: Usage: vtr.run(<architecture_file>,<circuit_file>,[OPTIONS])

    Arguments
    =========
        architecture_file : 
            Architecture file to target
        
        circuit_file     : 
            Circuit to implement
    
    Other Parameters
    ----------------
        power_tech_file  :  
            Technology power file.  Enables power analysis and runs ace
        
        start_stage      : 
            Stage of the flow to start at
        
        end_stage        : 
            Stage of the flow to finish at
        
        temp_dir         : 
            Directory to run in (created if non-existent)
        
        command_runner   : 
            A CommandRunner object used to run system commands
        
        verbosity        : 
            Weather to output error description or not
        
        odin_args        : 
            A dictionary of keyword arguments to pass on to ODIN II 
        
        abc_args         : 
            A dictionary of keyword arguments to pass on to ABC
        
        vpr_args         : 
            A dictionary of keyword arguments to pass on to VPR
        
        keep_intermediate_files : 
            Determines if intermediate files are kept or deleted
        
        keep_result_files : 
            Determines if the result files are kept or deleted
        
        min_hard_mult_size : 
            Tells ODIN II the minimum multiplier size that should be implemented using hard multiplier (if available)
        
        min_hard_adder_size : 
            Tells ODIN II the minimum adder size that should be implemented using hard adder (if available).
        
        check_equivalent  : 
            Enables Logical Equivalence Checks
        
        use_old_abc_script : 
            Enables the use of the old ABC script
        
        relax_W_factor    : 
            Factor by which to relax minimum channel width for critical path delay routing
        
        check_incremental_sta_consistency :  
            Do a second-run of the incremental analysis to compare the result files
        
    """
    if odin_args == None:
        odin_args = OrderedDict()
    if abc_args == None:
        abc_args = OrderedDict()
    if vpr_args == None:
        vpr_args = OrderedDict()
        
    #
    #Initial setup
    #

    #Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = vtr.util.verify_file(architecture_file, "Architecture")
    circuit_file = vtr.util.verify_file(circuit_file, "Circuit")
    if power_tech_file != None:
        power_tech_file = vtr.util.verify_file(power_tech_file, "Power tech")
    
    architecture_file_basename =architecture_file.name
    circuit_file_basename = circuit_file.name

    circuit_name = circuit_file.stem
    circuit_ext = circuit_file.suffixes
    architecture_name = architecture_file.stem

    vtr.mkdir_p(temp_dir)
    netlist_ext = ".blif"
    if ".eblif" in circuit_ext:
        netlist_ext = ".eblif"
    #Define useful filenames
    post_odin_netlist = Path(temp_dir)  / (circuit_name + '.odin' + netlist_ext)
    post_abc_netlist =Path(temp_dir)  / (circuit_name + '.abc' + netlist_ext)
    post_ace_netlist =Path(temp_dir)  / (circuit_name + ".ace" + netlist_ext)
    post_ace_activity_file = Path(temp_dir)  / (circuit_name + ".act")
    pre_vpr_netlist = Path(temp_dir)  / (circuit_name + ".pre-vpr" + netlist_ext)
    post_vpr_netlist = Path(temp_dir)  / "vpr.out" #circuit_name + ".vpr.blif"
    lec_base_netlist = None #Reference netlist for LEC
    gen_postsynthesis_netlist = Path(temp_dir) / (circuit_name + "_post_synthesis." + netlist_ext)
    rr_graph_ext=".xml"

    if "blif" in circuit_ext:
        #If the user provided a .blif or .eblif netlist, we use that as the baseline for LEC
        #(ABC can't LEC behavioural verilog)
        lec_base_netlist = circuit_file_basename

    #Copy the circuit and architecture
    circuit_copy = Path(temp_dir)  / circuit_file.name
    architecture_copy = Path(temp_dir)  / architecture_file.name
    shutil.copy(str(circuit_file), str(circuit_copy))
    shutil.copy(str(architecture_file), str(architecture_copy))


    #There are multiple potential paths for the netlist to reach a tool
    #We initialize it here to the user specified circuit and let downstream 
    #stages update it
    next_stage_netlist = circuit_copy

    #
    # RTL Elaboration & Synthesis
    #
    if should_run_stage(VTR_STAGE.odin, start_stage, end_stage):
        if circuit_ext != ".blif":
            vtr.odin.run(architecture_copy, next_stage_netlist, 
                     output_netlist=post_odin_netlist, 
                     command_runner=command_runner, 
                     temp_dir=temp_dir,
                     odin_args=odin_args,
                     min_hard_mult_size=min_hard_mult_size,
                     min_hard_adder_size=min_hard_adder_size)

            next_stage_netlist = post_odin_netlist

            if not lec_base_netlist:
                lec_base_netlist = post_odin_netlist

    #
    # Logic Optimization & Technology Mapping
    #
    if should_run_stage(VTR_STAGE.abc, start_stage, end_stage):
        vtr.abc.run(architecture_copy, next_stage_netlist, 
                output_netlist=post_abc_netlist, 
                command_runner=command_runner, 
                temp_dir=temp_dir,
                abc_args=abc_args,
                keep_intermediate_files=keep_intermediate_files,
                use_old_abc_script=use_old_abc_script)

        next_stage_netlist = post_abc_netlist

        if not lec_base_netlist:
            lec_base_netlist = post_abc_netlist


    #
    # Power Activity Estimation
    #
    if power_tech_file:
        #The user provided a tech file, so do power analysis

        if should_run_stage(VTR_STAGE.ace, start_stage, end_stage):
            vtr.ace.run(next_stage_netlist, old_netlist = post_odin_netlist, output_netlist=post_ace_netlist, 
                    output_activity_file=post_ace_activity_file, 
                    command_runner=command_runner, 
                    temp_dir=temp_dir)

        if not keep_intermediate_files:
            next_stage_netlist.unlink()
            post_odin_netlist.unlink()

        #Use ACE's output netlist
        next_stage_netlist = post_ace_netlist

        if not lec_base_netlist:
            lec_base_netlist = post_ace_netlist
        
        #Enable power analysis in VPR
        vpr_args["power"] = True
        vpr_args["tech_properties"] = str(power_tech_file.resolve())

    #
    # Pack/Place/Route
    #
    if should_run_stage(VTR_STAGE.vpr, start_stage, end_stage):
        #Copy the input netlist for input to vpr
        shutil.copyfile(str(next_stage_netlist), str(pre_vpr_netlist))
        route_fixed_W = "route_chan_width" in vpr_args
        if ("route" in vpr_args or "place" in vpr_args) and not route_fixed_W:
            vpr_args["route_chan_width"] = 300
            route_fixed_W = True
            
        if route_fixed_W:
            #The User specified a fixed channel width
            vtr.vpr.run(architecture_copy, pre_vpr_netlist, circuit_copy.stem,
                    output_netlist=post_vpr_netlist,
                    command_runner=command_runner, 
                    temp_dir=temp_dir, 
                    vpr_args=vpr_args,
                    rr_graph_ext=rr_graph_ext)
        else:
            #First find minW and then re-route at a relaxed W
            vtr.vpr.run_relax_W(architecture_copy, pre_vpr_netlist, circuit_copy.stem,
                            output_netlist=post_vpr_netlist,
                            command_runner=command_runner, 
                            relax_W_factor=relax_W_factor,
                            temp_dir=temp_dir, 
                            vpr_args=vpr_args)

        if not lec_base_netlist:
            lec_base_netlist = pre_vpr_netlist

    #
    # Logical Equivalence Checks (LEC)
    #
    if check_equivalent:
        for file in Path(temp_dir).iterdir():
            if "post_synthesis.blif" in str(file) :
                gen_postsynthesis_netlist = file.name
                break
        vtr.abc.run_lec(lec_base_netlist, gen_postsynthesis_netlist, command_runner=command_runner, temp_dir=temp_dir)

    # Do a second-run of the incremental analysis to compare the result files
    if check_incremental_sta_consistency:
        vtr.vpr.cmp_full_vs_incr_STA(architecture_copy, pre_vpr_netlist, circuit_copy.stem,
                            command_runner=command_runner, 
                            vpr_args=vpr_args,
                            rr_graph_ext=rr_graph_ext,
                            temp_dir=temp_dir
                            )
    
    if not keep_intermediate_files:
        next_stage_netlist.unlink()
        exts = ('.xml','.sdf','.v')
        if not keep_result_files:
            exts += ('.net', '.place', '.route')

        for file in Path(temp_dir).iterdir():
            if file.suffix in exts:
                file.unlink()

        if power_tech_file:
            post_ace_activity_file.unlink()

def should_run_stage(stage, flow_start_stage, flow_end_stage):
    """
    Returns True if stage falls between flow_start_stage and flow_end_stage
    """
    if flow_start_stage <= stage <= flow_end_stage:
        return True
    return False




