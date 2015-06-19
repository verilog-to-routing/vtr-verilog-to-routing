"""
this is to plot graphs based on the db output:
db should be like this:
     task | run | arch | benchmark | settings(fc, wl, sb, ...) | measuremnts(delay, min_cw, area ...)

take as input: 
    the user filtered table

prompt user input:
    the overlay axis, and the choice of geometric mean

input data from the database: 
    in the form of [(a,b,c), (a,b,c), ...]: a list of tuples
"""

import numpy as np
import matplotlib.pyplot as plt
import collections
import operator
import interface_db as idb
import os
import re
import sqlite3

"""
    to convert the input table (list of tuples) to the high dimensional array
    x: 1st attribute name: string
    y: attribute name starting at 2nd place: list of string / string(if y is only 1)
    rest: filters name: list of string
    return the object of data_collection
"""
err_msg = {"choose axis": "**** wrong name, input again ****",
           "choose method": "**** wrong method, input again ****",
           "overlay axis": "**** wrong axis selection, input again ****",
           "yes or no": "**** wrong input, please select \'y\' or \'n\' ****",
           "choose plot to show": "**** plot_generator: wrong input mode or don't have gmean yet ****",
           "choose overlay type": "**** wrong input for overlay type ****"}

def data_converter(data_list, x_name, y_name_list, filter_name_list):
    # just for convenience, support pass in y_name_list as single string or list
    if type(y_name_list) == type("str"):
        y_name_list = [y_name_list]
    if type(filter_name_list) == type("str"):
        filter_name_list = [filter_name_list]
    xy_name_map = ([item for sub in [[x_name], y_name_list] for item in sub],\
                    list(set([tup[0] for tup in data_list])))
    axis_name_supmap = {k:[] for k in filter_name_list}
    for i in range(len(filter_name_list)):
        axis_name_supmap[filter_name_list[i]] = list(set([tup[i+1+len(y_name_list)] for tup in data_list]))
    # initialize the big y array
    axis_len = [len(l) for (k, l) in axis_name_supmap.items()]
    axis_len.append(len(xy_name_map[1]))
    y_split_list = []
    for i in range(len(y_name_list)):
        sub = np.ndarray(shape=axis_len, dtype=float)
        sub.fill(-1)
        y_split_list.append(sub)
    # fill in the data
    for i in range(len(data_list)):
        array_index = {k: -1 for k in filter_name_list}
        # setup axis index for all filter axis
        for filt in filter_name_list:
            array_index[filt] = axis_name_supmap[filt].\
                    index(data_list[i][filter_name_list.index(filt)+1+len(y_name_list)])
        # append x index
        index_list = array_index.values()
        index_list.append(xy_name_map[1].index(data_list[i][0]))
        # fill in y data
        for y_i in range(len(y_name_list)):
            y_split_list[y_i][tuple(index_list)] = data_list[i][y_i+1]
    return Data_Collection(axis_name_supmap, xy_name_map, y_split_list)

"""
    holds all data and meta info needed for plotting and user interaction
"""
class Data_Collection:
    y_split_list = []
    y_gmean_list = []
    # a list of lists of axis name
    axis_name_supmap = None
    # separate the x axis from the axis_name_supmap
    xy_name_map = None
    # x axis is always the lowest dimension
    # this cost is added to the axis_pos to further order the axis by hierarchy,
    # the axes chosen by the second level filetering will have value 1, otherwise they will
    # have value 0. And after adding the cost (which should be less than 1), we further assign
    # hierarchy to filtered axes and the unfiltered axes.
    axis_split_cost = None
    axis_gmean_cost = None
    # to store the transposed axis order --> used by the restore function
    axis_cur_split_order = None
    axis_cur_gmean_order = None

    # fill in the data into the high dimensional x & y, from the input table
    def __init__(self, axis_name_supmap, xy_name_map, y_split_list):
        self.axis_name_supmap = axis_name_supmap
        self.xy_name_map = xy_name_map
        self.axis_split_cost = {k: 0 for k in self.axis_name_supmap.keys()}
        # self.axis_gmean_cost is set after gmean axis is chosen
        self.axis_cur_split_order = self.axis_name_supmap.keys()
        self.y_split_list = y_split_list

    """
        ask user to choose overlay axes parameters
        overlay_axis is a list of string
        y_type is to choose between y_split_list & y_gmean_list
        y_type = "gmean" / "split"
    """
    def transpose_overlay_axes(self, overlay_axis, y_type="split"):
        if y_type == "gmean" and self.axis_cur_gmean_order == []:
            print "**** CANNOT FILTER ON GMEAN YET. AXIS NOT SET ****"
        axis_cost_temp = (y_type == "gmean" and [self.axis_gmean_cost] or [self.axis_split_cost])[0]
        axis_cost_temp = {k: (v + (k in overlay_axis)) for (k,v) in axis_cost_temp.items()}
        # NOTE: now axis_split_cost is converted from dict to list of tuples, so that it is ordered by the cost value.
        # now set up y_split_list
        axis_cost_temp = sorted(axis_cost_temp.items(), key=operator.itemgetter(1))
        # save the transposed axes order, to facilitate the reverse transpose
        trans_order = [(y_type == "gmean" and self.axis_cur_gmean_order \
                       or self.axis_cur_split_order).index(v[0]) for v in axis_cost_temp]
        trans_order.append(len(axis_cost_temp))
        # transpose the axes, based on the order of axis_split_cost
        for i in range(len(self.y_split_list)):
            if y_type == "gmean":
                self.y_gmean_list[i] = self.y_gmean_list[i].transpose(trans_order)
            elif y_type == "split":
                self.y_split_list[i] = self.y_split_list[i].transpose(trans_order)
        if y_type == "gmean":
            self.axis_cur_gmean_order = [ax[0] for ax in axis_cost_temp]
        else:
            self.axis_cur_split_order = [ax[0] for ax in axis_cost_temp]

    """
        merge the overlay axis, calculate the gmean.
    """
    def merge_on_gmean(self):
        self.y_gmean_list = self.y_split_list
        # switch benchmark axis to the last one, easier for gmean
        # we cannot simply do mult on axis = n-1, cuz there are invalid points
        self.y_gmean_list = [y.swapaxes(len(y.shape)-2, len(y.shape)-1) for y in self.y_gmean_list]
        for i in range(len(self.y_gmean_list)):
            # store the shape of the array after compressing the benchmark dimension
            product = []
            temp = self.y_gmean_list[i].reshape(-1, self.y_gmean_list[i].shape[-1])
            for k in range(temp.shape[0]):
                cur_root = len(temp[k]) - list(temp[k]).count(-1)
                cur_prod = reduce(lambda x,y: ((x != -1 and x-1)+1)*((y != -1 and y-1)+1), temp[k])
                # if all benchmark values are -1, then append -1, otherwise, append the gmean
                product.append((cur_root != 0 and [cur_prod**(1.0/cur_root)] or [-1])[0])
            self.y_gmean_list[i] = np.asarray(product).reshape(self.y_gmean_list[i][...,0].shape)
        # update the axis_cur_gmean_order & axis_gmean_cost
        self.axis_cur_gmean_order = [ax for ax in self.axis_cur_split_order \
                                     if ax not in [self.axis_cur_split_order[len(self.axis_cur_split_order)-1]]]
        self.axis_gmean_cost = {k: self.axis_split_cost[k] for k in self.axis_cur_gmean_order}

"""
    actual plotter functions: show window, do subplots
"""
class UI:
    def subplot_traverser(self, y_sub_list, namemap, overlay_axis_left, xy_namemap, y_i, legend, plot_type="plot"):
        if overlay_axis_left == []:
            x = xy_namemap[1]
            y = y_sub_list
            # sort x
            dic = {x[i]: y[i] for i in range(len(x))}
            dic = collections.OrderedDict(sorted(dic.items()))
            x = dic.keys()
            y = dic.values()
            y = [(k == -1 and [None] or [k])[0] for k in y]
            y = np.array(y).astype(np.double)
            y_mask = np.isfinite(y)
            if x != []:
                if plot_type == "plot":
                    if type(x[0]) == type("str"):
                        plt.plot(np.array(range(len(x)))[y_mask], y[y_mask], 'o--', label=legend)
                        plt.xticks(range(len(x)), x)
                    else:
                        plt.plot(np.array(x)[y_mask], y[y_mask], 'o--', label=legend)
                    plt.xlabel(xy_namemap[0][0], fontsize = 12)
                    plt.ylabel(xy_namemap[0][1+y_i], fontsize = 12)
                    if legend != "":
                        plt.legend(loc="lower right")
        else:
            cur_filter_axis = overlay_axis_left[0]
            overlay_axis_left = [k for k in overlay_axis_left if k not in [cur_filter_axis]]
            for i in range(y_sub_list.shape[0]):
                legend_restore = legend
                legend += str(cur_filter_axis) + ": "
                legend += str(namemap[cur_filter_axis][i]) + "  "
                argu = overlay_axis_left
                self.subplot_traverser(y_sub_list[i], namemap, argu, xy_namemap, y_i, legend, plot_type)
                legend = legend_restore

    """
        plot_type should be universal among all figures created
        axis_left = [[plot_axis_left], [overlay_axis_left]]
        y_sublist should be just a ndarray, not a list of ndarray,
            i.e.: y_sublist = y_split_list[i]
        figure_name
    """
    # TODO: if some x,y series are all -1, then we should not create the figure
    def figure_traverser(self, y_sub_list, namemap, axis_left, xy_namemap, y_i, figure_name, plot_type="plot"): 
        if axis_left[0] == []:
            plt.figure(figure_name)
            self.subplot_traverser(y_sub_list, namemap, axis_left[1], xy_namemap, y_i, "", plot_type)
        else:
            cur_axis = axis_left[0][0]
            axis_left[0] = [k for k in axis_left[0] if k not in [cur_axis]]
            for i in range(y_sub_list.shape[0]):
                figure_name_restore = figure_name
                figure_name += str(namemap[cur_axis][i])
                # have to copy the list of list, or it will be like pass by reference
                temp1 = [tt for tt in axis_left[0]]
                temp2 = [tt for tt in axis_left[1]]
                argu = [temp1,temp2]
                self.figure_traverser(y_sub_list[i], namemap, argu, xy_namemap, y_i, figure_name, plot_type)
                figure_name = figure_name_restore

    """
        plot_type can be "plot" or "bar"
        y_list: store the values (y_split_list / y_gmean_list)
        namemap: store the values for different settings (axis_name_supmap)
        axis_order: to be used with name_map (axis_cur_gmean_order / axis_cur_split_order)
        filter_axis: (self.filter_axis_gmean / self.filter_axis_split)
        mode: is to select whether to plot gmean or split
    """
    def plot_generator(self, data_collection, axis_order, overlay_axis, mode, plot_type="plot"):
        namemap = data_collection.axis_name_supmap
        xy_namemap = data_collection.xy_name_map
        if mode == "split":
            y_list = data_collection.y_split_list
            overlay_axis = [k for k in data_collection.axis_cur_split_order if k in overlay_axis]
            axis_order = [k for k in data_collection.axis_cur_split_order if k in axis_order]
            for i in range(len(y_list)):
                self.figure_traverser(y_list[i], namemap, [axis_order, overlay_axis], xy_namemap, i, data_collection.xy_name_map[0][1+i], plot_type)
        elif mode == "gmean" and data_collection.y_gmean_list != []:
            y_list = data_collection.y_gmean_list
            overlay_axis = [k for k in data_collection.axis_cur_gmean_order if k in overlay_axis]
            axis_order = [k for k in data_collection.axis_cur_gmean_order if k in axis_order]
            for i in range(len(y_list)):
                self.figure_traverser(y_list[i], namemap, [axis_order, overlay_axis], xy_namemap, i, "gmean"+data_collection.xy_name_map[0][1+i], plot_type)
        else:
            print err_msg["choose plot to show"]
        plt.show()

"""
    connect the database to the plotter, setup the data_collection object for plotting
"""
# NOTE: not useful right now, cuz there is not enough data in the database yet
#       just ignore this db_connector function
filter_method = {'TEXT': 'IN',
                 'INTEGER': 'BETWEEN', 
                 'REAL': 'BETWEEN'}
def db_connector():
    tasks = idb.list_tasks()
    print "available tasks: ", len(tasks)
    task_num = int(raw_input("which task number to choose: "))
    available_choice = idb.describe_tasks([tasks[task_num]])
    available_name = [k.split()[0] for k in available_choice]
    available_type = [k.split()[1] for k in available_choice]
    print "==========================================================="
    print "available choices:"
    print "\n".join(i for i in available_choice)
    print "==========================================================="
    while 1:
        x = raw_input("choose a x axis name: ")
        if x in available_name:
            break
        print err_msg['choose axis']
    while 1:
        y = raw_input("choose a y axis name: ")
        if y in available_name:
            break
        print err_msg['choose axis']
    filt_list = []
    filt_name_list = []
    cur_choice = None
    print "==========================================================="
    while 1:
        while 1:
            cur_choice = raw_input("choose filter name (enter empty string to exit): ")
            if (cur_choice in available_name) or (cur_choice == ""):
                break
            print err_msg['choose axis']
        if cur_choice == '':
            break
        filt_name_list.append(cur_choice)
        cur_type = available_type[available_name.index(cur_choice)]
        fname = cur_choice
        fmethod = filter_method[cur_type]
        param_range = idb.describe_param(cur_choice + ' ' + cur_type, tasks[task_num])
        print "available range: ", param_range
        frange = None
        if len(param_range) == 1:
            print "set range to: ", param_range
            frange = param_range
        else:
            cur_range = raw_input("choose range: in the form of \"attr1 attr2 ...\" ")
            choice_fields = cur_range.split()
            if fmethod == 'BETWEEN' and choice_fields != []:
                frange = (float(choice_fields[0]), float(choice_fields[1]))
            elif fmethod == 'IN' and choice_fields != []:
                frange = choice_fields
            elif choice_fields == []:
                print "set range to: ", param_range
                frange = param_range
            else:
                print err_msg['choose method']
        filt_list.append(idb.Task_filter(fname, fmethod, frange))
    data = idb.retrieve_data(x, y, filt_list, tasks[task_num])
    return {"data": data, "filt_name_list": filt_name_list, "x": x, "y": y}


# NOTE: change the following dict to your own version

keyword_list = {'total_wl': 'Total wirelength',
                'delay_cp': 'Final critical path:',
                'area_plt': 'per logic tile'}
regexp_list = {'total_wl': "\d+",
               'delay_cp': "\d+.\d+",
               'area_plt': "\d+.\d+"}
posit_list = {'total_wl': 0,
              'delay_cp': 0,
              'area_plt': 1}

"""
    this function is only for the sb experiment (cuz the database now don't have enough data):
    to work with the output of the database, use the db_connector
"""
# NOTE: you should change this function to fetch your own data
def sb_exp_data_fetcher():
    x = raw_input("what is your x: ")
    data = []
    # y is automatically set to "min_chan_width", "total_wl", "area_plt", "delay_cp"
    attr_list = ['blif', 'min_cw', 'sb', 'wl', 'fc', 'total_wl', 'area_plt', 'delay_cp']
    x_index = attr_list.index(x)
    dire = '/home/zenghq/workspace/summer/fpga/vpr-sb-benchmark-22nm/lo-stress'
    #conn = sqlite3.connect('results.db')
    #c = conn.cursor()
    #c.execute('''DROP TABLE IF EXISTS sb_exp_22nm''')
    #c.execute('''CREATE TABLE sb_exp_22nm (circuit TEXT, min_chan_width INEGER, switch_block TEXT, wire_length INTEGER, fc REAL, total_wire_length INTEGER, area REAL, critical_path_delay REAL)''')
    for f_name in os.listdir(dire):
        # the naming convension in this "lo-stress" folder is: <blif>_<min chan width>_<switch block>_<wire length>_<fc>
        param = f_name.split('_')
        if len(param) > 5:
            name = param[0] + param[1]
            temp = [n for n in param if n not in [param[0], param[1]]]
            param = [name] + temp
        # cast min_chan_width, wire_length, fc
        param[1] = float(param[1])
        param[3] = float(param[3])
        param[4] = float(param[4])
        # find total_wl, area_plt and delay_cp in the log file
        for line in open(dire + '/' + f_name, 'r'):
            for (k, v) in keyword_list.items():
                if v in line:
                    d = re.findall(regexp_list[k], line)[posit_list[k]]
                    param.append(float(d))
        # if the experiment is failed, then the param list will have length < 8 (cuz some metrics don't have value in the log file)
        if len(param) < 8:
            # invalidate some fields, and fill in invalid value
            param = [p for p in [param[0], param[1], param[2], param[3], param[4]]]
            param += [-1,-1,-1]
        #c.execute("INSERT INTO sb_exp_22nm VALUES (?, ?, ?, ?, ?, ?, ?, ?)", param)
        # reorder the sequence, always keeps this order: x, y1, y2, ..., yn, filt1, filt2, ..., filtm
        y_l = [param[1], param[5], param[6], param[7]]
        x_l = [param[x_index]]
        ax_l = [param[0], param[2], param[3], param[4]]
        ax_l = [a for a in ax_l if a not in x_l]
        param = x_l + y_l + ax_l
        data.append(tuple(param))
    # setup the name list corresponds to the param values:
    # NOTE: the sequence in the name list should agree with the sequence of the param tuple.
    # e.g.: if the param tuple is: (0.05, 112, 'wilton', 'bgm.blif')
    #       then x should be 'wl', y should be ['min cw'], and filt_name_list should be ['sb', 'arch']
    #       (of course you can choose a different name, like 'switch block' instead of 'sb')
    #conn.commit()
    #conn.close()
    y = ['min_cw', 'total_wl', 'area_plt', 'delay_cp']
    filt_name_list = [n for n in attr_list if n not in [x]+y]
    # the returned dict is to provide info for data_converter function
    return {"data": data, "filt_name_list": filt_name_list, "x": x, "y": y}


"""
    control board
"""
def main():
    #ret = db_connector()
    ret = sb_exp_data_fetcher()
    data = ret["data"]
    filt_name_list = ret["filt_name_list"]
    data_collection = data_converter(data, ret["x"], ret["y"], filt_name_list)

    print "########################################"
    print "---- Description: ----\n" + \
          ">>\n" + \
          "VPR benchmark experiment should have 2 types of data: \n" + \
          "parameter: settings in for the experiment (e.g.: fc, wire length, switch block ...)\n" + \
          "metrics: measurements from the VPR output (e.g.: min chan width, critical path delay ...)\n" + \
          ">>\n" + \
          "Data passed into this plotter should have already been classified into 3 axes: \n" + \
          "one [x] axis (chosen from parameter)\n" + \
          "multiple [y] axis (chosen from metrics)\n" + \
          "multiple [filter] axis (all the unchosen parameters)\n" + \
          ">>\n" + \
          "For example, if the experiment has: \n" + \
          "[arch, circuit, wire length, switch block, fc, min chan width, critical path delay, area, total wire length]\n" + \
          "and you choose fc as x axis, [min chan width, critical path delay, area, total wire length] as y axes,\n" + \
          "then filter axes are the unchosen parameters, i.e.: arch, circuit, wire length, switch block. "
    print "#########################################"
    print "---- Usage ----\n" + \
          ">>\n" + \
          "1. choose overlay axes among the filter axes (overlay axes will become legend in a single plot)\n" + \
          "2. choose whether to whether to calculate the geo mean over the overlay axis (\"merge\" function)\n" + \
          "   (Notice: you can choose as many overlay axes as you like, but when you choose merge, it will only\n" + \
          "    calculate the geo mean over the last overlay axis. So for example, if your overlay axes are [fc, circuit],\n" + \
          "    the merge will only get geo mean over all the circuits rather that all the (circuit,fc) combination, and \n" + \
          "    fc will still be overlaid in the merged plot.)\n" + \
          "3. the data after geo mean calcultion will be referred to as \"gmean\", and the data before the geo mean will be \n" + \
          "   referred to as \"split\", you can switch the overlay axes for both gmean data and split data, for as many times \n" + \
          "   as you like. But once you \"merge\" on a new axis, the old gmean data will be replaced by the new one, and further\n" + \
          "   operation will be acted on only the new gmean data."
    while 1:
        print ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
        print "available axis to overlay: "
        print "for the split data", data_collection.axis_cur_split_order
        print "for the gmean data", data_collection.axis_cur_gmean_order
        overlay_type = None
        overlay_axis = []
        if data_collection.y_gmean_list != []:
            while 1:
                overlay_type = raw_input("which array to overlay (gmean / split): ")
                if overlay_type == "gmean":
                    break
                elif overlay_type == "split":
                    break
                else:
                    print err_msg["choose overlay type"]
        else:
            overlay_type = "split"
        while 1:
            overlay_axis = raw_input("which axes to overlay: (input separated by space): ")
            if overlay_axis != '':
                overlay_axis = overlay_axis.split()
                if reduce(lambda x,y: (x in data_collection.axis_cur_split_order)*(y in data_collection.axis_cur_split_order), overlay_axis) and overlay_type == "split":
                    break
                elif reduce(lambda x,y: (x in data_collection.axis_cur_gmean_order)*(y in data_collection.axis_cur_gmean_order), overlay_axis) and overlay_type == "gmean":
                    break
                else:
                    print err_msg["overlay axis"]
               
        data_collection.transpose_overlay_axes(overlay_axis, overlay_type)
        overlay_merge = 0
        if overlay_type == "split":
            while 1:
                choice = raw_input("merge the lowest axis (y/n)? ")
                if choice == 'y':
                    overlay_merge = 1
                    data_collection.merge_on_gmean()
                    break
                elif choice == 'n':
                    overlay_merge = 0
                    break
                else:
                    print err_msg['yes or no']
        ui = UI()
        if overlay_type == "split":
            axis_left = [k for k in data_collection.axis_cur_split_order if k not in overlay_axis]
        else:
            axis_left = [k for k in data_collection.axis_cur_gmean_order if k not in overlay_axis]
        while 1:
            show_plot_type = raw_input("show plot (gmean / split) ")
            if show_plot_type == "gmean":
                if overlay_merge == 1:
                    overlay_axis = [k for k in overlay_axis if k not in [overlay_axis[-1]]]
                ui.plot_generator(data_collection, axis_left, overlay_axis, "gmean")
                break
            elif show_plot_type == "split":
                ui.plot_generator(data_collection, axis_left, overlay_axis, "split")
                break
            elif show_plot_type == '':
                break
            else:
                print err_msg["choose plot to show"]

if __name__ == '__main__':
    main()
