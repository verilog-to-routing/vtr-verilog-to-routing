"""module to run mosaic as a synthesis frontend for the vtr flow.

mosaic is a yosys plugin that combines wildebeest-originated passes
(max_level / clk_domains) with mosaic-only arch rules (-vtr_arch).
"""

import os
import re
import shutil
import sys
import warnings
from collections import OrderedDict
from pathlib import Path
import vtr

# supported input file types by the mosaic yosys template
FILE_TYPES = {
    ".v": "Verilog",
    ".vh": "Verilog",
    ".sv": "SystemVerilog",
    ".svh": "SystemVerilog",
}


def create_circuits_list(main_circuit, include_files):
    """build the circuit + include list for the mosaic yosys script."""
    # copied locally so mosaic does not depend on or modify the parmys helper.
    circuit_list = []
    if include_files:
        for include in include_files:
            file_extension = os.path.splitext(include)[-1]
            # drop includes outside mosaic's hdl allowlist; the file is already
            # copied into the temp folder by the flow harness.
            if file_extension not in FILE_TYPES:
                continue
            include_file = vtr.verify_file(include, "Circuit")
            circuit_list.append(include_file.name)
    circuit_list.append(main_circuit.name)
    return circuit_list


def resolve_arch_support_dir(architecture_file_path):
    """resolve the mosaic per-arch policy support dir for an architecture xml.

    looks for vtr_flow/misc/mosaic/<arch_stem>/arch_config.tcl only and returns
    none when missing so synthesis runs facts-only from the arch xml with no
    silent family fallback or stem alias table.
    """
    arch_path = Path(architecture_file_path)
    arch_stem = arch_path.stem
    misc_path = Path(vtr.paths.mosaic_misc_path)
    support_dir = misc_path / arch_stem
    config_file = support_dir / "arch_config.tcl"

    if not config_file.is_file():
        warnings.warn(
            "mosaic: no policy support dir for arch '{}'; "
            "running facts-only (arch_facts.tcl from xml, no arch_config.tcl). "
            "add {} to supply costs/scripts/thresholds.".format(arch_stem, config_file),
            stacklevel=2,
        )
        return None

    return support_dir


# pylint: disable=too-many-arguments, too-many-locals
def init_script_file(
    yosys_script_full_path,
    circuit_list,
    top_module,
    raw_netlist_name,
    architecture_file_path,
):
    """fill the template tokens in the copied mosaic yosys script."""
    # yosys tcl wants forward slashes even on windows
    support_dir = resolve_arch_support_dir(architecture_file_path)
    if support_dir is None:
        arch_support_dir = ""
    else:
        arch_support_dir = str(support_dir.resolve()).replace("\\", "/")
    template_dir = str(vtr.paths.mosaic_template_path.resolve()).replace("\\", "/")

    # YYY makes this the mosaic leg because max_level -clk2clk takes clock cut
    # points from the arch xml instead of vendor cell lists.
    vtr_arch_flag = "-vtr_arch {}".format(architecture_file_path)

    vtr.file_replace(
        yosys_script_full_path,
        {
            "XXX": " ".join(str(s) for s in circuit_list),
            "TDIR": template_dir,
            "TTT": top_module,
            "ZZZ": raw_netlist_name,
            "ARCH_SUPPORT_DIR": arch_support_dir,
            "VVV": architecture_file_path,
            "YYY": vtr_arch_flag,
        },
    )


def parse_arch_blif_model_names(arch_xml_path):
    """collect model names declared in the arch xml."""
    text = Path(arch_xml_path).read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r'<model\s+name="([^"]+)"', text))


def prune_blif_models_not_in_arch(blif_path, arch_xml_path):
    """drop unused blackbox lib .model blocks (e.g. dffes) the arch never uses."""
    allowed = parse_arch_blif_model_names(arch_xml_path)
    text = blif_path.read_text(encoding="utf-8", errors="replace")
    used_subckts = set(re.findall(r"^\.subckt\s+(\S+)", text, re.MULTILINE))

    blocks = [b for b in re.split(r"(?=^\.model )", text, flags=re.MULTILINE) if b.strip()]

    # yosys prepends blackbox lib models before the design model, so the top is
    # the first .model whose block is not a blackbox lib entry.
    top_model = None
    for block in blocks:
        model_match = re.match(r"^\.model\s+(\S+)", block, re.MULTILINE)
        if model_match and ".blackbox" not in block:
            top_model = model_match.group(1)
            break

    kept_blocks = []
    for block in blocks:
        model_match = re.match(r"^\.model\s+(\S+)", block, re.MULTILINE)
        if not model_match:
            kept_blocks.append(block)
            continue
        model_name = model_match.group(1)
        is_blackbox_lib = ".blackbox" in block
        if (
            is_blackbox_lib
            and model_name not in allowed
            and model_name not in used_subckts
            and model_name != top_model
        ):
            continue
        kept_blocks.append(block)

    out_text = "".join(kept_blocks)
    if out_text and not out_text.endswith("\n"):
        out_text += "\n"
    blif_path.write_text(out_text, encoding="utf-8")


# pylint: disable=too-many-arguments, too-many-locals, too-many-statements
def run(
    architecture_file,
    circuit_file,
    include_files,
    output_netlist,
    command_runner=vtr.CommandRunner(),
    temp_dir=Path("."),
    mosaic_args=None,
    log_filename="mosaic.out",
    yosys_exec=None,
    mosaic_script=None,
):
    """
    runs mosaic on the specified architecture file and circuit.

    .. note :: Usage: vtr.mosaic.run(<architecture_file>,<circuit_file>,<output_netlist>,[OPTIONS])

    Arguments
    =========
        architecture_file :
            Architecture file to target

        circuit_file :
            Circuit file to optimize

        include_files :
            List of include files to a benchmark circuit. Passed in by run_vtr_flow with -include

        output_netlist :
            File name to output the resulting circuit to

    Other Parameters
    ----------------
        command_runner :
            A CommandRunner object used to run system commands

        temp_dir :
            Directory to run in (created if non-existent)

        mosaic_args :
            A dictionary of keyword arguments to pass on to Mosaic
            (top_module selects the hierarchy top, anything else is
            forwarded to the yosys command line)

        log_filename :
            File to log result to

        yosys_exec :
            Yosys executable to be run

        mosaic_script :
            Custom mosaic yosys template script (defaults to
            vtr_flow/misc/mosaic/template/synthesis.tcl)

    """
    temp_dir = Path(temp_dir) if not isinstance(temp_dir, Path) else temp_dir
    temp_dir.mkdir(parents=True, exist_ok=True)

    if mosaic_args is None:
        mosaic_args = OrderedDict()

    # verify that files are Paths or convert them to Paths and check that they exist
    architecture_file = vtr.verify_file(architecture_file, "Architecture")
    circuit_file = vtr.verify_file(circuit_file, "Circuit")
    output_netlist = vtr.verify_file(output_netlist, "Output netlist", False)

    if circuit_file.suffix not in FILE_TYPES:
        raise vtr.VtrError(
            "Mosaic expects an HDL input file {} (got '{}')".format(
                "[" + ", ".join(FILE_TYPES) + "]", circuit_file.name
            )
        )

    if yosys_exec is None:
        yosys_exec = str(vtr.paths.yosys_exe_path)

    plugin_path = Path(vtr.paths.mosaic_plugin_path)
    if not plugin_path.is_file():
        raise vtr.VtrError(
            "mosaic plugin not found at {} (installed as mosaic.so)\n".format(plugin_path)
            + "build vtr with WITH_MOSAIC=ON"
        )

    fix_blif_script = Path(vtr.paths.mosaic_fix_blif_script_path)
    if not fix_blif_script.is_file():
        raise vtr.VtrError("fix_blif_for_vpr.py not found at {}".format(fix_blif_script))

    if mosaic_script is None:
        base_script = str(vtr.paths.mosaic_script_path)
    else:
        base_script = str(Path(mosaic_script).resolve())

    # copy the template script into the run directory
    yosys_script = "synthesis_mosaic.tcl"
    yosys_script_full_path = str(temp_dir / yosys_script)
    shutil.copyfile(base_script, yosys_script_full_path)

    circuit_list = create_circuits_list(circuit_file, include_files)

    top_module = mosaic_args.pop("top_module", "") or ""

    # yosys writes the raw blif, and it is pruned and fixed into output_netlist below.
    raw_netlist_name = output_netlist.stem + ".raw" + output_netlist.suffix
    architecture_file_path = str(architecture_file.resolve()).replace("\\", "/")

    init_script_file(
        yosys_script_full_path,
        circuit_list,
        top_module,
        raw_netlist_name,
        architecture_file_path,
    )

    cmd = [yosys_exec, "-m", str(plugin_path.resolve())]

    for arg, value in mosaic_args.items():
        if isinstance(value, bool) and value:
            cmd += ["--" + arg]
        elif isinstance(value, (str, int, float)):
            cmd += ["--" + arg, str(value)]

    cmd += ["-c", yosys_script]

    command_runner.run_system_command(
        cmd, temp_dir=temp_dir, log_filename=log_filename, indent_depth=1
    )

    raw_netlist = temp_dir / raw_netlist_name
    if not raw_netlist.is_file():
        raise vtr.VtrError(
            "Mosaic did not produce {} (see {})".format(raw_netlist_name, log_filename)
        )

    # prune blackbox models the arch never declares, then apply vpr blif hygiene
    # fixes (ram addr pads, hierarchical net dots, latch-Q uniquify).
    shutil.copyfile(str(raw_netlist), str(output_netlist))
    prune_blif_models_not_in_arch(output_netlist, architecture_file)

    # arch_facts.tcl is written into the run dir by vtr_arch_rules, and we pass
    # it so ram addr-pad rewrites recognize aliased sp/dp model names.
    fix_blif_cmd = [sys.executable, str(fix_blif_script), output_netlist.name]
    arch_facts_path = temp_dir / "arch_facts.tcl"
    if arch_facts_path.is_file():
        fix_blif_cmd += ["--arch-facts", arch_facts_path.name]

    command_runner.run_system_command(
        fix_blif_cmd,
        temp_dir=temp_dir,
        log_filename="mosaic_fix_blif.out",
        indent_depth=1,
    )


# pylint: enable=too-many-arguments, too-many-locals, too-many-statements
