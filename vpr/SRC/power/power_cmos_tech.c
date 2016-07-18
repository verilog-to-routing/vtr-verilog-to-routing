/*********************************************************************
 *  The following code is part of the power modelling feature of VTR.
 *
 * For support:
 * http://code.google.com/p/vtr-verilog-to-routing/wiki/Power
 *
 * or email:
 * vtr.power.estimation@gmail.com
 *
 * If you are using power estimation for your researach please cite:
 *
 * Jeffrey Goeders and Steven Wilton.  VersaPower: Power Estimation
 * for Diverse FPGA Architectures.  In International Conference on
 * Field Programmable Technology, 2012.
 *
 ********************************************************************/

/**
 * This file provides functions relating to the cmos technology.  It
 * includes functions to read the transistor characteristics from the
 * xml file into data structures, and functions to search within
 * these data structures.
 */

/************************* INCLUDES *********************************/
#include <cstring>

#include <assert.h>

#include "pugixml.hpp"
#include "pugixml_util.hpp"

#include "power_cmos_tech.h"
#include "power.h"
#include "power_util.h"
#include "util.h"
#include "read_xml_util.h"
#include "PowerSpicedComponent.h"
#include "power_callibrate.h"

using namespace std;
using namespace pugiutil;

/************************* GLOBALS **********************************/
static t_transistor_inf * g_transistor_last_searched;
static t_power_buffer_strength_inf * g_buffer_strength_last_searched;
static t_power_mux_volt_inf * g_mux_volt_last_searched;

/************************* FUNCTION DECLARATIONS ********************/

static void power_tech_load_xml_file(char * cmos_tech_behavior_filepath);
static void process_tech_xml_load_transistor_info(pugi::xml_node parent, const pugiloc::loc_data& loc_data);
static void power_tech_xml_load_multiplexer_info(pugi::xml_node parent, const pugiloc::loc_data& loc_data);
static void power_tech_xml_load_nmos_st_leakages(pugi::xml_node parent, const pugiloc::loc_data& loc_data);
static int power_compare_transistor_size(const void * key_void, const void * elem_void);
static int power_compare_voltage_pair(const void * key_void, const void * elem_void);
static int power_compare_leakage_pair(const void * key_void, const void * elem_void);
//static void power_tech_xml_load_sc(pugi::xml_node parent, const pugiloc::loc_data& loc_data);
static int power_compare_buffer_strength(const void * key_void, const void * elem_void);
static int power_compare_buffer_sc_levr(const void * key_void, const void * elem_void);
static void power_tech_xml_load_components(pugi::xml_node parent, const pugiloc::loc_data& loc_data);
static void power_tech_xml_load_component(pugi::xml_node parent, const pugiloc::loc_data& loc_data,
		PowerSpicedComponent ** component, const char * name,
		float (*usage_fn)(int num_inputs, float transistor_size));
/************************* FUNCTION DEFINITIONS *********************/

void power_tech_init(char * cmos_tech_behavior_filepath) {
	power_tech_load_xml_file(cmos_tech_behavior_filepath);
}

/**
 * Reads the transistor properties from the .xml file
 */
void power_tech_load_xml_file(char * cmos_tech_behavior_filepath) {
	char msg[BUFSIZE];

	if (!file_exists(cmos_tech_behavior_filepath)) {
		/* .xml transistor characteristics is missing */
		sprintf(msg,
				"The CMOS technology behavior file ('%s') does not exist.  No power information will be calculated.",
				cmos_tech_behavior_filepath);
		power_log_msg(POWER_LOG_ERROR, msg);

		g_power_tech->NMOS_inf.num_size_entries = 0;
		g_power_tech->NMOS_inf.long_trans_inf = NULL;
		g_power_tech->NMOS_inf.size_inf = NULL;

		g_power_tech->PMOS_inf.num_size_entries = 0;
		g_power_tech->PMOS_inf.long_trans_inf = NULL;
		g_power_tech->PMOS_inf.size_inf = NULL;

		g_power_tech->Vdd = 0.;
		g_power_tech->temperature = 85;
		g_power_tech->PN_ratio = 1.;
		return;
	}

    pugi::xml_document doc;
    pugiloc::loc_data loc_data;
    try {
        loc_data = pugiutil::load_xml(doc, cmos_tech_behavior_filepath);
    } catch(XmlError& e) {
        vpr_throw(VPR_ERROR_POWER, cmos_tech_behavior_filepath, 0,
                  "Failed to load CMOS Tech Properties file '%s' (%s).\n", cmos_tech_behavior_filepath, e.what());
    }

    auto technology = get_single_child(doc, "technology", loc_data);

    get_attribute(technology, "file", loc_data); //Check exists


    auto tech_size = get_attribute(technology, "size", loc_data);
    g_power_tech->tech_size = tech_size.as_float();

    auto operating_point = get_single_child(technology, "operating_point", loc_data);
    g_power_tech->temperature = get_attribute(operating_point, "temperature", loc_data).as_float();
    g_power_tech->Vdd = get_attribute(operating_point, "Vdd", loc_data).as_float();

    auto p_to_n = get_single_child(technology, "p_to_n", loc_data);
    g_power_tech->PN_ratio = get_attribute(p_to_n, "ratio", loc_data).as_float();

	/* Transistor Information 
     *  We expect two <transistor> sections for P and N types
     */
    auto child = get_first_child(technology, "transistor", loc_data);
    process_tech_xml_load_transistor_info(child, loc_data);

    child = child.next_sibling("transistor");

	process_tech_xml_load_transistor_info(child, loc_data);

    //Should be no more
    auto next = child.next_sibling("transistor");
    if(next) {
        vpr_throw(VPR_ERROR_POWER, loc_data.filename_c_str(), loc_data.line(next),
                  "Encountered extra <transitor> section (expect 2 only: pmos and nmos)\n");
    }

	/* Multiplexer Voltage Information */
    child = get_single_child(technology, "multiplexers", loc_data);
	power_tech_xml_load_multiplexer_info(child, loc_data);

	/* Vds Leakage Information */
    child = get_single_child(technology, "nmos_leakages", loc_data);
	power_tech_xml_load_nmos_st_leakages(child, loc_data);

	/* Buffer SC Info */
	/*
     child = get_single_child(technology, "buffer_sc", loc_data);
	 power_tech_xml_load_sc(child);
	 */

	/* Components */
    child = get_single_child(technology, "components", loc_data);
	power_tech_xml_load_components(child, loc_data);
}

static void power_tech_xml_load_component(pugi::xml_node parent, const pugiloc::loc_data& loc_data,
		PowerSpicedComponent ** component, const char * name,
		float (*usage_fn)(int num_inputs, float transistor_size)) {

	string component_name(name);
	*component = new PowerSpicedComponent(component_name, usage_fn);

    auto cur = get_single_child(parent, name, loc_data);

    auto child = get_first_child(cur, "inputs", loc_data);
	while (child) {
		int num_inputs = get_attribute(child, "num_inputs", loc_data).as_int(0);

        auto gc = get_first_child(child, "size", loc_data);
		while (gc) {
			float transistor_size = get_attribute(gc, "transistor_size", loc_data).as_float(0.);
			float power = get_attribute(gc, "power", loc_data).as_float(0.);
			(*component)->add_data_point(num_inputs, transistor_size, power);

            gc = gc.next_sibling("size");
		}
        child = child.next_sibling("inputs");
	}
}

static void power_tech_xml_load_components(pugi::xml_node parent, const pugiloc::loc_data& loc_data) {

	g_power_commonly_used->component_callibration =
			(PowerSpicedComponent**) my_calloc(POWER_CALLIB_COMPONENT_MAX,
					sizeof(PowerSpicedComponent*));

	power_tech_xml_load_component(parent, loc_data,
			&g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER],
			"buf", power_usage_buf_for_callibration);

	power_tech_xml_load_component(parent, loc_data,
			&g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER_WITH_LEVR],
			"buf_levr", power_usage_buf_levr_for_callibration);

	power_tech_xml_load_component(parent, loc_data,
			&g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_FF],
			"dff", power_usage_ff_for_callibration);

	power_tech_xml_load_component(parent, loc_data,
			&g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_MUX],
			"mux", power_usage_mux_for_callibration);

	power_tech_xml_load_component(parent, loc_data,
			&g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_LUT],
			"lut", power_usage_lut_for_callibration);

}

/**
 * Read short-circuit buffer information from the transistor .xml file.
 * This contains values for buffers of various 1) # Stages 2) Stage strength 3) Input type & capacitance
 */
#if 0
static void power_tech_xml_load_sc(pugi::xml_node parent, const pugiloc::loc_data& loc_data) {
	int i, j, k;
	int num_buffer_sizes;

	/* Information for buffers, based on # of stages in buffer */
	num_buffer_sizes = CountChildren(parent, "stages", 1);
	g_power_tech->max_buffer_size = num_buffer_sizes; /* buffer size starts at 1, not 0 */
	g_power_tech->buffer_size_inf = (t_power_buffer_size_inf*) my_calloc(
			g_power_tech->max_buffer_size + 1, sizeof(t_power_buffer_size_inf));

	child = FindFirstElement(parent, "stages", true);
	i = 1;
	while (child) {
		t_power_buffer_size_inf * size_inf = &g_power_tech->buffer_size_inf[i];

		GetIntProperty(child, "num_stages", true, 1);

		/* For the given # of stages, find the records for the strength of each stage */
		size_inf->num_strengths = CountChildren(child, "strength", 1);
		size_inf->strength_inf = (t_power_buffer_strength_inf*) my_calloc(
				size_inf->num_strengths, sizeof(t_power_buffer_strength_inf));

		gc = FindFirstElement(child, "strength", true);
		j = 0;
		while (gc) {
			t_power_buffer_strength_inf * strength_inf =
			&size_inf->strength_inf[j];

			/* Get the short circuit factor for a buffer with no level restorer at the input */
			strength_inf->stage_gain = GetFloatProperty(gc, "gain", true, 0.0);
			strength_inf->sc_no_levr = GetFloatProperty(gc, "sc_nolevr", true,
					0.0);

			/* Get the short circuit factor for buffers with level restorers at the input */
			strength_inf->num_levr_entries = CountChildren(gc, "input_cap", 1);
			strength_inf->sc_levr_inf = (t_power_buffer_sc_levr_inf*) my_calloc(
					strength_inf->num_levr_entries,
					sizeof(t_power_buffer_sc_levr_inf));

			ggc = FindFirstElement(gc, "input_cap", true);
			k = 0;
			while (ggc) {
				t_power_buffer_sc_levr_inf * levr_inf =
				&strength_inf->sc_levr_inf[k];

				/* Short circuit factor is depdent on size of mux that drives the buffer */
				levr_inf->mux_size = GetIntProperty(ggc, "mux_size", true, 0);
				levr_inf->sc_levr = GetFloatProperty(ggc, "sc_levr", true, 0.0);

				prev = ggc;
				ggc = ggc->next;
				FreeNode(prev);
				k++;
			}

			prev = gc;
			gc = gc->next;
			FreeNode(prev);
			j++;
		}

		prev = child;
		child = child->next;
		FreeNode(prev);
		i++;
	}
}
#endif

/**
 *  Read NMOS subthreshold leakage currents from the .xml transistor characteristics
 *  This builds a table of (Vds,Ids) value pairs
 *  */
static void power_tech_xml_load_nmos_st_leakages(pugi::xml_node parent, const pugiloc::loc_data& loc_data) {
	int num_nmos_sizes;
	int num_leakage_pairs;
	int i;
	int nmos_idx;

    num_nmos_sizes = count_children(parent, "nmos", loc_data);
	g_power_tech->num_nmos_leakage_info = num_nmos_sizes;
	g_power_tech->nmos_leakage_info = (t_power_nmos_leakage_inf*) my_calloc(num_nmos_sizes, sizeof(t_power_nmos_leakage_inf));

    auto me = get_first_child(parent, "nmos", loc_data);
	nmos_idx = 0;
	while (me) {
		t_power_nmos_leakage_inf * nmos_info = &g_power_tech->nmos_leakage_info[nmos_idx];
		nmos_info->nmos_size = get_attribute(me, "size", loc_data).as_float(0.);

        num_leakage_pairs = count_children(me, "nmos_leakage", loc_data);
		nmos_info->num_leakage_pairs = num_leakage_pairs;
		nmos_info->leakage_pairs = (t_power_nmos_leakage_pair*) my_calloc(num_leakage_pairs, sizeof(t_power_nmos_leakage_pair));

		auto child = get_first_child(me, "nmos_leakage", loc_data);
		i = 0;
		while (child) {
			nmos_info->leakage_pairs[i].v_ds = get_attribute(child, "Vds", loc_data).as_float(0.0);
			nmos_info->leakage_pairs[i].i_ds = get_attribute(child, "Ids", loc_data).as_float(0.0);

            child = child.next_sibling("nmos_leakage");
			i++;
		}

        me = me.next_sibling("nmos");
		nmos_idx++;
	}

}

/**
 *  Read multiplexer information from the .xml transistor characteristics.
 *  This contains the estimates of mux output voltages, depending on 1) Mux Size 2) Mux Vin
 *  */
static void power_tech_xml_load_multiplexer_info(pugi::xml_node parent, const pugiloc::loc_data& loc_data) {
	int num_nmos_sizes;
	int num_mux_sizes;
	int i, j, nmos_idx;

	/* Process all nmos sizes */
    num_nmos_sizes = count_children(parent, "nmos", loc_data);
    assert(num_nmos_sizes > 0);
	g_power_tech->num_nmos_mux_info = num_nmos_sizes;
	g_power_tech->nmos_mux_info = (t_power_nmos_mux_inf*) my_calloc(
	        num_nmos_sizes, sizeof(t_power_nmos_mux_inf));

    auto me = get_first_child(parent, "nmos", loc_data);
	nmos_idx = 0;
	while (me) {
		t_power_nmos_mux_inf * nmos_inf = &g_power_tech->nmos_mux_info[nmos_idx];
        nmos_inf->nmos_size = get_attribute(me, "size", loc_data).as_float(0.0);

		/* Process all multiplexer sizes */
        num_mux_sizes = count_children(me, "multiplexer", loc_data);

		/* Add entries for 0 and 1, for convenience, although
		 * they will never be used
		 */
		nmos_inf->max_mux_sl_size = 1 + num_mux_sizes;
		nmos_inf->mux_voltage_inf = (t_power_mux_volt_inf*) my_calloc(
				nmos_inf->max_mux_sl_size + 1, sizeof(t_power_mux_volt_inf));

        auto child = get_first_child(me, "multiplexer", loc_data);
		i = 1;
		while (child) {
			int num_voltages;

            assert(i == get_attribute(child, "size", loc_data).as_int(0));

			/* For each mux size, process all of the Vin levels */
            num_voltages = count_children(child, "voltages", loc_data);

			nmos_inf->mux_voltage_inf[i].num_voltage_pairs = num_voltages;
			nmos_inf->mux_voltage_inf[i].mux_voltage_pairs =
					(t_power_mux_volt_pair*) my_calloc(num_voltages,
							sizeof(t_power_mux_volt_pair));

            auto gc = get_first_child(child, "voltages", loc_data);
			j = 0;
			while (gc) {
				/* For each mux size, and Vin level, get the min/max V_out */
				nmos_inf->mux_voltage_inf[i].mux_voltage_pairs[j].v_in      = get_attribute(gc, "in", loc_data).as_float(0.0);
				nmos_inf->mux_voltage_inf[i].mux_voltage_pairs[j].v_out_min = get_attribute(gc, "out_min", loc_data).as_float(0.0);
				nmos_inf->mux_voltage_inf[i].mux_voltage_pairs[j].v_out_max = get_attribute(gc, "out_max", loc_data).as_float(0.0);

                gc = gc.next_sibling("voltages");
				j++;
			}

            child = child.next_sibling("multiplexer");
			i++;
		}

        me = me.next_sibling("nmos");
		nmos_idx++;
	}
}

/**
 * Read the transistor information from the .xml transistor characteristics.
 * For each transistor size, it extracts the:
 * - transistor node capacitances
 * - subthreshold leakage
 * - gate leakage
 */
static void process_tech_xml_load_transistor_info(pugi::xml_node parent, const pugiloc::loc_data& loc_data) {
	t_transistor_inf * trans_inf;
	int i;

	/* Get transistor type: NMOS or PMOS */
    auto prop = get_attribute(parent, "type", loc_data);
	trans_inf = NULL;
	if (strcmp(prop.value(), "nmos") == 0) {
		trans_inf = &g_power_tech->NMOS_inf;
	} else if (strcmp(prop.value(), "pmos") == 0) {
		trans_inf = &g_power_tech->PMOS_inf;
	} else {
        vpr_throw(VPR_ERROR_POWER, loc_data.filename_c_str(), loc_data.line(parent),
                  "Unrecognized transistor type '%s', expected 'nmos' or 'pmos'\n", prop.value());
	}

	/* Get long transistor information (W=1,L=2) */
	trans_inf->long_trans_inf = (t_transistor_size_inf*) my_malloc(
			sizeof(t_transistor_size_inf));

    auto child = get_single_child(parent, "long_size", loc_data);
    assert(get_attribute(child, "L", loc_data).as_int(0) == 2);
	trans_inf->long_trans_inf->size = get_attribute(child, "W", loc_data).as_float(0.);

    auto grandchild = get_single_child(child, "leakage_current", loc_data);
	trans_inf->long_trans_inf->leakage_subthreshold = get_attribute(grandchild, "subthreshold", loc_data).as_float(0.);

    grandchild = get_single_child(child, "capacitance", loc_data);
	trans_inf->long_trans_inf->C_g = get_attribute(grandchild, "C_g", loc_data).as_float(0);
	trans_inf->long_trans_inf->C_d = get_attribute(grandchild, "C_d", loc_data).as_float(0);
	trans_inf->long_trans_inf->C_s = get_attribute(grandchild, "C_s", loc_data).as_float(0);

	/* Process all transistor sizes */
    trans_inf->num_size_entries = count_children(parent, "size", loc_data);
	trans_inf->size_inf = (t_transistor_size_inf*) my_calloc(
			trans_inf->num_size_entries, sizeof(t_transistor_size_inf));

    child = get_first_child(parent, "size", loc_data);
	i = 0;
	while (child) {
        assert(get_attribute(child, "L", loc_data).as_int(0) == 1);

		trans_inf->size_inf[i].size = get_attribute(child, "W", loc_data).as_float(0);

		/* Get leakage currents */
        grandchild = get_single_child(child, "leakage_current", loc_data);
		trans_inf->size_inf[i].leakage_subthreshold = get_attribute(grandchild, "subthreshold", loc_data).as_float(0);
		trans_inf->size_inf[i].leakage_gate = get_attribute(grandchild, "gate", loc_data).as_float(0);

		/* Get node capacitances */
        grandchild = get_single_child(child, "capacitance", loc_data);
		trans_inf->size_inf[i].C_g = get_attribute(grandchild, "C_g", loc_data).as_float(0);
		trans_inf->size_inf[i].C_s = get_attribute(grandchild, "C_s", loc_data).as_float(0);
		trans_inf->size_inf[i].C_d = get_attribute(grandchild, "C_d", loc_data).as_float(0);

        child = child.next_sibling("size");
		i++;
	}
}

/**
 * This function searches for a transistor by size
 * - lower: (Return value) The lower-bound matching transistor
 * - upper: (Return value) The upper-bound matching transistor
 * - type: The transistor type to search for
 * - size: The transistor size to search for (size = W/L)
 */
bool power_find_transistor_info(t_transistor_size_inf ** lower,
		t_transistor_size_inf ** upper, e_tx_type type, float size) {
	char msg[1024];
	t_transistor_size_inf key;
	t_transistor_size_inf * found;
	t_transistor_inf * trans_info;
	float min_size, max_size;
	bool error = false;

	key.size = size;

	/* Find the appropriate global transistor records */
	trans_info = NULL;
	if (type == NMOS) {
		trans_info = &g_power_tech->NMOS_inf;
	} else if (type == PMOS) {
		trans_info = &g_power_tech->PMOS_inf;
	} else {
		assert(0);
	}

	/* No transistor data exists */
	if (trans_info->size_inf == NULL) {
		power_log_msg(POWER_LOG_ERROR,
				"No transistor information exists.  Cannot determine transistor properties.");
		error = true;
		return error;
	}

	/* Make note of the transistor record we are searching in, and the bounds */
	g_transistor_last_searched = trans_info;
	min_size = trans_info->size_inf[0].size;
	max_size = trans_info->size_inf[trans_info->num_size_entries - 1].size;

	found = (t_transistor_size_inf*) bsearch(&key, trans_info->size_inf,
			trans_info->num_size_entries, sizeof(t_transistor_size_inf),
			&power_compare_transistor_size);
	assert(found);

	if (size < min_size) {
		/* Too small */
		assert(found == &trans_info->size_inf[0]);
		sprintf(msg,
				"Using %s transistor of size '%f', which is smaller than the smallest modeled transistor (%f) in the technology behavior file.",
				transistor_type_name(type), size, min_size);
		power_log_msg(POWER_LOG_WARNING, msg);
		*lower = NULL;
		*upper = found;
	} else if (size > max_size) {
		/* Too large */
		assert(
				found
						== &trans_info->size_inf[trans_info->num_size_entries
								- 1]);
		sprintf(msg,
				"Using %s transistor of size '%f', which is larger than the largest modeled transistor (%f) in the technology behavior file.",
				transistor_type_name(type), size, max_size);
		power_log_msg(POWER_LOG_WARNING, msg);
		*lower = found;
		*upper = NULL;
	} else {
		*lower = found;
		*upper = found + 1;
	}

	return error;
}

/**
 * This function searches for the Ids leakage current, based on a given Vds.
 * This function is used for minimum-sized NMOS transistors (used in muxs).
 * - lower: (Return value) The lower-bound matching V/I pair
 * - upper: (Return value) The upper-bound matching V/I pair
 * - v_ds: The drain/source voltage to search for
 */
t_power_nmos_leakage_inf * g_power_searching_nmos_leakage_info;
void power_find_nmos_leakage(t_power_nmos_leakage_inf * nmos_leakage_info,
		t_power_nmos_leakage_pair ** lower, t_power_nmos_leakage_pair ** upper,
		float v_ds) {
	t_power_nmos_leakage_pair key;
	t_power_nmos_leakage_pair * found;

	key.v_ds = v_ds;

	g_power_searching_nmos_leakage_info = nmos_leakage_info;

	found = (t_power_nmos_leakage_pair*) bsearch(&key,
			nmos_leakage_info->leakage_pairs,
			nmos_leakage_info->num_leakage_pairs,
			sizeof(t_power_nmos_leakage_pair), power_compare_leakage_pair);
	assert(found);

	if (found == &nmos_leakage_info->leakage_pairs[nmos_leakage_info->num_leakage_pairs - 1]) {
		/* The results equal to the max voltage (Vdd) */
		*lower = found;
		*upper = NULL;
	} else {
		*lower = found;
		*upper = found + 1;
	}
}

/**
 * This function searches for the information for a given buffer strength.
 * - lower: (Return value) The lower-bound matching record
 * - upper: (Return value) The upper-bound matching record
 * - stage_gain: The buffer strength to search for
 */
void power_find_buffer_strength_inf(t_power_buffer_strength_inf ** lower,
		t_power_buffer_strength_inf ** upper,
		t_power_buffer_size_inf * size_inf, float stage_gain) {
	t_power_buffer_strength_inf key;
	t_power_buffer_strength_inf * found;

	float min_size;
	float max_size;

	min_size = size_inf->strength_inf[0].stage_gain;
	max_size = size_inf->strength_inf[size_inf->num_strengths - 1].stage_gain;

	assert(stage_gain >= min_size && stage_gain <= max_size);

	key.stage_gain = stage_gain;

	found = (t_power_buffer_strength_inf*) bsearch(&key, size_inf->strength_inf,
			size_inf->num_strengths, sizeof(t_power_buffer_strength_inf),
			power_compare_buffer_strength);

	if (stage_gain == max_size) {
		*lower = found;
		*upper = NULL;
	} else {
		*lower = found;
		*upper = found + 1;
	}
}

/**
 * This function searches for short-circuit current information for a level-restoring buffer,
 * based on the size of the multiplexer driving the input
 * - lower: (Return value) The lower-bound matching record
 * - upper: (Return value) The upper-bound matching record
 * - buffer_strength: The set of records to search withing, which are for a specific buffer size/strength
 * - input_mux_size: The input mux size to search for
 */
void power_find_buffer_sc_levr(t_power_buffer_sc_levr_inf ** lower,
		t_power_buffer_sc_levr_inf ** upper,
		t_power_buffer_strength_inf * buffer_strength, int input_mux_size) {
	t_power_buffer_sc_levr_inf key;
	t_power_buffer_sc_levr_inf * found;
	char msg[1024];
	int max_size;

	assert(input_mux_size >= 1);

	key.mux_size = input_mux_size;

	g_buffer_strength_last_searched = buffer_strength;
	found = (t_power_buffer_sc_levr_inf*) bsearch(&key,
			buffer_strength->sc_levr_inf, buffer_strength->num_levr_entries,
			sizeof(t_power_buffer_sc_levr_inf), power_compare_buffer_sc_levr);

	max_size = buffer_strength->sc_levr_inf[buffer_strength->num_levr_entries
			- 1].mux_size;
	if (input_mux_size > max_size) {
		/* Input mux too large */
		assert(
				found
						== &buffer_strength->sc_levr_inf[buffer_strength->num_levr_entries
								- 1]);
		sprintf(msg,
				"Using buffer driven by mux of size '%d', which is larger than the largest modeled size (%d) in the technology behavior file.",
				input_mux_size, max_size);
		power_log_msg(POWER_LOG_WARNING, msg);
		*lower = found;
		*upper = NULL;
	} else {
		*lower = found;
		*upper = found + 1;
	}
}

/**
 * Comparison function, used by power_find_nmos_leakage
 */
static int power_compare_leakage_pair(const void * key_void, const void * elem_void) {
	const t_power_nmos_leakage_pair * key = (const t_power_nmos_leakage_pair*) key_void;
	const t_power_nmos_leakage_pair * elem = (const t_power_nmos_leakage_pair*) elem_void;
	const t_power_nmos_leakage_pair * next = (const t_power_nmos_leakage_pair*) elem + 1;

	/* Compare against last? */
	if (elem == &g_power_searching_nmos_leakage_info->leakage_pairs[g_power_searching_nmos_leakage_info->num_leakage_pairs - 1]) {
		if (key->v_ds >= elem->v_ds) {
			return 0;
		} else {
			return -1;
		}
	}

	/* Check for exact match to Vdd (upper end) */
	if (key->v_ds == elem->v_ds) {
		return 0;
	} else if (key->v_ds < elem->v_ds) {
		return -1;
	} else if (key->v_ds > next->v_ds) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * This function searches for multiplexer output voltage information, based on input voltage
 * - lower: (Return value) The lower-bound matching record
 * - upper: (Return value) The upper-bound matching record
 * - volt_inf: The set of records to search within, which are for a specific mux size
 * - v_in: The input voltage to search for
 */
void power_find_mux_volt_inf(t_power_mux_volt_pair ** lower,
		t_power_mux_volt_pair ** upper, t_power_mux_volt_inf * volt_inf,
		float v_in) {
	t_power_mux_volt_pair key;
	t_power_mux_volt_pair * found;

	key.v_in = v_in;

	g_mux_volt_last_searched = volt_inf;
	found = (t_power_mux_volt_pair*) bsearch(&key, volt_inf->mux_voltage_pairs,
			volt_inf->num_voltage_pairs, sizeof(t_power_mux_volt_pair),
			power_compare_voltage_pair);
	assert(found);

	if (found
			== &volt_inf->mux_voltage_pairs[volt_inf->num_voltage_pairs - 1]) {
		*lower = found;
		*upper = NULL;
	} else {
		*lower = found;
		*upper = found + 1;
	}
}

/**
 * Comparison function, used by power_find_buffer_sc_levr
 */
static int power_compare_buffer_sc_levr(const void * key_void,
		const void * elem_void) {
	const t_power_buffer_sc_levr_inf * key =
			(const t_power_buffer_sc_levr_inf*) key_void;
	const t_power_buffer_sc_levr_inf * elem =
			(const t_power_buffer_sc_levr_inf*) elem_void;
	const t_power_buffer_sc_levr_inf * next;

	/* Compare against last? */
	if (elem
			== &g_buffer_strength_last_searched->sc_levr_inf[g_buffer_strength_last_searched->num_levr_entries
					- 1]) {
		if (key->mux_size >= elem->mux_size) {
			return 0;
		} else {
			return -1;
		}
	}

	/* Compare against first? */
	if (elem == &g_buffer_strength_last_searched->sc_levr_inf[0]) {
		if (key->mux_size < elem->mux_size) {
			return 0;
		}
	}

	/* Check if the key is between elem and the next element */
	next = elem + 1;
	if (key->mux_size > next->mux_size) {
		return 1;
	} else if (key->mux_size < elem->mux_size) {
		return -1;
	} else {
		return 0;
	}
}

/**
 * Comparison function, used by power_find_buffer_strength_inf
 */
static int power_compare_buffer_strength(const void * key_void,
		const void * elem_void) {
	const t_power_buffer_strength_inf * key =
			(const t_power_buffer_strength_inf*) key_void;
	const t_power_buffer_strength_inf * elem =
			(const t_power_buffer_strength_inf*) elem_void;
	const t_power_buffer_strength_inf * next =
			(const t_power_buffer_strength_inf*) elem + 1;

	/* Check for exact match */
	if (key->stage_gain == elem->stage_gain) {
		return 0;
	} else if (key->stage_gain < elem->stage_gain) {
		return -1;
	} else if (key->stage_gain > next->stage_gain) {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Comparison function, used by power_find_transistor_info
 */
static int power_compare_transistor_size(const void * key_void,
		const void * elem_void) {
	const t_transistor_size_inf * key = (const t_transistor_size_inf*) key_void;
	const t_transistor_size_inf * elem =
			(const t_transistor_size_inf*) elem_void;
	const t_transistor_size_inf * next;

	/* Check if we are comparing against the last element */
	if (elem
			== &g_transistor_last_searched->size_inf[g_transistor_last_searched->num_size_entries
					- 1]) {
		/* Match if the desired value is larger than the largest item in the list */
		if (key->size >= elem->size) {
			return 0;
		} else {
			return -1;
		}
	}

	/* Check if we are comparing against the first element */
	if (elem == &g_transistor_last_searched->size_inf[0]) {
		/* Match the smallest if it is smaller than the smallest */
		if (key->size < elem->size) {
			return 0;
		}
	}

	/* Check if the key is between elem and the next element */
	next = elem + 1;
	if (key->size > next->size) {
		return 1;
	} else if (key->size < elem->size) {
		return -1;
	} else {
		return 0;
	}

}

/**
 * Comparison function, used by power_find_mux_volt_inf
 */
static int power_compare_voltage_pair(const void * key_void,
		const void * elem_void) {
	const t_power_mux_volt_pair * key = (const t_power_mux_volt_pair *) key_void;
	const t_power_mux_volt_pair * elem =
			(const t_power_mux_volt_pair *) elem_void;
	const t_power_mux_volt_pair * next = (const t_power_mux_volt_pair *) elem
			+ 1;

	/* Check if we are comparing against the last element */
	if (elem
			== &g_mux_volt_last_searched->mux_voltage_pairs[g_mux_volt_last_searched->num_voltage_pairs
					- 1]) {
		/* Match if the desired value is larger than the largest item in the list */
		if (key->v_in >= elem->v_in) {
			return 0;
		} else {
			return -1;
		}
	}

	/* Check for exact match to Vdd (upper end) */
	if (key->v_in == elem->v_in) {
		return 0;
	} else if (key->v_in < elem->v_in) {
		return -1;
	} else if (key->v_in > next->v_in) {
		return 1;
	} else {
		return 0;
	}
}

