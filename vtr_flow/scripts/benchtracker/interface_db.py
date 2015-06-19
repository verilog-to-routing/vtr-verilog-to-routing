#!/usr/bin/python

import sqlite3
import os.path
import sys
import re
from subprocess import call
import argparse
import textwrap

def list_tasks(dbname = "results.db"):
    """Return a list of all task names in a database"""
    db = connect_db(dbname)
    cursor = db.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    return [row[0] for row in cursor.fetchall()]


def describe_tasks(tasks, dbname = "results.db"):
    """Return a list of all shared parameters for some tasks of a database"""
    db = connect_db(dbname)
    cursor = db.cursor()
    if not isinstance(tasks,list):
        tasks = [tasks]
    parameter_sub = sql_substitute(tasks)
    query_command = "SELECT sql FROM sqlite_master WHERE type='table' AND name IN ({});".format(parameter_sub)
    cursor.execute(query_command, tasks)
    schemas = cursor.fetchall()
    # only the shared columns will remain
    shared_params = []
    for schema in schemas:
        params = [param.strip() for param in schema[0].split('(')[1].split(')')[0].split(',')]
        if (params[-1] == 'PRIMARY KEY'):
            params.pop()
        if shared_params:
            shared_params = intersection(shared_params, params)
        else:
            shared_params = params
    return shared_params
    

def retrieve_data(x_param, y_param, filters, tasks, dbname = "results.db"):
    """
    Return a list of selected parameters and a data structure (list of list of tuples),

    - 1st index corresponds to the task,
    - 2nd index corresponds to the row,
    - 3rd index corresponds to the selected parameter.
    The key parameters that define a benchmark are always selected.
    """
    db = connect_db(dbname)
    data = []

    cols_to_select = [x_param, y_param]

    # always pass shared primary key information (they define a distinct benchmark)
    primary_keys = []
    for t in range(len(tasks)):
        if t == 0:
            primary_keys = retrieve_primary_keys(tasks[t], db)
        else:
            primary_keys = intersection(primary_keys, retrieve_primary_keys(tasks[t], db))
    for key in primary_keys:
        if key not in cols_to_select:
            cols_to_select.append(key)

    # also pass filter parameter value in
    for f in filters:
        if f.param not in cols_to_select:
            cols_to_select.append(f.param)

    sql_val_args = []
    filter_command = ""
    for t in range(len(tasks)):
        select_command = "SELECT DISTINCT {} FROM {} ".format(','.join(cols_to_select), task_name(tasks[t]))
        if filters:
            # first time, still need to populate sql_val_args and make filter_command
            if t == 0: 
                filter_command = "WHERE "
                for f in range(len(filters)):

                    filter_command += str(filters[f])
                    sql_val_args.extend(filters[f].args)

                    if f < len(filters) - 1:
                        filter_command += " AND "

        select_command += filter_command
        select_command += ';'

        print(select_command)
        cursor = db.cursor()
        cursor.execute(select_command, sql_val_args)
        data.append(cursor.fetchall());
        
    return cols_to_select, data


def describe_param(param, mode, tasks, dbname = "results.db"):
    """
    Give back metainformation about a parameter to allow for easier filtering.
    
    Param would be an element of the list returned by describe_tasks - space separated name and type
    Returns a 2-tuple describing the parameter type and values for some tasks of a database.
    - 1st value is either 'range' or 'categorical'
    - 2nd value is either a 2-tuple for range types, or a n-tuple for categorical
    """
    db = connect_db(dbname)
    cursor = db.cursor()

    (param_name, param_type) = param.split()
    if param_type == "TEXT":
        mode = 'categorical'
    elif mode not in {'categorical', 'range'}:
        raise ValueError

    subquery = ""
    min_param = "min_p"
    max_param = "max_p"
    if not isinstance(tasks,list):
        subquery = task_name(tasks)
        min_param = max_param = param_name
    else:
        subquery += '('
        for t in range(len(tasks)):
            if mode == "categorical":
                subquery += "SELECT DISTINCT {} FROM {}".format(param_name, task_name(tasks[t]))
            else:
                subquery += "SELECT MIN({0}) as min_p, MAX({0}) as max_p FROM {1}".format(param_name, task_name(tasks[t]))
            
            if t < len(tasks) - 1:
                subquery += " UNION ALL "
        subquery += ')'

    print(subquery)

    # categorical data, return a list of all distinct values
    if mode == 'categorical':
        cursor.execute("SELECT DISTINCT {} FROM {};".format(param_name, subquery))
        return (mode,tuple(row[0] for row in cursor.fetchall()))
    # ranged data, return (min, max)
    else:
        cursor.execute("SELECT MIN({}), MAX({}) FROM {};".format(min_param, max_param, subquery))
        return (mode,tuple(cursor.fetchone()))



def connect_db(dbname = "results.db"):
    """Attempt a database connection, exiting with 1 if dbname does not exist, else return with db connection"""
    if not os.path.isfile(dbname):
        print("{} does not exist".format(dbname))
        sys.exit(1)
    db = sqlite3.connect(dbname)
    db.row_factory = sqlite3.Row
    return db


# filter object
valid_filter_methods = {"IN", "BETWEEN", "LIKE", "=", "<>", "!=", ">", "<", ">=", "<="}
class Task_filter:
    def __init__(self, param, method, args):
        self.param = param
        if method.upper() in valid_filter_methods:
            self.method = method.upper()
        else:
            print(method, "is not a supported filter method")
            raise ValueError
        self.args = args
    def __str__(self):
        substitutions = sql_substitute(self.args)
        if self.method == "BETWEEN":
            substitutions = "? AND ?"
        elif self.method == "IN":
            substitutions = '('+substitutions+')'
        return "({} {} {})".format(self.param, self.method, substitutions)
        

# internal utilities
def task_name(task):
    return '['+task+']'
def intersection(first, other):
    intersection_set = set.intersection(set(first), set(other))
    # reimpose order
    return [item for item in first if item in intersection_set]

def sql_substitute(args):
    return ('?,'*len(args)).rstrip(',')

def retrieve_primary_keys(task, db):
    cursor = db.cursor()
    cursor.execute("PRAGMA table_info(%s)" % task_name(task))
    column_info = cursor.fetchall()
    primary_keys = []
    for info in column_info:
        print(info)
        if info[5] != 0:
            print("key param:", info[1])
            primary_keys.append(info[1])

    return primary_keys

