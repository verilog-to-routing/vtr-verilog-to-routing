// "use strict";
// globals
var root_url = 'http://' + window.location.href.split('/')[2];
var tasks = new Set();
var x_sel = "";
var y_sel = "";
var task_cached = "";
var params = [];
var labels = [];
var overlayed_params = [];
var subplotted_params = [];
var active_querying_filter;
var filter_method_map = {"range":"BETWEEN", "categorical":"IN"};
// serialize without [] for array parameters
jQuery.ajaxSettings.traditional = true; 

// DOM modifying functions
$(document).ready(function() {

// update all other selection boxes if necessary when tasks are changed
var task_select = document.getElementById("task_select").getElementsByClassName('task');
for (var i=0; i < task_select.length; ++i) {
	task_select[i].addEventListener('click', function(e) {
		e.preventDefault();
		toggle_task(this);
	});
}

var add_filter = document.getElementById("add_filter");
add_filter.addEventListener('click', create_filter_window);

var gen_plot = document.getElementById('generate_plot');
gen_plot.addEventListener('click', generate_plot);

var gen_csv = document.getElementById('generate_csv');
gen_csv.addEventListener('click', download_csv);

var get_query = document.getElementById('get_query');
get_query.addEventListener('click', copy_data_query);

// cached persistent DOM objects
var x_param = document.getElementById('x_param');
var y_param = document.getElementById('y_param');

});



// actions upon selecting a task
// all edit on DOM is done by populate_param_windows, which looks at params array
function update_params() {
	remove_all_filters();
	if (tasks.size === 0) {
		params = [];
		add_filter.style.visibility = 'hidden';
		report_debug("cleared params");
		populate_param_windows();
	}
	else {
		$.getJSON(root_url+'/params/?'+create_task_query(), get_params);
		add_filter.style.visibility = 'visible';
		report_debug("updated params");
	}
	// to assure correct sequence, populate param windows must be called inside the functions that update params
}
function clear_param_windows() {
	while (x_param.firstChild) x_param.removeChild(x_param.firstChild);
	while (y_param.firstChild) y_param.removeChild(y_param.firstChild);
}
function populate_param_windows() {
	clear_param_windows();

	for (var p of params) {
		var p_pair = p.split(' ');
		var val = document.createElement('a');
		val.className = "param";
		val.title = p;
		val.text = p_pair[0];

		if (p_pair[1] !== "TEXT") {
			var val_y = val.cloneNode(true);
			val_y.addEventListener('click', function(e) {
				e.preventDefault();
				toggle_y(this);
			}, false);
			y_param.appendChild(val_y);
		}

		val.addEventListener('click', function(e) {
			e.preventDefault();
			toggle_x(this);
		}, false);
		x_param.appendChild(val);
	}
	if (x_sel) {
		var x;
		if (typeof x_sel === "string") x = x_param.querySelector(["a[title='", x_sel, "']"].join(""));
		else x = x_param.querySelector(["a[title='", x_sel.title, "']"].join(""));
		if (x) toggle_x(x);
		else x_sel = undefined;
	} 
	if (y_sel) {
		var y;
		if (typeof y_sel === "string") y = y_param.querySelector(["a[title='", y_sel, "']"].join(""));
		else y = y_param.querySelector(["a[title='", y_sel.title, "']"].join(""));
		if (y) toggle_y(y);
		else y_sel = undefined;
	} 
	report_debug("populated param windows");
}

// actions upon adding a filter
function create_filter_window(selected) {
	var sel_bar = document.getElementById('selection_bar');
	var filter = document.createElement('div');
	filter.className = "filter selection_box";

	// create selection menu
	var param_texts = [];
	for (var p of params) param_texts.push(p.split(' ')[0]);
	var select_param = create_selection(param_texts, params, "--- parameters ---", false);
	select_param.className = "filter_param";
	select_param.addEventListener('change', query_param, false);

	// create close button
	var close = document.createElement('a');
	close.className = "close";
	close.text = 'X';
	close.addEventListener('click', remove_filter, false);

	filter.appendChild(select_param);
	filter.appendChild(close);
	var wrapper = document.createElement('div');
	wrapper.className = "box_wrapper filter_wrapper";
	wrapper.appendChild(filter);
	sel_bar.insertBefore(wrapper, add_filter);

	report_debug(filter);

	if (typeof selected === "string") {
		report_debug("selected");
		select_param.value = selected;
		report_debug(select_param.value);
		fire_event(select_param, "change"); 
	}
	else {report_debug(selected);}

	return filter;

}

// actions upon clicking the generate plot button
function generate_plot() {
	var data_query = create_data_query();
	if (data_query) {
		data_query = [root_url, '/data?', data_query].join('');
	    console.log(">>>>  data query >>>>");
		report_debug(data_query);
		$.getJSON(data_query, plotter_setup, false);
	}
}

function download_csv() {
	var data_query = create_data_query();
	if (data_query) {
		data_query = [root_url, '/csv?', data_query].join('');
		console.log(">>>>  csv query >>>>");
		report_debug(data_query);
		window.location = data_query;
	}
}

function copy_data_query() {
	var data_query = create_data_query();
	if (data_query) {
		window.prompt(["Copy with Ctrl+c, Enter\n\
Prefix with:  ",root_url,
"\nand one of:\n\
view? - to see the same selection in the viewer\n\
csv? - to download the raw data in csv format\n\
data? - to download the raw data in JSON format"].join(""), data_query);
	}
}

// JSON request callbacks
function get_params( data ) {
	if (data.status !== "OK") {
		report_error(data.status);
		return;
	}
	params = data.params;
	report_debug("got params");
	populate_param_windows();
}

// after selecting a parameter, describe its properties
function describe_param( data ) {
	if (data.status !== "OK") {
		report_error(data.status);
		return;
	}
	clean_querying_filter(data);

	// add a selection if it doesn't exist
	if (active_querying_filter.getElementsByClassName('filter_sel').length === 0) {
		var data_type = document.createElement('select');
		data_type.className = "filter_sel";

		var categorical = document.createElement('option');
		var range = document.createElement('option');

		categorical.value = categorical.text = 'categorical';
		range.value = range.text = 'range';

		if (data.type === "categorical") categorical.selected = "selected";
		else if (data.type === "range") range.selected = "selected";

		data_type.appendChild(categorical);
		data_type.appendChild(range);

		data_type.addEventListener('change', function() {
			active_querying_filter = this.parentNode;
			mode = this.value;
			var query_param = active_querying_filter.getElementsByClassName("filter_param")[0].value;
			report_debug(query_param);
			$.getJSON([root_url,'/param?',create_task_query(),'&p=',query_param,'&m=',mode].join(""), 
				describe_param_adjust, false );
		});

		active_querying_filter.appendChild(data_type);
	}
	// need to update event listener
	else {
		var data_type = active_querying_filter.getElementsByClassName('filter_sel')[0];

	}

	// fill in either a range or a multi-select
	create_filter_val(active_querying_filter, data);
	// query completed
	active_querying_filter = "";
}

// update existing elements
function describe_param_adjust( data ) {
	if (data.status !== "OK") {
		report_error(data.status);
		return;
	}
	clean_querying_filter(data);

	report_debug("adjusting parameter query");

	create_filter_val(active_querying_filter, data);
	// query adjustment completed
	active_querying_filter = "";
}


// remove old elements inside active querying filter (to be updated)
function clean_querying_filter( data ) {
	var old_filter_vals = active_querying_filter.getElementsByClassName("filter_val");
	while (old_filter_vals[0]) active_querying_filter.removeChild(old_filter_vals[0]);

	var filter_sel = active_querying_filter.getElementsByClassName("filter_sel");
	if (filter_sel.length > 0) {
		report_debug(filter_sel.length);
		filter_sel[0].value = data.type;
	}
}

function create_filter_val(target, data) {
	// create either 2 selection boxes for min and max or a multi-select for in filter
	var filter_val = document.createElement('div');
	filter_val.className = "filter_val";
	if (data.type === "categorical") {
		var select_in = create_selection(data.val, data.val, "ctrl+click for multiple, shift+click for range", true);
		select_in.className = "in";
		filter_val.appendChild(select_in);
	}
	else if (data.type === "range") {
		var min = document.createElement('input');
		var max = document.createElement('input');
		min.type = max.type = "text";
		min.className = "min";
		max.className = "max";
		min.min = data.val[0];
		max.max = data.val[1];
		min.placeholder = "min " + min.min;
		max.placeholder = "max " + max.max;
		filter_val.appendChild(min);
		filter_val.appendChild(max);
	}	
	target.appendChild(filter_val);

	return filter_val;
}


// click functions
function toggle_task(task) {
	task_cached = "";
	if (task.className === "task") {tasks.add(task.title); task.className = "task sel_task";}
	else {tasks.delete(task.title); task.className = "task";}
	update_params();
}
function toggle_y(selected_y) {	
	if (selected_y.className === "param") {
		if (y_sel) y_sel.className = "param"; 
		y_sel = selected_y; 
		selected_y.className = "param sel_param";
	}
	else {selected_y.className = "param"; y_sel = "";}
}
function toggle_x(selected_x) {	
	if (selected_x.className === "param") {
		if (x_sel) x_sel.className = "param"; 
		x_sel = selected_x; 
		selected_x.className = "param sel_param";
	}
	else {selected_x.className = "param"; x_sel = "";}
}


// utility functions
// always attached to a close button inside the filter
function remove_filter() {
	var filter_box = this.parentNode.parentNode;
	filter_box.parentNode.removeChild(filter_box);
}

function remove_all_filters() {
	var filters = document.getElementsByClassName('filter');
	while (filters[0]) {
		// box wrapper surrounding filter
		filters[0].parentNode.parentNode.removeChild(filters[0].parentNode);
	}
}

function parse_filter( filter ) {
	var parsed_filter = ['fp='];
	var filtered_param = filter.getElementsByClassName('filter_param');
	if (filtered_param.length === 0) return [];
	filtered_param = filtered_param[0].value;
	parsed_filter.push(filtered_param);
	
	var filter_type = filter.getElementsByClassName('filter_sel');
	if (filter_type.length === 0) return [];
	filter_type = filter_type[0].value;
	parsed_filter.push('&fm=', filter_method_map[filter_type]);

	if (filter_type === "categorical") {
		var select = filter.getElementsByClassName('in')[0];
		var selected = get_selected(select);
		if (!selected || !selected[0]) {report_debug("nothing selected"); return [];}
		for (var s=0, len=selected.length; s < len; ++s) 
			parsed_filter.push('&fa=', selected[s]);
	}
	else if (filter_type === "range") {
		var min = filter.getElementsByClassName('min')[0].value;
		var max = filter.getElementsByClassName('max')[0].value;
		if (!min || !max) {report_debug("min or max missing"); return [];}
		parsed_filter.push('&fa=', min, '&fa=', max);
	}
	parsed_filter.push('&');
	return parsed_filter;
}

function query_param() {
	var param = this.value;
	active_querying_filter = this.parentNode;
	$.getJSON([root_url,'/param?',create_task_query(),'&p=',param].join(""), describe_param, false );
	report_debug(this.value);
}

function get_selected(select) { 
	var result = [];
	var options = select && select.options;
	var opt;

	for (var i=0, len=options.length; i < len; ++i) {
		opt = options[i];
		if (opt.selected) result.push(opt.value);
	}
	return result;
}
function set_selected(select, sels) {	// sels is a Set of selected values
	var options = select && select.options;
	var opt;
	// unselect the prompt
	options[0].selected = false;
	for (var i=0, len=options.length; i < len; ++i) {
		opt = options[i];
		if (sels.has(opt.value)) opt.selected = true;
	}
}
function create_selection(texts, values, prompt_text, multiple) {
	var select = document.createElement('select');
	if (multiple) select.multiple = true;
	var prompt = document.createElement('option');
	prompt.disabled = "disabled";
	prompt.selected = "selected";
	prompt.text = prompt_text;
	prompt.value = "";
	select.appendChild(prompt);
	for (var i = 0, len = values.length; i < len; ++i) {
		var opt = document.createElement('option');
		opt.value = values[i];
		opt.text = texts[i];
		select.appendChild(opt);
	}	
	return select;
}
function create_task_query() {
	if (!task_cached) {
		var qs = ["db=", database, '&'];
		for (var task of tasks) 
			qs.push("t=",encodeURIComponent(task),"&");	
		if (qs.length > 0) 
			qs.pop();	// chop last & (mostly harmless)

		task_cached = qs.join("");
	}
	else {
		report_debug("task cached");
	}
	return task_cached;
}
function create_data_query() {
	var data_query = [create_task_query(), '&x='];
	if (!x_sel.title) {report_error("x parameter not selected"); return '';}
	if (!y_sel.title) {report_error("y parameter not selected"); return '';}

	data_query.push(x_sel.title, '&y=', y_sel.title, '&');

	var sel_bar = document.getElementById('selection_bar');

	var filters = sel_bar.getElementsByClassName('filter');
	for (var f = 0, len = filters.length; f < len; ++f) {
		var filter = parse_filter(filters[f]);		
		report_debug(filter);
		data_query.push.apply(data_query, filter);
	}
	return data_query.join('');	
}

function fire_event(element, event) {
    if (document.createEventObject) {
	    // dispatch for IE
	    var evt = document.createEventObject();
	    return element.fireEvent('on'+event,evt);
    }
    else {
	    // dispatch for firefox + others
	    var evt = document.createEvent("HTMLEvents");
	    evt.initEvent(event, true, true ); // event type,bubbling,cancelable
	    return !element.dispatchEvent(evt);
    }
}

function report_error(error) {
	alert(error);
}
function report_debug(debug) {
	console.log(debug)
}
