#!/usr/bin/env python3
"""
Module to parse the vtr flow results.
"""
import sys
from pathlib import Path
import glob
from collections import OrderedDict, defaultdict

# pylint: disable=wrong-import-position
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import vtr
from vtr import paths

# pylint: enable=wrong-import-position


def parse_file_and_update_results(filename, patterns, results):

    """
    Find filename, and then look through for the matching patterns, updating results
    """

    # We interpret the parse pattern's filename as a glob pattern
    filepaths = glob.glob(filename)

    if len(filepaths) > 1:
        raise vtr.InspectError(
            "File pattern '{}' is ambiguous ({} files matched)".format(filename, len(filepaths)),
            len(filepaths),
            filepaths,
        )

    if len(filepaths) == 1:
        assert Path(filepaths[0]).exists

        with open(filepaths[0], "r") as file:
            for line in file:
                while line[0] == "#":
                    line = line[1:]

                for parse_pattern in patterns:
                    match = parse_pattern.regex().match(line)
                    if match and match.groups():
                        # Extract the first group value
                        results[parse_pattern] = match.groups()[0]


def parse_vtr_flow(arg_list):
    """
    parse vtr flow output
    """
    parse_path = arg_list[0]
    parse_config_file = arg_list[1]
    extra_params = arg_list[2:]

    parse_config_file = vtr.util.verify_file(parse_config_file, "parse config")
    if parse_config_file is None:
        parse_config_file = str(paths.vtr_benchmarks_parse_path)

    parse_patterns = vtr.load_parse_patterns(str(parse_config_file))

    results = OrderedDict()

    extra_params_parsed = OrderedDict()

    for param in extra_params:
        key, value = param.split("=", 1)
        extra_params_parsed[key] = value
        print(key, end="\t")

    for parse_pattern in parse_patterns.values():
        # Set defaults
        results[parse_pattern] = (
            parse_pattern.default_value() if parse_pattern.default_value() is not None else "-1"
        )

        # Print header row
        print(parse_pattern.name(), end="\t")
    print("")

    for key, value in extra_params_parsed.items():
        print(value, end="\t")

    # Group parse patterns by filename so that we only need to read each log file from disk once
    parse_patterns_by_filename = defaultdict(list)
    for parse_pattern in parse_patterns.values():
        parse_patterns_by_filename[parse_pattern.filename()].append(parse_pattern)

    # Process each pattern
    for filename, patterns in parse_patterns_by_filename.items():
        parse_file_and_update_results(str(Path(parse_path) / filename), patterns, results)

    # Print results
    for parse_pattern in parse_patterns.values():
        print(results[parse_pattern], end="\t")
    print("")

    return 0


if __name__ == "__main__":
    parse_vtr_flow(sys.argv[1:])
