#!/usr/bin/env python3
"""
Module to parse the vtr flow results.
"""
import sys
from pathlib import Path
import glob
from collections import OrderedDict

# pylint: disable=wrong-import-position
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import vtr
from vtr import paths

# pylint: enable=wrong-import-position


def parse_vtr_flow(arg_list):
    """
        parse vtr flow output
    """
    parse_path = arg_list[0]
    parse_config_file = arg_list[1]
    parse_config_file = vtr.util.verify_file(parse_config_file, "parse config")

    extra_params = arg_list[2:]
    if parse_config_file is None:
        parse_config_file = str(paths.vtr_benchmarks_parse_path)

    parse_patterns = vtr.load_parse_patterns(str(parse_config_file))

    metrics = OrderedDict()

    extra_params_parsed = OrderedDict()

    for param in extra_params:
        key, value = param.split("=", 1)
        extra_params_parsed[key] = value
        print(key, end="\t")

    # Set defaults
    for parse_pattern in parse_patterns.values():
        metrics[parse_pattern.name()] = (
            parse_pattern.default_value() if parse_pattern.default_value() is not None else ""
        )
        print(parse_pattern.name(), end="\t")
    print("")

    for key, value in extra_params_parsed.items():
        print(value, end="\t")

    # Process each pattern
    for parse_pattern in parse_patterns.values():

        # We interpret the parse pattern's filename as a glob pattern
        filepaths = glob.glob(str(Path(parse_path) / parse_pattern.filename()))

        if len(filepaths) > 1:
            raise vtr.InspectError(
                "File pattern '{}' is ambiguous ({} files matched)".format(
                    parse_pattern.filename(), len(filepaths)
                ),
                len(filepaths),
                filepaths,
            )

        if len(filepaths) == 1:

            assert Path(filepaths[0]).exists
            metrics[parse_pattern.name()] = "-1"
            with open(filepaths[0], "r") as file:
                for line in file:
                    while line[0] == "#":
                        line = line[1:]
                    match = parse_pattern.regex().match(line)
                    if match and match.groups():
                        # Extract the first group value
                        metrics[parse_pattern.name()] = match.groups()[0]
            print(metrics[parse_pattern.name()], end="\t")
        else:
            # No matching file, skip
            print("-1", end="\t")
            assert len(filepaths) == 0
    print("")

    return 0


if __name__ == "__main__":
    parse_vtr_flow(sys.argv[1:])
