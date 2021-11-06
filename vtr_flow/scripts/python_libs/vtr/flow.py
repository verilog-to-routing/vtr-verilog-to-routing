"""
    Module to run the VTR flow. This module calls other modules that then access the tools like VPR.
"""
import shutil
from pathlib import Path
from collections import OrderedDict
from enum import Enum
import vtr


class VtrStage(Enum):
    """
        Enum class for the VTR stages\
    """

    ODIN = 1
    YOSYS = 2
    ABC = 3
    ACE = 4
    VPR = 5

    def __le__(self, other):
        if self.__class__ is other.__class__:
            return int(self.value) <= other.value
        return NotImplemented

    def __ge__(self, other):
        if self.__class__ is other.__class__:
            return int(self.value) >= other.value
        return NotImplemented


# pylint: disable=too-many-arguments, too-many-locals, too-many-branches, too-many-statements
def run(
    architecture_file,
    circuit_file,
    power_tech_file=None,
    include_files=None,
    start_stage=VtrStage.ODIN,
    end_stage=VtrStage.VPR,
    command_runner=vtr.CommandRunner(),
    temp_dir=Path("./temp"),
    odin_args=None,
    yosys_args=None,
    abc_args=None,
    vpr_args=None,
    keep_intermediate_files=True,
    keep_result_files=True,
    odin_config=None,
    yosys_script=None,
    min_hard_mult_size=3,
    min_hard_adder_size=1,
    check_equivalent=False,
    check_incremental_sta_consistency=False,
    use_old_abc_script=False,
    relax_w_factor=1.3,
    check_route=False,
    check_place=False,
):
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
            whether to output error description or not

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
            Tells ODIN II/YOSYS the minimum multiplier size that should
            be implemented using hard multiplier (if available)

        min_hard_adder_size :
            Tells ODIN II/YOSYS the minimum adder size that should be implemented
            using hard adder (if available).

        check_equivalent  :
            Enables Logical Equivalence Checks

        use_old_abc_script :
            Enables the use of the old ABC script

        relax_w_factor    :
            Factor by which to relax minimum channel width for critical path delay routing

        check_incremental_sta_consistency :
            Do a second-run of the incremental analysis to compare the result files

        check_route:
            Check first placement and routing by enabling VPR analysis

        check_place:
            Route existing placement by enabling VPR routing.
    """

    #
    # Initial setup
    #
    vpr_args = OrderedDict() if not vpr_args else vpr_args
    odin_args = OrderedDict() if not odin_args else odin_args
    yosys_args = OrderedDict() if not yosys_args else yosys_args
    abc_args = OrderedDict() if not abc_args else abc_args
    # Verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = vtr.util.verify_file(architecture_file, "Architecture")
    circuit_file = vtr.util.verify_file(circuit_file, "Circuit")
    if architecture_file.suffix != ".xml":
        raise vtr.VtrError(
            "Expected Architecture file as second argument (was {})".format(architecture_file.name)
        )
    power_tech_file = vtr.util.verify_file(power_tech_file, "Power") if power_tech_file else None
    temp_dir = Path(temp_dir)
    temp_dir.mkdir(parents=True, exist_ok=True)
    netlist_ext = ".blif" if ".eblif" not in circuit_file.suffixes else ".eblif"

    # Define useful filenames
    post_odin_netlist = temp_dir / (circuit_file.stem + ".odin" + netlist_ext)
    post_yosys_netlist = temp_dir / (circuit_file.stem + ".yosys" + netlist_ext)
    post_abc_netlist = temp_dir / (circuit_file.stem + ".abc" + netlist_ext)
    post_ace_netlist = temp_dir / (circuit_file.stem + ".ace" + netlist_ext)
    post_ace_activity_file = temp_dir / (circuit_file.stem + ".act")
    pre_vpr_netlist = temp_dir / (circuit_file.stem + ".pre-vpr" + netlist_ext)

    # If the user provided a .blif or .eblif netlist, we use that as the baseline for LEC
    # (ABC can't LEC behavioural verilog)
    lec_base_netlist = circuit_file.name if "blif" in circuit_file.suffixes else None
    # Reference netlist for LEC

    gen_postsynthesis_netlist = temp_dir / (circuit_file.stem + "_post_synthesis" + netlist_ext)

    # Copy the circuit and architecture
    circuit_copy = temp_dir / circuit_file.name
    architecture_copy = temp_dir / architecture_file.name
    shutil.copy(str(circuit_file), str(circuit_copy))
    shutil.copy(str(architecture_file), str(architecture_copy))

    # Check whether any inclulde is specified
    if include_files:
        # Verify include files are Paths or convert them to Path + check that they exist
        # Copy include files to the run directory
        for include in include_files:
            include_file = vtr.util.verify_file(include, "Circuit")
            include_copy = temp_dir / include_file.name
            shutil.copy(str(include), str(include_copy))

    # There are multiple potential paths for the netlist to reach a tool
    # We initialize it here to the user specified circuit and let downstream
    # stages update it
    next_stage_netlist = circuit_copy

    #
    # RTL Elaboration & Synthesis (ODIN-II)
    #
    if should_run_stage(VtrStage.ODIN, start_stage, end_stage) and circuit_file.suffixes != ".blif":
        vtr.odin.run(
            architecture_copy,
            next_stage_netlist,
            include_files,
            output_netlist=post_odin_netlist,
            command_runner=command_runner,
            temp_dir=temp_dir,
            odin_args=odin_args,
            odin_config=odin_config,
            min_hard_mult_size=min_hard_mult_size,
            min_hard_adder_size=min_hard_adder_size,
        )

        next_stage_netlist = post_odin_netlist

        lec_base_netlist = post_odin_netlist if not lec_base_netlist else lec_base_netlist
    #
    # RTL Elaboration & Synthesis (YOSYS)
    #
    elif should_run_stage(VtrStage.YOSYS, start_stage, end_stage):
        vtr.yosys.run(
            architecture_copy,
            next_stage_netlist,
            include_files,
            output_netlist=post_yosys_netlist,
            command_runner=command_runner,
            temp_dir=temp_dir,
            yosys_args=yosys_args,
            yosys_script=yosys_script,
            min_hard_mult_size=min_hard_mult_size,
            min_hard_adder_size=min_hard_adder_size,
        )

        next_stage_netlist = post_yosys_netlist

        lec_base_netlist = post_yosys_netlist if not lec_base_netlist else lec_base_netlist

    #
    # Logic Optimization & Technology Mapping
    #
    if should_run_stage(VtrStage.ABC, start_stage, end_stage):
        vtr.abc.run(
            architecture_copy,
            next_stage_netlist,
            output_netlist=post_abc_netlist,
            command_runner=command_runner,
            temp_dir=temp_dir,
            abc_args=abc_args,
            keep_intermediate_files=keep_intermediate_files,
            use_old_abc_script=use_old_abc_script,
        )

        next_stage_netlist = post_abc_netlist
        lec_base_netlist = post_abc_netlist if not lec_base_netlist else lec_base_netlist

    #
    # Power Activity Estimation
    #
    if power_tech_file:
        # The user provided a tech file, so do power analysis

        if should_run_stage(VtrStage.ACE, start_stage, end_stage):
            vtr.ace.run(
                next_stage_netlist,
                old_netlist=post_odin_netlist,
                output_netlist=post_ace_netlist,
                output_activity_file=post_ace_activity_file,
                command_runner=command_runner,
                temp_dir=temp_dir,
            )

        if not keep_intermediate_files:
            next_stage_netlist.unlink()
            post_odin_netlist.unlink()

        # Use ACE's output netlist
        next_stage_netlist = post_ace_netlist
        lec_base_netlist = post_ace_netlist if not lec_base_netlist else lec_base_netlist

        # Enable power analysis in VPR
        vpr_args["power"] = True
        vpr_args["tech_properties"] = str(power_tech_file.resolve())

    #
    # Pack/Place/Route
    #
    if should_run_stage(VtrStage.VPR, start_stage, end_stage):
        # Copy the input netlist for input to vpr
        shutil.copyfile(str(next_stage_netlist), str(pre_vpr_netlist))
        route_fixed_w = "route_chan_width" in vpr_args
        if (check_route or check_place) and not route_fixed_w:
            vpr_args["route_chan_width"] = 300
            route_fixed_w = True

        if route_fixed_w:
            # The User specified a fixed channel width
            do_second_run = False
            second_run_args = vpr_args

            if "write_rr_graph" in vpr_args or "analysis" in vpr_args or "route" in vpr_args:
                do_second_run = True

            vtr.vpr.run(
                architecture_copy,
                pre_vpr_netlist,
                circuit_copy.stem,
                command_runner=command_runner,
                temp_dir=temp_dir,
                vpr_args=vpr_args,
            )
            if do_second_run:
                # Run vpr again with additional parameters.
                # This is used to ensure that files generated by VPR can be re-loaded by it
                rr_graph_ext = (
                    Path(second_run_args["write_rr_graph"]).suffix
                    if "write_rr_graph" in second_run_args
                    else ".xml"
                )
                if check_place:
                    second_run_args["route"] = True
                if check_route:
                    second_run_args["analysis"] = True
                vtr.vpr.run_second_time(
                    architecture_copy,
                    pre_vpr_netlist,
                    circuit_copy.stem,
                    command_runner=command_runner,
                    temp_dir=temp_dir,
                    second_run_args=second_run_args,
                    rr_graph_ext=rr_graph_ext,
                )
        else:
            # First find minW and then re-route at a relaxed W
            vtr.vpr.run_relax_w(
                architecture_copy,
                pre_vpr_netlist,
                circuit_copy.stem,
                command_runner=command_runner,
                relax_w_factor=relax_w_factor,
                temp_dir=temp_dir,
                vpr_args=vpr_args,
            )
        lec_base_netlist = pre_vpr_netlist if not lec_base_netlist else lec_base_netlist

    #
    # Logical Equivalence Checks (LEC)
    #
    if check_equivalent:
        for file in Path(temp_dir).iterdir():
            if "post_synthesis.blif" in str(file):
                gen_postsynthesis_netlist = file.name
                break
        vtr.abc.run_lec(
            lec_base_netlist,
            gen_postsynthesis_netlist,
            command_runner=command_runner,
            temp_dir=temp_dir,
        )

    # Do a second-run of the incremental analysis to compare the result files
    if check_incremental_sta_consistency:
        vtr.vpr.cmp_full_vs_incr_sta(
            architecture_copy,
            pre_vpr_netlist,
            circuit_copy.stem,
            command_runner=command_runner,
            vpr_args=vpr_args,
            temp_dir=temp_dir,
        )

    if not keep_intermediate_files:
        delete_intermediate_files(
            next_stage_netlist,
            post_ace_activity_file,
            keep_result_files,
            temp_dir,
            power_tech_file,
        )


# pylint: enable=too-many-arguments, too-many-locals, too-many-branches, too-many-statements


def delete_intermediate_files(
    next_stage_netlist,
    post_ace_activity_file,
    keep_result_files,
    temp_dir,
    power_tech_file,
):
    """
    delete intermediate files
    """
    next_stage_netlist.unlink()
    exts = (".xml", ".sdf", ".v")
    exts += (".net", ".place", ".route") if not keep_result_files else None

    for file in temp_dir.iterdir():
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
