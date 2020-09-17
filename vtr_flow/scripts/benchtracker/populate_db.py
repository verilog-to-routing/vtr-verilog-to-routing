#!/usr/bin/python

from __future__ import print_function, division
import sys
from subprocess import call
import os
import shlex
import argparse
import textwrap
import sqlite3
import socket  # for hostname
import getpass  # for username
from util import (
    walk_runs,
    get_trailing_num,
    get_result_file,
    get_task_table_name,
    immediate_subdir,
    natural_sort,
    is_int,
    is_float,
    convert_strictest,
)

nullval = "-1"
type_map = {int: "INT", float: "REAL", str: "TEXT"}

# main program sequence
def main():
    params = Params()
    parse_args(params)

    # operate on database
    db = sqlite3.connect(params.database)
    db.row_factory = sqlite3.Row

    while params.task_dir:
        if params.clean:
            drop_table(params, db)
        else:
            verify_paths(params)
            initialize_tracked_columns(params, db)
            update_db(params, db)

        load_next_task(params)

    db.close()


def verify_paths(params):
    """verify paths exist as either relational or absolute paths, modifying them if necessary"""
    if os.path.isdir(params.task_dir):
        walk_runs(params, check_result_exists)
    else:
        print(params.task_dir, " directory does not exist")


def update_db(params, db):
    # check if table for task exists; if not then create it
    task_table_name = params.task_table_name
    create_table(params, db, task_table_name)
    # load up latest run and parsed date for task
    def check_last_runs_table(runs):
        natural_sort(runs)
        cursor = db.cursor()
        cursor.execute(
            "SELECT MAX({}), MAX({}) FROM {}".format("parsed_date", "run", params.task_table_name)
        )
        row = cursor.fetchone()
        if row[0]:
            # storing in database truncates decimal, so add 1 second
            last_parsed_date = row[0] + 1
            last_run = row[1]
            print("last parsed date", last_parsed_date)
            print("last run", last_run)
            params.last_parsed_date = last_parsed_date
            params.last_run = last_run
        else:
            print("first population")
            params.last_run = 0
        # else first run, nothing in table yet

    def add_run_to_db(params, run):
        run_number = get_trailing_num(run)
        resfilename = get_result_file(params, params.run_prefix, run_number)
        try:
            parsed_date = os.path.getmtime(resfilename)
            # throw away unless newer than latest or run number greater than maximum
            if parsed_date <= params.last_parsed_date and run_number <= params.last_run:
                return
        except OSError:
            print("file {} not found; skipping".format(resfilename))
            return

        params.last_run += 1
        print("run {} added ({}) ({})".format(run_number, params.last_run, parsed_date))

        with open(resfilename, "r") as res:
            # make sure table is compatible with run data by inserting any new columns
            # always called "run" in table (converted from whatever prefix users use)
            result_params = ["run", "parsed_date"]
            result_params.extend(res.readline().split("\t"))
            if result_params[-1] == "\n":
                result_params.pop()
            result_params = ["".join(('"', p.strip(), '"')) for p in result_params]

            pre_sample_pos = res.tell()
            result_params_sample = res.readline().split("\t")
            # go back to presample location for normal line iteration
            res.seek(pre_sample_pos, os.SEEK_SET)

            # add new column to table
            for c in range(len(result_params)):
                if result_params[c] not in params.tracked_columns:
                    print("ADDING {} as new column".format(result_params[c]))
                    add_column_to_table(
                        params, db, result_params[c], result_params_sample[c - 2]
                    )  # -2 accounts for run and parsed date

            # add value rows
            rows_to_add = []
            for line in res:
                # run number and parsed_date are always recorded
                result_params_val = [params.last_run, parsed_date]
                result_params_val.extend(line.split("\t"))
                if result_params_val[-1] == "\n":
                    result_params_val.pop()
                # something must be wrong here
                if len(result_params_val) > len(result_params):
                    print(
                        "There are {} values for only {} parameters in run {}; \
                        skipping run".format(
                            len(result_params_val), len(result_params), run_number
                        )
                    )
                    # skip this run
                    params.last_run -= 1
                    return

                # for when the last column value is the empty string
                while len(result_params_val) < len(result_params):
                    result_params_val.append("")

                rows_to_add.append(
                    tuple(
                        convert_strictest(param) if param != nullval else None
                        for param in result_params_val
                    )
                )

            param_placeholders = ("?," * len(result_params)).rstrip(",")
            # specify columns to insert into since ordering of columns in file may not match table
            insert_rows_command = "INSERT OR IGNORE INTO {} ({}) VALUES ({})".format(
                params.task_table_name, ",".join(result_params), param_placeholders
            )
            cursor = db.cursor()
            cursor.executemany(insert_rows_command, rows_to_add)

    walk_runs(params, add_run_to_db, check_last_runs_table)
    db.commit()


def drop_table(params, db):
    cursor = db.cursor()
    cursor.execute("DROP TABLE IF EXISTS {}".format(params.task_table_name))


def create_table(params, db, task_table_name):
    # creates table schema based on the result file of run 1

    with open(get_result_file(params, params.run_prefix, 1), "r") as res:
        result_params = [param.strip() for param in res.readline().rstrip().split("\t")]
        result_params_sample = [value.strip() for value in res.readline().rstrip().split("\t")]
        while len(result_params_sample) < len(result_params):
            result_params_sample.append("")

        # check if table name already exists
        cursor = db.cursor()
        cursor.execute(
            "SELECT name FROM sqlite_master WHERE type='table' AND name='{}'".format(
                params.task_table_name.strip("[]")
            )
        )
        if cursor.fetchone():
            return

        # table identifiers cannot be parameterized, so have to use string concatenation
        create_table_command = """CREATE TABLE IF NOT EXISTS {}(""".format(task_table_name)

        # include one item for each result_params
        create_table_command += "{} INTEGER, ".format(params.run_prefix)
        # time is in seconds since epoch
        create_table_command += "parsed_date REAL, "
        for p in range(len(result_params)):
            p_schema = "".join(
                (
                    '"',
                    result_params[p].strip(),
                    '" ',
                    type_map.get(type(convert_strictest(result_params_sample[p])), "TEXT"),
                )
            )
            p_schema += ", "
            create_table_command += p_schema

        # treat with unique keys
        primary_keys = "PRIMARY KEY({},".format(
            params.run_prefix
        )  # run always considered primary key
        for primary_key in params.key_params:
            if primary_key not in result_params:
                print("{} does not exist in result file of run 1".format(primary_key))
                sys.exit(3)
            primary_keys += primary_key
            primary_keys += ","
        primary_keys = primary_keys[:-1]  # remove trailing ,
        primary_keys += ")"
        create_table_command += primary_keys

        create_table_command += ")"

        print(create_table_command)
        cursor = db.cursor()
        cursor.execute(create_table_command)

        # reinitialize tracked columns for newly created table
        initialize_tracked_columns(params, db)


def initialize_tracked_columns(params, db):
    cursor = db.cursor()
    cursor.execute("PRAGMA table_info(%s)" % params.task_table_name)
    column_info = cursor.fetchall()
    column_names = set()
    for info in column_info:
        column_names.add('"' + info[1] + '"')
    setattr(params, "tracked_columns", column_names)
    # print('tracked params: ', end='')
    # print(params.tracked_columns)


# generating program parameters
class Params(object):
    def __iter__(self):
        for attr, value in self.__dict__.iteritems():
            yield attr, value


def parse_args(ns=None):
    """parse arguments from command line and return as namespace object"""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(
            """\
            populates a central database with benchmark information
            
        Generated database:
            Each task will become its own table, identified by its absolute path.
            Each table will have as many columns as the result file the first time
            the table is generated. The columns of the result file should not change
            after the table is generated."""
        ),
        usage="%(prog)s <task_dir> [OPTIONS]",
    )

    # arguments should either end with _dir or _file for use by other functions
    parser.add_argument(
        "task_list", nargs="+", default=[], help="directories (relative or absolute) holding runs"
    )
    parser.add_argument(
        "--root_directory",
        default="",
        help="where the task paths are relative to (only set if task paths are relative)",
    )
    parser.add_argument(
        "-f",
        "--result_file",
        default="parse_results.txt",
        help="result file to look for inside each run; default: %(default)s",
    )
    parser.add_argument(
        "-p",
        "--run_prefix",
        default="run",
        help="prefix of run directories under the task directory; default: %(default)s",
    )
    parser.add_argument(
        "-s",
        "--parse_script",
        default="echo parsed result file not found {run_number} for {task_name}",
        help="bash command string to run if parsed file not found;\
                use {run_number} and {task_name} to retrieve run information;\
                default: %(default)s",
    )
    parser.add_argument(
        "-d",
        "--database",
        default="results.db",
        help="name of database to store results in; default: %(default)s",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        default=False,
        help="clean (remove) the table for this task; exits afterwards",
    )
    parser.add_argument(
        "-k",
        "--key_params",
        required=True,
        nargs="+",
        default=[],
        help="the combination of key parameters that defines a unique benchmark;",
    )
    params = parser.parse_args(namespace=ns)
    params.database = os.path.expanduser(params.database)
    # initialize to -1 as everything valid must be greater than -1
    setattr(params, "last_parsed_date", -1)
    setattr(params, "last_run", -1)
    # if a task list is given (where each line is the path to a task)
    if params.root_directory:
        if params.root_directory[-1] != "/":
            params.root_directory += "/"
        # make directory if it does not exist
        if not os.path.exists(params.root_directory):
            os.makedirs(directory)
        params.task_list = [params.root_directory + task for task in params.task_list]

    if os.path.isfile(params.task_list[0]):
        print("given task list")
        task_list = []
        with open(params.task_list[0], "r") as tl:
            for line in tl:
                task_list.append(params.root_directory + line.rstrip())
            params.task_list = task_list

    # load first task
    load_next_task(params)
    return params


def load_next_task(params):
    if not params.task_list:
        setattr(params, "task_dir", None)
        return

    active_task = params.task_list.pop()
    print("active task:", active_task)

    setattr(params, "task_dir", active_task)
    # name of task
    setattr(params, "task_name", params.task_dir.split("/")[-1])
    # path to task
    setattr(params, "task_path", os.path.abspath(os.path.join(params.task_dir, os.pardir)))
    # table name in the database
    setattr(params, "task_table_name", get_task_table_name(params))
    return params


# walk operations; all take params and run as arguments
def check_result_exists(params, run):

    run_number = get_trailing_num(run)
    if not os.path.isfile(get_result_file(params, params.run_prefix, run_number)):
        parsed_call = params.parse_script.format(
            task_dir=params.task_dir, task_name=params.task_name, run_number=get_trailing_num(run)
        )
        print(run, " called ", parsed_call)
        parsed_call = shlex.split(parsed_call)
        parsed_call[0] = os.path.expanduser(parsed_call[0])
        call(parsed_call)
    else:
        print(run, " OK")


def add_column_to_table(params, db, column_name, sample_val):
    cursor = db.cursor()
    col_type = type_map.get(type(convert_strictest(sample_val)), "TEXT")
    cursor.execute(
        "ALTER TABLE {table} ADD COLUMN {col} {type}".format(
            table=params.task_table_name, col=column_name, type=col_type
        )
    )
    params.tracked_columns.add(column_name)


if __name__ == "__main__":
    main()
