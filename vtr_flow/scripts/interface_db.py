#!/usr/bin/python

import sqlite3
import os.path
import sys
import re
from subprocess import call
import argparse
import textwrap

def list_tasks(dbname = "results.db"):
    db = connect_db(dbname)
    cursor = db.cursor()
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table';")
    return [row[0] for row in cursor.fetchall()]


def describe_tasks(tasks, dbname = "results.db"):
    db = connect_db(dbname)
    cursor = db.cursor()
    parameter_sub = ("?,"*len(tasks)).rstrip(',')
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
    
# attempt a connection, exiting with 1 if dbname does not exist, else return with db connection
def connect_db(dbname = "results.db"):
    if not os.path.isfile(dbname):
        print("{} does not exist".format(dbname))
        sys.exit(1)
    db = sqlite3.connect(dbname)
    db.row_factory = sqlite3.Row
    return db


# utilities
def intersection(first, other):
    intersection_set = set.intersection(set(first), set(other))
    # reimpose order
    return [item for item in first if item in intersection_set]
