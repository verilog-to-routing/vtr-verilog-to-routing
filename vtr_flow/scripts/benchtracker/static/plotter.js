// TODO:
// 1. auto-resizing graph, change the axis range
// 2. set axis range, set title
// 4. show coordinate
// 6. fix the offline plotter

// Error msg:
var maxPlotsAllowed = 20;
errMsg = {ENUMPLOTS: 'More than'+maxPlotsAllowed+' will be generated.\n'
                     +'Please consider adding filters or geometric mean / legend to '
                     +'reduce the number of plots!\n'
                     +'Current plotting aborted' };

// Some const
// if x axis is an element of timeParam, then the default behavior is to gmean everything
var timeParam = ['run', 'parsed_date'];
var unixEpoch = ['parsed_date'];
var defaultGmean = 'circuit';
// the value for invalid y. these will be eliminated by the plotter
var invalidY = -1;
// create the actual plot in the display_canvas
//
var overlay_list = [];
var gmean_list = [];
// raw data means it is not processed by gmean, and its param name has not been manipulated.
// NOTE: we don't store the data of default plot (cuz they are not manipulated)
var raw_data = null;
// store data after gmean
// each task has a folder, each plot has a subfolder, each line has a file (containing only x and y)
// {task: {plot: {line: [[x, y]]}}}
var manipulatedData = null;
var gmean_data = null;
var range = [];
// xNameMap is only set when value in x axis is string
var xNameMap = null;

var formGmean;
var formOverlay;

var first_plot = false;
// all margins are the margins between canvsvg and canvsvg -> rect
// not between the canvsvg and canvsvg -> g
var ratio = {canvToWholeWid: 1060/(1060+260),
             legToWholeWid: 240/(1060+260),
             wholeHeiToWid: 750/(1060+260),
             // margin.top / wholeHeight
             marginTopToWhole: 100/750,
             marginBotToWhole: 100/750,
             // margin.left / wholeWidth
             marginLefToWhole: 150/(1060+260),
             marginRigToWhole: 10/(1060+260)};
// display option
var numPlotsPerRow = 1;

// attach event listeners when ready
$(document).ready(function() {

var custom_panel = document.getElementById("custom_panel");

var customize_plot = document.getElementById("customizePlot");
customize_plot.addEventListener('click', function () {
    generate_overlay_selector();
}, false);

var save_plot = document.getElementById("savePlot");
save_plot.addEventListener('click', savePlot);
var save_plot_data = document.getElementById("savePlotData");
save_plot_data.addEventListener('click', savePlotData);

formGmean = document.getElementById("gmean_select").getElementsByTagName("fieldset")[0];
formOverlay = document.getElementById("overlay_select").getElementsByTagName("fieldset")[0];

var get_custom_plot = document.getElementById("get_custom_plot");
get_custom_plot.addEventListener('click', function () { 
    gmean_list = [];
    //var checkbox = formGmean.getElementsByTagName('input');
    var checkbox = d3.select('#gmean_select').selectAll('input');
    checkbox.each(function () {
                               if (d3.select(this).property('checked')) {
                                   gmean_list.push(d3.select(this).attr('index'));
                               }
                             });

    overlay_list = [];
    //var checkbox = formOverlay.getElementsByTagName('input');
    checkbox = d3.select('#overlay_select').selectAll('input');
    checkbox.each(function () {
                               if (d3.select(this).property('checked') 
                                && gmean_list.indexOf(d3.select(this).attr('index')) == -1) {
                                   overlay_list.push(d3.select(this).attr('index'));
                               }
                             });
    plot_generator();} 
);


});

/*
 * high level function after data is obtained from database interface
 * -- generate the default plots
 */
function plotter_setup(data) {
    if (data.status !== 'OK') {
        report_error(data.status);
        return;
    }
    console.log('--- data ---');
    console.log(data);
    // initialize
    raw_data = data;
    raw_data.tasks = _.map(raw_data.tasks, function(d){return d.split('|')[0]; });
    gmean_data = null;
    overlay_list = [];
    gmean_list = [];
    range = [];
    xNameMap = [];
    document.getElementById('customizePlot').style.visibility = 'visible';
    document.getElementById('savePlot').style.visibility = 'visible';
    document.getElementById('savePlotData').style.visibility = 'visible';
    d3.select('#generate_plot').html('');
    d3.select('#generate_plot').text('Regenerate Plots');
    d3.select('#chart').html('');
    // clear up
    remove_inputs(formGmean);
    remove_inputs(formOverlay);
    document.getElementById('custom_panel').style.visibility = 'hidden';
    // convert x value to numerical
    indexXString();
    // check is x is run / parsed_date
    // if yes, then geomean on everything
    // if not, then supress on params that will generate the least subplots
    range = [];
    manipulatedData = {};
    for (var i = 0; i < raw_data.data.length; i++) {
        range.push({x: [], y: []});
        range[i]['x'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        range[i]['y'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        manipulatedData[raw_data.tasks[i]] = {};
    }
    if (timeParam.indexOf(raw_data.params[0]) !== -1) {
        // geomean on everything. 
        defaultToGmeanTimePlot();
        // possibly show the generate_overlay_selector as well
    } else {
        // choose the axis with the most distinct values and overlay on it
        defaultToGmeanSubPlot();
    }
}
/*
 * generate the default plot if x axis is time
 * methodology: gmean over everything
 */
function defaultToGmeanTimePlot() {
    first_plot = true;
    d3.select('#chart').html('');
    // flattenedData = _.flatten(raw_data, true);
    for (var i = 0; i < raw_data.data.length; i++) {
        groupedX = _.groupBy(raw_data.data[i], function (dotSeries) {return dotSeries[0]} );
        seriesXY = [];
        for (var k in groupedX) {
            groupedX[k] = reduceToGmean(groupedX[k]);
            // now {x1: y1, x2: y2, ...}
            // should convert it [[x1,y1],[x2,y2],...]
            seriesXY.push([Number(k), groupedX[k], raw_data.tasks[i]]);
        }
        findAxisScale(seriesXY, xNameMap[i], i);
        d3.select('#chart').append('div').attr('class', 'task_container').attr('id', 'task_'+i);
        simple_plot(raw_data.params, seriesXY, [], xNameMap[i], i, 'taskTitle');
        sizingTaskContainer(1, 'task_'+i);
    }

}
/*
 * generate the default plot when x is not time
 * methodology: gmean over circuits, overlay on the filter which has the most distinct values
 */
function defaultToGmeanSubPlot() {
    first_plot = true;
    // check if circuit is in the filter: 
    // it should be, cuz it is the primary key
    var gmeanIndex = raw_data.params.indexOf(defaultGmean);
    if (gmeanIndex == -1) {
        alert('circuit (primary key) is not in the param list\nskip default plot');
        generate_overlay_selector();
    } else if (gmeanIndex == 0 || gmeanIndex == 1) {
        alert('circuit is in x y axis: \nskip default plot');
        generate_overlay_selector();
    } else {
        for (var k in raw_data.data) {
            var data = [];
            gmeanPrepData = data_transform(raw_data.data[k], [(gmeanIndex-2)+''], 'non-overlay');
            for (var p in gmeanPrepData) {
                // now gmeanPrepData[p] is a subplot overlaying on circuit
                // next: gruop on x and gmean on y
                var groupedX = data_transform(gmeanPrepData[p], [(0-2)+''], 'xy group');
                var seriesXY = [];
                for (var l in groupedX) {
                    groupedX[l] = reduceToGmean(groupedX[l]);
                    seriesXY.push([l, groupedX[l], p]);
                }
                findAxisScale(seriesXY, xNameMap[k], k);
                data.push(seriesXY);
            }
            var newRawData = [];
            var newParams = [];
            for (var i in raw_data.params) {
                if (raw_data.params[i] != defaultGmean ) {
                    newParams.push(raw_data.params[i]);
                }
            }
            for (var i in data) {
                //simple_plot(raw_data.params, data[i], []);
                // convert data again to support overlay again
                for (var j in data[i]) {
                    var paramSplit = data[i][j][2].split('  ');
                    var temp = [];
                    for (var s in paramSplit) {
                        if (s != 0) {
                            temp.push(paramSplit[s]);
                        }
                    }
                    paramSplit = temp;
                    //console.log(paramSplit);
                    data[i][j] = [data[i][j][0], data[i][j][1]].concat(paramSplit);
                    newRawData.push(data[i][j]);
                }
            }
            
            newOverlayList = [findMostExpensiveAxis(newRawData, newParams) + ''];
            var newData = data_transform(newRawData, newOverlayList, 'non-overlay');
            var c = d3.select('#chart').append('div').attr('class', 'task_container').attr('id', 'task_'+k);
            var t = c.append('h3').attr('class', 'task_title');
            t.append('span').attr('class', 'h_grey').append('text').text('Task Name: ');
            t.append('span').attr('class', 'h_dark').append('text').text(raw_data.tasks[k]);
            var s = c.append('h4').attr('class', 'task_title');
            s.append('span').attr('class', 'h_grey').append('text').text('Geo Mean Axis: ');
            s.append('span').attr('class', 'h_dark').append('text').text(defaultGmean);
            var count = 0;
            for (var j in newData) {
                count += 1;
            }
            if (count <= maxPlotsAllowed) {
                for (var j in newData) {
                    simple_plot(newParams, newData[j], newOverlayList, xNameMap[k], k, 'normalTitle');
                }
            } else {
                count = 0;
                alert(errMsg['ENUMPLOTS']);
            }
            sizingTaskContainer(count, 'task_'+k);
        }
    }
}
/*
 * reduce a list to a single gmean value
 */
function reduceToGmean(groupedX) {
    // groupedX: eliminate all other irrelevant values
    groupedX = _.map(groupedX, function (list) {return list[1];} );
    // count the number of tuples with the valid y value
    var validCount = 0;
    groupedX = _.reduce( groupedX, 
                            function (memo, num) {
                                if (num > 0) {
                                    memo *= num;
                                    validCount += 1;
                                }
                                return memo;
                            }, 
                            1);
    // setup the gmean entry for groupedX
    return (validCount > 0) ? Math.pow(groupedX, 1.0/validCount) : -1;

}

/*
 * to generate the overlay selector panel
 * the plot will be generated after button is clicked.
 *  type is to used to decide whether to generate axes of raw data or gmean data
 */
function remove_inputs(parent) {
    var inputs = parent.getElementsByTagName('input');
    var labels = parent.getElementsByTagName('label');
    var br = parent.getElementsByTagName('br');
    while (inputs.length) {
        inputs[0].parentNode.removeChild(inputs[0]);
        labels[0].parentNode.removeChild(labels[0]);
        br[0].parentNode.removeChild(br[0]);
    }
}
function generate_overlay_selector() {
    // clear up
    remove_inputs(formGmean);
    remove_inputs(formOverlay);
    d3.select('#chart').html('');
    var choice = raw_data.params;

    // choose gmean axis
    for (var i = 2; i < choice.length; i ++ ) {
        var input = document.createElement('input');
        var label = document.createElement('label');
        input.type = 'checkbox';
        label.textContent = input.value = choice[i];
        label.htmlFor = input.id = 'gip-' + choice[i];
        input.setAttribute('index', i - 2);
        input.addEventListener('change', function(){updateLegendCheckBox(this.id);});
        //var label = form.append('label').attr('class', 'param_label');
        formGmean.appendChild(input);
        formGmean.appendChild(label);
        formGmean.appendChild(document.createElement('br'));
        if (raw_data.params[0] == 'parsed_date' && choice[i] == 'run') {
            $('#gip-'+choice[i]).prop('checked', true);
        }

    }

    function updateLegendCheckBox(gid) {
        var oid = gid.substring(4, gid.length);
        if (d3.select('#'+gid).property('checked')) {
            $('#oip-'+oid).prop('checked', false);
            d3.select('#oip-'+oid).attr('disabled', 'on');
            d3.select('#olabel-'+oid).attr('style', 'color: rgb(188,188,188)');
        } else {
            d3.select('#oip-'+oid).attr('disabled', null);
            d3.select('#olabel-'+oid).attr('style', 'color: rgb(0,0,0)');
        }
    }
    // choose overlay axis
    for (var i = 2; i < choice.length; i ++ ) {
        //var label = form.append('label').attr('class', 'param_label');
        var input = document.createElement('input');
        var label = document.createElement('label');
        input.type = 'checkbox';
        label.textContent = input.value = choice[i];
        label.htmlFor = input.id = 'oip-' + choice[i];
        input.setAttribute('index', i - 2);
        label.id = 'olabel-'+choice[i];
        //var label = form.append('label').attr('class', 'param_label');
        formOverlay.appendChild(input);
        formOverlay.appendChild(label);
        formOverlay.appendChild(document.createElement('br'));
    }

    // make selection pannel visible
    custom_panel.style.visibility = 'visible';
}
/*
 * if x is originally a int or float, this function does nothing
 * if x is originally a string, this function replaces the x value with index,
 * and setup the xNameMap
 */
function indexXString () {
    xNameMap = [];
    for (var t = 0; t < raw_data.tasks.length; t ++) {
        var xValue = new Set();
        for (j in raw_data.data[t]) {
            xValue.add(raw_data.data[t][j][0]);
        }
        // check if xValue contains NaN
        var needConvert = 0;
        for (v of xValue) {
            if (isNaN(Number(v))) {
                needConvert = 1;
                break;
            }
        }
        // do conversion
        if (needConvert == 1) {
            // setup xNameMap
            var index = 0;
            var xNM = {index:[], values:[]};
            for (v of xValue) {
                xNM.index.push(index);
                xNM.values.push(v);
                index += 1;
            }
            xNameMap.push(xNM);
        } else {
            xNameMap.push({index: [], values: []});
        }
    }
}

/*
 * expensive in terms of having most distinct values
 * the input rawData is NOT necessarily the same as the global var raw_data
 */
function findMostExpensiveAxis(rawData, params) {
    var maxValue = 0;
    var maxIndex = -1;
    for (var i = 0; i < params.length - 2; i++) {
        var temp = new Set();
        for (var j = 0; j < rawData.length; j++) {
            temp.add(rawData[j][i+2]);
        }
        var filt_name = [];
        // convert set to list
        for (v of temp) {
            filt_name.push(v);
        }
        if (maxValue < filt_name.length) {
            maxValue = filt_name.length;
            maxIndex = i;
        }
    }
    return maxIndex;
}
/*
 * generate the plot with user specified legend 
 * this function is called after user is in the 'customer plot' mode
 */
function plot_generator() {
    first_plot = true;
    // clear up
    d3.select('#chart').html('');
    var series = raw_data.data;
    // filter is an array of array: filter[i][*] stores all possible values of axis i
    // NOTE: I am actually not using the 'filter' variable. 
    var filter = [];
    for (var t = 0; t < raw_data.tasks.length; t++) {
        for (var i = 0; i < raw_data.params.length - 2; i++) {
            var temp = new Set();
            for (var j = 0; j < raw_data.data[t].length; j++) {
                temp.add(raw_data.data[t][j][i+2]);
            }
            var filt_name = [];
            // convert set to list
            for (v of temp) {
                filt_name.push(v);
            }
            filter.push(filt_name);
        }
    }
    // dealing with geo mean
    gmean_data = [];
    for (var i in series) {
        gmean_data.push([]);
        var gmeanPrep = data_transform(series[i], gmean_list, 'non-overlay');
        for (var k in gmeanPrep) {
            var groupedX = data_transform(gmeanPrep[k], [(0-2)+''], 'xy group');
            seriesXY = [];
            for (var l in groupedX) {
                groupedX[l] = reduceToGmean(groupedX[l]);
                var paramReduced = k.substring(2, k.length).split('  ');
                for (var h in gmean_list) {
                    paramReduced.splice(gmean_list[h], 0, '');
                }
                seriesXY.push([l, groupedX[l]].concat(paramReduced));
                gmean_data[i].push([l,groupedX[l]].concat(paramReduced));
            }
            findAxisScale(seriesXY, xNameMap[i], i);
        }

    }
    series = gmean_data;
    // dealing with overlay axes
    // traverse all tasks
    range = [];
    manipulatedData = {};
    for (var i = 0; i < series.length; i++) {
        var grouped_series = data_transform(series[i], overlay_list, 'non-overlay');
        range.push({x: [], y: []});
        range[i]['x'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        range[i]['y'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        manipulatedData[raw_data.tasks[i]] = {};
        for (var k in grouped_series) {
            findAxisScale(grouped_series[k], xNameMap[i], i);
        }
        // add task name
        var c = d3.select('#chart').append('div').attr('class', 'task_container').attr('id', 'task_'+i);
        var t = c.append('h3').attr('class', 'task_title');
        t.append('span').attr('class', 'h_grey').text('Task Name: ');
        t.append('span').attr('class', 'h_dark').text(raw_data.tasks[i]);
        var s = c.append('h4').attr('class', 'task_title');
        s.append('span').attr('class', 'h_grey').text('Geo Mean Axes: ');
        var temp = _.map(gmean_list, function(d){return raw_data.params[Number(d)+2];}).join();
        if (temp == '') {
            s.append('span').attr('class', 'h_dark').text('None');
        } else {
            s.append('span').attr('class', 'h_dark').text(temp);
        }
        var count = 0;
        for (var k in grouped_series) {
            count += 1;
        }
        if (count <= maxPlotsAllowed) {
            for (var k in grouped_series) {
                simple_plot(raw_data.params, grouped_series[k], overlay_list, xNameMap[i], i, 'normalTitle');
            }
        } else {
            count = 0;
            alert(errMsg['ENUMPLOTS']);
        }
        sizingTaskContainer(count, 'task_'+i);
    }

    // hide customization panel
    //custom_panel.style.visibility = 'hidden';
}

function scroll_to(element) {
    console.log("Scrolling to:");
    console.log(element);
   $('html, body').animate({
        scrollTop: $(element).offset().top - 200
    }, 1000); 
}

function sizingTaskContainer (numPlots, id) {
    var level = Math.ceil(numPlots / numPlotsPerRow);
    // TODO: should get height of canvas directly
    var wholeWidth = document.getElementById('display_canvas').offsetWidth / numPlotsPerRow;
    var wholeHeight = wholeWidth * ratio.wholeHeiToWid;
    // TODO: should not hard code 180
    d3.select('#'+id).style('height', level*wholeHeight + 180);
}

/*
 * to find the max and min of the x y axis
 * used to unify the axis scale over all subplots
 */
function findAxisScale(series, xNM, t) {
    for (var i = 0; i < series.length; i++) {
        if (xNM.values.length == 0) {
            range[t]['x'][0] = (range[t]['x'][0] > Number(series[i][0])) ? Number(series[i][0]) : range[t]['x'][0];
            range[t]['x'][1] = (range[t]['x'][1] < Number(series[i][0])) ? Number(series[i][0]) : range[t]['x'][1];
        }
        if (Number(series[i][1]) != invalidY) {
            range[t]['y'][0] = (range[t]['y'][0] > Number(series[i][1])) ? Number(series[i][1]) : range[t]['y'][0];
            range[t]['y'][1] = (range[t]['y'][1] < Number(series[i][1])) ? Number(series[i][1]) : range[t]['y'][1];

        }
    }
}
/*
 * return the data grouped by a certain axis (filter or x, y)
 */
function data_transform (series, overlay_list, mode) {
    // group by
    if (mode == 'non-overlay') {
        return _.groupBy(series, function (dot_info) {  axis_group = "";
                                                        for (var i = 2; i < dot_info.length; i++) {
                                                            if ($.inArray(i-2+"", overlay_list) == -1){
                                                                axis_group += '  '+dot_info[i];
                                                            } 
                                                        }   
                                                        return axis_group;
                                                     } );
    } else if (mode == 'overlay') {
        return _.groupBy(series, function (dot_info) {  axis_group = "";
                                                        for (var i = 2; i < dot_info.length; i++) {
                                                            if ($.inArray(i-2+"", overlay_list) > -1){
                                                                axis_group += '  '+dot_info[i];
                                                            } 
                                                        }   
                                                        return axis_group;
                                                     } );
    } else if (mode == 'xy group') {
        return _.groupBy(series, function (dot_info) {  axis_group = '';
                                                        for (var i = 0; i < 2; i++) {
                                                            if ($.inArray(i-2+'', overlay_list) > -1){
                                                                // don't add space when group by x or y axis
                                                                axis_group += dot_info[i];
                                                            }
                                                        }
                                                        return axis_group;
                                                     } );
    } else {
        alert('data_transform: unknown mode');
    }
}

function epochTimeConverter(unixEpochTime, paramXY) {
    // deal with unix epoch time
    var date = unixEpochTime;
    if (unixEpoch.indexOf(paramXY) != -1) {
        var date = new Date(Number(date) * 1000);
    }
    return date;
}
/*
 * setup data for plotter:
 *      1. convert to data structure representing series
 *      2. dealing with some corner case of range
 */
function prepareData(params, series, overlay_list, xNM, t, titleMani) {
    //setup data
    if (range[t]['y'][0] == Number.NEGATIVE_INFINITY) {
        range[t]['y'][0] = 0;
        range[t]['y'][1] = 0;
    }
    if (range[t]['x'][0] == range[t]['x'][1]) {
        range[t]['x'][0] -= 0.5*range[t]['x'][0];
        range[t]['x'][1] += 0.5*range[t]['x'][1];
    }
    if (range[t]['y'][0] == range[t]['y'][1]) {
        range[t]['y'][0] -= 0.5*range[t]['y'][0];
        range[t]['y'][1] += 0.5*range[t]['y'][1];
    }
    var lineData = data_transform(series, overlay_list, 'overlay');
    console.log('///// lineData /////');
    console.log(lineData);
    // setup manipulatedData
    for (var i in lineData) {
        var val = i.split('  ');
        var name = '';
        for (var c = 1; c < val.length; c++) {
            name += raw_data.params[Number(overlay_list[c-1]) + 2] + '..';
            name += val[c] + '--';
        }
        manipulatedData[raw_data.tasks[t]][titleMani][name] = lineData[i];
    }
    for (var i in manipulatedData) {
        for (var j in manipulatedData[i]) {
            for (var k in manipulatedData[i][j]) {
                for (var m in manipulatedData[i][j][k]) {
                    manipulatedData[i][j][k][m] = [manipulatedData[i][j][k][m][0],
                                                   manipulatedData[i][j][k][m][1]];
                }
            }
        }
    }
    var lineInfo = [];
    for (var k in lineData) {
        var lineVal = [];
        for (var j = 0; j < lineData[k].length; j ++ ){
            // ignore invalid data points
            if (lineData[k][j][1] == invalidY) {
                continue;
            }
            if (isNaN(Number(lineData[k][j][0]))) {
                lineVal.push({x: lineData[k][j][0], y: lineData[k][j][1]});
            } else {
                lineData[k][j][0] = Number(lineData[k][j][0]);
                lineVal.push({x: lineData[k][j][0], y: lineData[k][j][1]});
            }
        }
        if (xNM.values.length == 0) {
            lineVal = _.sortBy(lineVal, 'x');
            // =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
            lineVal = _.map(lineVal, function(d) {return {x: epochTimeConverter(d.x, params[0]), y: epochTimeConverter(d.y, params[1])};});
        } else {
            // sort by the order in xNM.values
            lineVal = _.map(lineVal, function(d) {return {x: xNM.values.indexOf(d.x), y: epochTimeConverter(d.y,params[1])};} );
            lineVal = _.sortBy(lineVal, 'x');
            lineVal = _.map(lineVal, function(d) {return {x: xNM.values[d.x], y: d.y};});
        }
        if (isNaN(Number(k))) {
            lineInfo.push({values: lineVal, key: k});
        } else {
            // TODO: this Number() won't work if k is a combination of numerical and non-numerical
            lineInfo.push({values: lineVal, key: Number(k)});
        }
    }
    lineInfo = _.sortBy(lineInfo, 'key');
    console.log('-=-=-=-=-= lineInfo =-=-=-=-=-=-');
    console.log(lineInfo);
    return lineInfo;
}
/*
 * setup axis for simple_plot
 *      dealing with different axis types: numerical, non-numerical, time
 *      y axis is always numerical
 */
function setupAxis(params, xNM, t, width, height) {
    // setup axis
    var x = null;
    if (xNM.values.length == 0) {
        var xAxisRange = [range[t]['x'][0] - 0.1*(range[t]['x'][1]-range[t]['x'][0]), range[t]['x'][1] + 0.1*(range[t]['x'][1]-range[t]['x'][0])];
        xAxisRange[0] = epochTimeConverter(xAxisRange[0], params[0]);
        xAxisRange[1] = epochTimeConverter(xAxisRange[1], params[0]);
        if (unixEpoch.indexOf(params[0]) == -1) {
            x = d3.scale.linear()
                  .domain(xAxisRange)
                  .range([0, width]);
        } else {
            x = d3.time.scale().domain(xAxisRange)
                  .range([0, width]);
        }
    } else {
        x = d3.scale.ordinal()
              .domain(xNM.values)
              .rangePoints([0, width], 0.3);
    }
    var y = null;
    var yAxisRange = [range[t]['y'][0] - 0.2*(range[t]['y'][1]-range[t]['y'][0]), range[t]['y'][1] + 0.2*(range[t]['y'][1]-range[t]['y'][0])];
    yAxisRange[0] = epochTimeConverter(yAxisRange[0], params[1]);
    yAxisRange[1] = epochTimeConverter(yAxisRange[1], params[1]);
    if (unixEpoch.indexOf(params[1]) == -1) {
        y = d3.scale.linear()
            .domain(yAxisRange)
            .range([height, 0]);
    } else {
        y = d3.time.scale().domain(yAxisRange)
              .range([height, 0]);
    }
    return [x, y];
}
/*
 * generate the svg plot
 * using d3.js
 * t: task index
 * element layout:
 *      task_container
 *          |
 *          `-- task_title
 *          |
 *          `-- chart_container
 *                  |
 *                  `-- svg
 *                  |    |
 *                  |    `-- g
 *                  |        |
 *                  |        `-- rect, axis, path
 *                  |
 *                  `--legend_container
 *                          |
 *                          `-- svg
 *                               |
 *                               `-- legend_title, legend
 *
 */
function simple_plot(params, series, overlay_list, xNM, t, titleMode) {
    var titleMani = '';
    for (var i = 2; i < series[0].length; i ++){
        if ($.inArray(i-2+'', overlay_list) == -1 
         && String(series[0][i]).length != 0) {
            titleMani += params[i];
            titleMani += '..';
            titleMani += series[0][i];
            titleMani += '--';
        }
    }
    console.log(raw_data.tasks[t]);
    console.log(manipulatedData);
    manipulatedData[raw_data.tasks[t]][titleMani] = {};

    // ...............................
    // setup geometry of plotting area
    // all geometry values are derived by the width of the whole page
    var wholeWidth = document.getElementById('display_canvas').offsetWidth / numPlotsPerRow;
    var wholeHeight = wholeWidth * ratio.wholeHeiToWid;
    // basically width: whole = canvas + legend + small margin 
    //           height: whole = canvas = legend
    var canvWidth = wholeWidth * ratio.canvToWholeWid;
    var canvHeight = wholeHeight;
    // width & height is the size of the actual plotting area (i.e.: rect in canvas svg)
    var width = canvWidth - wholeWidth * (ratio.marginLefToWhole + ratio.marginRigToWhole);
    var height = canvHeight - wholeHeight * (ratio.marginTopToWhole + ratio.marginBotToWhole);
    // width and height for legend container
    var legWidth = wholeWidth * ratio.legToWholeWid;
    var legHeight = wholeHeight;
    /*
    console.log('---- whole:');
    console.log('Width: '+wholeWidth+'  Height: '+wholeHeight);
    console.log('---- canv:');
    console.log('Width: '+canvWidth+'  Height: '+canvHeight);
    console.log('---- plot');
    console.log('Width: '+width+'  Height: '+height);
    */
    // ................................
    // small tuning of plot title position
    var plotTitleY = -10;
    // const for legend, unit: px
    var dataDotRadius = 4;
    var legendSize = 14;
    var legendMargin = 8;
    var legendPeriGap = 10;
    // .................................
    // data
    var lineInfo = prepareData(params, series, overlay_list, xNM, t, titleMani);
    // .................................
    // axis
    var xy = setupAxis(params, xNM, t, width, height);
    var x = xy[0];
    var y = xy[1];
    // 's' is for SI prefix in axis ticks
    var xAxis = d3.svg.axis()
        .scale(x)
        .orient("bottom")
        .ticks(10, 's')
        .tickSize(-height);
    var yAxis = d3.svg.axis()
        .scale(y)
        .orient("left")
        .ticks(10, 's')
        .tickSize(-width);
    // ...................................
    // behavior
    var zoom = d3.behavior.zoom()
        .x(x)
        .y(y)
        .scaleExtent([1, 10])
        .on("zoom", zoomed);
    // ...................................
    // assembly
    var chart = d3.select('#task_'+t).append('div').attr('class', 'chart_container');
    var svg = chart.append('svg').attr('class', 'canvas_container')
        .attr("width", canvWidth)
        .attr("height", canvHeight)
        .attr('shape-rendering', 'geometricPrecision');
    var gTransX = wholeWidth * ratio.marginLefToWhole;
    var gTransY = wholeHeight * ratio.marginTopToWhole; 
    // NB: the translation on g is not strict. By inspecting the actual generated webpage,
    // it seems that the translation of (gTransX, gTransY) is eventually falling on rect.
    svg = svg.append("g")
             .attr('transform', 'translate(' + gTransX + ',' + gTransY + ')');
             //.attr("transform", "translate(" + plotMargin['left'] + "," + plotMargin['top'] + ")")
    // only support zooming for numerical x axis
    if (xNM.values.length == 0) {
        svg.call(zoom);
    }
    // svg -> g -> rect: background color
    svg.append("rect")
        .attr('fill', '#f0f0f0')
        .attr("width", width)
        .attr("height", height);
    svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + height + ")")
        .call(xAxis);
    svg.append("g")
        .attr("class", "y axis")
        .call(yAxis);
    // reassemble x and y axis name, with underscore eliminated
    var xParam = _.reduce(params[0].split('_'), function(memo, num) {return memo == '' ? num : memo+' '+num;}, '');
    var yParam = _.reduce(params[1].split('_'), function(memo, num) {return memo == '' ? num : memo+' '+num;}, '');
    // the hardcoded 30 in "attr('y', height+30)" should be fine. cuz vertical px is irrelevant to window size
    svg.append('text').attr('class', 'x label').attr('text-anchor', 'middle')
       .attr('x', width/2).attr('y', height+30).style('font-size','14px')
       .text(xParam);
    // TODO: the hardcoded -50 in "attr('y', -50)" may need adjustment
    svg.append('text').attr('class', 'y label').attr('text-anchor', 'middle')
       .attr('x', -height/2).attr('y', -50).attr('dy', '.75em')
       .attr('transform', 'rotate(-90)').style('font-size','14px')
       .text(yParam);
    //.on("click", reset);
    // .............
    // add line and dots
    var lineGen = d3.svg.line()
                    .x(function(d) {return x(d['x']);})
                    .y(function(d) {return y(d['y']);});
    var clip = svg.append('svg:clipPath').attr('id', 'clip')
                  .append('svg:rect').attr('fill', '#f0f0f0').attr('x', 0).attr('y', 0)
                  .attr('width', width).attr('height', height);
    var color = d3.scale.category10();
    for (var i in lineInfo){
        var svgg = svg.append('g').attr('clip-path', 'url(#clip)');
        svgg.append('svg:path').attr('fill', 'none')
            .datum(lineInfo[i]['values'])
            .attr('class', 'line').attr('d', lineGen)
            .style('stroke', function() {return color(lineInfo[i]['key']);}); // here each key is associated with a color --> for future legend
        svgg.selectAll('.dots')
            .data(lineInfo[i]['values']) 
            .enter()
            .append('circle')
            .attr('class', 'dots')
            .attr('r', dataDotRadius)
            .attr('fill', function() {return color(lineInfo[i]['key']);})
            .attr('transform', function(d) {return 'translate(' + x(d['x'])+ ',' + y(d['y']) + ')'; });
    }
    /* 
    // TODO: add display coordination function
    var focus = svg.append('g').attr('class', 'focus').style('display', 'none');
    focus.append('circle').attr('r', 4.5);
    focus.append('x', 9).attr('dy', '.35em');

    svg.append('rect').attr('class', 'overlay').attr('width', width).attr('height', height)
       .on('mouseover', function() {focus.style('display', null);})
       .on('mouseout', function() {focus.style('display', 'none'); })
       .on('mousemove', mousemove);
    */ 
    // .............
    // add legend
    // the svg size should be chosen to fit the text
    var svgLegWidth = 0;
    var svgLegHeight = 0;
    var divLegWidth = wholeWidth * ratio.legToWholeWid;
    var divLegHeight = wholeHeight * (1 - ratio.marginBotToWhole);
    var svgLegend = chart.append('div').attr('class', 'legend_container')
                         .style('width', divLegWidth).style('height', divLegHeight)
                         .append('svg').attr('class', 'legend_svg');
    // NOTE: has to set font here, or getBBox will return inprecise value due to the unloaded font
    d3.selectAll('svg').style('font', '10px sans-serif');
    if (lineInfo.length > 1 || lineInfo[0]['key'] != '') {
        var legend = svgLegend.selectAll('.legend')
                        .data(color.domain())   // this step is the key to legend
                        .enter()
                        .append('g')
                        .attr('class', 'legend')
                        .attr('transform', function (d, i) {
                                               var height = legendSize + legendMargin;
                                               var horz = legendPeriGap;
                                               var vert = (i+1) * height + wholeHeight * ratio.marginTopToWhole + legendPeriGap;
                                               return 'translate(' + horz + ', ' + vert + ')';
                                           });
        var legendTitle = _.map(overlay_list, function(d) {return '<'+params[Number(d)+2]+'>';});
        for (var i in legendTitle) {
            legendTitle[i] = _.reduce(legendTitle[i].split('_'), function(memo, num){return memo == '' ? num : memo+' '+num;}, '');
        }
    
        var textElement = svgLegend.append('text').attr('x', legendPeriGap).attr('y', legendPeriGap + wholeHeight * ratio.marginTopToWhole)
           .attr('text-anchor', 'left').style('font-size', '12px')
           .style('font-weight', 'bold').text(String(legendTitle));
        // dynamically adjust the svg legend size 
        svgLegWidth = (svgLegWidth > textElement.node().getBBox().width) ? svgLegWidth : textElement.node().getBBox().width;
        svgLegHeight += textElement.node().getBBox().height;

        legend.append('rect')
              .attr('width', legendSize).attr('height', legendSize)
              .attr('x', 0).attr('y', -5)
              .style('fill', color).style('stroke', color);
        legend.append('text')
              .attr('x', legendSize + legendMargin).attr('y', legendSize - legendMargin)
              .style('font-size', '12px')
              .text(function(d) {return d;});
        legend.each(function() {
                        svgLegWidth = (svgLegWidth > this.getBBox().width) ? svgLegWidth : this.getBBox().width;
                        svgLegHeight += legendSize+legendMargin; });
        svgLegWidth += legendPeriGap;
        svgLegHeight += legendPeriGap + wholeHeight * ratio.marginTopToWhole + legendSize;
        svgLegend.attr('width', svgLegWidth).attr('height', svgLegHeight);
    } else {
        // no legend at all
        svgLegend.attr('width', 0).attr('height', 0);
    }
    // .............
    // add title
    var cTitle = svg.append("text")
                    .attr("x", width/2)
                    .attr("y", plotTitleY)
                    .attr("text-anchor", "middle")
                    .attr("class", "chart-title")
                    .style("font-size", "16px");

    for (var i = 2; i < series[0].length; i ++){
        if ($.inArray(i-2+'', overlay_list) == -1 
         && String(series[0][i]).length != 0) {
            var paramReduced;
            if (titleMode == 'normalTitle') {
                paramRejoin = _.reduce(params[i].split('_'), function(memo, num) {return memo == '' ? num : memo+' '+num;}, '');
                cTitle.append('svg:tspan')
                      .style('fill', 'rgb(111,111,111)')
                      .text(paramRejoin + ':');
            }
            cTitle.append('svg:tspan')
                  .style('font-weight', 'bold')
                  .text('  '+series[0][i]+'  ');
        }
    }
    // dealing with styling
    // have to append this rect after the plots, in order for the peripheral stroke 
    // being placed at the very front
    svg.append("rect")
        .attr('class', 'canvas_peripheral')
        .attr('fill', 'none')
        .attr("width", width)
        .attr("height", height);
    d3.selectAll('rect.canvas_peripheral').style('stroke', '#000');
    d3.selectAll('path').style('fill', 'none');
    d3.selectAll('path.domain').style('stroke', '#fff');
    d3.selectAll('.axis line').style('stroke', '#fff').style('fill', 'none');
    d3.selectAll('.axis path').style('stroke', '#fff').style('fill', 'none');
    d3.selectAll('.line').style('fill', 'none').style('stroke-width','1.5px');
    // ..............
    // interaction
    function zoomed() {
        if (xNM.values.length == 0) {
            // update styling
            d3.selectAll('.axis line').style('stroke', '#fff').style('fill','none');
            svg.select(".x.axis").call(xAxis);
            svg.select(".y.axis").call(yAxis);
            svg.selectAll('.line').attr('class', 'line').attr('d', lineGen);
            svg.selectAll('.dots').attr('transform', function(d) {
                return 'translate(' + x(d['x']) + ',' + y(d['y']) + ')';});
        } else {
            //
        }
    }
    /*
     * have not implemented function of 'reset canvas' yet
     */
    function reset() {
      d3.transition().duration(750).tween("zoom", function() {
        var ix = d3.interpolate(x.domain(), [-width / 2, width / 2]),
            iy = d3.interpolate(y.domain(), [-height / 2, height / 2]);
        return function(t) {
          zoom.x(x.domain(ix(t))).y(y.domain(iy(t)));
          zoomed();
        };
      });
    }

    if (first_plot) {
        scroll_to(svg.node());
        first_plot = false;
    }
}
// ..............
// save image
function savePlot() {
    var count = 0;
    // var all is for compatibility of firefox. FF won't load img correctly unless you call onload function
    // so I have to put zip.file() inside onload function
    var all = 0;
    d3.selectAll('.chart_container').each(function () {all += 1;});
    var zip = new JSZip();
    d3.selectAll('.chart_container').each(function () {
        var html1 = d3.select(this).select('svg')
                     .attr('version', 1.1).attr('xmlns', 'http://www.w3.org/2000/svg')
                     .node().parentNode.innerHTML;
        var html2 = d3.select(this).select('.legend_container').select('svg')
                     .attr('version', 1.1).attr('xmlns', 'http://www.w3.org/2000/svg')
                     .node().parentNode.innerHTML;
        var html = html2 + html1;

        var plotWidth = Number(d3.select(this).select('.canvas_container').attr('width'));
        var plotHeight = Number(d3.select(this).select('.canvas_container').attr('height'));
        var legendWidth = Number(d3.select(this).select('.legend_svg').attr('width'));
        var legendHeight = Number(d3.select(this).select('.legend_svg').attr('height'));
        var svgWidth = plotWidth + legendWidth;
        var svgHeight = plotHeight > legendHeight ? plotHeight : legendHeight;

        d3.select('#temp').append('svg').attr('width', svgWidth).attr('height', svgHeight)
          .attr('version',1.1).attr('xmlns', 'http://www.w3.org/2000/svg').html(html);
        d3.select('#temp').select('.legend_svg').attr('x', plotWidth);
        var tempEle = document.getElementById('temp');
        var tempSvg = tempEle.getElementsByTagName('svg')[0];
        var rmDiv = tempSvg.getElementsByClassName('legend_container')[0];
        tempSvg.removeChild(rmDiv);
        html = d3.select('#temp').node().innerHTML;
        var imgsrc = 'data:image/svg+xml;base64,'+btoa(html);
        console.log('/// imgsrc ///');
        console.log(imgsrc);
        d3.select('#temp').append('canvas').attr('width', svgWidth).attr('height', svgHeight);
        var canvas = document.querySelector('canvas');
        var context = canvas.getContext('2d');
        var image = new Image;
        image.src = imgsrc;
        
        image.onload = function() {
            count += 1;
            context.drawImage(image, 0, 0);
            var canvasdata = canvas.toDataURL('image/png');
            /*
            // this is an alternative option for saving img
            var a = document.createElement('a');
            a.download = 'plot'+count+'.png';
            a.href = canvasdata;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);
            */
            zip.file('png/plotPng-'+count+'.png', canvasdata.substr(canvasdata.indexOf(',')+1), {base64: true});
            zip.file('svg/plotSvg-'+count+'.svg', btoa(html), {base64: true});
            if (count == all) {
                var content =  zip.generate({type: 'blob'});
                saveAs(content, 'plots.zip');
            }
        }
        d3.select('#temp').html('');
    });
}

function savePlotData() {
    var zip = new JSZip();
    var task, plot, line;
    for (var i in manipulatedData) {
        task = (i == '') ? 'task':i;
        for (var j in manipulatedData[i]) {
            plot = j;
            if (plot[plot.length-1] == '-' && plot[plot.length-2] == '-') {
                plot = plot.substring(0, plot.length-2);
            }
            plot = (plot == '') ? 'plot':plot;
            for (var k in manipulatedData[i][j]) {
                line = k;
                if (line[line.length-1] == '-' && line[line.length-2] == '-') {
                    line = line.substring(0, line.length-2);
                }
                line = (line == '') ? 'line':line;
                var arr = '';
                for (var m in manipulatedData[i][j][k]) {
                    arr += String(manipulatedData[i][j][k][m][0]) + '\t';
                    arr += String(manipulatedData[i][j][k][m][1]) + '\n';
                }
                zip.file(task+'/'+plot+'/'+line, arr);
            }
        }
    }
    var content = zip.generate({type: 'blob'});
    saveAs(content, 'plot_data.zip');
}

