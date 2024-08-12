#!/usr/bin/env python3

import os
import shutil
import sys
import pandas as pd
from sklearn.metrics import mean_squared_error
import matplotlib.pyplot as plt
import distinctipy as dp
from colour import Color
from enum import Enum
import seaborn as sns
import argparse
from pathlib import Path
from multiprocessing import Process, Lock, Queue

# Output directory
output_dir = "../tasks/lookahead_verifier_output"
# The graph types (pie, heatmap, bar, scatter) that will be created
graph_types: list
# The components that will be used for graphs (cost, delay, congestion)
components: list
# Whether to create graphs of absolute error
absolute_output: bool
# Whether to create graphs of signed error
signed_output: bool
# Whether to create csv with all data extracted/calculated
csv_data: bool
# Whether to create graphs using data from first iterations
first_it_output: bool
# Whether to create graphs using data from all iterations
all_it_output: bool
# Do not overwrite existing files
no_replace: bool
# Print to terminal
should_print: bool
# The percent threshold to the be upper limit for the error of the data used to create pie graphs
percent_error_threshold: float
# Exclusions in the form exclusions[column_name] = [(boolean_op, value), (boolean_op, value), ...]
exclusions = {}

# Column names. IMPORTANT: Up until "criticality", these column names must match
# those that are written out by LookaheadProfiler.
column_names = [
    "iteration no.",
    "source node",
    "sink node",
    "sink block name",
    "sink atom block model",
    "sink cluster block type",
    "sink cluster tile height",
    "sink cluster tile width",
    "current node",
    "node type",
    "node length",
    "num. nodes from sink",
    "delta x",
    "delta y",
    "actual cost",
    "actual delay",
    "actual congestion",
    "predicted cost",
    "predicted delay",
    "predicted congestion",
    "criticality",
    "cost error",
    "cost absolute error",
    "cost % error",
    "delay error",
    "delay absolute error",
    "delay % error",
    "congestion error",
    "congestion absolute error",
    "congestion % error",
    "test name"
]

# Lock and Queue for multithreading
lock = Lock()
q = Queue()


# Check if a component is valid, otherwise raise exception
def check_valid_component(comp: str):
    if not (comp == "cost" or comp == "delay" or comp == "congestion"):
        raise Exception("ComponentType")


# Check if a column name is valid, otherwise raise exception
def check_valid_column(column: str):
    if column not in column_names:
        raise Exception("ColumnName")


# Check if a DataFrame is valid, otherwise raise exception
def check_valid_df(df: pd.DataFrame):
    if df.columns.to_list() != column_names:
        raise Exception("IncompleteDataFrame")


# Create a directory
def make_dir(directory: str, clean: bool):
    lock.acquire()

    if os.path.exists(directory):
        if clean:
            shutil.rmtree(directory)
        else:
            lock.release()
            return

    os.makedirs(directory)

    lock.release()


# Convert column names with spaces to (shorter) path names with underscores
def column_file_name(column_name: str) -> str:
    file_name = ""

    if len(column_name.split()) > 2:
        if column_name == "sink block name":
            file_name = "block_name"
        elif column_name == "sink atom block model":
            file_name = "atom_model"
        elif column_name == "sink cluster block type":
            file_name = "cluster_type"
        elif column_name == "sink cluster tile height":
            file_name = "tile_height"
        elif column_name == "sink cluster tile width":
            file_name = "tile_width"
        elif column_name == "num. nodes from sink":
            file_name = "node_distance"
    else:
        file_name = column_name.replace(" ", "_")

    return file_name


# For lists which include "--", make sure this comes at the top of the list, followed
# by the rest of the list sorted normally.
def custom_sort(column: list) -> list:
    try:
        list(map(int, column))
    except ValueError:
        new_column = [value for value in column if value != "--"]

        try:
            new_column = list(map(int, new_column))
        except ValueError:
            return sorted(column)

        return ["--"] + list(map(str, sorted(new_column)))

    return sorted(column)


class ScatterPlotType(Enum):
    PREDICTION = 0
    ERROR = 1


class Graphs:
    def __init__(self, df: pd.DataFrame, results_folder: str, test_name: str):
        self.__df = df.copy()
        self.__df.insert(len(self.__df.columns), "color", "#0000ff")

        # Create a DataFrame with only data from first iteration
        self.__df_first_it = self.__df[self.__df["iteration no."].isin([1])][:]

        self.__directory = os.path.join(output_dir, results_folder)

        self.__sub_dirs = [
            os.path.join(self.__directory, "actual_vs_prediction"),
            os.path.join(self.__directory, "actual_vs_error"),
            os.path.join(self.__directory, "bar_absolute_error"),
            os.path.join(self.__directory, "bar_error"),
            os.path.join(self.__directory, "heatmap_absolute_error"),
            os.path.join(self.__directory, "heatmap_error"),
            os.path.join(self.__directory, "proportion_under_threshold"),
        ]
        for directory in self.__sub_dirs:
            make_dir(directory, False)

        self.__test_name = test_name

        # The standard columns to use to color scatter plots
        self.__standard_scatter_columns = [
            "iteration no.",
            "sink atom block model",
            "sink cluster block type",
            "node type",
            "node length",
            "test name",
        ]
        # The standard columns to use to split data by for bar graphs and pie charts
        self.__standard_bar_columns = [
            "num. nodes from sink",
            "sink atom block model",
            "sink cluster block type",
            "node type",
            "node length",
            "test name",
        ]

        # The columns for which horizontal bar graphs should be produced
        self.__barh_types = ["sink block name",
                             "sink atom block model",
                             "sink cluster block type",
                             "node type",
                             "test name"]

    # Write list of exclusions onto output png
    def write_exclusions_info(self):
        if len(exclusions):
            excl_msg = "Exclusions: "
            for key, val in exclusions.items():
                for item in val:
                    excl_msg += key + " " + item[0] + " " + item[1] + ", "

            excl_msg = excl_msg[0:-2]

            plt.figtext(0, -0.08, excl_msg, horizontalalignment="left", fontsize="x-small")

    # Create a scatter plot displaying actual [comp] vs. either predicted [comp] or error
    # comp: The component (cost, delay, or congestion)
    # plot_type: PREDICTION or ERROR (vs. actual [comp])
    # legend_column: The DF column to use to color the plot
    # first_it_only: Whether to create a graph only using self.__df_first_it
    def make_scatter_plot(
            self, comp: str, plot_type: ScatterPlotType, legend_column: str, first_it_only: bool
    ):
        if (not first_it_output and first_it_only) or (not all_it_output and not first_it_only):
            return

        # Check that the component and column name are valid
        check_valid_component(comp)
        check_valid_column(legend_column)

        # Determine the graph title, directory, and file name
        title, curr_dir = "", ""
        if plot_type == ScatterPlotType.PREDICTION:
            title = "Actual vs Predicted " + comp + " for "
            curr_dir = os.path.join(self.__directory, "actual_vs_prediction", comp)
        elif plot_type == ScatterPlotType.ERROR:
            title = "Actual " + comp + " vs Error for "
            curr_dir = os.path.join(self.__directory, "actual_vs_error", comp)

        title = title.title()
        title += self.__test_name

        if first_it_only:
            title += " (first iteration)"
            file_name = "_first_it"
            curr_df = self.__df_first_it
            curr_dir = os.path.join(curr_dir, "first_it")
        else:
            title += " (all iterations)"
            file_name = "_all_it"
            curr_df = self.__df
            curr_dir = os.path.join(curr_dir, "all_it")

        file_name = "color_" + column_file_name(legend_column) + "_" + comp + file_name + ".png"

        if no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
            return

        make_dir(curr_dir, False)

        # Determine colors for legend
        num_colors = self.__df[legend_column].nunique() + 1
        if legend_column == "iteration no.":
            green = Color("green")
            colors_c = list(green.range_to(Color("red"), num_colors))
            colors = [color.hex_l for color in colors_c]
        else:
            colors_rgb = dp.get_colors(num_colors)
            colors = []
            for color in colors_rgb:
                color = tuple(map(round, tuple(255 * i for i in color)))
                colors.append("#{:02x}{:02x}{:02x}".format(color[0], color[1], color[2]))

        value_map = {}

        i = 0
        for name in curr_df[legend_column].unique():
            value_map[name] = i
            i += 1

        for row in curr_df.index:
            curr_df.at[row, "color"] = colors[
                value_map[(curr_df.at[row, legend_column])] % len(colors)
                ]

        # Create graph
        fig, ax = plt.subplots()
        for elem in custom_sort(list(curr_df[legend_column].unique())):
            section = curr_df[curr_df[legend_column].isin([elem])][:]
            section = section.reset_index(drop=True)

            if plot_type == ScatterPlotType.PREDICTION:
                y_data = list(section.loc[:, f"predicted {comp}"])
            else:
                y_data = list(section.loc[:, f"{comp} error"])

            ax.scatter(
                x=list(section.loc[:, f"actual {comp}"]),
                y=y_data,
                color=section.at[0, "color"],
                label=elem,
                edgecolors="none",
                s=40,
            )

        ax.legend(title=legend_column, loc="upper left", bbox_to_anchor=(1.04, 1))
        ax.grid(True)
        plt.xlabel(f"actual {comp}")
        plt.title(title)

        if plot_type == ScatterPlotType.PREDICTION:
            ax.plot(curr_df[f"actual {comp}"], curr_df[f"actual {comp}"], color="black")
            plt.ylabel(f"predicted {comp}")
        elif plot_type == ScatterPlotType.ERROR:
            ax.plot(curr_df[f"actual {comp}"], [0.0] * len(curr_df), color="black")
            plt.ylabel(f"{comp} error")

        self.write_exclusions_info()
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    # Make all scatter plots in self.__standard_scatter_columns
    # test_name_plot: whether to create plots wherein legend is by test name. This option
    # is useful when creating graphs for an entire benchmark, but not specific circuits
    # (tests).
    def make_standard_scatter_plots(self, test_name_plot: bool):
        scatter_types = [ScatterPlotType.PREDICTION, ScatterPlotType.ERROR]

        if test_name_plot:
            legend_columns = self.__standard_scatter_columns
        else:
            legend_columns = self.__standard_scatter_columns[0:-1]

        for comp in components:
            for plot_type in scatter_types:
                for col in legend_columns:
                    for first_it in [True, False]:
                        if first_it and col == "iteration no.":
                            continue

                        proc = Process(
                            target=self.make_scatter_plot, args=(comp, plot_type, col, first_it)
                        )
                        q.put(proc)

    # Create a bar graph displaying average error
    # comp: The component (cost, delay, or congestion)
    # column: The DF column to use to split data (i.e. create each bar)
    # first_it_only: Whether to create a graph only using self.__df_first_it
    # use_absolute: Whether to produce a graph using absolute error, as opposed to signed error
    def make_bar_graph(self, comp: str, column: str, first_it_only: bool, use_absolute: bool):
        if (
                (not absolute_output and use_absolute)
                or (not signed_output and not use_absolute)
                or (not first_it_output and first_it_only)
                or (not all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column name are valid
        check_valid_component(comp)
        check_valid_column(column)

        # Determine the graph title, directory, and file name
        title = "Average " + comp

        if use_absolute:
            title += " Absolute"
            curr_dir = os.path.join(self.__directory, "bar_absolute_error", comp)
            y_label = "average absolute error"
        else:
            curr_dir = os.path.join(self.__directory, "bar_error", comp)
            y_label = "average error"

        title += " Error by " + column + " for "
        title = title.title()
        title += self.__test_name

        if first_it_only:
            title += " (first iteration)"
            file_name = "_first_it"
            curr_df = self.__df_first_it
            curr_dir = os.path.join(curr_dir, "first_it")
        else:
            title += " (all iterations)"
            file_name = "_all_it"
            curr_df = self.__df
            curr_dir = os.path.join(curr_dir, "all_it")

        file_name = "by_" + column_file_name(column) + "_" + comp + file_name + ".png"

        if no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
            return

        make_dir(curr_dir, False)

        # Get DF with average error for each "type" encountered in column
        avg_error = {}
        for elem in custom_sort(list(curr_df[column].unique())):
            rows = curr_df[column].isin([elem])

            if use_absolute:
                error_by_elem = curr_df[rows][f"{comp} absolute error"]
            else:
                error_by_elem = curr_df[rows][f"{comp} error"]

            avg_error[elem] = error_by_elem.mean()

        avg_error_df = pd.DataFrame({"col": avg_error})

        # Create graph
        if column in self.__barh_types:
            avg_error_df.plot.barh(title=title, xlabel=y_label, ylabel=column, legend=False)
        else:
            avg_error_df.plot.bar(title=title, xlabel=column, ylabel=y_label, legend=False)

        self.write_exclusions_info()
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    # Make all bar graphs in self.__standard_bar_columns
    # test_name_plot: whether to create graph where data is split by test name. This option
    # is useful when creating graphs for an entire benchmark, but not specific circuits
    # (tests).
    def make_standard_bar_graphs(self, test_name_plot: bool):
        if test_name_plot:
            columns = self.__standard_bar_columns
        else:
            columns = self.__standard_bar_columns[0:-1]

        for comp in components:
            for col in columns:
                for use_abs in [True, False]:
                    for first_it in [True, False]:
                        proc = Process(
                            target=self.make_bar_graph, args=(comp, col, use_abs, first_it)
                        )
                        q.put(proc)

    # Create a heatmap comparing two quantitative columns
    # comp: The component (cost, delay, or congestion)
    # x_column: The column to be used for the x-axis
    # y_column: The column to be used for the y-axis
    # first_it_only: Whether to create a graph only using self.__df_first_it
    # use_absolute: Whether to produce a graph using absolute error, as opposed to signed error
    def make_heatmap(
            self, comp: str, x_column: str, y_column: str, first_it_only: bool, use_absolute: bool
    ):
        if (
                (not absolute_output and use_absolute)
                or (not signed_output and not use_absolute)
                or (not first_it_output and first_it_only)
                or (not all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column names are valid
        check_valid_component(comp)
        check_valid_column(x_column)
        check_valid_column(y_column)

        # Determine the graph title, directory, and file name
        title = "Average " + comp

        if use_absolute:
            title += " Absolute"
            curr_dir = os.path.join(self.__directory, "heatmap_absolute_error", comp)
            scale_column = f"{comp} absolute error"
        else:
            curr_dir = os.path.join(self.__directory, "heatmap_error", comp)
            scale_column = f"{comp} error"

        title += " Error Heatmap for "

        title = title.title()
        title += self.__test_name

        if first_it_only:
            title += " (first iteration)"
            file_name = "_first_it"
            curr_df = self.__df_first_it
            curr_dir = os.path.join(curr_dir, "first_it")
        else:
            title += " (all iterations)"
            file_name = "_all_it"
            curr_df = self.__df
            curr_dir = os.path.join(curr_dir, "all_it")

        file_name = (
                column_file_name(x_column)
                + "_and_"
                + column_file_name(y_column)
                + "_"
                + comp
                + file_name
                + ".png"
        )

        if no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
            return

        make_dir(curr_dir, False)

        # Get DF with average error for each "coordinate" in the heatmap
        df_avgs = pd.DataFrame(columns=[x_column, y_column, scale_column])

        for i in custom_sort(list(curr_df[x_column].unique())):
            section = curr_df[curr_df[x_column].isin([i])][:]

            for j in custom_sort(list(curr_df[y_column].unique())):
                subsection = section[section[y_column].isin([j])][:]

                if subsection.empty:
                    continue

                new_row = {
                    x_column: [i],
                    y_column: [j],
                    scale_column: [subsection[scale_column].mean()],
                }
                df_avgs = pd.concat([df_avgs, pd.DataFrame(new_row)], ignore_index=True)

        df_avgs = df_avgs.reset_index(drop=True)
        df_avgs = df_avgs.pivot(index=y_column, columns=x_column, values=scale_column)
        df_avgs = df_avgs.reindex(index=df_avgs.index[::-1])

        # Create heatmap
        if use_absolute:
            ax = sns.heatmap(df_avgs, cmap="rocket_r", vmin=0.0)
        else:
            ax = sns.heatmap(df_avgs, cmap="rocket_r")

        self.write_exclusions_info()
        ax.set_title(title, y=1.08)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    # Make "standard" heatmaps: (sink cluster tile width and sink cluster tile height) and
    # (delta_x and delta_y)
    def make_standard_heatmaps(self):
        for comp in components:
            for first_it in [True, False]:
                for use_abs in [True, False]:
                    proc = Process(
                        target=self.make_heatmap,
                        args=(
                            comp,
                            "sink cluster tile width",
                            "sink cluster tile height",
                            first_it,
                            use_abs,
                        ),
                    )
                    q.put(proc)

                    proc = Process(
                        target=self.make_heatmap,
                        args=(comp, "delta x", "delta y", first_it, use_abs),
                    )
                    q.put(proc)

    # Create a pie chart showing the proportion of cases where error is under percent_error_threshold
    # comp: The component (cost, delay, or congestion)
    # column: The column to split data by
    # first_it_only: Whether to create a graph only using self.__df_first_it
    # weighted: Whether to weight each section of the pie graph by the % error of that section
    def make_pie_chart(self, comp: str, column: str, first_it_only: bool, weighted: bool):
        if (not first_it_output and first_it_only) or (not all_it_output and not first_it_only):
            return

        # Check that the component and column names are valid
        check_valid_component(comp)
        check_valid_column(column)

        # Determine the graph title, directory, and file name
        title = f"Num. Mispredicts Under {percent_error_threshold:.1f}% {comp} Error"

        if weighted:
            title += ", Weighted by % Error,"

        title += " for "
        title = title.title()
        title += self.__test_name

        curr_dir = os.path.join(self.__directory, "proportion_under_threshold", comp)

        if first_it_only:
            title += " (first iteration)"
            file_name = "_first_it"
            curr_df = self.__df_first_it
            curr_dir = os.path.join(curr_dir, "first_it")
        else:
            title += " (all iterations)"
            file_name = "_all_it"
            curr_df = self.__df
            curr_dir = os.path.join(curr_dir, "all_it")

        file_name = "by_" + column_file_name(column) + "_" + comp + file_name

        if weighted:
            file_name += "_weighted"
            curr_dir = os.path.join(curr_dir, "weighted")
        else:
            curr_dir = os.path.join(curr_dir, "unweighted")

        file_name += ".png"

        if no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
            return

        make_dir(curr_dir, False)

        # Constrict DF to columns whose error is under threshold
        curr_df = curr_df[curr_df[f"{comp} % error"] < percent_error_threshold]

        # Determine colors for sections
        num_colors = self.__df[column].nunique() + 1
        colors_rgb = dp.get_colors(num_colors)
        colors = []
        for color in colors_rgb:
            color = tuple(map(round, tuple(255 * i for i in color)))
            colors.append("#{:02x}{:02x}{:02x}".format(color[0], color[1], color[2]))

        # Get DF which is maps elements to the number of occurrences found in curr_df
        proportion = {}
        for elem in custom_sort(list(curr_df[column].unique())):
            section = curr_df[curr_df[column].isin([elem])][:]

            proportion[elem] = len(section)

            if weighted:
                proportion[elem] *= abs(section[f"{comp} % error"].mean())

        pie_df = pd.DataFrame.from_dict(proportion, orient="index", columns=[" "])

        # Create pie chart
        pie_df.plot.pie(y=" ", labeldistance=None, colors=colors)

        self.write_exclusions_info()
        plt.title(title, fontsize=8)
        plt.legend(loc="upper left", bbox_to_anchor=(1.08, 1), title=column)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep='')

    # Make all pie chars in self.__standard_bar_columns
    # test_name_plot: whether to create graph where data is split by test name. This option
    # is useful when creating graphs for an entire benchmark, but not specific circuits
    # (tests).
    def make_standard_pie_charts(self, test_name_plot: bool):
        if test_name_plot:
            columns = self.__standard_bar_columns
        else:
            columns = self.__standard_bar_columns[0:-1]

        for comp in components:
            for col in columns:
                for first_it in [True, False]:
                    for weighted in [True, False]:
                        proc = Process(
                            target=self.make_pie_chart, args=(comp, col, first_it, weighted)
                        )
                        q.put(proc)

    # Make "standard" graphs of all types.
    # test_name_plot: whether to create plots where data is split by test name. This option
    # is useful when creating plots for an entire benchmark, but not specific circuits
    # (tests).
    def make_standard_plots(self, test_name_plot: bool):
        if "pie" in graph_types:
            self.make_standard_pie_charts(test_name_plot)

        if "heatmap" in graph_types:
            self.make_standard_heatmaps()

        if "bar" in graph_types:
            self.make_standard_bar_graphs(test_name_plot)

        if "scatter" in graph_types:
            self.make_standard_scatter_plots(test_name_plot)


# Print to terminal and write to lookahead_stats_summary.txt
def print_and_write(string, directory: str):
    if should_print:
        print(string, sep="")

    orig_out = sys.stdout
    stats_file = open(os.path.join(directory, "lookahead_stats_summary.txt"), "a")
    sys.stdout = stats_file

    print(string, sep="")

    sys.stdout = orig_out


# Print stats for a given component
# df: The DataFrame with all info
# comp: cost, delay, or congestion
# directory: output directory
def print_stats(df: pd.DataFrame, comp: str, directory: str):
    check_valid_component(comp)

    print_and_write("\n#### BACKWARDS PATH " + comp.upper() + " ####", directory)
    print_and_write("Mean error: " + str(df[f"{comp} error"].mean()), directory)
    print_and_write("Mean absolute error: " + str(df[f"{comp} absolute error"].mean()), directory)
    print_and_write("Median error: " + str(df[f"{comp} error"].median()), directory)
    print_and_write("Highest error: " + str(df[f"{comp} error"].max()), directory)
    print_and_write("Lowest error: " + str(df[f"{comp} error"].min()), directory)
    print_and_write(
        "Mean squared error: "
        + str(mean_squared_error(df[f"actual {comp}"], df[f"predicted {comp}"])),
        directory,
    )


# Write out stats for all components
# df: The DataFrame with all info
# directory: output directory
def print_df_info(df: pd.DataFrame, directory: str):
    print_and_write(df, directory)

    for comp in components:
        print_stats(df, comp, directory)


# Write out stats for the first iteration only, and for all iterations, as well
# as the csv_file containing the entire DataFrame
# df: The DataFrame with all info
# directory: output directory
def record_df_info(df: pd.DataFrame, directory: str):
    check_valid_df(df)

    df_first_iteration = df[df["iteration no."].isin([1])][:]

    print_and_write(
        "\nFIRST ITERATION RESULTS ----------------------------------------------------------------------\n",
        directory,
    )

    print_df_info(df_first_iteration, directory)

    print_and_write(
        "\nALL ITERATIONS RESULTS -----------------------------------------------------------------------\n",
        directory,
    )

    print_df_info(df, directory)

    print_and_write(
        "\n----------------------------------------------------------------------------------------------\n",
        directory,
    )

    def make_csv(df_out: pd.DataFrame, file_name: str):
        df_out.to_csv(file_name, index=False)

    # Write out the csv
    if csv_data and (not os.path.exists(os.path.join(directory, "data.csv")) or not no_replace):
        proc = Process(
            target=make_csv, args=(df, os.path.join(directory, "data.csv"))
        )
        q.put(proc)

        if should_print:
            print("Created ", os.path.join(directory, "data.csv"), sep="")


# Calculate error columns from given csv file
def create_error_columns(df: pd.DataFrame):
    df["cost error"] = df["predicted cost"] - df["actual cost"]
    df["cost absolute error"] = abs(df["cost error"])
    df["cost % error"] = 100.0 * ((df["predicted cost"] / df["actual cost"]) - 1.0)

    df["delay error"] = df["predicted delay"] - df["actual delay"]
    df["delay absolute error"] = abs(df["delay error"])
    df["delay % error"] = 100.0 * ((df["predicted delay"] / df["actual delay"]) - 1.0)

    df["congestion error"] = df["predicted congestion"] - df["actual congestion"]
    df["congestion absolute error"] = abs(df["congestion error"])
    df["congestion % error"] = 100.0 * (
            (df["predicted congestion"] / df["actual congestion"]) - 1.0
    )

    df.fillna(0.0, inplace=True)


def main():
    # Parse arguments
    description = (
            "Parses data from lookahead_verifier_info.csv file(s) generated when running VPR on this branch. "
            + "Create one or more csv files with, in the first column, the circuit/test name, and in the second "
            + "column, the corresponding path to its lookahead info csv file. Give it a unique name, and pass "
            + "it in as csv_files_list."
    )
    parser = argparse.ArgumentParser(prog="ParseLookaheadData", description=description, epilog="")

    parser.add_argument(
        "csv_files_list",
        help="the text file which contains the csv files that will be parsed",
        nargs="+",
    )

    parser.add_argument(
        "-j", type=int, default=1, help="number of processes to use", metavar="NUM_PROC"
    )

    parser.add_argument(
        "-g",
        "--graph-types",
        type=str,
        default="",
        metavar="GRAPH_TYPE",
        nargs="+",
        help="create graphs of this type (bar, scatter, heatmap, pie, all)",
        choices=["bar", "scatter", "heatmap", "pie", "all"],
    )

    parser.add_argument(
        "-c",
        "--components",
        type=str,
        default="",
        metavar="COMPONENT",
        nargs="+",
        help="produce results investigating this backwards-path component (cost, delay, congestion, "
             "all)",
        choices=["cost", "delay", "congestion", "all"],
    )

    parser.add_argument(
        "--absolute", action="store_true", help="create graphs using absolute error"
    )

    parser.add_argument(
        "--signed", action="store_true", help="create graphs graphs using signed error"
    )

    parser.add_argument(
        "--first-it", action="store_true", help="produce results using data from first iteration"
    )

    parser.add_argument(
        "--all-it", action="store_true", help="produce results using data from all iterations"
    )

    parser.add_argument("--csv-data", action="store_true", help="create csv file with all data")
    parser.add_argument(
        "--all",
        action="store_true",
        help="create all graphs, regardless of other options",
    )

    parser.add_argument("--exclude", type=str, default="", metavar="EXCLUSION", nargs="+",
                        help="exclude cells that meet a given condition, in the form \"column_name [comparison "
                             "operator] value\"")

    parser.add_argument(
        "--collect",
        type=str,
        default="",
        metavar="FOLDER_NAME",
        nargs=1,
        help="instead of producing results from individual input csv files, collect data from all input"
             "files, and produce results for this collection (note: FOLDER_NAME should not be a path)",
    )

    parser.add_argument(
        "--threshold",
        type=float,
        default=-16.7,
        metavar="THRESHOLD",
        nargs=1,
        help="the percent threshold to the be upper limit for the error of the data used to create "
             "pie graphs",
    )

    parser.add_argument(
        "--replace",
        action="store_true",
        help="overwrite existing csv files and images if encountered",
    )

    parser.add_argument(
        "--dir-app", type=str,
        default="",
        metavar="FOLDER_SUFFIX",
        nargs=1, help="append output results folder name (ignored when using --collect)"
    )

    parser.add_argument(
        "--print",
        action="store_true",
        help="print summaries and file creation messages to terminal",
    )

    args = parser.parse_args()

    global q
    q = Queue(args.j)

    global graph_types
    global components
    global absolute_output
    global signed_output
    global first_it_output
    global all_it_output

    if args.all:
        graph_types = ["bar", "scatter", "heatmap", "pie"]
        components = ["cost", "delay", "congestion"]
        absolute_output = True
        signed_output = True
        first_it_output = True
        all_it_output = True
    else:
        graph_types = args.graph_types
        if "all" in graph_types:
            graph_types = ["bar", "scatter", "heatmap", "pie"]

        if len(graph_types) == 0:
            print(
                "Warning: no graphs will be produced. Use -g to select graph types (bar, scatter, heatmap, pie, all).")

        components = args.components
        if "all" in components:
            components = ["cost", "delay", "congestion"]

        if len(components) == 0:
            print("Warning: no graphs will be produced. Use -c to select components (cost, delay, congestion, all).")

        absolute_output = args.absolute
        signed_output = args.signed

        if not absolute_output and not signed_output:
            print("Warning: no graphs will be produced. Use --signed and/or --absolute.")

        first_it_output = args.first_it
        all_it_output = args.all_it

        if not first_it_output and not all_it_output:
            print("Warning: no graphs will be produced. Use --first_it and/or --all_it.")

    global csv_data
    csv_data = args.csv_data

    global percent_error_threshold
    percent_error_threshold = args.threshold

    global no_replace
    no_replace = not args.replace

    global should_print
    should_print = args.print

    global exclusions
    for excl in args.exclude:
        operators = ["==", "!=", ">=", "<=", ">", "<"]
        key, val, op = "", "", ""
        for op_check in operators:
            if op_check in excl:
                key, val = excl.split(op_check)
                op = op_check
                break

        key, val = key.strip(), val.strip()

        if len(key) == 0 or len(val) == 0 or key not in column_names:
            print("Invalid entry in --exclusions:", excl)
            exit()

        if key not in exclusions:
            exclusions[key] = []

        try:
            int(val)
            exclusions[key].append((op, val))
        except ValueError:
            exclusions[key].append((op, "\"" + val + "\""))

    make_dir(output_dir, False)

    df_complete = pd.DataFrame(columns=column_names)  # The DF containing info across all given csv files

    csv_files_list = args.csv_files_list
    for file_list in csv_files_list:
        output_folder = Path(file_list).stem
        if len(args.dir_app) > 0:
            output_folder += f"{args.dir_app[0]}"

        # Get list of csv files with data
        df_files = pd.read_csv(file_list, header=None)
        tests = list(df_files.itertuples(index=False, name=None))

        if len(tests) < 1:
            continue

        df_benchmark = pd.DataFrame(columns=column_names)  # The DF containing all info for this benchmark

        for test_name, file_path in tests:
            if not os.path.isfile(file_path):
                print("ERROR:", file_path, "could not be found")
                continue

            if should_print:
                print("Reading data at " + file_path)

            results_folder = os.path.join(output_folder, test_name)
            results_folder_path = os.path.join(output_dir, results_folder)
            make_dir(results_folder_path, False)

            # Read csv with lookahead data (or, a csv previously created by this script)
            df = pd.read_csv(file_path)
            df = df.reset_index(drop=True)
            df = df.drop(columns=["cost error",
                                  "cost absolute error",
                                  "cost % error",
                                  "delay error",
                                  "delay absolute error",
                                  "delay % error",
                                  "congestion error",
                                  "congestion absolute error",
                                  "congestion % error",
                                  "test name"], errors="ignore")

            # Determine exclusions, and remove them from df
            for key, val in exclusions.items():
                for item in val:
                    ldict = {}
                    code_line = f"df = df.drop(index=df[df[\"{key}\"] {item[0]} {item[1]}].index.values.tolist())"
                    exec(code_line, locals(), ldict)
                    df = ldict["df"]
                    df = df.reset_index(drop=True)

            # Calculate error columns and add test name column
            create_error_columns(df)
            df.insert(len(df.columns), "test name", test_name)

            # Add current DF to DF for entire benchmark
            df_benchmark = pd.concat([df_benchmark, df])

            # If --collect option used, skip creating files for individual circuits
            if args.collect != "":
                continue

            # Write out stats summary and csv file
            print_and_write("Test: " + test_name, results_folder_path)
            record_df_info(df, results_folder_path)

            # Create plots
            curr_plots = Graphs(df, os.path.join(results_folder, "plots"), test_name)
            curr_plots.make_standard_plots(False)

            if should_print:
                print(
                    "\n----------------------------------------------------------------------------------------------"
                )

        # Create output files for entire benchmark

        if len(tests) == 1:
            continue

        results_folder = os.path.join(output_folder, "__all__")
        results_folder_path = os.path.join(output_dir, results_folder)
        make_dir(results_folder_path, False)

        df_benchmark = df_benchmark.reset_index(drop=True)
        if args.collect != "":
            df_complete = pd.concat([df_complete, df_benchmark])
            continue

        print_and_write("Global results:\n", results_folder_path)
        record_df_info(df_benchmark, results_folder_path)

        global_plots = Graphs(df_benchmark, os.path.join(results_folder, "plots"), f"All Tests ({output_folder})")
        global_plots.make_standard_plots(True)

    # If --collect used, create output files for all csv files provided

    if args.collect == "":
        return

    results_folder = args.collect[0]
    results_folder_path = os.path.join(output_dir, results_folder)
    if len(args.dir_app) > 0:
        results_folder_path += f"{args.dir_app[0]}"
    make_dir(results_folder_path, False)

    df_complete = df_complete.reset_index(drop=True)

    record_df_info(df_complete, results_folder_path)

    global_plots = Graphs(df_complete, os.path.join(results_folder, "plots"), "All Tests")
    global_plots.make_standard_plots(True)


if __name__ == "__main__":
    main()
