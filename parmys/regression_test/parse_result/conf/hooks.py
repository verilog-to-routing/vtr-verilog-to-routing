import os
import pwd
import re
import sys
from collections import OrderedDict
import json

PRE_HOOKS = ["normalize_yosys_stat_line"]

POST_HOOKS = ["drop_entries_on_failure", "patch_logs", "inverse_result_from_expectation"]


def do_something_on_the_raw_log(line: str):
    """
    this is an example preprocessing hook.
    """
    return line


# Yosys >= 0.58 prints the `stat` report as "<count> <name>", e.g.
#     698 wires
#      53   $_AND_
# where older versions printed
#     Number of wires:                698
#       $_AND_                         53
# The regexes in parmys.toml, cell_*.toml, std_mem.toml and arch_blocks.toml
# are written for the old layout, so rewrite new-style lines into it.
_STAT_TOTALS = (
    "wires",
    "wire bits",
    "public wires",
    "public wire bits",
    "ports",
    "port bits",
    "memories",
    "memory bits",
    "processes",
    "cells",
)
_NEW_STAT_TOTAL_RE = re.compile(
    r"^\s*(\d+)\s+(" + "|".join(re.escape(total) for total in _STAT_TOTALS) + r")\s*$"
)
_NEW_STAT_CELL_RE = re.compile(r"^\s*(\d+)\s+(\S+)\s*$")


def normalize_yosys_stat_line(line: str):
    """
    pre-processing hook: rewrite new-style Yosys stat lines into the old layout
    """
    match = _NEW_STAT_TOTAL_RE.match(line)
    if match is not None:
        return "   Number of {}: {}\n".format(match.group(2), match.group(1))

    match = _NEW_STAT_CELL_RE.match(line)
    if match is not None:
        return "     {} {}\n".format(match.group(2), match.group(1))

    return line


def do_something_on_the_parsed_log(values: OrderedDict):
    """
    this is an example post-processing hook.
    """
    #     if 'exit' in values:
    #         # do

    return values


def drop_entries_on_failure(values):
    """
    if we failed we drop the folowing sections
    Failure may happen late in the log, so we may still end up parsing them
    """
    if "exit" in values:
        if values["exit"] != 0:
            for sections in [
                # all the statistical values are not relevant if we failed
                "max_rss(MiB)",
                "exec_time(ms)",
                "synthesis_time(ms)",
                "techmap_time(ms)",
                "simulation_time(ms)",
                "Latch Drivers",
                "Pi",
                "Po",
                "logic element",
                "latch",
                "Adder",
                "Multiplier",
                "Memory",
                "Hard Ip",
                "generic logic size",
                "Longest Path",
                "Average Path",
                "Estimated LUTs",
                "Total Node",
            ]:
                if sections in values:
                    del values[sections]
    return values


def patch_logs(values):
    """
    patch the string logs
    """
    sub_re = {
        # strip username from the logs
        r"(" + pwd.getpwuid(os.getuid()).pw_name + r")": "",
        # strip path from known file extensions
        r"([\/]?[a-zA-Z_.\-0-9]*\/)(?=[^\/\s]*(_input|_output|\.xml|\.v|\.vh|\.blif|\.log|\.do|\.dot|_vectors|_activity)[\s\n]+)": "",
        # bison used to call EOF $end, but switched to end of file since
        r"syntax error, unexpected \$end": r"syntax error, unexpected end of file",
    }

    if isinstance(values, str):
        for old_str, new_string in sub_re.items():
            values = re.sub(old_str, new_string, values)
    elif isinstance(values, list):
        values = [patch_logs(log_entry) for log_entry in values]
    elif isinstance(values, (OrderedDict, dict)):
        for section in values:
            values[section] = patch_logs(values[section])

    return values


def inverse_result_from_expectation(values):

    should_fail = False
    if "expectation" in values:
        for log in values["expectation"]:
            if log == "failure":
                should_fail = True
                break

    if "exit" in values:
        if values["exit"] == 0 and should_fail:
            values["exit"] = 51
        elif values["exit"] != 0 and should_fail:
            values["exit"] = 0
            values["expectation"].append(
                "Failure caught and flipped to success by the post processor"
            )

    return values
