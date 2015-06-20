// TODO:
// 1. auto-resizing graph, change the axis range
// 2. set axis range, set title
// 3. gmean
// 5. save graph
// 4. fix the offline plotter

//PROBLEM:
// 1. have to make sure x and y are numbers --> convert string
// 2. if a filter is overlapping with the x / y axis, then you should remove it from the params list

// Some const
// if x axis is an element of timeParam, then the default behavior is to gmean everything
var timeParam = ['run', 'parsed_date'];
var defaultGmean = 'circuit';
// create the actual plot in the display_canvas
//
var overlay_list = [];
// raw data means it is not processed by gmean, and its param name has not been manipulated.
// NOTE: we don't store the data of default plot (cuz they are not manipulated)
var raw_data = null;
var gmean_data = null;
var range = [];
var plotSize = {'width': 650, 'height': 500};
var plotMargin = {'left': 100, 'right': 140, 'top': 70, 'bottom': 70};
// a list of svg elements in d3 type: used for future plot manipulation
var chartList = [];
// xNameMap is only set when value in x axis is string
var xNameMap = null;
/*
 * high level function after data is obtained from database interface
 * -- generate the default plots
 */
function plotter_setup(data) {
    if (data.status !== 'OK') {
        report_error(data.status);
        return;
    }
    // initialize
    raw_data = data;
    gmean_data = null;
    overlay_list = [];
    range = [];
    xNameMap = [];
    d3.select('#chart').html('');
    d3.select('#overlay_select').html('');
    // convert x value to numerical
    indexXString();
    // check is x is run / parsed_date
    // if yes, then geomean on everything
    // if not, then supress on params that will generate the least subplots
    if (timeParam.indexOf(raw_data.params[0]) !== -1) {
        // geomean on everything. 
        defaultToGmeanTimePlot();
        // possibly show the generate_overlay_selector as well
    } else {
        // choose the axis with the most distinct values and overlay on it
        defaultToGmeanSubPlot();
        //generate_overlay_selector('raw');
    }
}
/*
 * generate the default plot if x axis is time
 * methodology: gmean over everything
 */
function defaultToGmeanTimePlot() {
    d3.select('#chart').html('');
    // flattenedData = _.flatten(raw_data, true);
    for (var i = 0; i < raw_data.data.length; i++) {
        groupedX = _.groupBy(raw_data.data[i], function (dotSeries) {return dotSeries[0]} );
        seriesXY = [];
        for (k in groupedX) {
            groupedX[k] = reduceToGmean(groupedX[k]);
            // now {x1: y1, x2: y2, ...}
            // should convert it [[x1,y1],[x2,y2],...]
            seriesXY.push([Number(k), groupedX[k], raw_data.tasks[i]]);
        }
        range.push({x: [], y: []});
        range[i]['x'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        range[i]['y'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        findAxisScale(seriesXY, xNameMap[i], i);

        simple_plot(raw_data.params, seriesXY, [], xNameMap[i], i);
    }
}
/*
 * generate the default plot when x is not time
 * methodology: gmean over circuits, overlay on the filter which has the most distinct values
 */
function defaultToGmeanSubPlot() {
    // check if circuit is in the filter: 
    // it should be, cuz it is the primary key
    var gmeanIndex = raw_data.params.indexOf(defaultGmean);
    if (gmeanIndex == -1) {
        alert('circuit (primary key) is not in the param list\ncheck if this is a bug');
        generate_overlay_selector('raw');
    } else if (gmeanIndex == 0 || gmeanIndex == 1) {
        alert('circuit is in x y axis: \nskip default plot');
        generate_overlay_selector('raw');
    } else {
        for (var k in raw_data.data) {
            var data = [];
            range.push({x: [], y: []});
            range[k]['x'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
            range[k]['y'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
            gmeanPrepData = data_transform(raw_data.data[k], [(gmeanIndex-2)+''], 'non-overlay');
            for (p in gmeanPrepData) {
                // now gmeanPrepData[p] is a subplot overlaying on circuit
                // next: gruop on x and gmean on y
                var groupedX = data_transform(gmeanPrepData[p], [(0-2)+''], 'xy group');
                var seriesXY = [];
                for (l in groupedX) {
                    groupedX[l] = reduceToGmean(groupedX[l]);
                    seriesXY.push([l, groupedX[l], p]);
                }
                findAxisScale(seriesXY, xNameMap[k], k);
                data.push(seriesXY);
            }
            var newRawData = [];
            var newParams = [];
            for (var i in raw_data.params) {
                if (raw_data.params[i] != 'circuit' ) {
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
            for (j in newData) {
                console.log('xNameMap[k]');
                console.log('k = ', k);
                console.log(xNameMap[k]);
                simple_plot(newParams, newData[j], newOverlayList, xNameMap[k], k);
            }
        }
        d3.select('#chart').append('button').attr('type', 'button').attr('id', 'customizePlot').text('customize plot');
        $('#customizePlot').click(function () {d3.select('#chart').html(''); 
                                               generate_overlay_selector('raw');});
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
function generate_overlay_selector(type) {
    var selector_div = d3.select('#overlay_select');
    // clear up
    selector_div.html('');
    var choice = null;
    if (type == 'raw') {
        choice = raw_data.params;
    } else {
        choice = gmean_data.params;
    }
    selector_div.append('h5').text('select overlay axis');
    var form = selector_div.append('form').attr("id", "overlay_options").attr('action', '').append("fieldset");
    form.append('legend').text('select overlay axes:');
    for (var i = 2; i < choice.length; i ++ ) {
        //var label = form.append('label').attr('class', 'param_label');
        form.append('input').attr('type', 'checkbox').attr('value', choice[i]).attr('index', i-2);
        form.append('label').text(choice[i]).append('br');
    }
    form.append('button').attr('type', 'button').attr('id', 'overlay_submission').text('Get Plot!');
    $('#overlay_submission').click(function () {overlay_list = [];
                                                var checkbox = form.selectAll('input', '[type="checkbox"]');
                                                checkbox.each(function () {
                                                                           if (d3.select(this).property('checked')) {
                                                                               overlay_list.push(d3.select(this).attr('index'));
                                                                           }
                                                                         });
                                                plot_generator(type);} );
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
            // update raw_data
            // we don't need this. d3 can plot string values
            /*
            for (j in raw_data.data[t]) {
                raw_data.data[t][j][0] = xNM.index[xNM.values.indexOf(raw_data.data[t][j][0])];
            }
            */
        } else {
            xNameMap.push({index: [], values: []});
        }
    }
    console.log('/////');
    console.log(xNameMap);
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
function plot_generator(type) {
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
            console.log("filt_name: " + raw_data.params[i+2]);
            console.log("filt_temp " + filt_name);
        }
    }
    // traverse all tasks
    range = [];
    for (var i = 0; i < series.length; i++) {
        var grouped_series = data_transform(series[i], overlay_list, 'non-overlay');
        console.log(">>>>>");
        console.log(overlay_list);
        console.log(grouped_series);
        range.push({x: [], y: []});
        range[i]['x'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        range[i]['y'] = [Number.POSITIVE_INFINITY, Number.NEGATIVE_INFINITY];
        for (var k in grouped_series) {
            findAxisScale(grouped_series[k], xNameMap[i], i);
        }
        for (var k in grouped_series) {
            simple_plot(raw_data.params, grouped_series[k], overlay_list, xNameMap[i], i);
        }
    }
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
        range[t]['y'][0] = (range[t]['y'][0] > Number(series[i][1])) ? Number(series[i][1]) : range[t]['y'][0];
        range[t]['y'][1] = (range[t]['y'][1] < Number(series[i][1])) ? Number(series[i][1]) : range[t]['y'][1];
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
/*
 * generate the real plot
 * using d3.js
 * t: task index
 */
function simple_plot(params, series, overlay_list, xNM, t) {
    //setup data
    var lineData = data_transform(series, overlay_list, 'overlay');
    var lineInfo = [];
    for (var k in lineData) {
        var lineVal = [];
        for (var j = 0; j < lineData[k].length; j ++ ){
            lineVal.push({x: lineData[k][j][0], y: lineData[k][j][1]});
        }
        if (xNM.values.length == 0) {
            lineVal = _.sortBy(lineVal, 'x');
        } else {
            // sort by the order in xNM.values
            lineVal = _.map(lineVal, function(d) {return {x: xNM.values.indexOf(d.x), y: d.y};} );
            lineVal = _.sortBy(lineVal, 'x');
            lineVal = _.map(lineVal, function(d) {return {x: xNM.values[d.x], y: d.y};});
        }
        console.log('>>>>>');
        console.log(lineVal);
        lineInfo.push({values: lineVal, key: k});
    }
    // plot
    // width & heigth is the size of the actual plotting area
    var width = plotSize['width'] - plotMargin['left'] - plotMargin['right'];
    var height = plotSize['height'] - plotMargin['top'] - plotMargin['bottom'];
    // ..............
    // setup axis
    var x = null;
    if (xNM.values.length == 0) {
        x = d3.scale.linear()
              .domain(range[t]['x'])
              .range([0, width]);
    } else {
        x = d3.scale.ordinal()
              .domain(xNM.values)
              .rangePoints([0, width]);
    }

    var y = d3.scale.linear()
        .domain(range[t]['y'])
        .range([height, 0]);

    var xAxis = d3.svg.axis()
        .scale(x)
        .orient("bottom")
        .tickSize(-height);
    /*
    if (xNM.values.length != 0) {
        xAxis.tickValues([100,120,140,160]);
    }
    */
    var yAxis = d3.svg.axis()
        .scale(y)
        .orient("left")
        .ticks(5)
        .tickSize(-width);
    // ...............
    // behavior
    var zoom = d3.behavior.zoom()
        .x(x)
        .y(y)
        .scaleExtent([1, 10])
        .on("zoom", zoomed);
    // ...............
    // assemble
    var svg = d3.select('#chart').append('svg')
        .attr("width", width + plotMargin['left'] + plotMargin['right'])
        .attr("height", height + plotMargin['top'] + plotMargin['bottom'])
        .attr('shape-rendering', 'geometricPrecision')
      .append("g")
        .attr("transform", "translate(" + plotMargin['left'] + "," + plotMargin['top'] + ")")
    if (xNM.values.length == 0) {
        svg.call(zoom);
    }

    var canvas = svg.append("rect")
        .attr("width", width)
        .attr("height", height);

    svg.append("g")
        .attr("class", "x axis")
        .attr("transform", "translate(0," + height + ")")
        .call(xAxis);

    svg.append("g")
        .attr("class", "y axis")
        .call(yAxis);
    svg.append('text').attr('class', 'x label').attr('text-anchor', 'middle')
       .attr('x', width/2).attr('y', height+30).style('font-size','14px')
       .text(params[0]);
    svg.append('text').attr('class', 'y label').attr('text-anchor', 'middle')
       .attr('x', -height/2).attr('y', -50).attr('dy', '.75em')
       .attr('transform', 'rotate(-90)').style('font-size','14px')
       .text(params[1]);
    //.on("click", reset);
    // .............
    // add line and dots
    console.log('>>>> converted data');
    console.log(series);
    // TODO: possibly the problem of x(d['x']): why not simply d['x']
    var lineGen = d3.svg.line()
                    .x(function(d) {return x(d['x']);})
                    .y(function(d) {return y(d['y']);});
    var clip = svg.append('svg:clipPath').attr('id', 'clip')
                  .append('svg:rect').attr('x', 0).attr('y', 0)
                  .attr('width', width).attr('height', height);
    var color = d3.scale.category10();
    
    for (var i in lineInfo){
        var svgg = svg.append('g').attr('clip-path', 'url(#clip)');
        svgg.append('svg:path')
            .datum(lineInfo[i]['values'])
            .attr('class', 'line').attr('d', lineGen)
            .style('stroke', function() {return color(lineInfo[i]['key']);}); // here each key is associated with a color --> for future legend
        // TODO: add coordination
        svgg.selectAll('.dots')
            .data(lineInfo[i]['values']) 
            .enter()
            .append('circle')
            .attr('class', 'dots')
            .attr('r', 4)
            .attr('fill', function() {return color(lineInfo[i]['key']);})
            .attr('transform', function(d) {return 'translate(' + x(d['x'])+ ',' + y(d['y']) + ')'; });
    }
    // .............
    // add legend
    var legendSize = 14;
    var legendMargin = 8;
    var legend = svg.selectAll('.legend')
                    .data(color.domain())   // this step is the key to legend
                    .enter()
                    .append('g')
                    .attr('class', 'legend')
                    .attr('transform', function (d, i) {
                                           var height = legendSize + legendMargin;
                                           //var offset = height * color.domain().length / 2;
                                           var horz = width + 10;
                                           var vert = i * height;
                                           return 'translate(' + horz + ', ' + vert + ')';
                                       });
    legend.append('rect')
          .attr('width', legendSize).attr('height', legendSize)
          .style('fill', color).style('stroke', color);
    legend.append('text')
          .attr('x', legendSize + legendMargin).attr('y', legendSize - legendMargin)
          .style('font-size', '12px')
          .text(function(d) {return d;});
    // .............
    // add title
    var cTitle = svg.append("text")
                    .attr("x", width/2)
                    .attr("y", -10)
                    .attr("text-anchor", "middle")
                    .attr("class", "chart-title")
                    .style("font-size", "16px");

    for (var i = 2; i < series[0].length; i ++){
        if ($.inArray(i-2+'', overlay_list) == -1) {
            cTitle.append('svg:tspan')
                  .style('fill', 'rgb(111,111,111)')
                  .text(params[i] + ':');
            cTitle.append('svg:tspan')
                  .style('font-weight', 'bold')
                  .text('  '+series[0][i]+'  ');
        }
    }
    // ..............
    // interaction
    function zoomed() {
        if (xNM.values.length == 0) {
            svg.select(".x.axis").call(xAxis);
            svg.select(".y.axis").call(yAxis);
            svg.selectAll('.line').attr('class', 'line').attr('d', lineGen);
            svg.selectAll('.dots').attr('transform', function(d) {
                return 'translate(' + x(d['x']) + ',' + y(d['y']) + ')';});
        } else {
            //
        }
    }

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
}
/*
 * Utility function, called by plot_generator()
 * actually generating plot by calling d3 function
 * params: raw_data.params
 * series: in the format of list of list (list of tuples)
 * overlay_list: a list of index(int)
 */
/*
function simple_plot(params, series, overlay_list) {
    // setup plot name
    var plot_name = "";
    for (var i = 2; i < series[0].length; i ++){
        if ($.inArray(i-2+'', overlay_list) == -1) {
            plot_name += series[0][i];
            plot_name += '  ';
        }
    }
    // setup raw_data lines
    var lineData = data_transform(series, overlay_list, 'overlay');
    var lineInfo = [];
    for (k in lineData) {
        var lineVal = [];
        for (var j = 0; j < lineData[k].length; j ++ ){
            lineVal.push({x: lineData[k][j][0], y: lineData[k][j][1]});
        }
        lineVal = _.sortBy(lineVal, 'x');
        lineInfo.push({values: lineVal, key: k});
    }
    nv.addGraph(function() {
       var chart = nv.models.lineChart()
                            .margin(plotMargin) 
                            .useInteractiveGuideline(true)
                            .showLegend(true)
                            .showYAxis(true)
                            .showXAxis(true);
        chart.forceX(range['x']);
        chart.forceY(range['y']);
        chartList.push(chart);
        chart.xAxis.axisLabel(params[0])
                   .tickFormat(d3.format(',r'));
        chart.yAxis.axisLabel(params[1])
                   .tickFormat(d3.format('.02f'));
        chart.legend.margin({top:30});
        var svg = d3.select('#chart')
                    .append('svg').attr('id', plot_name)
                    .attr('height', plotSize['height']).attr('width', plotSize['width']);
        
        //var $svg = $(document.getElementById(plot_name));
        //$svg.parent().append('<div class="chart-title">' + plot_name + '</div>');
        
        //$(svg).parent().append('<div class="chart-title">' + plot_name + '</div>');
        svg.append("text")
           .attr("x", plotSize['width']/2)
           .attr("y", 20)
           .attr("text-anchor", "middle")
           .attr("class", "chart-title")
           .style("font-size", "16px")
           .text(plot_name);
        svg.datum(lineInfo).call(chart);
        nv.utils.windowResize(function() { chart.update(); } );
    });
}
*/
// we can choose a slightly more neat method: first group by all the axes
// that is not overlaid, then do legend plot upon every group. this will
// not require any recursive function.


