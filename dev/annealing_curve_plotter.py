#!/usr/bin/env python3
"""
This is a script that parses a VPR placement logs and generates a plots of
data dumped during placement. This is useful for debugging / analyzing
behaviour of the simulated annealing placement algorithm.

To print the usage and option help run this script with the "-h" switch.

The script uses matplotlib to generate plots. A plot may be displayed
immediately when no "-o" option is given or written to an image file.
"""

import argparse
from collections import namedtuple

import matplotlib.pyplot as plt
from matplotlib.backends.backend_agg import FigureCanvasAgg

# =============================================================================


Column = namedtuple("Column", "index name label")

# =============================================================================


def extract_annealing_log(log):
    """
    Parses a VPR log line by line. Extracts the table with simulated annealing
    data. Returns the table header and data line lists as separate.
    """

    # Find a line starting with "# Placement"
    for i, line in enumerate(log):
        if line.startswith("# Placement"):
            log = log[i:]
            break
    else:
        return [], []

    # Find and cut the header
    header_lines = []
    for i, line in enumerate(log):
        if line.startswith("------"):
            header_lines = log[i : i + 3]
            log = log[i + 2 :]
            break

    # Gather data lines until '# Placement took' is encountered
    data_lines = []
    for line in log:

        # Reached the end of the placement section
        if line.startswith("# Placement took"):
            break

        # Split into fields. All of them have to be numbers
        fields = line.strip().split()
        try:
            for f in fields:
                number = float(f)
        except ValueError:
            continue

        # Append the line in text format
        data_lines.append(line.strip())

    return header_lines, data_lines


def extract_fields(header):
    """
    Given the thee header lines of the data table, extracts field names
    and indices.
    """

    assert len(header) == 3

    # Detect column boundaries.
    # FIXME: this assumes a single space between fields.
    widths = [len(s) + 1 for s in header[0].strip().split()]
    labels = header[1]

    columns = []
    for i, width in enumerate(widths):
        p0 = sum(widths[:i])
        p1 = sum(widths[: i + 1])

        label = labels[p0:p1].strip()
        name = label.lower().replace(" ", "_")

        columns.append(Column(i, name, label))

    return columns


# =============================================================================


def main():

    # Parse arguments
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument("log", type=str, nargs="*", help="A VPR placement log(s)")

    parser.add_argument("--xdata", type=str, default="t", help="Horizontal axis data source")

    parser.add_argument("--ydata", type=str, default="av_cost", help="Vertical axis data source")

    parser.add_argument("--logx", action="store_true", help="Log scale on X axis")

    parser.add_argument("--logy", action="store_true", help="Log scale on Y axis")

    parser.add_argument("--title", type=str, default=None, help="Plot title")

    parser.add_argument("--labels", type=str, nargs="*", help="Plot labels")

    parser.add_argument(
        "--list-columns", action="store_true", help="List available column names to plot"
    )

    parser.add_argument(
        "-o", type=str, default=None, help="Output file name to save the plot to or none to show it"
    )

    args = parser.parse_args()

    def col_name_error(name):
        print("Unknown data column '{}', available are:".format(name))
        for n in column_by_name.keys():
            print("", n)

    # Load and parse logs, gather data
    print("Parsing logs...")
    logs = []
    for log_file in args.log:
        print(log_file)

        # Load the log
        with open(log_file, "r") as fp:
            full_log = fp.readlines()

        # Extract the simulated annealing log
        header, annealing_log = extract_annealing_log(full_log)
        if len(annealing_log) == 0:
            print("No simulated annealing data found!")
            exit(-1)

        # Extract column names and indices from the table header
        columns = extract_fields(header)
        column_by_index = {c.index: c for c in columns}
        column_by_name = {c.name: c for c in columns}

        # List column names only
        if args.list_columns:
            for col in columns:
                print("", col.name.ljust(12), "-", col.label)
            continue

        # Parse the data,
        data = {c.name: [] for c in columns}
        for line in annealing_log:

            # Split fields, convert to floats
            fields = [float(f) for f in line.split()]

            # Distribute data over columns
            for i, val in enumerate(fields):
                col = column_by_index[i]
                data[col.name].append(val)

        # Get X and Y data
        if args.xdata not in column_by_name:
            col_name_error(args.xdata)
            exit(-1)

        if args.ydata not in column_by_name:
            col_name_error(args.ydata)
            exit(-1)

        if args.xdata == args.ydata:
            print("Plotting '{}' vs '{}' makes no sense".format(args.ydata, args.xdata))
            exit(-1)

        logs.append(
            {
                "xdata": data[args.xdata],
                "ydata": data[args.ydata],
            }
        )

        # This assumes identical labels for all logs!
        xlabel = column_by_name[args.xdata].label
        ylabel = column_by_name[args.ydata].label

    # List column names only
    if args.list_columns:
        return

    # Render the plot
    fig = plt.figure()
    ax = fig.gca()

    xmins = []
    xmaxs = []
    ymins = []
    ymaxs = []

    for i, log in enumerate(logs):
        xdata = log["xdata"]
        ydata = log["ydata"]

        if args.labels is not None and i < len(args.labels):
            label = args.labels[i]
        else:
            label = "{}".format(i + 1)

        if not args.logx and not args.logy:
            ax.plot(xdata, ydata, label=label)
        if args.logx and not args.logy:
            ax.semilogx(xdata, ydata, label=label)
        if not args.logx and args.logy:
            ax.semilogy(xdata, ydata, label=label)
        if args.logx and args.logy:
            ax.loglog(xdata, ydata, label=label)

        xmins.append(min(xdata))
        xmaxs.append(max(xdata))
        ymins.append(min(ydata))
        ymaxs.append(max(ydata))

    if len(logs) > 1:
        ax.legend()

    ax.set_xlim([min(xmins), max(xmaxs)])
    ax.set_ylim([min(ymins), max(ymaxs)])
    ax.grid(True)

    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)

    if args.title is not None:
        ax.set_title(args.title)
    else:
        ax.set_title("{} vs. {}".format(ylabel, xlabel))

    fig.tight_layout()

    # Display or save the plot
    if args.o is None:
        plt.show()

    else:
        canvas = FigureCanvasAgg(fig)
        canvas.print_figure(args.o, 120)  # 120 DPI


# =============================================================================


if __name__ == "__main__":
    main()
