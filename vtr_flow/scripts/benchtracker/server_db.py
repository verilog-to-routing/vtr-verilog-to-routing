#!/usr/bin/python
# web service, so return only JSON (no HTML)
from flask import Flask, jsonify, request, url_for, make_response, render_template, send_file
import sqlite3
import interface_db as d
import os.path
import urlparse
import argparse
import textwrap
import functools  # need to wrap own decorators to comply with flask views
import zipfile
from io import BytesIO

try:
    from flask.ext.cors import CORS  # The typical way to import flask-cors
except ImportError:
    # Path hack allows examples to be run without installation.
    import os

    parentdir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    os.sys.path.insert(0, parentdir)

    from flask_cors import CORS


app = Flask(__name__)
app.debug = False
if not app.debug:
    print("adding logger")
    import logging
    from logging import FileHandler
    from logging import Formatter

    data_dir = os.path.expanduser("~/benchtracker_data/")
    if not os.path.isdir(data_dir):
        os.makedirs(data_dir)
    file_handler = FileHandler(os.path.join(data_dir, "log.txt"))
    file_handler.setFormatter(
        Formatter("%(asctime)s %(levelname)s: %(message)s " "[in %(pathname)s:%(lineno)d")
    )
    file_handler.setLevel(logging.WARNING)
    app.logger.addHandler(file_handler)

cors = CORS(app)
port = 5000
database = None
root_directory = None


def parse_args(ns=None):
    """parse arguments from command line and return as namespace object"""
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent(
            """\
            serve a central database with benchmark information
            
        Generated database:
            Database should be created by populate_db.py, with each task
            organized as a table. A task is a collection of related benchmarks
            that are commonly run together."""
        ),
        usage="%(prog)s [OPTIONS]",
    )

    parser.add_argument(
        "-d",
        "--database",
        default="results.db",
        help="name of database to store results in; default: %(default)s",
    )
    parser.add_argument(
        "-r",
        "--root_directory",
        default="~/benchtracker_data/",
        help="name of the directory to store databases in; default: %(default)s",
    )
    parser.add_argument(
        "-p",
        "--port",
        default=5000,
        type=int,
        help="port number to listen on; default: %(default)s",
    )
    params = parser.parse_args(namespace=ns)
    global database, port, root_directory
    root_directory = os.path.expanduser(params.root_directory)
    database = os.path.expanduser(params.database)
    port = params.port
    return params


def catch_operation_errors(func):
    @functools.wraps(func)
    def task_checker(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except IOError as e:
            return jsonify({"status": "File does not exist! ({})".format(e)})
        except IndexError:
            return jsonify({"status": "Index out of bounds! (likely table index)"})
        except sqlite3.OperationalError as e:
            return jsonify({"status": "Malformed request! ({})".format(e)})

    return task_checker


# each URI should be a noun since it is a resource (vs a verb for most library functions)


@app.route("/")
@app.route("/tasks", methods=["GET"])
@catch_operation_errors
def get_tasks():
    database = parse_db()
    return jsonify({"tasks": d.list_tasks(database), "database": database})


@app.route("/db", methods=["GET"])
def get_database():
    return jsonify({"database": database})


@app.route("/param", methods=["GET"])
@catch_operation_errors
def get_param_desc():
    database = parse_db()
    tasks = parse_tasks()
    param = request.args.get("p")
    mode = request.args.get("m", "range")  # by default give ranges, overriden if param is text
    try:
        (param_type, param_val) = d.describe_param(param, mode, tasks, real_db(database))
    except ValueError as e:
        return jsonify({"status": "Parameter value error: {}".format(e)})
    return jsonify(
        {
            "status": "OK",
            "database": database,
            "tasks": tasks,
            "param": param,
            "type": param_type,
            "val": param_val,
        }
    )


@app.route("/params/", methods=["GET"])
@catch_operation_errors
def get_shared_params():
    database = parse_db()
    tasks = parse_tasks()
    params = d.describe_tasks(tasks, real_db(database))
    return jsonify({"status": "OK", "database": database, "tasks": tasks, "params": params})


@app.route("/data", methods=["GET"])
@catch_operation_errors
def get_filtered_data():
    (exception, payload) = parse_data()
    if exception:
        return payload
    else:
        (databaes, tasks, params, data) = payload
        return jsonify(
            {"status": "OK", "database": database, "tasks": tasks, "params": params, "data": data}
        )


@app.route("/csv", methods=["GET"])
@catch_operation_errors
def get_csv_data():
    """Return a zipped archive of csv files for selected tasks"""
    (exception, payload) = parse_data()
    if exception:
        return payload
    else:
        (database, tasks, params, data) = payload
        memory_file = BytesIO()
        with zipfile.ZipFile(memory_file, "a", zipfile.ZIP_DEFLATED) as zf:
            t = 0
            for csvf in d.export_data_csv(params, data):
                # characters the filesystem might complain about (/|) are replaced
                zf.writestr(
                    "benchmark_results/" + tasks[t].replace("/", ".").replace("|", "__"),
                    csvf.getvalue(),
                )
                t += 1

        # prepare to send over network
        memory_file.seek(0)
        return send_file(
            memory_file, attachment_filename="benchmark_results.zip", as_attachment=True
        )


@app.route("/view")
@catch_operation_errors
def get_view():
    database = parse_db()
    print(real_db(database))
    tasks = {task_name for (task_name) in d.list_tasks(real_db(database))}
    queried_tasks = parse_tasks()
    x = y = filters = ""
    # only pass other argument values if valid tasks selected
    if queried_tasks:
        x = request.args.get("x")
        y = request.args.get("y")
        (temp, filters) = parse_filters(verbose=True)

    return render_template(
        "viewer.html",
        database=database,
        tasks=tasks,
        queried_tasks=queried_tasks,
        queried_x=x,
        queried_y=y,
        filters=filters,
    )


@app.errorhandler(404)
def not_found(error):
    resp = jsonify({"error": "Not found"})
    resp.status_code = 404
    return resp


# library call knows path to database, web viewer doesn't for security reasons
def real_db(relative_db):
    return os.path.join(root_directory, relative_db)


# should always be run before all other querying since it determines where to look from
def parse_db():
    db = request.args.get("db")
    if db:
        return db
    # default to the global database
    return database


def parse_tasks():
    tasks = request.args.getlist("t")
    if tasks and tasks[0].isdigit():
        all_tasks = d.list_tasks(database)
        tasks = [all_tasks[int(t)] for t in tasks]
    return tasks


def parse_filters(verbose=False):
    """
    Parse filter from current request query string and return the filtered parameters and filters in a list

    verbose mode returns filters without splitting out the type
    """
    filter_param = None
    filter_method = None
    filters = []
    filter_args = []
    filter_params = []
    for arg in urlparse.parse_qsl(request.query_string):
        if arg[0][0] != "f":
            continue
        # new filter parameter
        if arg[0] == "fp":
            # previous filter ready to be built
            if filter_param and filter_method and filter_args:
                filters.append(d.Task_filter(filter_param, filter_method, filter_args))
                filter_args = []  # clear arguments; important!
                print("{}: {}".format(filters[-1], filters[-1].args))
                filter_params.append(filter_param)
            # split out the optional type following parameter name
            if verbose:
                filter_param = arg[1]
            else:
                filter_param = arg[1].split()[0]

        if arg[0] == "fm":
            filter_method = arg[1]
        if arg[0] == "fa":
            filter_args.append(arg[1])
    # last filter to be added
    if (not filters or filter_param != filters[-1].param) and filter_args:
        filters.append(d.Task_filter(filter_param, filter_method, filter_args))
        print("{}: {}".format(filters[-1], filters[-1].args))
        filter_params.append(filter_param)

    return filter_params, filters


def parse_data():
    """
    Parses request for data and returns a 2-tuple payload.

    First item is True for an exception occurance, with the 2nd item being the json response for error.
    Else false for no exception occruance, with second item being a 3-tuple.
    """
    database = parse_db()
    # split to get name only in case type is also given
    x_param = request.args.get("x")
    y_param = request.args.get("y")

    if not x_param:
        return (True, jsonify({"status": "Missing x parameter!"}))
    if not y_param:
        return (True, jsonify({"status": "Missing y parameter!"}))
    x_param = x_param.split()[0]
    y_param = y_param.split()[0]

    try:
        (filtered_params, filters) = parse_filters()
    except IndexError:
        return (True, jsonify({"status": "Incomplete filter arguments!"}))
    except ValueError:
        return (True, jsonify({"status": "Unsupported filter method!"}))

    tasks = parse_tasks()

    (params, data) = d.retrieve_data(x_param, y_param, filters, tasks, real_db(database))
    return (False, (database, tasks, params, data))


if __name__ == "__main__":
    parse_args()
    app.run(host="0.0.0.0", port=port)
