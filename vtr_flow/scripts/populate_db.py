#!/usr/bin/python

from __future__ import print_function, division
import sys
import re
from subprocess import call
import os.path
import argparse
import textwrap
import sqlite3
import hashlib
import socket   # for hostname
import getpass  # for username

# main program sequence
def main():
    params = Params()
    parse_args(params)
    if (params.clean):
        drop_table(params)
    verify_paths(params)

    # operate on database
    db = sqlite3.connect(params.database)
    db.row_factory = sqlite3.Row
    update_db(params, db)

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
    #print(task_table_name)
    create_table(params, db, task_table_name)
    # verify that the max runs in the task_dir is >= to the max runs in the database
    def check_runs_match_table(runs):
        natural_sort(runs)
        highest_run = get_trailing_num(runs[-1])
        cursor = db.cursor()
        cursor.execute("SELECT MAX({}) FROM {}".format(params.run_prefix, params.task_table_name))
        row = cursor.fetchone()
        if row[0]:
            if highest_run < row[0]:
                print("stored run ({}) is higher than existing run ({}); \
consider running with --clean to remake task table".format(row[0], highest_run))
        # else first run, nothing in table yet

    def add_run_to_db(params, run):
        with open(get_result_file(params, run), 'r') as res:
            next(res)  # chomp first header line
            rows_to_add = []
            for line in res:
                result_params_val = [get_trailing_num(run)]
                result_params_val.extend(line.split('\t'))
                if result_params_val[-1] == '\n':
                    result_params_val.pop()

                if len(result_params_val) != params.result_param_num:
                    print("mismatching parameter number; original: {}\t{}: {}".format(
                        params.result_param_num, run, len(result_params_val)))
                    db.rollback()
                    sys.exit(2)

                rows_to_add.append(tuple(convert_strictest(param) for param in result_params_val))

            param_placeholders = ("?,"*params.result_param_num).rstrip(',')
            insert_rows_command = "INSERT OR IGNORE INTO {} VALUES ({})".format(params.task_table_name, param_placeholders)
            cursor = db.cursor()
            cursor.executemany(insert_rows_command, rows_to_add)


    walk_runs(params, add_run_to_db, check_runs_match_table)
    db.commit()
        

def drop_table(params):
    db = sqlite3.connect(params.database)
    cursor = db.cursor()
    cursor.execute("DROP TABLE IF EXISTS {}".format(params.task_table_name))
    db.commit()
    db.close()
    sys.exit(0)

def create_table(params, db, task_table_name):
    # creates table schema based on the result file of run 1
    with open(get_result_file(params, params.run_prefix + "1"), 'r') as res:
        result_params = res.readline().rstrip().split('\t')
        result_params_sample = res.readline().rstrip().split('\t')
        while len(result_params_sample) < len(result_params):
            result_params_sample.append('')

        # store number of attributes for validation against other runs; INCLUDES RUN COL!
        setattr(params, 'result_param_num', len(result_params)+1)

        # table identifiers cannot be parameterized, so have to use string concatenation
        create_table_command = '''CREATE TABLE IF NOT EXISTS {}('''.format(task_table_name)

        # include one item for each result_params
        create_table_command += "{} INTEGER, ".format(params.run_prefix)
        for p in range(len(result_params)):
            p_schema = result_params[p].strip() + ' '
            if (is_int(result_params_sample[p])):
                p_schema += "INTEGER"
            elif (is_float(result_params_sample[p])):
                p_schema += "REAL"
            else:
                p_schema += "TEXT"

            if p < len(result_params) - 1:
                p_schema += ", "

            create_table_command += p_schema

        # treat with unique keys
        primary_keys = ", PRIMARY KEY({},".format(params.run_prefix) # run always considered primary key
        for primary_key in params.key_params:
            if primary_key not in result_params:
                print("{} does not exist in result file".format(primary_key))
                sys.exit(3)
            primary_keys += primary_key
            primary_keys += ','
        primary_keys = primary_keys[:-1]  # remove trailing ,
        primary_keys += ')'
        create_table_command += primary_keys

        create_table_command += ')'

        print(create_table_command)
        cursor = db.cursor()
        cursor.execute(create_table_command)


# generating program parameters
class Params(object):
    def __iter__(self):
        for attr, value in self.__dict__.iteritems():
            yield attr, value

def parse_args(ns=None):
    """parse arguments from command line and return as namespace object"""
    parser = argparse.ArgumentParser(
            formatter_class=argparse.RawDescriptionHelpFormatter,
            description=textwrap.dedent("""\
            populates a central database with benchmark information
            
        Generated database:
            Each task will become its own table, identified by its absolute path.
            Each table will have as many columns as the result file the first time
            the table is generated. The columns of the result file should not change
            after the table is generated."""),
            usage="%(prog)s <task_dir> [OPTIONS]")

    # arguments should either end with _dir or _file for use by other functions
    parser.add_argument("task_dir", 
            help="directory (relative or absolute) holding runs")
    parser.add_argument("-f", "--result_file", 
            default="parse_results.txt",
            help="result file to look for inside each run; default: %(default)s")
    parser.add_argument("-p", "--run_prefix", 
            default="run",
            help="prefix of run directories under the task directory; default: %(default)s")
    parser.add_argument("-s", "--parse_script",
            default="echo parsed result file not found {run_number} for {task_name}",
            help="bash command string to run if parsed file not found;\
                use \{run_number\} and \{task_name\} to retrieve run information;\
                default: %(default)s")
    parser.add_argument("-d", "--database",
            default="results.db",
            help="name of database to store results in; default: %(default)s")
    parser.add_argument("--clean",
            action='store_true',
            default=False,
            help="clean (remove) the table for this task; exits afterwards")
    parser.add_argument("-k","--key_params",
            nargs='+',
            default=[],
            help="a list of key parameters that define a unique benchmark; default: %(default)s")
    params = parser.parse_args(namespace=ns)
    # name of task
    setattr(params, 'task_name', params.task_dir.split('/')[-1])
    # an 8 digit hash for task directory
    setattr(params, 'task_path', os.path.abspath(os.path.join(params.task_dir, os.pardir)))
    # table name in the database
    setattr(params, 'task_table_name', get_task_table_name(params))
    return params


# working on the task directory
def sort_runs(runs):
    natural_sort(runs)

def walk_runs(params, operation, select=sort_runs):
    """walk the selected run# directories and apply operation on each"""
    runs = [run for run in immediate_subdir(params.task_dir) if run.startswith(params.run_prefix)]
    # select how to and which runs to use for a certain range
    select(runs)
    if not runs:
        print("no {}s in {}".format(params.run_prefix, params.task_dir))
        sys.exit(1)
    for run in runs:
        operation(params, run)


# walk operations; all take params and run as arguments
def check_result_exists(params, run):
    if not os.path.isfile(get_result_file(params, run)):
        parsed_call = params.parse_script.format(
            task_dir = params.task_dir,
            task_name = params.task_name,
            run_number = get_trailing_num(run))
        print(run, " called ", parsed_call)
        call(parsed_call.split())
    else:
        print(run, " OK")


    
# utilities
def natural_sort(l):
    convert = lambda text: int(text) if text.isdigit() else text.lower()
    alphanum_key = lambda key: [ convert(c) for c in re.split('([0-9]+)', key) ]
    l.sort(key=alphanum_key)

def get_result_file(params, run):
    return os.path.join(params.task_dir, run, params.result_file)

def get_task_table_name(params):
    return '[' + "|".join([params.task_name, socket.gethostname(), getpass.getuser(), params.task_path]) + ']'

def immediate_subdir(root):
    return [name for name in os.listdir(root) if os.path.isdir(os.path.join(root, name))]

def get_trailing_num(s):
    match = re.search(r'\d+$', s)
    return int(match.group()) if match else None

def is_type_of(s, convert):
    try:
        convert(s)
        return True
    except ValueError:
        return False

def is_int(s):
    return is_type_of(s, int)

def is_float(s):
    return is_type_of(s, float)

# try converting to numerical types using the strictest converters first
def convert_strictest(s):
    try:
        return int(s)
    except ValueError:
        try:
            return float(s)
        except ValueError:
            return s

if __name__ == "__main__":
    main()
