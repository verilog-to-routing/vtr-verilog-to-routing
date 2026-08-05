"""module to run mosaic as a synthesis frontend for the vtr flow.

mosaic is a yosys plugin that combines wildebeest-originated passes
(max_level / clk_domains) with mosaic-only arch rules (-vtr_arch).
"""

import re
import shutil
import sys
import warnings
from collections import OrderedDict
from pathlib import Path
import vtr
from ..parmys.parmys import create_circuits_list

# supported input file types by the mosaic yosys template
FILE_TYPES = {
    ".v": "Verilog",
    ".vh": "Verilog",
    ".sv": "SystemVerilog",
    ".svh": "SystemVerilog",
}


# USE: resolve the mosaic per-arch policy support dir for an architecture xml.
# this looks for vtr_flow/misc/mosaic/<arch_stem>/arch_config.tcl only and
# returns None when missing so synthesis runs facts-only from the arch xml with
# no silent family fallback or stem alias table.
def resolve_arch_support_dir(architecture_file_path):
    archPath = Path(architecture_file_path)
    archStem = archPath.stem
    miscPath = Path(vtr.paths.mosaic_misc_path)
    supportDir = miscPath / archStem
    configFile = supportDir / "arch_config.tcl"

    if not configFile.is_file():
        warnings.warn(
            "mosaic: no policy support dir for arch '{}'; "
            "running facts-only (arch_facts.tcl from xml, no arch_config.tcl). "
            "add {} to supply costs/scripts/thresholds."
            .format(archStem, configFile),
            stacklevel=2,
        )
        return None

    return supportDir


# pylint: disable=too-many-arguments, too-many-locals
# USE: fill the template tokens in the copied mosaic yosys script.
def init_script_file(
    yosys_script_full_path,
    circuit_list,
    top_module,
    raw_netlist_name,
    architecture_file_path,
):
    # yosys tcl wants forward slashes even on windows
    supportDir = resolve_arch_support_dir(architecture_file_path)
    if supportDir is None:
        archSupportDir = ""
    else:
        archSupportDir = str(supportDir.resolve()).replace("\\", "/")
    templateDir = str(vtr.paths.mosaic_template_path.resolve()).replace("\\", "/")

    # YYY makes this the mosaic leg because max_level -clk2clk takes clock cut
    # points from the arch xml instead of vendor cell lists.
    vtrArchFlag = "-vtr_arch {}".format(architecture_file_path)

    vtr.file_replace(
        yosys_script_full_path,
        {
            "XXX": " ".join(str(s) for s in circuit_list),
            "TDIR": templateDir,
            "TTT": top_module,
            "ZZZ": raw_netlist_name,
            "ARCH_SUPPORT_DIR": archSupportDir,
            "VVV": architecture_file_path,
            "YYY": vtrArchFlag,
        },
    )


# USE: collect model names declared in the arch xml.
def parse_arch_blif_model_names(arch_xml_path):
    text = Path(arch_xml_path).read_text(encoding="utf-8", errors="replace")
    return set(re.findall(r'<model\s+name="([^"]+)"', text))


# USE: drop unused blackbox lib .model blocks (e.g. dffes) the arch never uses.
def prune_blif_models_not_in_arch(blif_path, arch_xml_path):
    allowed = parse_arch_blif_model_names(arch_xml_path)
    text = blif_path.read_text(encoding="utf-8", errors="replace")
    usedSubckts = set(re.findall(r"^\.subckt\s+(\S+)", text, re.MULTILINE))

    blocks = [b for b in re.split(r"(?=^\.model )", text, flags=re.MULTILINE) if b.strip()]

    # yosys prepends blackbox lib models before the design model, so the top is
    # the first .model whose block is not a blackbox lib entry.
    topModel = None
    for block in blocks:
        modelMatch = re.match(r"^\.model\s+(\S+)", block, re.MULTILINE)
        if modelMatch and ".blackbox" not in block:
            topModel = modelMatch.group(1)
            break

    keptBlocks = []
    for block in blocks:
        modelMatch = re.match(r"^\.model\s+(\S+)", block, re.MULTILINE)
        if not modelMatch:
            keptBlocks.append(block)
            continue
        modelName = modelMatch.group(1)
        isBlackboxLib = ".blackbox" in block
        if (
            isBlackboxLib
            and modelName not in allowed
            and modelName not in usedSubckts
            and modelName != topModel
        ):
            continue
        keptBlocks.append(block)

    outText = "".join(keptBlocks)
    if outText and not outText.endswith("\n"):
        outText += "\n"
    blif_path.write_text(outText, encoding="utf-8")


# pylint: disable=too-many-arguments, too-many-locals, too-many-statements
# USE: run mosaic on the specified architecture file and circuit.
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

    plugin_path = Path(vtr.paths.wildebeest_plugin_path)
    if not plugin_path.is_file():
        raise vtr.VtrError(
            "mosaic plugin not found at {} (installed as wildebeest.so)\n".format(
                plugin_path
            )
            + "build it with: bash {}".format(vtr.paths.mosaic_build_script_path)
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

    circuit_list = create_circuits_list(circuit_file, include_files, FILE_TYPES)

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
    fixBlifCmd = [sys.executable, str(fix_blif_script), output_netlist.name]
    archFactsPath = temp_dir / "arch_facts.tcl"
    if archFactsPath.is_file():
        fixBlifCmd += ["--arch-facts", archFactsPath.name]

    command_runner.run_system_command(
        fixBlifCmd,
        temp_dir=temp_dir,
        log_filename="mosaic_fix_blif.out",
        indent_depth=1,
    )


# pylint: enable=too-many-arguments, too-many-locals, too-many-statements
