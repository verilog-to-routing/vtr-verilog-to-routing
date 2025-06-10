#!/usr/bin/env python3
"""
    Module to verify VTR test suites run by the GitHub CI.

    Test suites in VTR are verified by ensuring that all tasks in the test suite
    appear in the task list and that there are no tasks in the task list which
    are not in the test suite.

    A JSON file is used to tell this module which test suites to verify.

    This module is designed to be used within the CI of VTR to ensure that tasks
    within test suites are running all the tasks they are intended to.
"""
import os
import argparse
import json
import sys
from dataclasses import dataclass, field
from typing import List, Set
from pathlib import Path


@dataclass(order=True, frozen=True)
class TestSuite:
    """
    Data class used to store information about a test suite.
    """

    name: str
    ignored_tasks: List[str] = field(default_factory=list)


def parse_test_suite_info(test_suite_info_file: str) -> List[TestSuite]:
    """
    Parses the given test_suite_info file. The test suite info file is expected
    to be a JSON file which contains information on which test suites in the
    regression tests to verify and if any of the tasks should be ignored.

    The JSON should have the following form:
    {"test_suites": [
        {
            "name": "<test_suite_name>",
            "ignored_tasks": [
                "<ignored_task_name>",
                ...
            ]
        },
        {
            ...
        }
    ]}
    """
    with open(test_suite_info_file, "r") as file:
        data = json.load(file)

    assert isinstance(data, dict), "Test suite info should be a dictionary"
    assert "test_suites" in data, "A list of test suites must be provided"

    test_suites = []
    for test_suite in data["test_suites"]:
        assert isinstance(test_suite, dict), "Test suite should be a dictionary"
        assert "name" in test_suite, "All test suites must have names"
        assert "ignored_tasks" in test_suite, "All test suite must have an ignored task list"

        test_suites.append(
            TestSuite(
                name=test_suite["name"],
                ignored_tasks=test_suite["ignored_tasks"],
            )
        )

    return test_suites


def parse_task_list(task_list_file: str) -> Set[str]:
    """
    Parses the given task_list file and returns a list of the tasks within
    the task list.
    """
    tasks = set()
    with open(task_list_file, "r") as file:
        for line in file:
            # Strip the whitespace from the line.
            line.strip()
            # If this is a comment line, skip it.
            if line[0] == "#":
                continue
            # Split the line. This is used in case there is a comment on the line.
            split_line = line.split()
            if split_line:
                # If the line can be split (i.e. the line is not empty), add
                # the first part of the line to the tasks list, stripping any
                # trailing "/" characters.
                tasks.add(split_line[0].rstrip("/"))

    return tasks


def get_expected_task_list(test_suite_dir: str, reg_tests_parent_dir: str) -> Set[str]:
    """
    Get the expected task list by parsing the test suite directory and finding
    all files that look like config files.
    """
    # Get all config files in the test suite. These will indicated where all
    # the tasks are in the suite.
    base_path = Path(test_suite_dir)
    assert base_path.is_dir()
    config_files = list(base_path.rglob("config.txt"))

    # Get a list of all the expected tasks in the task list
    expected_task_list = set()
    for config_file in config_files:
        config_dir = os.path.dirname(config_file)
        task_dir = os.path.dirname(config_dir)
        # All tasks in the task list are relative to the parent of the regression
        # tests directory.
        expected_task_list.add(os.path.relpath(task_dir, reg_tests_parent_dir))

    return expected_task_list


def verify_test_suite(test_suite: TestSuite, regression_tests_dir: str):
    """
    Verifies the given test suite by looking into the regression tests directory
    for the suite and ensures that all expected tasks are present in the suite's
    task list.

    Returns the number of failures found in the test suite.
    """
    # Check that the test suite exists in the regression tests directory
    test_suite_dir = os.path.join(regression_tests_dir, test_suite.name)
    if not os.path.exists(test_suite_dir):
        print("\tError: Test suite not found in regression tests directory")
        return 1

    # Get the expected tasks list from the test suite directory.
    reg_tests_parent_dir = os.path.dirname(regression_tests_dir.rstrip("/"))
    expected_task_list = get_expected_task_list(test_suite_dir, reg_tests_parent_dir)

    # Get the task list file from the test suite and parse it to get the actual
    # task list.
    task_list_file = os.path.join(test_suite_dir, "task_list.txt")
    if not os.path.exists(task_list_file):
        print("\tError: Test suite does not have a root-level task list")
        return 1
    actual_task_list = parse_task_list(task_list_file)

    # Keep track of the number of failures
    num_failures = 0

    # Process the ignored tests
    ignored_tasks = set()
    for ignored_task in test_suite.ignored_tasks:
        # Ignored tasks are relative to the test directory, get their full path.
        ignored_task_path = os.path.join(test_suite_dir, ignored_task)
        # Check that the task exists.
        if not os.path.exists(ignored_task_path):
            print(f"\tError: Ignored task '{ignored_task}' not found in test suite")
            num_failures += 1
            continue
        # If the task exists, add it to the ignored tasks list relative to the
        # reg test's parent directory so it can be compared properly.
        ignored_tasks.add(os.path.relpath(ignored_task_path, reg_tests_parent_dir))

    if len(ignored_tasks) > 0:
        print(f"\tWarning: {len(ignored_tasks)} tasks were ignored")

    # Check for any missing tasks in the task list
    for task in expected_task_list:
        # If this task is ignored, it is expected to be missing.
        if task in ignored_tasks:
            continue
        # If the task is not in the actual task list, this is an error.
        if task not in actual_task_list:
            print(f"\tError: Failed to find task '{task}' in task list!")
            num_failures += 1

    # Check for any tasks in the task list which should not be there
    for task in actual_task_list:
        # If a task is in the task list, but is not in the test directory, this
        # is a failure.
        if task not in expected_task_list:
            print(f"\tError: Task '{task}' found in task list but not in test directory")
            num_failures += 1
        # If a task is in the task list, but is marked as ignored, this must be
        # a mistake.
        if task in ignored_tasks:
            print(f"\tError: Task '{task}' found in task list but was marked as ignored")

    return num_failures


def verify_test_suites():
    """
    Verify the VTR test suites.

    Test suites are verified by checking the tasks within their test directory
    and the tasks within the task list at the root of that directory and ensuring
    that they match. If there are any tasks which appear in one but not the other,
    an error is produced and this script will return an error code.
    """
    # Set up the argument parser object.
    parser = argparse.ArgumentParser(description="Verifies the test suites used in VTR.")
    parser.add_argument(
        "-vtr_regression_tests_dir",
        type=str,
        required=True,
        help="The path to the vtr_flow/tasks/regression_tests directory in VTR.",
    )
    parser.add_argument(
        "-test_suite_info",
        type=str,
        required=True,
        help="Information on the test suite (must be a JSON file).",
    )

    # Parse the arguments from the command line.
    args = parser.parse_args()

    # Verify each of the test suites.
    num_failures = 0
    test_suites = parse_test_suite_info(args.test_suite_info)
    for test_suite in test_suites:
        print(f"Verifying test suite: {test_suite.name}")
        test_suite_failures = verify_test_suite(test_suite, args.vtr_regression_tests_dir)
        print(f"\tTest suite had {test_suite_failures} failures\n")
        num_failures += test_suite_failures

    # If any failures were found in any suite, return exit code 1.
    if num_failures != 0:
        print(f"Failure: Test suite verifcation failed with {num_failures} failures")
        print(f"Please fix the failing test suites found in {args.vtr_regression_tests_dir}")
        print(f"If necessary, update the test suites info found here: {args.test_suite_info}")
        sys.exit(1)

    print(f"Success: All test suites in {args.test_suite_info} passed")


if __name__ == "__main__":
    verify_test_suites()
