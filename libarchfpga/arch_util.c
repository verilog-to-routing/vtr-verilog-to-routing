#include "arch_util.h"

#include <cstring>
#include <cassert>

#include "arch_types.h"
#include "read_xml_arch_file.h"

t_port * findPortByName(const char * name, t_pb_type * pb_type,
		int * high_index, int * low_index) {
	t_port * port;
	int i;
	unsigned int high;
	unsigned int low;
	unsigned int bracket_pos;
	unsigned int colon_pos;

	bracket_pos = strcspn(name, "[");

	/* Find port by name */
	port = NULL;
	for (i = 0; i < pb_type->num_ports; i++) {
		char * compare_to = pb_type->ports[i].name;

		if (strlen(compare_to) == bracket_pos
				&& strncmp(name, compare_to, bracket_pos) == 0) {
			port = &pb_type->ports[i];
			break;
		}
	}
	if (i >= pb_type->num_ports) {
		return NULL;
	}

	/* Get indices */
	if (strlen(name) > bracket_pos) {
		high = atoi(&name[bracket_pos + 1]);

		colon_pos = strcspn(name, ":");

		if (colon_pos < strlen(name)) {
			low = atoi(&name[colon_pos + 1]);
		} else {
			low = high;
		}
	} else {
		high = port->num_pins - 1;
		low = 0;
	}

	if (high_index && low_index) {
		*high_index = high;
		*low_index = low;
	}

	return port;
}

void SetupEmptyType(struct s_type_descriptor* cb_type_descriptors,
                    t_type_ptr EMPTY_TYPE) {
	t_type_descriptor * type;
	type = &cb_type_descriptors[EMPTY_TYPE->index];
	type->name = my_strdup("<EMPTY>");
	type->num_pins = 0;
	type->width = 1;
	type->height = 1;
	type->capacity = 0;
	type->num_drivers = 0;
	type->num_receivers = 0;
	type->pinloc = NULL;
	type->num_class = 0;
	type->class_inf = NULL;
	type->pin_class = NULL;
	type->is_global_pin = NULL;
	type->is_Fc_frac = NULL;
	type->is_Fc_full_flex = NULL;
	type->Fc = NULL;
	type->pb_type = NULL;
	type->area = UNDEFINED;

	/* Used as lost area filler, no definition */
	type->grid_loc_def = NULL;
	type->num_grid_loc_def = 0;
}

void alloc_and_load_default_child_for_pb_type( INOUTP t_pb_type *pb_type,
		char *new_name, t_pb_type *copy) {
	int i, j;
	char *dot;

	assert(pb_type->blif_model != NULL);

	copy->name = my_strdup(new_name);
	copy->blif_model = my_strdup(pb_type->blif_model);
	copy->class_type = pb_type->class_type;
	copy->depth = pb_type->depth;
	copy->model = pb_type->model;
	copy->modes = NULL;
	copy->num_modes = 0;
	copy->num_clock_pins = pb_type->num_clock_pins;
	copy->num_input_pins = pb_type->num_input_pins;
	copy->num_output_pins = pb_type->num_output_pins;
	copy->num_pb = 1;

	/* Power */
	copy->pb_type_power = (t_pb_type_power*) my_calloc(1,
			sizeof(t_pb_type_power));
	copy->pb_type_power->estimation_method = power_method_inherited(
			pb_type->pb_type_power->estimation_method);

	/* Ports */
	copy->num_ports = pb_type->num_ports;
	copy->ports = (t_port*) my_calloc(pb_type->num_ports, sizeof(t_port));
	for (i = 0; i < pb_type->num_ports; i++) {
		copy->ports[i].is_clock = pb_type->ports[i].is_clock;
		copy->ports[i].model_port = pb_type->ports[i].model_port;
		copy->ports[i].type = pb_type->ports[i].type;
		copy->ports[i].num_pins = pb_type->ports[i].num_pins;
		copy->ports[i].parent_pb_type = copy;
		copy->ports[i].name = my_strdup(pb_type->ports[i].name);
		copy->ports[i].port_class = my_strdup(pb_type->ports[i].port_class);
		copy->ports[i].port_index_by_type = pb_type->ports[i].port_index_by_type;

		copy->ports[i].port_power = (t_port_power*) my_calloc(1,
				sizeof(t_port_power));
		//Defaults
		if (copy->pb_type_power->estimation_method == POWER_METHOD_AUTO_SIZES) {
			copy->ports[i].port_power->wire_type = POWER_WIRE_TYPE_AUTO;
			copy->ports[i].port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
		} else if (copy->pb_type_power->estimation_method
				== POWER_METHOD_SPECIFY_SIZES) {
			copy->ports[i].port_power->wire_type = POWER_WIRE_TYPE_IGNORED;
			copy->ports[i].port_power->buffer_type = POWER_BUFFER_TYPE_NONE;
		}
	}

	copy->max_internal_delay = pb_type->max_internal_delay;
	copy->annotations = (t_pin_to_pin_annotation*) my_calloc(
			pb_type->num_annotations, sizeof(t_pin_to_pin_annotation));
	copy->num_annotations = pb_type->num_annotations;
	for (i = 0; i < copy->num_annotations; i++) {
		copy->annotations[i].clock = my_strdup(pb_type->annotations[i].clock);
		dot = strstr(pb_type->annotations[i].input_pins, ".");
		copy->annotations[i].input_pins = (char*) my_malloc(
				sizeof(char) * (strlen(new_name) + strlen(dot) + 1));
		copy->annotations[i].input_pins[0] = '\0';
		strcat(copy->annotations[i].input_pins, new_name);
		strcat(copy->annotations[i].input_pins, dot);
		if (pb_type->annotations[i].output_pins != NULL) {
			dot = strstr(pb_type->annotations[i].output_pins, ".");
			copy->annotations[i].output_pins = (char*) my_malloc(
					sizeof(char) * (strlen(new_name) + strlen(dot) + 1));
			copy->annotations[i].output_pins[0] = '\0';
			strcat(copy->annotations[i].output_pins, new_name);
			strcat(copy->annotations[i].output_pins, dot);
		} else {
			copy->annotations[i].output_pins = NULL;
		}
		copy->annotations[i].line_num = pb_type->annotations[i].line_num;
		copy->annotations[i].format = pb_type->annotations[i].format;
		copy->annotations[i].type = pb_type->annotations[i].type;
		copy->annotations[i].num_value_prop_pairs =
				pb_type->annotations[i].num_value_prop_pairs;
		copy->annotations[i].prop = (int*) my_malloc(
				sizeof(int) * pb_type->annotations[i].num_value_prop_pairs);
		copy->annotations[i].value = (char**) my_malloc(
				sizeof(char *) * pb_type->annotations[i].num_value_prop_pairs);
		for (j = 0; j < pb_type->annotations[i].num_value_prop_pairs; j++) {
			copy->annotations[i].prop[j] = pb_type->annotations[i].prop[j];
			copy->annotations[i].value[j] = my_strdup(
					pb_type->annotations[i].value[j]);
		}
	}

}

/* populate special lut class */
void ProcessLutClass(INOUTP t_pb_type *lut_pb_type) {
	char *default_name;
	t_port *in_port;
	t_port *out_port;
	int i, j;

	if (strcmp(lut_pb_type->name, "lut") != 0) {
		default_name = my_strdup("lut");
	} else {
		default_name = my_strdup("lut_child");
	}

	lut_pb_type->num_modes = 2;
	lut_pb_type->pb_type_power->leakage_default_mode = 1;
	lut_pb_type->modes = (t_mode*) my_calloc(lut_pb_type->num_modes,
			sizeof(t_mode));

	/* First mode, route_through */
	lut_pb_type->modes[0].name = my_strdup("wire");
	lut_pb_type->modes[0].parent_pb_type = lut_pb_type;
	lut_pb_type->modes[0].index = 0;
	lut_pb_type->modes[0].num_pb_type_children = 0;
	lut_pb_type->modes[0].mode_power = (t_mode_power*) my_calloc(1,
			sizeof(t_mode_power));

	/* Process interconnect */
	/* TODO: add timing annotations to route-through */
	assert(lut_pb_type->num_ports == 2);
	if (strcmp(lut_pb_type->ports[0].port_class, "lut_in") == 0) {
		assert(strcmp(lut_pb_type->ports[1].port_class, "lut_out") == 0);
		in_port = &lut_pb_type->ports[0];
		out_port = &lut_pb_type->ports[1];
	} else {
		assert(strcmp(lut_pb_type->ports[0].port_class, "lut_out") == 0);
		assert(strcmp(lut_pb_type->ports[1].port_class, "lut_in") == 0);
		out_port = &lut_pb_type->ports[0];
		in_port = &lut_pb_type->ports[1];
	}
	lut_pb_type->modes[0].num_interconnect = 1;
	lut_pb_type->modes[0].interconnect = (t_interconnect*) my_calloc(1,
			sizeof(t_interconnect));
	lut_pb_type->modes[0].interconnect[0].name = (char*) my_calloc(
			strlen(lut_pb_type->name) + 10, sizeof(char));
	sprintf(lut_pb_type->modes[0].interconnect[0].name, "complete:%s",
			lut_pb_type->name);
	lut_pb_type->modes[0].interconnect[0].type = COMPLETE_INTERC;
	lut_pb_type->modes[0].interconnect[0].input_string = (char*) my_calloc(
			strlen(lut_pb_type->name) + strlen(in_port->name) + 2,
			sizeof(char));
	sprintf(lut_pb_type->modes[0].interconnect[0].input_string, "%s.%s",
			lut_pb_type->name, in_port->name);
	lut_pb_type->modes[0].interconnect[0].output_string = (char*) my_calloc(
			strlen(lut_pb_type->name) + strlen(out_port->name) + 2,
			sizeof(char));
	sprintf(lut_pb_type->modes[0].interconnect[0].output_string, "%s.%s",
			lut_pb_type->name, out_port->name);

	lut_pb_type->modes[0].interconnect[0].parent_mode_index = 0;
	lut_pb_type->modes[0].interconnect[0].parent_mode = &lut_pb_type->modes[0];
	lut_pb_type->modes[0].interconnect[0].interconnect_power =
			(t_interconnect_power*) my_calloc(1, sizeof(t_interconnect_power));

	lut_pb_type->modes[0].interconnect[0].annotations =
			(t_pin_to_pin_annotation*) my_calloc(lut_pb_type->num_annotations,
					sizeof(t_pin_to_pin_annotation));
	lut_pb_type->modes[0].interconnect[0].num_annotations =
			lut_pb_type->num_annotations;
	for (i = 0; i < lut_pb_type->modes[0].interconnect[0].num_annotations;
			i++) {
		lut_pb_type->modes[0].interconnect[0].annotations[i].clock = my_strdup(
				lut_pb_type->annotations[i].clock);
		lut_pb_type->modes[0].interconnect[0].annotations[i].input_pins =
				my_strdup(lut_pb_type->annotations[i].input_pins);
		lut_pb_type->modes[0].interconnect[0].annotations[i].output_pins =
				my_strdup(lut_pb_type->annotations[i].output_pins);
		lut_pb_type->modes[0].interconnect[0].annotations[i].line_num =
				lut_pb_type->annotations[i].line_num;
		lut_pb_type->modes[0].interconnect[0].annotations[i].format =
				lut_pb_type->annotations[i].format;
		lut_pb_type->modes[0].interconnect[0].annotations[i].type =
				lut_pb_type->annotations[i].type;
		lut_pb_type->modes[0].interconnect[0].annotations[i].num_value_prop_pairs =
				lut_pb_type->annotations[i].num_value_prop_pairs;
		lut_pb_type->modes[0].interconnect[0].annotations[i].prop =
				(int*) my_malloc(
						sizeof(int)
								* lut_pb_type->annotations[i].num_value_prop_pairs);
		lut_pb_type->modes[0].interconnect[0].annotations[i].value =
				(char**) my_malloc(
						sizeof(char *)
								* lut_pb_type->annotations[i].num_value_prop_pairs);
		for (j = 0; j < lut_pb_type->annotations[i].num_value_prop_pairs; j++) {
			lut_pb_type->modes[0].interconnect[0].annotations[i].prop[j] =
					lut_pb_type->annotations[i].prop[j];
			lut_pb_type->modes[0].interconnect[0].annotations[i].value[j] =
					my_strdup(lut_pb_type->annotations[i].value[j]);
		}
	}

	/* Second mode, LUT */

	lut_pb_type->modes[1].name = my_strdup(lut_pb_type->name);
	lut_pb_type->modes[1].parent_pb_type = lut_pb_type;
	lut_pb_type->modes[1].index = 1;
	lut_pb_type->modes[1].num_pb_type_children = 1;
	lut_pb_type->modes[1].mode_power = (t_mode_power*) my_calloc(1,
			sizeof(t_mode_power));
	lut_pb_type->modes[1].pb_type_children = (t_pb_type*) my_calloc(1,
			sizeof(t_pb_type));
	alloc_and_load_default_child_for_pb_type(lut_pb_type, default_name,
			lut_pb_type->modes[1].pb_type_children);
	/* moved annotations to child so delete old annotations */
	for (i = 0; i < lut_pb_type->num_annotations; i++) {
		for (j = 0; j < lut_pb_type->annotations[i].num_value_prop_pairs; j++) {
			free(lut_pb_type->annotations[i].value[j]);
		}
		free(lut_pb_type->annotations[i].value);
		free(lut_pb_type->annotations[i].prop);
		if (lut_pb_type->annotations[i].input_pins) {
			free(lut_pb_type->annotations[i].input_pins);
		}
		if (lut_pb_type->annotations[i].output_pins) {
			free(lut_pb_type->annotations[i].output_pins);
		}
		if (lut_pb_type->annotations[i].clock) {
			free(lut_pb_type->annotations[i].clock);
		}
	}
	lut_pb_type->num_annotations = 0;
	free(lut_pb_type->annotations);
	lut_pb_type->annotations = NULL;
	lut_pb_type->modes[1].pb_type_children[0].depth = lut_pb_type->depth + 1;
	lut_pb_type->modes[1].pb_type_children[0].parent_mode =
			&lut_pb_type->modes[1];
	for (i = 0; i < lut_pb_type->modes[1].pb_type_children[0].num_ports; i++) {
		if (lut_pb_type->modes[1].pb_type_children[0].ports[i].type
				== IN_PORT) {
			lut_pb_type->modes[1].pb_type_children[0].ports[i].equivalent =
					true;
		}
	}

	/* Process interconnect */
	lut_pb_type->modes[1].num_interconnect = 2;
	lut_pb_type->modes[1].interconnect = (t_interconnect*) my_calloc(2,
			sizeof(t_interconnect));
	lut_pb_type->modes[1].interconnect[0].name = (char*) my_calloc(
			strlen(lut_pb_type->name) + 10, sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[0].name, "direct:%s",
			lut_pb_type->name);
	lut_pb_type->modes[1].interconnect[0].type = DIRECT_INTERC;
	lut_pb_type->modes[1].interconnect[0].input_string = (char*) my_calloc(
			strlen(lut_pb_type->name) + strlen(in_port->name) + 2,
			sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[0].input_string, "%s.%s",
			lut_pb_type->name, in_port->name);
	lut_pb_type->modes[1].interconnect[0].output_string = (char*) my_calloc(
			strlen(default_name) + strlen(in_port->name) + 2, sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[0].output_string, "%s.%s",
			default_name, in_port->name);
	lut_pb_type->modes[1].interconnect[0].infer_annotations = true;

	lut_pb_type->modes[1].interconnect[0].parent_mode_index = 1;
	lut_pb_type->modes[1].interconnect[0].parent_mode = &lut_pb_type->modes[1];
	lut_pb_type->modes[1].interconnect[0].interconnect_power =
			(t_interconnect_power*) my_calloc(1, sizeof(t_interconnect_power));

	lut_pb_type->modes[1].interconnect[1].name = (char*) my_calloc(
			strlen(lut_pb_type->name) + 11, sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[1].name, "direct:%s",
			lut_pb_type->name);

	lut_pb_type->modes[1].interconnect[1].type = DIRECT_INTERC;
	lut_pb_type->modes[1].interconnect[1].input_string = (char*) my_calloc(
			strlen(default_name) + strlen(out_port->name) + 4, sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[1].input_string, "%s.%s",
			default_name, out_port->name);
	lut_pb_type->modes[1].interconnect[1].output_string = (char*) my_calloc(
			strlen(lut_pb_type->name) + strlen(out_port->name)
					+ strlen(in_port->name) + 2, sizeof(char));
	sprintf(lut_pb_type->modes[1].interconnect[1].output_string, "%s.%s",
			lut_pb_type->name, out_port->name);
	lut_pb_type->modes[1].interconnect[1].infer_annotations = true;

	lut_pb_type->modes[1].interconnect[1].parent_mode_index = 1;
	lut_pb_type->modes[1].interconnect[1].parent_mode = &lut_pb_type->modes[1];
	lut_pb_type->modes[1].interconnect[1].interconnect_power =
			(t_interconnect_power*) my_calloc(1, sizeof(t_interconnect_power));

	free(default_name);

	free(lut_pb_type->blif_model);
	lut_pb_type->blif_model = NULL;
	lut_pb_type->model = NULL;
}

/* populate special memory class */
void ProcessMemoryClass(INOUTP t_pb_type *mem_pb_type) {
	char *default_name;
	char *input_name, *input_port_name, *output_name, *output_port_name;
	int i, j, i_inter, num_pb;

	if (strcmp(mem_pb_type->name, "memory_slice") != 0) {
		default_name = my_strdup("memory_slice");
	} else {
		default_name = my_strdup("memory_slice_1bit");
	}

	mem_pb_type->modes = (t_mode*) my_calloc(1, sizeof(t_mode));
	mem_pb_type->modes[0].name = my_strdup(default_name);
	mem_pb_type->modes[0].parent_pb_type = mem_pb_type;
	mem_pb_type->modes[0].index = 0;
	mem_pb_type->modes[0].mode_power = (t_mode_power*) my_calloc(1,
			sizeof(t_mode_power));
	num_pb = OPEN;
	for (i = 0; i < mem_pb_type->num_ports; i++) {
		if (mem_pb_type->ports[i].port_class != NULL
				&& strstr(mem_pb_type->ports[i].port_class, "data")
						== mem_pb_type->ports[i].port_class) {
			if (num_pb == OPEN) {
				num_pb = mem_pb_type->ports[i].num_pins;
			} else if (num_pb != mem_pb_type->ports[i].num_pins) {
				vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
						"memory %s has inconsistent number of data bits %d and %d\n",
						mem_pb_type->name, num_pb,
						mem_pb_type->ports[i].num_pins);
			}
		}
	}

	mem_pb_type->modes[0].num_pb_type_children = 1;
	mem_pb_type->modes[0].pb_type_children = (t_pb_type*) my_calloc(1,
			sizeof(t_pb_type));
	alloc_and_load_default_child_for_pb_type(mem_pb_type, default_name,
			&mem_pb_type->modes[0].pb_type_children[0]);
	mem_pb_type->modes[0].pb_type_children[0].depth = mem_pb_type->depth + 1;
	mem_pb_type->modes[0].pb_type_children[0].parent_mode =
			&mem_pb_type->modes[0];
	mem_pb_type->modes[0].pb_type_children[0].num_pb = num_pb;

	mem_pb_type->num_modes = 1;

	free(mem_pb_type->blif_model);
	mem_pb_type->blif_model = NULL;
	mem_pb_type->model = NULL;

	mem_pb_type->modes[0].num_interconnect = mem_pb_type->num_ports * num_pb;
	mem_pb_type->modes[0].interconnect = (t_interconnect*) my_calloc(
			mem_pb_type->modes[0].num_interconnect, sizeof(t_interconnect));

	for (i = 0; i < mem_pb_type->modes[0].num_interconnect; i++) {
		mem_pb_type->modes[0].interconnect[i].parent_mode_index = 0;
		mem_pb_type->modes[0].interconnect[i].parent_mode =
				&mem_pb_type->modes[0];
	}

	/* Process interconnect */
	i_inter = 0;
	for (i = 0; i < mem_pb_type->num_ports; i++) {
		mem_pb_type->modes[0].interconnect[i_inter].type = DIRECT_INTERC;
		input_port_name = mem_pb_type->ports[i].name;
		output_port_name = mem_pb_type->ports[i].name;

		if (mem_pb_type->ports[i].type == IN_PORT) {
			input_name = mem_pb_type->name;
			output_name = default_name;
		} else {
			input_name = default_name;
			output_name = mem_pb_type->name;
		}

		if (mem_pb_type->ports[i].port_class != NULL
				&& strstr(mem_pb_type->ports[i].port_class, "data")
						== mem_pb_type->ports[i].port_class) {

			mem_pb_type->modes[0].interconnect[i_inter].name =
					(char*) my_calloc(i_inter / 10 + 8, sizeof(char));
			sprintf(mem_pb_type->modes[0].interconnect[i_inter].name,
					"direct%d", i_inter);
			mem_pb_type->modes[0].interconnect[i_inter].infer_annotations = true;

			if (mem_pb_type->ports[i].type == IN_PORT) {
				/* force data pins to be one bit wide and update stats */
				mem_pb_type->modes[0].pb_type_children[0].ports[i].num_pins = 1;
				mem_pb_type->modes[0].pb_type_children[0].num_input_pins -=
						(mem_pb_type->ports[i].num_pins - 1);

				mem_pb_type->modes[0].interconnect[i_inter].input_string =
						(char*) my_calloc(
								strlen(input_name) + strlen(input_port_name)
										+ 2, sizeof(char));
				sprintf(
						mem_pb_type->modes[0].interconnect[i_inter].input_string,
						"%s.%s", input_name, input_port_name);
				mem_pb_type->modes[0].interconnect[i_inter].output_string =
						(char*) my_calloc(
								strlen(output_name) + strlen(output_port_name)
										+ 2 * (6 + num_pb / 10), sizeof(char));
				sprintf(
						mem_pb_type->modes[0].interconnect[i_inter].output_string,
						"%s[%d:0].%s", output_name, num_pb - 1,
						output_port_name);
			} else {
				/* force data pins to be one bit wide and update stats */
				mem_pb_type->modes[0].pb_type_children[0].ports[i].num_pins = 1;
				mem_pb_type->modes[0].pb_type_children[0].num_output_pins -=
						(mem_pb_type->ports[i].num_pins - 1);

				mem_pb_type->modes[0].interconnect[i_inter].input_string =
						(char*) my_calloc(
								strlen(input_name) + strlen(input_port_name)
										+ 2 * (6 + num_pb / 10), sizeof(char));
				sprintf(
						mem_pb_type->modes[0].interconnect[i_inter].input_string,
						"%s[%d:0].%s", input_name, num_pb - 1, input_port_name);
				mem_pb_type->modes[0].interconnect[i_inter].output_string =
						(char*) my_calloc(
								strlen(output_name) + strlen(output_port_name)
										+ 2, sizeof(char));
				sprintf(
						mem_pb_type->modes[0].interconnect[i_inter].output_string,
						"%s.%s", output_name, output_port_name);
			}

			/* Allocate interconnect power structures */
			mem_pb_type->modes[0].interconnect[i_inter].interconnect_power =
					(t_interconnect_power*) my_calloc(1,
							sizeof(t_interconnect_power));
			i_inter++;
		} else {
			for (j = 0; j < num_pb; j++) {
				/* Anything that is not data must be an input */
				mem_pb_type->modes[0].interconnect[i_inter].name =
						(char*) my_calloc(i_inter / 10 + j / 10 + 10,
								sizeof(char));
				sprintf(mem_pb_type->modes[0].interconnect[i_inter].name,
						"direct%d_%d", i_inter, j);
				mem_pb_type->modes[0].interconnect[i_inter].infer_annotations = true;

				if (mem_pb_type->ports[i].type == IN_PORT) {
					mem_pb_type->modes[0].interconnect[i_inter].type =
							DIRECT_INTERC;
					mem_pb_type->modes[0].interconnect[i_inter].input_string =
							(char*) my_calloc(
									strlen(input_name) + strlen(input_port_name)
											+ 2, sizeof(char));
					sprintf(
							mem_pb_type->modes[0].interconnect[i_inter].input_string,
							"%s.%s", input_name, input_port_name);
					mem_pb_type->modes[0].interconnect[i_inter].output_string =
							(char*) my_calloc(
									strlen(output_name)
											+ strlen(output_port_name)
											+ 2 * (6 + num_pb / 10),
									sizeof(char));
					sprintf(
							mem_pb_type->modes[0].interconnect[i_inter].output_string,
							"%s[%d:%d].%s", output_name, j, j,
							output_port_name);
				} else {
					mem_pb_type->modes[0].interconnect[i_inter].type =
							DIRECT_INTERC;
					mem_pb_type->modes[0].interconnect[i_inter].input_string =
							(char*) my_calloc(
									strlen(input_name) + strlen(input_port_name)
											+ 2 * (6 + num_pb / 10),
									sizeof(char));
					sprintf(
							mem_pb_type->modes[0].interconnect[i_inter].input_string,
							"%s[%d:%d].%s", input_name, j, j, input_port_name);
					mem_pb_type->modes[0].interconnect[i_inter].output_string =
							(char*) my_calloc(
									strlen(output_name)
											+ strlen(output_port_name) + 2,
									sizeof(char));
					sprintf(
							mem_pb_type->modes[0].interconnect[i_inter].output_string,
							"%s.%s", output_name, output_port_name);

				}

				/* Allocate interconnect power structures */
				mem_pb_type->modes[0].interconnect[i_inter].interconnect_power =
						(t_interconnect_power*) my_calloc(1,
								sizeof(t_interconnect_power));
				i_inter++;
			}
		}
	}

	mem_pb_type->modes[0].num_interconnect = i_inter;

	free(default_name);
}

e_power_estimation_method power_method_inherited(
		e_power_estimation_method parent_power_method) {
	switch (parent_power_method) {
	case POWER_METHOD_IGNORE:
	case POWER_METHOD_AUTO_SIZES:
	case POWER_METHOD_SPECIFY_SIZES:
	case POWER_METHOD_TOGGLE_PINS:
		return parent_power_method;
	case POWER_METHOD_C_INTERNAL:
	case POWER_METHOD_ABSOLUTE:
		return POWER_METHOD_IGNORE;
	case POWER_METHOD_UNDEFINED:
		return POWER_METHOD_UNDEFINED;
	case POWER_METHOD_SUM_OF_CHILDREN:
		/* Just revert to the default */
		return POWER_METHOD_AUTO_SIZES;
	default:
		assert(0);
		return POWER_METHOD_UNDEFINED; // Should never get here, but avoids a compiler warning.
	}
}

void CreateModelLibrary(OUTP struct s_arch *arch) {
	t_model* model_library;

	model_library = (t_model*) my_calloc(4, sizeof(t_model));
	model_library[0].name = my_strdup("input");
	model_library[0].index = 0;
	model_library[0].inputs = NULL;
	model_library[0].instances = NULL;
	model_library[0].next = &model_library[1];
	model_library[0].outputs = (t_model_ports*) my_calloc(1,
			sizeof(t_model_ports));
	model_library[0].outputs->dir = OUT_PORT;
	model_library[0].outputs->name = my_strdup("inpad");
	model_library[0].outputs->next = NULL;
	model_library[0].outputs->size = 1;
	model_library[0].outputs->min_size = 1;
	model_library[0].outputs->index = 0;
	model_library[0].outputs->is_clock = false;

	model_library[1].name = my_strdup("output");
	model_library[1].index = 1;
	model_library[1].inputs = (t_model_ports*) my_calloc(1,
			sizeof(t_model_ports));
	model_library[1].inputs->dir = IN_PORT;
	model_library[1].inputs->name = my_strdup("outpad");
	model_library[1].inputs->next = NULL;
	model_library[1].inputs->size = 1;
	model_library[1].inputs->min_size = 1;
	model_library[1].inputs->index = 0;
	model_library[1].inputs->is_clock = false;
	model_library[1].instances = NULL;
	model_library[1].next = &model_library[2];
	model_library[1].outputs = NULL;

	model_library[2].name = my_strdup("latch");
	model_library[2].index = 2;
	model_library[2].inputs = (t_model_ports*) my_calloc(2,
			sizeof(t_model_ports));
	model_library[2].inputs[0].dir = IN_PORT;
	model_library[2].inputs[0].name = my_strdup("D");
	model_library[2].inputs[0].next = &model_library[2].inputs[1];
	model_library[2].inputs[0].size = 1;
	model_library[2].inputs[0].min_size = 1;
	model_library[2].inputs[0].index = 0;
	model_library[2].inputs[0].is_clock = false;
	model_library[2].inputs[1].dir = IN_PORT;
	model_library[2].inputs[1].name = my_strdup("clk");
	model_library[2].inputs[1].next = NULL;
	model_library[2].inputs[1].size = 1;
	model_library[2].inputs[1].min_size = 1;
	model_library[2].inputs[1].index = 0;
	model_library[2].inputs[1].is_clock = true;
	model_library[2].instances = NULL;
	model_library[2].next = &model_library[3];
	model_library[2].outputs = (t_model_ports*) my_calloc(1,
			sizeof(t_model_ports));
	model_library[2].outputs->dir = OUT_PORT;
	model_library[2].outputs->name = my_strdup("Q");
	model_library[2].outputs->next = NULL;
	model_library[2].outputs->size = 1;
	model_library[2].outputs->min_size = 1;
	model_library[2].outputs->index = 0;
	model_library[2].outputs->is_clock = false;

	model_library[3].name = my_strdup("names");
	model_library[3].index = 3;
	model_library[3].inputs = (t_model_ports*) my_calloc(1,
			sizeof(t_model_ports));
	model_library[3].inputs->dir = IN_PORT;
	model_library[3].inputs->name = my_strdup("in");
	model_library[3].inputs->next = NULL;
	model_library[3].inputs->size = 1;
	model_library[3].inputs->min_size = 1;
	model_library[3].inputs->index = 0;
	model_library[3].inputs->is_clock = false;
	model_library[3].instances = NULL;
	model_library[3].next = NULL;
	model_library[3].outputs = (t_model_ports*) my_calloc(1,
			sizeof(t_model_ports));
	model_library[3].outputs->dir = OUT_PORT;
	model_library[3].outputs->name = my_strdup("out");
	model_library[3].outputs->next = NULL;
	model_library[3].outputs->size = 1;
	model_library[3].outputs->min_size = 1;
	model_library[3].outputs->index = 0;
	model_library[3].outputs->is_clock = false;

	arch->model_library = model_library;
}

void SyncModelsPbTypes(INOUTP struct s_arch *arch,
		INP t_type_descriptor * Types, INP int NumTypes) {
	int i;
	for (i = 0; i < NumTypes; i++) {
		if (Types[i].pb_type != NULL) {
			SyncModelsPbTypes_rec(arch, Types[i].pb_type);
		}
	}
}

void SyncModelsPbTypes_rec(INOUTP struct s_arch *arch,
		INOUTP t_pb_type * pb_type) {
	int i, j, p;
	t_model *model_match_prim, *cur_model;
	t_model_ports *model_port;
	struct s_linked_vptr *old;
	char* blif_model_name;

	bool found;

	if (pb_type->blif_model != NULL) {

		/* get actual name of subckt */
		if (strstr(pb_type->blif_model, ".subckt ") == pb_type->blif_model) {
			blif_model_name = strchr(pb_type->blif_model, ' ');
		} else {
			blif_model_name = strchr(pb_type->blif_model, '.');
		}
		if (blif_model_name) {
			blif_model_name++; /* get character after the '.' or ' ' */
		} else {
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
					"Unknown blif model %s in pb_type %s\n",
					pb_type->blif_model, pb_type->name);
		}

		/* There are two sets of models to consider, the standard library of models and the user defined models */
		if ((strcmp(blif_model_name, "input") == 0)
				|| (strcmp(blif_model_name, "output") == 0)
				|| (strcmp(blif_model_name, "names") == 0)
				|| (strcmp(blif_model_name, "latch") == 0)) {
			cur_model = arch->model_library;
		} else {
			cur_model = arch->models;
		}

		/* Determine the logical model to use */
		found = false;
		model_match_prim = NULL;
		while (cur_model && !found) {
			/* blif model always starts with .subckt so need to skip first 8 characters */
			if (strcmp(blif_model_name, cur_model->name) == 0) {
				found = true;
				model_match_prim = cur_model;
			}
			cur_model = cur_model->next;
		}
		if (found != true) {
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
					"No matching model for pb_type %s\n", pb_type->blif_model);
		}

		pb_type->model = model_match_prim;
		old = model_match_prim->pb_types;
		model_match_prim->pb_types = (struct s_linked_vptr*) my_malloc(
				sizeof(struct s_linked_vptr));
		model_match_prim->pb_types->next = old;
		model_match_prim->pb_types->data_vptr = pb_type;

		for (p = 0; p < pb_type->num_ports; p++) {
			found = false;
			/* TODO: Parse error checking - check if INPUT matches INPUT and OUTPUT matches OUTPUT (not yet done) */
			model_port = model_match_prim->inputs;
			while (model_port && !found) {
				if (strcmp(model_port->name, pb_type->ports[p].name) == 0) {
					if (model_port->size < pb_type->ports[p].num_pins) {
						model_port->size = pb_type->ports[p].num_pins;
					}
					if (model_port->min_size > pb_type->ports[p].num_pins
							|| model_port->min_size == -1) {
						model_port->min_size = pb_type->ports[p].num_pins;
					}
					pb_type->ports[p].model_port = model_port;
					assert(pb_type->ports[p].type == model_port->dir);
					assert(pb_type->ports[p].is_clock == model_port->is_clock);
					found = true;
				}
				model_port = model_port->next;
			}
			model_port = model_match_prim->outputs;
			while (model_port && !found) {
				if (strcmp(model_port->name, pb_type->ports[p].name) == 0) {
					if (model_port->size < pb_type->ports[p].num_pins) {
						model_port->size = pb_type->ports[p].num_pins;
					}
					if (model_port->min_size > pb_type->ports[p].num_pins
							|| model_port->min_size == -1) {
						model_port->min_size = pb_type->ports[p].num_pins;
					}
					pb_type->ports[p].model_port = model_port;
					assert(pb_type->ports[p].type == model_port->dir);
					found = true;
				}
				model_port = model_port->next;
			}
			if (found != true) {
				vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
						"No matching model port for port %s in pb_type %s\n",
						pb_type->ports[p].name, pb_type->name);
			}
		}
	} else {
		for (i = 0; i < pb_type->num_modes; i++) {
			for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
				SyncModelsPbTypes_rec(arch,
						&(pb_type->modes[i].pb_type_children[j]));
			}
		}
	}
}

void UpdateAndCheckModels(INOUTP struct s_arch *arch) {
	t_model * cur_model;
	t_model_ports *port;
	int i, j;
	cur_model = arch->models;
	while (cur_model) {
		if (cur_model->pb_types == NULL) {
			vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), 0,
					"No pb_type found for model %s\n", cur_model->name);
		}
		port = cur_model->inputs;
		i = 0;
		j = 0;
		while (port) {
			if (port->is_clock) {
				port->index = i;
				i++;
			} else {
				port->index = j;
				j++;
			}
			port = port->next;
		}
		port = cur_model->outputs;
		i = 0;
		while (port) {
			port->index = i;
			i++;
			port = port->next;
		}
		cur_model = cur_model->next;
	}
}

/* Date:July 10th, 2013								
 * Author: Daniel Chen								
 * Purpose: Attempts to match a clock_name specified in an 
 *			timing annotation (Tsetup, Thold, Tc_to_q) with the
 *			clock_name specified in the primitive. Applies
 *			to flipflop/memory right now.
 */
void primitives_annotation_clock_match(
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type) {

	int i_port;
	bool clock_valid = false; //Determine if annotation's clock is same as primtive's clock

	if (!parent_pb_type || !annotation) {
		vpr_throw(VPR_ERROR_OTHER, __FILE__, __LINE__,
				"Annotation_clock check encouters invalid annotation or primitive.\n");
	}

	for (i_port = 0; i_port < parent_pb_type->num_ports; i_port++) {
		if (parent_pb_type->ports[i_port].is_clock) {
			if (strcmp(parent_pb_type->ports[i_port].name, annotation->clock)
					== 0) {
				clock_valid = true;
				break;
			}
		}
	}

	if (!clock_valid) {
		vpr_throw(VPR_ERROR_ARCH, get_arch_file_name(), annotation->line_num,
				"Clock '%s' does not match any clock defined in pb_type '%s'.\n",
				annotation->clock, parent_pb_type->name);
	}

}
