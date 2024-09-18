#!/usr/bin/env python3

# pylint: disable=too-many-lines
# pylint: disable=invalid-name

"""
Parses data from lookahead_verifier_info.csv file(s) generated when running VPR with
--router_lookahead_profiler on.
"""

import os
import shutil
import sys
import argparse
from collections import deque
from enum import Enum
from pathlib import Path
from multiprocessing import Lock, Process
import pandas as pd
from sklearn.metrics import mean_squared_error
import matplotlib.pyplot as plt
import distinctipy as dp
from colour import Color
import seaborn as sns

lock = Lock()  # Used for multiprocessing


# pylint: disable=too-many-instance-attributes
# pylint: disable=too-few-public-methods
class GlobalVars:
    """Global parameters and lists accessed across many functions."""

    # pylint: disable=too-many-arguments
    def __init__(self,
                 graph_types: list,
                 components: list,
                 absolute_output: bool,
                 signed_output: bool,
                 csv_data: bool,
                 first_it_output: bool,
                 all_it_output: bool,
                 no_replace: bool,
                 should_print: bool,
                 percent_error_threshold: float,
                 exclusions: dict):
        # Output directory
        self.output_dir = "./vtr_flow/tasks/lookahead_verifier_output"
        # The graph types (pie, heatmap, bar, scatter) that will be created
        self.graph_types = graph_types
        # The components that will be used for graphs (cost, delay, congestion)
        self.components = components
        # Whether to create graphs of absolute error
        self.absolute_output = absolute_output
        # Whether to create graphs of signed error
        self.signed_output = signed_output
        # Whether to create csv with all data extracted/calculated
        self.csv_data = csv_data
        # Whether to create graphs using data from first iterations
        self.first_it_output = first_it_output
        # Whether to create graphs using data from all iterations
        self.all_it_output = all_it_output
        # Do not overwrite existing files
        self.no_replace = no_replace
        # Print to terminal
        self.should_print = should_print
        # The percent threshold to the be upper limit for the error of the data used to create pie
        # graphs
        self.percent_error_threshold = percent_error_threshold
        # Exclusions in the form:
        # exclusions[column_name] = [(boolean_op, value), (boolean_op, value), ...]
        self.exclusions = exclusions

        # Column names. IMPORTANT: Up until "criticality", these column names must match
        # those that are written out by LookaheadProfiler.
        self.column_names = [
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

        # Processes list for multiprocessing
        self.processes = []


def check_valid_component(comp: str):
    """Check if a component is valid, otherwise raise exception"""

    if comp not in ["cost", "delay", "congestion"]:
        raise Exception("ComponentType")


def check_valid_column(column: str, gv: GlobalVars):
    """Check if a column name is valid, otherwise raise exception"""

    if column not in gv.column_names:
        raise Exception("ColumnName")


def check_valid_df(df: pd.DataFrame, gv: GlobalVars):
    """Check if a DataFrame is valid, otherwise raise exception"""

    if df.columns.to_list() != gv.column_names:
        raise Exception("IncompleteDataFrame")


def make_dir(directory: str, clean: bool):
    """Create a directory"""

    lock.acquire()

    if os.path.exists(directory):
        if clean:
            shutil.rmtree(directory)
        else:
            lock.release()
            return

    os.makedirs(directory)

    lock.release()


def column_file_name(column_name: str) -> str:
    """Convert column names with spaces to (shorter) path names with underscores"""

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
        file_name = column_name.replace(" ", "_").replace(".", "")

    return file_name


def custom_sort(column: list) -> list:
    """For lists which include "--", make sure this comes at the top of the list, followed by the
    rest of the list sorted normally.
    """

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
    """Two scatter plot types, PREDICTION (actual vs. prediction) and ERROR
    (actual vs. error)
    """

    PREDICTION = 0
    ERROR = 1


# pylint: disable=too-many-instance-attributes
class Graphs:
    """Attributes and methods for creating plots and graphs from given data"""

    def __init__(
            self, df: pd.DataFrame, results_folder: str, test_name: str, gv: GlobalVars
    ):
        self.__df = df.copy()
        self.__df.insert(len(self.__df.columns), "color", "#0000ff")

        # Create a DataFrame with only data from first iteration
        self.__df_first_it = self.__df[self.__df["iteration no."].isin([1])][:]

        self.__directory = os.path.join(gv.output_dir, results_folder)

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

    # pylint: disable=no-self-use
    def write_exclusions_info(self, gv: GlobalVars):
        """Write list of exclusions onto output png."""

        if len(gv.exclusions):
            excl_msg = "Exclusions: "
            for key, val in gv.exclusions.items():
                for item in val:
                    excl_msg += key + " " + item[0] + " " + item[1] + ", "

            excl_msg = excl_msg[0:-2]

            plt.figtext(0, -0.08, excl_msg, horizontalalignment="left", fontsize="x-small")

    # pylint: disable=too-many-arguments
    # pylint: disable=too-many-locals
    # pylint: disable=too-many-branches
    # pylint: disable=too-many-statements
    def make_scatter_plot(
            self,
            comp: str,
            plot_type: ScatterPlotType,
            legend_column: str,
            first_it_only: bool,
            gv: GlobalVars
    ):
        """Create a scatter plot displaying actual [comp] vs. either predicted [comp] or error.

        :param comp: The component (cost, delay, or congestion).
        :type comp: str
        :param plot_type: PREDICTION or ERROR (vs. actual [comp]).
        :type plot_type: ScatterPlotType
        :param legend_column: The column to use to color the plot.
        :type legend_column: str
        :param first_it_only: Whether to create a graph only using data from first iteration.
        :type first_it_only: bool
        :param gv:
        :type gv: GlobalVars
        """

        if (
                (not gv.first_it_output and first_it_only) or
                (not gv.all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column name are valid
        check_valid_component(comp)
        check_valid_column(legend_column, gv)

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

        if gv.no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
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
        _, ax = plt.subplots()
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

        self.write_exclusions_info(gv)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if gv.should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    def make_standard_scatter_plots(self, test_name_plot: bool, gv: GlobalVars):
        """
        Make all scatter plots in self.__standard_scatter_columns

        :param test_name_plot: whether to create plots wherein legend is by test name. This option
        is useful when creating graphs for an entire benchmark, but not specific circuits (tests).
        :type test_name_plot: bool
        :param gv:
        :type gv: GlobalVars
        """

        scatter_types = [ScatterPlotType.PREDICTION, ScatterPlotType.ERROR]

        if test_name_plot:
            legend_columns = self.__standard_scatter_columns
        else:
            legend_columns = self.__standard_scatter_columns[0:-1]

        for comp in gv.components:
            for plot_type in scatter_types:
                for col in legend_columns:
                    for first_it in [True, False]:
                        if first_it and col == "iteration no.":
                            continue

                        gv.processes.append((self.make_scatter_plot, (comp,
                                                                      plot_type,
                                                                      col,
                                                                      first_it,
                                                                      gv)))

    # pylint: disable=too-many-locals
    def make_bar_graph(self,
                       comp: str,
                       column: str,
                       first_it_only: bool,
                       use_absolute: bool,
                       gv: GlobalVars):
        """
        Create a bar graph displaying average error.

        :param comp: The component (cost, delay, or congestion).
        :type comp: str
        :param column: The column to use to split data (i.e. create each bar).
        :type column: str
        :param first_it_only: Whether to create a graph only using data from first iteration.
        :type first_it_only: bool
        :param use_absolute: Whether to produce a graph using absolute error, as opposed to signed
        error.
        :type use_absolute: bool
        :param gv:
        :type gv: GlobalVars
        """

        if (
                (not gv.absolute_output and use_absolute)
                or (not gv.signed_output and not use_absolute)
        ):
            return

        if (
                (not gv.first_it_output and first_it_only)
                or (not gv.all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column name are valid
        check_valid_component(comp)
        check_valid_column(column, gv)

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

        if gv.no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
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

        self.write_exclusions_info(gv)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if gv.should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    def make_standard_bar_graphs(self, test_name_plot: bool, gv: GlobalVars):
        """
        Make all bar graphs in self.__standard_bar_columns

        :param test_name_plot: Whether to create graph where data is split by test name. This option
        is useful when creating graphs for an entire benchmark, but not specific circuits (tests).
        :type test_name_plot: bool
        :param gv:
        :type gv: GlobalVars
        """

        if test_name_plot:
            columns = self.__standard_bar_columns
        else:
            columns = self.__standard_bar_columns[0:-1]

        for comp in gv.components:
            for col in columns:
                for use_abs in [True, False]:
                    for first_it in [True, False]:
                        gv.processes.append((self.make_bar_graph, (comp,
                                                                   col,
                                                                   use_abs,
                                                                   first_it,
                                                                   gv)))

    # pylint: disable=too-many-locals
    def make_heatmap(
            self,
            comp: str,
            x_column: str,
            y_column: str,
            first_it_only: bool,
            use_absolute: bool,
            gv: GlobalVars
    ):
        """
        Create a heatmap comparing two quantitative columns.

        :param comp: The component (cost, delay, or congestion).
        :type comp: str
        :param x_column: The column to be used for the x-axis.
        :type x_column: str
        :param y_column: The column to be used for the y-axis.
        :type y_column: str
        :param first_it_only: Whether to create a graph only using data from first iteration.
        :type first_it_only: bool
        :param use_absolute: Whether to produce a graph using absolute error, as opposed to signed
        error.
        :type use_absolute: bool
        :param gv:
        :type gv: GlobalVars
        """

        # pylint: disable=too-many-boolean-expressions
        if (
                (not gv.absolute_output and use_absolute)
                or (not gv.signed_output and not use_absolute)
        ):
            return

        if (
                (not gv.first_it_output and first_it_only)
                or (not gv.all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column names are valid
        check_valid_component(comp)
        check_valid_column(x_column, gv)
        check_valid_column(y_column, gv)

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

        if gv.no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
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

        self.write_exclusions_info(gv)
        ax.set_title(title, y=1.08)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if gv.should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep="")

    def make_standard_heatmaps(self, gv: GlobalVars):
        """Make "standard" heatmaps: (sink cluster tile width and sink cluster tile height) and
        (delta_x and delta_y)
        """

        for comp in gv.components:
            for first_it in [True, False]:
                for use_abs in [True, False]:
                    gv.processes.append((self.make_heatmap, (comp,
                                                             "sink cluster tile width",
                                                             "sink cluster tile height",
                                                             first_it,
                                                             use_abs,
                                                             gv)))
                    gv.processes.append((self.make_heatmap, (comp,
                                                             "delta x",
                                                             "delta y",
                                                             first_it,
                                                             use_abs,
                                                             gv)))

    # pylint: disable=too-many-locals
    def make_pie_chart(
            self, comp: str, column: str, first_it_only: bool, weighted: bool, gv: GlobalVars
    ):
        """
        Create a pie chart showing the proportion of cases where error is under
        percent_error_threshold.

        :param comp: The component (cost, delay, or congestion).
        :type comp: str
        :param column: The column to split data by.
        :type column: str
        :param first_it_only: Whether to create a graph only using data from first iteration.
        :type first_it_only: bool
        :param weighted: Whether to weight each section of the pie graph by the % error of that
        section.
        :type weighted: bool
        :param gv:
        :type gv: GlobalVars
        """
        if (
                (not gv.first_it_output and first_it_only) or
                (not gv.all_it_output and not first_it_only)
        ):
            return

        # Check that the component and column names are valid
        check_valid_component(comp)
        check_valid_column(column, gv)

        # Determine the graph title, directory, and file name
        title = f"Num. Mispredicts Under {gv.percent_error_threshold:.1f}% {comp} Error"

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

        if gv.no_replace and os.path.exists(os.path.join(curr_dir, file_name)):
            return

        make_dir(curr_dir, False)

        # Constrict DF to columns whose error is under threshold
        curr_df = curr_df[curr_df[f"{comp} % error"] < gv.percent_error_threshold]

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

        self.write_exclusions_info(gv)
        plt.title(title, fontsize=8)
        plt.legend(loc="upper left", bbox_to_anchor=(1.08, 1), title=column)
        plt.savefig(os.path.join(curr_dir, file_name), dpi=300, bbox_inches="tight")
        plt.close()

        if gv.should_print:
            print("Created ", os.path.join(curr_dir, file_name), sep='')

    def make_standard_pie_charts(self, test_name_plot: bool, gv: GlobalVars):
        """
        Make all pie chars in self.__standard_bar_columns.

        :param test_name_plot: Whether to create graph where data is split by test name. This option
        is useful when creating graphs for an entire benchmark, but not specific circuits(tests).
        :type test_name_plot: bool
        :param gv:
        :type gv: GlobalVars
        """

        if test_name_plot:
            columns = self.__standard_bar_columns
        else:
            columns = self.__standard_bar_columns[:-1]

        for comp in gv.components:
            for col in columns:
                for first_it in [True, False]:
                    for weighted in [True, False]:
                        gv.processes.append((self.make_pie_chart, (comp,
                                                                   col,
                                                                   first_it,
                                                                   weighted,
                                                                   gv)))

    def make_standard_plots(self, test_name_plot: bool, gv: GlobalVars):
        """
        Make "standard" graphs of all types

        :param test_name_plot: Whether to create plots where data is split by test name. This option
        is useful when creating plots for an entire benchmark, but not specific circuits (tests).
        :type test_name_plot: bool
        :param gv:
        :type gv: GlobalVars
        """

        if "pie" in gv.graph_types:
            self.make_standard_pie_charts(test_name_plot, gv)

        if "heatmap" in gv.graph_types:
            self.make_standard_heatmaps(gv)

        if "bar" in gv.graph_types:
            self.make_standard_bar_graphs(test_name_plot, gv)

        if "scatter" in gv.graph_types:
            self.make_standard_scatter_plots(test_name_plot, gv)


def print_and_write(string, directory: str, gv: GlobalVars):
    """Print to terminal and write to lookahead_stats_summary.txt."""

    if gv.should_print:
        print(string, sep="")

    orig_out = sys.stdout
    stats_file = open(os.path.join(directory, "lookahead_stats_summary.txt"), "a")
    sys.stdout = stats_file

    print(string, sep="")

    sys.stdout = orig_out


def print_stats(df: pd.DataFrame, comp: str, directory: str, gv: GlobalVars):
    """
    Print stats for a given component.

    :param df: The DataFrame with all info.
    :type df: pd.DataFrame
    :param comp: cost, delay, or congestion.
    :type comp: str
    :param directory: The output directory.
    :type directory: str
    :param gv:
    :type gv: GlobalVars
    """

    check_valid_component(comp)

    print_and_write("\n#### BACKWARDS PATH " + comp.upper() + " ####", directory, gv)
    print_and_write("Mean error: " + str(df[f"{comp} error"].mean()), directory, gv)
    print_and_write("Mean absolute error: " + str(df[f"{comp} absolute error"].mean()),
                    directory,
                    gv)
    print_and_write("Median error: " + str(df[f"{comp} error"].median()), directory, gv)
    print_and_write("Highest error: " + str(df[f"{comp} error"].max()), directory, gv)
    print_and_write("Lowest error: " + str(df[f"{comp} error"].min()), directory, gv)
    print_and_write(
        "Mean squared error: "
        + str(mean_squared_error(df[f"actual {comp}"], df[f"predicted {comp}"])),
        directory,
        gv
    )


def print_df_info(df: pd.DataFrame, directory: str, gv: GlobalVars):
    """
    Write out stats for all components.

    :param df: The DataFrame with all info.
    :type df: pd.DataFrame
    :param directory: The output directory.
    :type directory: str
    :param gv:
    :type gv: GlobalVars
    """

    print_and_write(df, directory, gv)

    for comp in gv.components:
        print_stats(df, comp, directory, gv)


def record_df_info(df: pd.DataFrame, directory: str, gv: GlobalVars):
    """
    Write out stats for the first iteration only, and for all iterations, as well as the csv_file
    containing the entire DataFrame.

    :param df: The DataFrame with all info.
    :type df: pd.DataFrame
    :param directory: The output directory.
    :type directory: str
    :param gv:
    :type gv: GlobalVars
    """

    check_valid_df(df, gv)

    df_first_iteration = df[df["iteration no."].isin([1])][:]

    print_and_write(
        "\nFIRST ITERATION RESULTS "
        "----------------------------------------------------------------------\n",
        directory,
        gv
    )

    print_df_info(df_first_iteration, directory, gv)

    print_and_write(
        "\nALL ITERATIONS RESULTS "
        "-----------------------------------------------------------------------\n",
        directory,
        gv
    )

    print_df_info(df, directory, gv)

    print_and_write(
        "\n-------------------------------------------------------------------------------------\n",
        directory,
        gv
    )

    def make_csv(df_out: pd.DataFrame, file_name: str):
        df_out.to_csv(file_name, index=False)

    # Write out the csv
    if (
            gv.csv_data and
            (not os.path.exists(os.path.join(directory, "data.csv")) or not gv.no_replace)
    ):
        gv.processes.append((make_csv, (df, os.path.join(directory, "data.csv"))))

        if gv.should_print:
            print("Created ", os.path.join(directory, "data.csv"), sep="")


def create_error_columns(df: pd.DataFrame):
    """Calculate and insert error columns from given csv file"""

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


# pylint: disable=too-many-locals
# pylint: disable=too-many-branches
# pylint: disable=too-many-statements
def parse_args():
    """Parse arguments and load global variables"""

    description = (
        "Parses data from lookahead_verifier_info.csv file(s) generated when running VPR with "
        "--router_lookahead_profiler on. Create one or more csv files (ideally, one per "
        "benchmark with, in the first column, the circuit/test name, and in the second column, "
        "the corresponding path to its lookahead info csv file. Give it a unique name "
        "(ideally, the benchmark name), and pass it in as csv_files_list."
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
        help="produce results investigating this backwards-path component (cost, delay, "
             "congestion, all)",
        choices=["cost", "delay", "congestion", "all"],
    )

    parser.add_argument(
        "--absolute", action="store_true", help="create graphs using absolute error"
    )

    parser.add_argument(
        "--signed", action="store_true", help="create graphs graphs using signed error"
    )

    parser.add_argument(
        "--first-it",
        action="store_true",
        help="produce results using data from first iteration"
    )

    parser.add_argument(
        "--all-it",
        action="store_true",
        help="produce results using data from all iterations"
    )

    parser.add_argument(
        "--csv-data", action="store_true", help="create csv file with all data"
    )

    parser.add_argument(
        "--all",
        action="store_true",
        help="create all graphs, regardless of other options",
    )

    parser.add_argument("--exclude", type=str, default="", metavar="EXCLUSION", nargs="+",
                        help="exclude cells that meet a given condition, in the form \"column_name "
                             "[comparison operator] value\"")

    parser.add_argument(
        "--collect",
        type=str,
        default="",
        metavar="FOLDER_NAME",
        nargs=1,
        help="instead of producing results from individual input csv files, collect data from all "
             "input files, and produce results for this collection (note: FOLDER_NAME should not "
             "be a path)",
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
                "Warning: no graphs will be produced. Use -g to select graph types (bar, scatter, "
                "heatmap, pie, all).")

        components = args.components
        if "all" in components:
            components = ["cost", "delay", "congestion"]

        if len(components) == 0:
            print("Warning: no graphs will be produced. Use -c to select components (cost, delay, "
                  "congestion, all).")

        absolute_output = args.absolute
        signed_output = args.signed

        if not absolute_output and not signed_output:
            print("Warning: no graphs will be produced. Use --signed and/or --absolute.")

        first_it_output = args.first_it
        all_it_output = args.all_it

        if not first_it_output and not all_it_output:
            print("Warning: no graphs will be produced. Use --first_it and/or --all_it.")

    gv = GlobalVars(
        graph_types,
        components,
        absolute_output,
        signed_output,
        args.csv_data,
        first_it_output,
        all_it_output,
        not args.replace,
        args.print,
        args.threshold,
        {},
    )

    for excl in args.exclude:
        operators = ["==", "!=", ">=", "<=", ">", "<"]
        key, val, op = "", "", ""
        for op_check in operators:
            if op_check in excl:
                key, val = excl.split(op_check)
                op = op_check
                break

        key, val = key.strip(), val.strip()

        if len(key) == 0 or len(val) == 0 or key not in gv.column_names:
            print("Invalid entry in --exclusions:", excl)
            sys.exit()

        if key not in gv.exclusions:
            gv.exclusions[key] = []

        try:
            int(val)
            gv.exclusions[key].append((op, val))
        except ValueError:
            gv.exclusions[key].append((op, "\"" + val + "\""))

    return args, gv


def create_complete_outputs(
        args, df_complete: pd.DataFrame, gv: GlobalVars
):
    """Where applicable, create output files (summary, plots) for all given data"""

    results_folder = args.collect[0]
    results_folder_path = os.path.join(gv.output_dir, results_folder)
    if len(args.dir_app) > 0:
        results_folder_path += f"{args.dir_app[0]}"

    make_dir(results_folder_path, False)

    df_complete = df_complete.reset_index(drop=True)

    record_df_info(df_complete, results_folder_path, gv)

    global_plots = Graphs(df_complete, os.path.join(results_folder, "plots"), "All Tests", gv)
    global_plots.make_standard_plots(True, gv)


# pylint: disable=too-many-arguments
def create_benchmark_outputs(
        args,
        df_benchmark: pd.DataFrame,
        df_complete: pd.DataFrame,
        gv: GlobalVars,
        output_folder: str,
        tests: list
):
    """Where applicable, create output files (summary, plots) for a given benchmark"""

    if len(tests) == 1:
        return df_complete

    results_folder = os.path.join(output_folder, "__all__")
    results_folder_path = os.path.join(gv.output_dir, results_folder)
    make_dir(results_folder_path, False)

    df_benchmark = df_benchmark.reset_index(drop=True)

    if args.collect != "":
        df_complete = pd.concat([df_complete, df_benchmark])
        return df_complete

    print_and_write("Global results:\n", results_folder_path, gv)
    record_df_info(df_benchmark, results_folder_path, gv)

    global_plots = Graphs(
        df_benchmark,
        os.path.join(results_folder, "plots"),
        f"All Tests ({output_folder})",
        gv
    )
    global_plots.make_standard_plots(True, gv)

    return df_complete


# pylint disable=too-many-arguments
def create_circuit_outputs(
        args,
        df_benchmark: pd.DataFrame,
        file_path: str,
        gv: GlobalVars,
        output_folder: str,
        test_name: str
):
    """Where applicable, create output files (summary, plots) for a given circuit (test)"""

    if not os.path.isfile(file_path):
        print("ERROR:", file_path, "could not be found")
        return df_benchmark

    if gv.should_print:
        print("Reading data at " + file_path)

    results_folder = os.path.join(output_folder, test_name)
    results_folder_path = os.path.join(gv.output_dir, results_folder)
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
    for key, val in gv.exclusions.items():
        for item in val:
            ldict = {}
            code_line = (
                f"df = df.drop(index=df[df[\"{key}\"] {item[0]} {item[1]}]"
                ".index.values.tolist())"
            )
            # pylint: disable=exec-used
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
        return df_benchmark

    # Write out stats summary and csv file
    print_and_write("Test: " + test_name, results_folder_path, gv)
    record_df_info(df, results_folder_path, gv)

    # Create plots
    curr_plots = Graphs(df, os.path.join(results_folder, "plots"), test_name, gv)
    curr_plots.make_standard_plots(False, gv)

    if gv.should_print:
        print(
            "\n----------------------------------------------------------------------------"
        )

    return df_benchmark


# pylint: disable=missing-function-docstring
def main():
    args, gv = parse_args()

    make_dir(gv.output_dir, False)

    # The DF containing info across all given csv files
    df_complete = pd.DataFrame(columns=gv.column_names)

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

        # The DF containing all info for this benchmark
        df_benchmark = pd.DataFrame(columns=gv.column_names)

        for test_name, file_path in tests:
            # Create output files for this circuit/test
            df_benchmark = create_circuit_outputs(args,
                                                  df_benchmark,
                                                  file_path,
                                                  gv,
                                                  output_folder,
                                                  test_name)

        # Create output files for entire benchmark
        df_complete = create_benchmark_outputs(args,
                                               df_benchmark,
                                               df_complete,
                                               gv,
                                               output_folder,
                                               tests)

    # If --collect used, create output files for all csv files provided
    if args.collect != "":
        create_complete_outputs(args, df_complete, gv)

    # Create output graphs simultaneously
    # This is the best way I have found to do this that avoids having a while
    # loop continually checking if any processes have finished while using an
    # entire CPU. Pool, concurrent.futures.ProcessPoolExecutor, and
    # concurrent.futures.ThreadPoolExecutor seem to have a lot of overhead.
    # This loop assumes that the graph-creating functions take approximately
    # the same amount of time, which is not the case.
    q = deque()
    for func, params in gv.processes:
        while len(q) >= args.j:
            q.popleft().join()

        proc = Process(target=func, args=params)
        proc.start()
        q.append(proc)


if __name__ == "__main__":
    main()
