#!/usr/bin/env python3

import os
import argparse
import json
import sys
from dataclasses import dataclass, field
from typing import List
from pathlib import Path

@dataclass(order=True, frozen=True)
class TestSuite:
    name: str
    ignored_tasks: List[str] = field(default_factory=list)


def parse_test_suite_info(test_suite_info_file: str) -> List[TestSuite]:
    with open(test_suite_info_file, 'r') as file:
        data = json.load(file)

    assert isinstance(data, dict), "Test suite info should be a dictionary"
    assert "test_suites" in data, "A list of test suites must be provided"

    test_suites = []
    for test_suite in data["test_suites"]:
        assert isinstance(test_suite, dict), "Test suite should be a dictionary"
        assert "name" in test_suite, "All test suites must have names"
        assert "ignored_tasks" in test_suite, "All test suite must have an ignored task list"

        test_suites.append(TestSuite(name=test_suite["name"],
                                     ignored_tasks=test_suite["ignored_tasks"],
                                     ))

    return test_suites


def parse_task_list(task_list_file: str) -> List[str]:
    tasks = []
    with open(task_list_file, 'r') as file:
        for line in file:
            line.strip()
            if line[0] == '#':
                continue
            split_line = line.split()
            if split_line:
                tasks.append(split_line[0].rstrip("/"))

    return tasks


def verify_test_suite(test_suite: TestSuite, regression_tests_dir: str):
    # Check that the test suite exists in the regression tests directory
    test_suite_dir = os.path.join(regression_tests_dir, test_suite.name)
    if not os.path.exists(test_suite_dir):
        print("Test suite not found in regression tests directory")
        sys.exit(1)

    # Get the task list file from the test suite
    task_list_file = os.path.join(test_suite_dir, "task_list.txt")
    if not os.path.exists(task_list_file):
        print("Test suite does not have a root-level task list")
        sys.exit(1)

    # Get all config files in the test suite. These will indicated where all
    # the tasks are in the suite.
    base_path = Path(test_suite_dir)
    assert base_path.is_dir()
    config_files = list(base_path.rglob("config.txt"))

    # Get a list of all the expected tasks in the task list
    expected_task_list = []
    for config_file in config_files:
        config_dir = os.path.dirname(config_file)
        task_dir = os.path.dirname(config_dir)
        # All tasks in the task list are relative to the parent of the regression
        # tests directory.
        expected_task_list.append(os.path.relpath(task_dir, os.path.dirname(os.path.dirname(regression_tests_dir))))

    # Parse the task list to get the actual task list
    actual_task_list = parse_task_list(task_list_file)

    # Process the ignored tests
    ignored_tasks = set()
    for ignored_task in test_suite.ignored_tasks:
        ignored_task_path = os.path.join(test_suite_dir, ignored_task)
        if not os.path.exists(ignored_task_path):
            print(f"Ignored task '{ignored_task}' not found in test suite directory")
            sys.exit(1)
        ignored_tasks.add(os.path.relpath(ignored_task_path, os.path.dirname(os.path.dirname(regression_tests_dir))))

    num_failures = 0
    # Check for any missing tasks in the task list
    expected_task_list = set(expected_task_list)
    actual_task_list = set(actual_task_list)
    for task in expected_task_list:
        if task in ignored_tasks:
            continue
        if task not in actual_task_list:
            print(f"\tError: Failed to find task '{task}' in task list!")
            num_failures += 1

    # Check for any tasks in the task list which should not be there
    for task in actual_task_list:
        if task not in expected_task_list:
            print(f"\tError: Task '{task}' found in task list but not in test directory")
            num_failures += 1
        if task in ignored_tasks:
            print(f"\tError: Task '{task}' found in task list but was marked as ignored")

    return num_failures


def verify_test_suites():
    parser = argparse.ArgumentParser(description="TODO")
    parser.add_argument(
        "-vtr_regression_tests_dir",
        type=str,
        help="The path to the vtr_flow/tasks/regression_tests directory in VTR."
    )
    parser.add_argument(
        "-test_suite_info",
        type=str,
        help="Information on the test suite (must be a JSON file)."
    )

    args = parser.parse_args()

    test_suites = parse_test_suite_info(args.test_suite_info)

    num_failures = 0
    for test_suite in test_suites:
        print(f"Verifying test suite: {test_suite.name}")
        test_suite_failures = verify_test_suite(test_suite, args.vtr_regression_tests_dir)
        print(f"\tTest suite had {test_suite_failures} failures\n")
        num_failures += test_suite_failures

    if num_failures != 0:
        sys.exit(1)


if __name__ == "__main__":
    verify_test_suites()
