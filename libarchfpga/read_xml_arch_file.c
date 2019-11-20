/* The XML parser processes an XML file into a tree data structure composed of      *
 * ezxml_t nodes.  Each ezxml_t node represents an XML element.  For example        *
 * <a> <b/> </a> will generate two ezxml_t nodes.  One called "a" and its           *
 * child "b".  Each ezxml_t node can contain various XML data such as attribute     *
 * information and text content.  The XML parser provides several functions to      *
 * help the developer build, traverse, and free this ezxml_t tree.                  *
 *                                                                                  *
 * The function ezxml_parse_file reads in an architecture file.                     *
 *                                                                                  *
 * The function FindElement returns a child ezxml_t node for a given ezxml_t        *
 * node that matches a name provided by the developer.                              *
 *                                                                                  *
 * The function FreeNode frees a child ezxml_t node.  All children nodes must       *
 * be freed before the parent can be freed.                                         *
 *                                                                                  *
 * The function FindProperty is used to extract attributes from an ezxml_t node.    *
 * The corresponding ezxml_set_attr is used to then free an attribute after it      *
 * is read.  We have several helper functions that perform this common              *
 * read/store/free operation in one step such as GetIntProperty and                 *
 * GetFloatProperty.                                                                *
 *                                                                                  *
 * Because of how the XML tree traversal works, we free everything when we're       *
 * done reading an architecture file to make sure that there isn't some part        *
 * of the architecture file that got missed.                                        *
 */

#include <string.h>
#include <assert.h>
#include "util.h"
#include "arch_types.h"
#include "ReadLine.h"
#include "ezxml.h"
#include "read_xml_arch_file.h"
#include "read_xml_util.h"

enum Fc_type {
	FC_ABS, FC_FRAC, FC_FULL
};

/* This identifies the t_type_ptr of an IO block */
static t_type_ptr IO_TYPE = NULL;

/* This identifies the t_type_ptr of an Empty block */
static t_type_ptr EMPTY_TYPE = NULL;

/* This identifies the t_type_ptr of the default logic block */
static t_type_ptr FILL_TYPE = NULL;

/* Describes the different types of CLBs available */
static struct s_type_descriptor *cb_type_descriptors;

/* Function prototypes */
/*   Populate data */
static void SetupEmptyType(void);
static void SetupPinLocationsAndPinClasses(ezxml_t Locations,
		t_type_descriptor * Type);
static void SetupGridLocations(ezxml_t Locations, t_type_descriptor * Type);
#if 0
static void SetupTypeTiming(ezxml_t timing,
		t_type_descriptor * Type);
#endif
/*    Process XML hiearchy */
static void ProcessPb_Type(INOUTP ezxml_t Parent, t_pb_type * pb_type,
		t_mode * mode);
static void ProcessPb_TypePort(INOUTP ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method);
static void ProcessPinToPinAnnotations(ezxml_t parent,
		t_pin_to_pin_annotation *annotation);
static void ProcessInterconnect(INOUTP ezxml_t Parent, t_mode * mode);
static void ProcessMode(INOUTP ezxml_t Parent, t_mode * mode,
		boolean * default_leakage_mode);
static void Process_Fc(ezxml_t Node, t_type_descriptor * Type);
static void ProcessComplexBlockProps(ezxml_t Node, t_type_descriptor * Type);
static void ProcessChanWidthDistr(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch);
static void ProcessChanWidthDistrDir(INOUTP ezxml_t Node, OUTP t_chan * chan);
static void ProcessModels(INOUTP ezxml_t Node, OUTP struct s_arch *arch);
static void ProcessLayout(INOUTP ezxml_t Node, OUTP struct s_arch *arch);
static void ProcessDevice(INOUTP ezxml_t Node, OUTP struct s_arch *arch,
		INP boolean timing_enabled);
static void alloc_and_load_default_child_for_pb_type(INOUTP t_pb_type *pb_type,
		char *new_name, t_pb_type *copy);
static void ProcessLutClass(INOUTP t_pb_type *lut_pb_type);
static void ProcessMemoryClass(INOUTP t_pb_type *mem_pb_type);
static void ProcessComplexBlocks(INOUTP ezxml_t Node,
		OUTP t_type_descriptor ** Types, OUTP int *NumTypes,
		INP boolean timing_enabled);
static void ProcessSwitches(INOUTP ezxml_t Node,
		OUTP struct s_switch_inf **Switches, OUTP int *NumSwitches,
		INP boolean timing_enabled);
static void ProcessDirects(INOUTP ezxml_t Parent, OUTP t_direct_inf **Directs,
		OUTP int *NumDirects, INP boolean timing_enabled);
static void ProcessSegments(INOUTP ezxml_t Parent,
		OUTP struct s_segment_inf **Segs, OUTP int *NumSegs,
		INP struct s_switch_inf *Switches, INP int NumSwitches,
		INP boolean timing_enabled);
static void ProcessCB_SB(INOUTP ezxml_t Node, INOUTP boolean * list,
		INP int len);
static void ProcessPower( INOUTP ezxml_t parent,
		INOUTP t_power_arch * power_arch, INP t_type_descriptor * Types,
		INP int NumTypes);

static void ProcessClocks(ezxml_t Parent, t_clock_arch * clocks);

static void CreateModelLibrary(OUTP struct s_arch *arch);
static void UpdateAndCheckModels(INOUTP struct s_arch *arch);
static void SyncModelsPbTypes(INOUTP struct s_arch *arch,
		INP t_type_descriptor * Types, INP int NumTypes);
static void SyncModelsPbTypes_rec(INOUTP struct s_arch *arch,
		INP t_pb_type *pb_type);

static void PrintPb_types_rec(INP FILE * Echo, INP const t_pb_type * pb_type,
		int level);
static void ProcessPb_TypePowerEstMethod(ezxml_t Parent, t_pb_type * pb_type);
static void ProcessPb_TypePort_Power(ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method);
e_power_estimation_method power_method_inherited(
		e_power_estimation_method parent_power_method);

/* Sets up the pinloc map and pin classes for the type. Unlinks the loc nodes
 * from the XML tree.
 * Pins and pin classses must already be setup by SetupPinClasses */
static void SetupPinLocationsAndPinClasses(ezxml_t Locations,
		t_type_descriptor * Type) {
	int i, j, k, Count, Len;
	int capacity, pin_count;
	int num_class;
	const char * Prop;

	ezxml_t Cur, Prev;
	char **Tokens, **CurTokens;

	capacity = Type->capacity;

	Prop = FindProperty(Locations, "pattern", TRUE);
	if (strcmp(Prop, "spread") == 0) {
		Type->pin_location_distribution = E_SPREAD_PIN_DISTR;
	} else if (strcmp(Prop, "custom") == 0) {
		Type->pin_location_distribution = E_CUSTOM_PIN_DISTR;
	} else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] %s is an invalid pin location pattern.\n",
				Locations->line, Prop);
		exit(1);
	}
	ezxml_set_attr(Locations, "pattern", NULL);

	/* Alloc and clear pin locations */
	Type->pinloc = (int ***) my_malloc(Type->height * sizeof(int **));
	Type->pin_height = (int *) my_calloc(Type->num_pins, sizeof(int));
	for (i = 0; i < Type->height; ++i) {
		Type->pinloc[i] = (int **) my_malloc(4 * sizeof(int *));
		for (j = 0; j < 4; ++j) {
			Type->pinloc[i][j] = (int *) my_malloc(
					Type->num_pins * sizeof(int));
			for (k = 0; k < Type->num_pins; ++k) {
				Type->pinloc[i][j][k] = 0;
			}
		}
	}

	Type->pin_loc_assignments = (char****) my_malloc(
			Type->height * sizeof(char***));
	Type->num_pin_loc_assignments = (int**) my_malloc(
			Type->height * sizeof(int*));
	for (i = 0; i < Type->height; i++) {
		Type->pin_loc_assignments[i] = (char***) my_calloc(4, sizeof(char**));
		Type->num_pin_loc_assignments[i] = (int*) my_calloc(4, sizeof(int));
	}

	/* Load the pin locations */
	if (Type->pin_location_distribution == E_CUSTOM_PIN_DISTR) {
		Cur = Locations->child;
		while (Cur) {
			CheckElement(Cur, "loc");

			/* Get offset */
			i = GetIntProperty(Cur, "offset", FALSE, 0);
			if ((i < 0) || (i >= Type->height)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] %d is an invalid offset for type '%s'.\n",
						Cur->line, i, Type->name);
				exit(1);
			}

			/* Get side */
			Prop = FindProperty(Cur, "side", TRUE);
			if (0 == strcmp(Prop, "left")) {
				j = LEFT;
			}

			else if (0 == strcmp(Prop, "top")) {
				j = TOP;
			}

			else if (0 == strcmp(Prop, "right")) {
				j = RIGHT;
			}

			else if (0 == strcmp(Prop, "bottom")) {
				j = BOTTOM;
			}

			else {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] '%s' is not a valid side.\n", Cur->line,
						Prop);
				exit(1);
			}
			ezxml_set_attr(Cur, "side", NULL);

			/* Check location is on perimeter */
			if ((TOP == j) && (i != (Type->height - 1))) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Locations are only allowed on large block "
								"perimeter. 'top' side should be at offset %d only.\n",
						Cur->line, (Type->height - 1));
				exit(1);
			}
			if ((BOTTOM == j) && (i != 0)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Locations are only allowed on large block "
								"perimeter. 'bottom' side should be at offset 0 only.\n",
						Cur->line);
				exit(1);
			}

			/* Go through lists of pins */
			CountTokensInString(Cur->txt, &Count, &Len);
			Type->num_pin_loc_assignments[i][j] = Count;
			if (Count > 0) {
				Tokens = GetNodeTokens(Cur);
				CurTokens = Tokens;
				Type->pin_loc_assignments[i][j] = (char**) my_calloc(Count,
						sizeof(char*));
				for (k = 0; k < Count; k++) {
					/* Store location assignment */
					Type->pin_loc_assignments[i][j][k] = my_strdup(*CurTokens);

					/* Advance through list of pins in this location */
					++CurTokens;
				}
				FreeTokens(&Tokens);
			}
			Prev = Cur;
			Cur = Cur->next;
			FreeNode(Prev);
		}
	}

	/* Setup pin classes */
	num_class = 0;
	for (i = 0; i < Type->pb_type->num_ports; i++) {
		if (Type->pb_type->ports[i].equivalent) {
			num_class += capacity;
		} else {
			num_class += capacity * Type->pb_type->ports[i].num_pins;
		}
	}
	Type->class_inf = (struct s_class*) my_calloc(num_class,
			sizeof(struct s_class));
	Type->num_class = num_class;
	Type->pin_class = (int*) my_malloc(Type->num_pins * sizeof(int) * capacity);
	Type->is_global_pin = (boolean*) my_malloc(
			Type->num_pins * sizeof(boolean) * capacity);
	for (i = 0; i < Type->num_pins * capacity; i++) {
		Type->pin_class[i] = OPEN;
		Type->is_global_pin[i] = (boolean) OPEN;
	}

	pin_count = 0;

	/* Equivalent pins share the same class, non-equivalent pins belong to different pin classes */
	num_class = 0;
	for (i = 0; i < capacity; ++i) {
		for (j = 0; j < Type->pb_type->num_ports; ++j) {
			if (Type->pb_type->ports[j].equivalent) {
				Type->class_inf[num_class].num_pins =
						Type->pb_type->ports[j].num_pins;
				Type->class_inf[num_class].pinlist = (int *) my_malloc(
						sizeof(int) * Type->pb_type->ports[j].num_pins);
			}

			for (k = 0; k < Type->pb_type->ports[j].num_pins; ++k) {
				if (!Type->pb_type->ports[j].equivalent) {
					Type->class_inf[num_class].num_pins = 1;
					Type->class_inf[num_class].pinlist = (int *) my_malloc(
							sizeof(int) * 1);
					Type->class_inf[num_class].pinlist[0] = pin_count;
				} else {
					Type->class_inf[num_class].pinlist[k] = pin_count;
				}

				if (Type->pb_type->ports[j].type == IN_PORT) {
					Type->class_inf[num_class].type = RECEIVER;
				} else {
					assert(Type->pb_type->ports[j].type == OUT_PORT);
					Type->class_inf[num_class].type = DRIVER;
				}
				Type->pin_class[pin_count] = num_class;
				Type->is_global_pin[pin_count] =
						(boolean) (Type->pb_type->ports[j].is_clock
								|| Type->pb_type->ports[j].is_non_clock_global);
				pin_count++;

				if (!Type->pb_type->ports[j].equivalent) {
					num_class++;
				}
			}
			if (Type->pb_type->ports[j].equivalent) {
				num_class++;
			}
		}
	}
	assert(num_class == Type->num_class);
	assert(pin_count == Type->num_pins);
}

/* Sets up the grid_loc_def for the type. Unlinks the loc nodes
 * from the XML tree. */
static void SetupGridLocations(ezxml_t Locations, t_type_descriptor * Type) {
	int i;

	ezxml_t Cur, Prev;
	const char *Prop;

	Type->num_grid_loc_def = CountChildren(Locations, "loc", 1);
	Type->grid_loc_def = (struct s_grid_loc_def *) my_calloc(
			Type->num_grid_loc_def, sizeof(struct s_grid_loc_def));

	/* Load the pin locations */
	Cur = Locations->child;
	i = 0;
	while (Cur) {
		CheckElement(Cur, "loc");

		/* loc index */
		Prop = FindProperty(Cur, "type", TRUE);
		if (Prop) {
			if (strcmp(Prop, "perimeter") == 0) {
				if (Type->num_grid_loc_def != 1) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] Another loc specified for perimeter.\n",
							Cur->line);
					exit(1);
				}
				Type->grid_loc_def[i].grid_loc_type = BOUNDARY;
				assert(IO_TYPE == Type);
				/* IO goes to boundary */
			} else if (strcmp(Prop, "fill") == 0) {
				if (Type->num_grid_loc_def != 1 || FILL_TYPE != NULL) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] Another loc specified for fill.\n",
							Cur->line);
					exit(1);
				}
				Type->grid_loc_def[i].grid_loc_type = FILL;
				FILL_TYPE = Type;
			} else if (strcmp(Prop, "col") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REPEAT;
			} else if (strcmp(Prop, "rel") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REL;
			} else {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Unknown grid location type '%s' for type '%s'.\n",
						Cur->line, Prop, Type->name);
				exit(1);
			}
			ezxml_set_attr(Cur, "type", NULL);
		}
		Prop = FindProperty(Cur, "start", FALSE);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] grid location property 'start' must be specified for grid location type 'col'.\n",
						Cur->line);
				exit(1);
			}
			Type->grid_loc_def[i].start_col = my_atoi(Prop);
			ezxml_set_attr(Cur, "start", NULL);
		} else if (Prop != NULL) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] grid location property 'start' valid for grid location type 'col' only.\n",
					Cur->line);
			exit(1);
		}
		Prop = FindProperty(Cur, "repeat", FALSE);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop != NULL) {
				Type->grid_loc_def[i].repeat = my_atoi(Prop);
				ezxml_set_attr(Cur, "repeat", NULL);
			}
		} else if (Prop != NULL) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] grid location property 'repeat' valid for grid location type 'col' only.\n",
					Cur->line);
			exit(1);
		}
		Prop = FindProperty(Cur, "pos", FALSE);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REL) {
			if (Prop == NULL) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] grid location property 'pos' must be specified for grid location type 'rel'.\n",
						Cur->line);
				exit(1);
			}
			Type->grid_loc_def[i].col_rel = (float) atof(Prop);
			ezxml_set_attr(Cur, "pos", NULL);
		} else if (Prop != NULL) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] grid location property 'pos' valid for grid location type 'rel' only.\n",
					Cur->line);
			exit(1);
		}

		Type->grid_loc_def[i].priority = GetIntProperty(Cur, "priority", FALSE,
				1);

		Prev = Cur;
		Cur = Cur->next;
		FreeNode(Prev);
		i++;
	}
}

static void ProcessPinToPinAnnotations(ezxml_t Parent,
		t_pin_to_pin_annotation *annotation) {
	int i = 0;
	const char *Prop;

	if (FindProperty(Parent, "max", FALSE)) {
		i++;
	}
	if (FindProperty(Parent, "min", FALSE)) {
		i++;
	}
	if (FindProperty(Parent, "type", FALSE)) {
		i++;
	}
	if (FindProperty(Parent, "value", FALSE)) {
		i++;
	}
	if (0 == strcmp(Parent->name, "C_constant")
			|| 0 == strcmp(Parent->name, "C_matrix")
			|| 0 == strcmp(Parent->name, "pack_pattern")) {
		i = 1;
	}

	annotation->num_value_prop_pairs = i;
	annotation->prop = (int*) my_calloc(i, sizeof(int));
	annotation->value = (char**) my_calloc(i, sizeof(char *));

	/* Todo: This is slow, I should use a case lookup */
	i = 0;
	if (0 == strcmp(Parent->name, "delay_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "max", FALSE);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MAX;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "max", NULL);
			i++;
		}
		Prop = FindProperty(Parent, "min", FALSE);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MIN;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "min", NULL);
			i++;
		}
		annotation->line_num = Parent->line;
		Prop = FindProperty(Parent, "in_port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", TRUE);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else if (0 == strcmp(Parent->name, "delay_matrix")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
		Prop = FindProperty(Parent, "type", TRUE);
		annotation->value[i] = my_strdup(Parent->txt);
		ezxml_set_txt(Parent, "");
		if (0 == strcmp(Prop, "max")) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MAX;
		} else {
			assert(0 == strcmp(Prop, "min"));
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MIN;
		}
		ezxml_set_attr(Parent, "type", NULL);
		i++;
		Prop = FindProperty(Parent, "in_port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", TRUE);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else if (0 == strcmp(Parent->name, "C_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "C", TRUE);
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "C", NULL);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
		i++;

		Prop = FindProperty(Parent, "in_port", FALSE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", FALSE);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
		assert(
				annotation->output_pins != NULL || annotation->input_pins != NULL);
	} else if (0 == strcmp(Parent->name, "C_matrix")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
		annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
		annotation->value[i] = my_strdup(Parent->txt);
		ezxml_set_txt(Parent, "");
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
		i++;
		Prop = FindProperty(Parent, "in_port", FALSE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", FALSE);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
		assert(
				annotation->output_pins != NULL || annotation->input_pins != NULL);
	} else if (0 == strcmp(Parent->name, "T_setup")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "value", TRUE);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_TSETUP;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "value", NULL);
		i++;
		Prop = FindProperty(Parent, "port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", TRUE);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);
	} else if (0 == strcmp(Parent->name, "T_clock_to_Q")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "max", FALSE);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "max", NULL);
			i++;
		}
		Prop = FindProperty(Parent, "min", FALSE);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "min", NULL);
			i++;
		}

		Prop = FindProperty(Parent, "port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", TRUE);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);
	} else if (0 == strcmp(Parent->name, "T_hold")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "value", TRUE);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_THOLD;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "value", NULL);
		i++;

		Prop = FindProperty(Parent, "port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", TRUE);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);
	} else if (0 == strcmp(Parent->name, "pack_pattern")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "name", TRUE);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
		i++;

		Prop = FindProperty(Parent, "in_port", TRUE);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", TRUE);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Unknown port type %s in %s in %s", Parent->line,
				Parent->name, Parent->parent->name,
				Parent->parent->parent->name);
		exit(1);
	}
	assert(i == annotation->num_value_prop_pairs);
}

static t_port * findPortByName(const char * name, t_pb_type * pb_type,
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
				&& strncmp(name, compare_to, bracket_pos)==0) {
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

static void ProcessPb_TypePowerPinToggle(ezxml_t parent, t_pb_type * pb_type) {
	ezxml_t cur, prev;
	const char * prop;
	t_port * port;
	int high, low;

	cur = FindFirstElement(parent, "port", FALSE);
	while (cur) {
		prop = FindProperty(cur, "name", TRUE);

		port = findPortByName(prop, pb_type, &high, &low);
		if (!port) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Could not find port '%s' needed for energy per toggle.",
					prop);
			return;
		}
		if (high != port->num_pins - 1 || low != 0) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Pin-toggle does not support pin indices (%s)", prop);
		}

		if (port->port_power->pin_toggle_initialized) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Duplicate pin-toggle energy for port '%s'", port->name);
		}
		port->port_power->pin_toggle_initialized = TRUE;
		ezxml_set_attr(cur, "name", NULL);

		/* Get energy per toggle */
		port->port_power->energy_per_toggle = GetFloatProperty(cur,
				"energy_per_toggle", TRUE, 0.);

		/* Get scaled by factor */
		boolean reverse_scaled = FALSE;
		prop = FindProperty(cur, "scaled_by_static_prob", FALSE);
		if (!prop) {
			prop = FindProperty(cur, "scaled_by_static_prob_n", FALSE);
			if (prop) {
				reverse_scaled = TRUE;
			}
		}

		if (prop) {
			port->port_power->scaled_by_port = findPortByName(prop, pb_type,
					&high, &low);
			if (high != low) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Pin-toggle 'scaled_by_static_prob' must be a single pin (%s)",
						prop);
				return;
			}
			port->port_power->scaled_by_port_pin_idx = high;
			port->port_power->reverse_scaled = reverse_scaled;
		}
		ezxml_set_attr(cur, "scaled_by_static_prob", NULL);
		ezxml_set_attr(cur, "scaled_by_static_prob_n", NULL);

		prev = cur;
		cur = cur->next;
		FreeNode(prev);
	}
}

static void ProcessPb_TypePower(ezxml_t Parent, t_pb_type * pb_type) {
	ezxml_t cur, child;
	boolean require_dynamic_absolute = FALSE;
	boolean require_static_absolute = FALSE;
	boolean require_dynamic_C_internal = FALSE;

	cur = FindFirstElement(Parent, "power", FALSE);
	if (!cur) {
		return;
	}

	switch (pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_TOGGLE_PINS:
		ProcessPb_TypePowerPinToggle(cur, pb_type);
		require_static_absolute = TRUE;
		break;
	case POWER_METHOD_C_INTERNAL:
		require_dynamic_C_internal = TRUE;
		require_static_absolute = TRUE;
		break;
	case POWER_METHOD_ABSOLUTE:
		require_dynamic_absolute = TRUE;
		require_static_absolute = TRUE;
		break;
	default:
		break;
	}

	if (require_static_absolute) {
		child = FindElement(cur, "static_power", TRUE);
		pb_type->pb_type_power->absolute_power_per_instance.leakage =
				GetFloatProperty(child, "power_per_instance", TRUE, 0.);
		FreeNode(child);
	}

	if (require_dynamic_absolute) {
		child = FindElement(cur, "dynamic_power", TRUE);
		pb_type->pb_type_power->absolute_power_per_instance.dynamic =
				GetFloatProperty(child, "power_per_instance", TRUE, 0.);
		FreeNode(child);
	}

	if (require_dynamic_C_internal) {
		child = FindElement(cur, "dynamic_power", TRUE);
		pb_type->pb_type_power->C_internal = GetFloatProperty(child,
				"C_internal", TRUE, 0.);
		FreeNode(child);
	}

	if (cur) {
		FreeNode(cur);
	}

}

static void ProcessPb_TypePowerEstMethod(ezxml_t Parent, t_pb_type * pb_type) {
	ezxml_t cur;
	const char * prop;

	e_power_estimation_method parent_power_method;

	prop = NULL;

	cur = FindFirstElement(Parent, "power", FALSE);
	if (cur) {
		prop = FindProperty(cur, "method", FALSE);
	}

	if (pb_type->parent_mode && pb_type->parent_mode->parent_pb_type) {
		parent_power_method =
				pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method;
	} else {
		parent_power_method = POWER_METHOD_AUTO_SIZES;
	}

	if (!prop) {
		/* default method is auto-size */
		pb_type->pb_type_power->estimation_method = power_method_inherited(
				parent_power_method);
	} else if (strcmp(prop, "auto-size") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_AUTO_SIZES;
	} else if (strcmp(prop, "specify-size") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_SPECIFY_SIZES;
	} else if (strcmp(prop, "pin-toggle") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_TOGGLE_PINS;
	} else if (strcmp(prop, "c-internal") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_C_INTERNAL;
	} else if (strcmp(prop, "absolute") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_ABSOLUTE;
	} else if (strcmp(prop, "ignore") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_IGNORE;
	} else if (strcmp(prop, "sum-of-children") == 0) {
		pb_type->pb_type_power->estimation_method = POWER_METHOD_SUM_OF_CHILDREN;
	} else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Invalid power estimation method for pb_type '%s'",
				pb_type->name);
	}

	if (prop) {
		ezxml_set_attr(cur, "method", NULL);
	}

}

/* Takes in a pb_type, allocates and loads data for it and recurses downwards */
static void ProcessPb_Type(INOUTP ezxml_t Parent, t_pb_type * pb_type,
		t_mode * mode) {
	int num_ports, i, j, k, num_annotations;
	const char *Prop;
	ezxml_t Cur, Prev;
	char* class_name;

	pb_type->parent_mode = mode;
	if (mode != NULL && mode->parent_pb_type != NULL) {
		pb_type->depth = mode->parent_pb_type->depth + 1;
		Prop = FindProperty(Parent, "name", TRUE);
		pb_type->name = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
	} else {
		pb_type->depth = 0;
		/* same name as type */
	}

	Prop = FindProperty(Parent, "blif_model", FALSE);
	pb_type->blif_model = my_strdup(Prop);
	ezxml_set_attr(Parent, "blif_model", NULL);

	pb_type->class_type = UNKNOWN_CLASS;
	Prop = FindProperty(Parent, "class", FALSE);
	class_name = my_strdup(Prop);

	if (class_name) {
		ezxml_set_attr(Parent, "class", NULL);
		if (0 == strcmp(class_name, "lut")) {
			pb_type->class_type = LUT_CLASS;
		} else if (0 == strcmp(class_name, "flipflop")) {
			pb_type->class_type = LATCH_CLASS;
		} else if (0 == strcmp(class_name, "memory")) {
			pb_type->class_type = MEMORY_CLASS;
		} else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Unknown class %s in pb_type %s\n", Parent->line,
					class_name, pb_type->name);
			exit(1);
		}
		free(class_name);
	}

	if (mode == NULL) {
		pb_type->num_pb = 1;
	} else {
		pb_type->num_pb = GetIntProperty(Parent, "num_pb", TRUE, 0);
	}

	assert(pb_type->num_pb > 0);
	num_ports = 0;
	num_ports += CountChildren(Parent, "input", 0);
	num_ports += CountChildren(Parent, "output", 0);
	num_ports += CountChildren(Parent, "clock", 0);
	pb_type->ports = (t_port*) my_calloc(num_ports, sizeof(t_port));
	pb_type->num_ports = num_ports;

	/* Initialize Power Structure */
	pb_type->pb_type_power = (t_pb_type_power*) my_calloc(1,
			sizeof(t_pb_type_power));
	ProcessPb_TypePowerEstMethod(Parent, pb_type);

	/* process ports */
	j = 0;
	for (i = 0; i < 3; i++) {
		if (i == 0) {
			k = 0;
			Cur = FindFirstElement(Parent, "input", FALSE);
		} else if (i == 1) {
			k = 0;
			Cur = FindFirstElement(Parent, "output", FALSE);
		} else {
			k = 0;
			Cur = FindFirstElement(Parent, "clock", FALSE);
		}
		while (Cur != NULL) {
			ProcessPb_TypePort(Cur, &pb_type->ports[j],
					pb_type->pb_type_power->estimation_method);
			pb_type->ports[j].parent_pb_type = pb_type;
			pb_type->ports[j].index = j;
			pb_type->ports[j].port_index_by_type = k;

			/* get next iteration */
			Prev = Cur;
			Cur = Cur->next;
			j++;
			k++;
			FreeNode(Prev);
		}
	}
	assert(j == num_ports);

	/* Count stats on the number of each type of pin */
	pb_type->num_clock_pins = pb_type->num_input_pins =
			pb_type->num_output_pins = 0;
	for (i = 0; i < pb_type->num_ports; i++) {
		if (pb_type->ports[i].type == IN_PORT
				&& pb_type->ports[i].is_clock == FALSE) {
			pb_type->num_input_pins += pb_type->ports[i].num_pins;
		} else if (pb_type->ports[i].type == OUT_PORT) {
			assert(pb_type->ports[i].is_clock == FALSE);
			pb_type->num_output_pins += pb_type->ports[i].num_pins;
		} else {
			assert(
					pb_type->ports[i].is_clock && pb_type->ports[i].type == IN_PORT);
			pb_type->num_clock_pins += pb_type->ports[i].num_pins;
		}
	}

	/* set max_internal_delay if exist */
	pb_type->max_internal_delay = UNDEFINED;
	Cur = FindElement(Parent, "max_internal_delay", FALSE);
	if (Cur) {
		pb_type->max_internal_delay = GetFloatProperty(Cur, "value", TRUE,
				UNDEFINED);
		FreeNode(Cur);
	}

	pb_type->annotations = NULL;
	pb_type->num_annotations = 0;
	i = 0;
	/* Determine if this is a leaf or container pb_type */
	if (pb_type->blif_model != NULL) {
		/* Process delay and capacitance annotations */
		num_annotations = 0;
		num_annotations += CountChildren(Parent, "delay_constant", 0);
		num_annotations += CountChildren(Parent, "delay_matrix", 0);
		num_annotations += CountChildren(Parent, "C_constant", 0);
		num_annotations += CountChildren(Parent, "C_matrix", 0);
		num_annotations += CountChildren(Parent, "T_setup", 0);
		num_annotations += CountChildren(Parent, "T_clock_to_Q", 0);
		num_annotations += CountChildren(Parent, "T_hold", 0);

		pb_type->annotations = (t_pin_to_pin_annotation*) my_calloc(
				num_annotations, sizeof(t_pin_to_pin_annotation));
		pb_type->num_annotations = num_annotations;

		j = 0;
		Cur = NULL;
		for (i = 0; i < 7; i++) {
			if (i == 0) {
				Cur = FindFirstElement(Parent, "delay_constant", FALSE);
			} else if (i == 1) {
				Cur = FindFirstElement(Parent, "delay_matrix", FALSE);
			} else if (i == 2) {
				Cur = FindFirstElement(Parent, "C_constant", FALSE);
			} else if (i == 3) {
				Cur = FindFirstElement(Parent, "C_matrix", FALSE);
			} else if (i == 4) {
				Cur = FindFirstElement(Parent, "T_setup", FALSE);
			} else if (i == 5) {
				Cur = FindFirstElement(Parent, "T_clock_to_Q", FALSE);
			} else if (i == 6) {
				Cur = FindFirstElement(Parent, "T_hold", FALSE);
			}
			while (Cur != NULL) {
				ProcessPinToPinAnnotations(Cur, &pb_type->annotations[j]);

				/* get next iteration */
				Prev = Cur;
				Cur = Cur->next;
				j++;
				FreeNode(Prev);
			}
		}
		assert(j == num_annotations);

		/* leaf pb_type, if special known class, then read class lib otherwise treat as primitive */
		if (pb_type->class_type == LUT_CLASS) {
			ProcessLutClass(pb_type);
		} else if (pb_type->class_type == MEMORY_CLASS) {
			ProcessMemoryClass(pb_type);
		} else {
			/* other leaf pb_type do not have modes */
			pb_type->num_modes = 0;
			assert(CountChildren(Parent, "mode", 0) == 0);
		}
	} else {
		boolean default_leakage_mode = FALSE;

		/* container pb_type, process modes */
		assert(pb_type->class_type == UNKNOWN_CLASS);
		pb_type->num_modes = CountChildren(Parent, "mode", 0);
		pb_type->pb_type_power->leakage_default_mode = 0;

		if (pb_type->num_modes == 0) {
			/* The pb_type operates in an implied one mode */
			pb_type->num_modes = 1;
			pb_type->modes = (t_mode*) my_calloc(pb_type->num_modes,
					sizeof(t_mode));
			pb_type->modes[i].parent_pb_type = pb_type;
			pb_type->modes[i].index = i;
			ProcessMode(Parent, &pb_type->modes[i], &default_leakage_mode);
			i++;
		} else {
			pb_type->modes = (t_mode*) my_calloc(pb_type->num_modes,
					sizeof(t_mode));

			Cur = FindFirstElement(Parent, "mode", TRUE);
			while (Cur != NULL) {
				if (0 == strcmp(Cur->name, "mode")) {
					pb_type->modes[i].parent_pb_type = pb_type;
					pb_type->modes[i].index = i;
					ProcessMode(Cur, &pb_type->modes[i], &default_leakage_mode);
					if (default_leakage_mode) {
						pb_type->pb_type_power->leakage_default_mode = i;
					}

					/* get next iteration */
					Prev = Cur;
					Cur = Cur->next;
					i++;
					FreeNode(Prev);
				}
			}
		}
		assert(i == pb_type->num_modes);
	}

	ProcessPb_TypePower(Parent, pb_type);

}

static void ProcessPb_TypePort_Power(ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method) {
	ezxml_t cur;
	const char * prop;
	bool wire_defined = FALSE;

	port->port_power = (t_port_power*) my_calloc(1, sizeof(t_port_power));

	//Defaults
	if (power_method == POWER_METHOD_AUTO_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_AUTO;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
	} else if (power_method == POWER_METHOD_SPECIFY_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_IGNORED;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_NONE;
	}

	cur = FindElement(Parent, "power", FALSE);

	if (cur) {
		/* Wire capacitance */

		/* Absolute C provided */
		prop = FindProperty(cur, "wire_capacitance", FALSE);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Wire capacitance defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = TRUE;
				port->port_power->wire_type = POWER_WIRE_TYPE_C;
				port->port_power->wire.C = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_capacitance", NULL);
		}

		/* Wire absolute length provided */
		prop = FindProperty(cur, "wire_length", FALSE);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Wire length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Multiple wire properties defined for port '%s', pb_type '%s'.",
						port->name, port->parent_pb_type->name);
			} else if (strcmp(prop, "auto") == 0) {
				wire_defined = TRUE;
				port->port_power->wire_type = POWER_WIRE_TYPE_AUTO;
			} else {
				wire_defined = TRUE;
				port->port_power->wire_type = POWER_WIRE_TYPE_ABSOLUTE_LENGTH;
				port->port_power->wire.absolute_length = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_length", NULL);
		}

		/* Wire relative length provided */
		prop = FindProperty(cur, "wire_relative_length", FALSE);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Wire relative length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Multiple wire properties defined for port '%s', pb_type '%s'.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = TRUE;
				port->port_power->wire_type = POWER_WIRE_TYPE_RELATIVE_LENGTH;
				port->port_power->wire.relative_length = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_relative_length", NULL);
		}

		/* Buffer Size */
		prop = FindProperty(cur, "buffer_size", FALSE);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"Buffer size defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (strcmp(prop, "auto") == 0) {
				port->port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
			} else {
				port->port_power->buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
				port->port_power->buffer_size = (float) atof(prop);
			}
			ezxml_set_attr(cur, "buffer_size", NULL);
		}

		FreeNode(cur);
	}
}

static void ProcessPb_TypePort(INOUTP ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method) {
	const char *Prop;
	Prop = FindProperty(Parent, "name", TRUE);
	port->name = my_strdup(Prop);
	ezxml_set_attr(Parent, "name", NULL);

	Prop = FindProperty(Parent, "port_class", FALSE);
	port->port_class = my_strdup(Prop);
	ezxml_set_attr(Parent, "port_class", NULL);

	Prop = FindProperty(Parent, "chain", FALSE);
	port->chain_name = my_strdup(Prop);
	ezxml_set_attr(Parent, "chain", NULL);

	port->equivalent = GetBooleanProperty(Parent, "equivalent", FALSE, FALSE);
	port->num_pins = GetIntProperty(Parent, "num_pins", TRUE, 0);
	port->is_non_clock_global = GetBooleanProperty(Parent,
			"is_non_clock_global", FALSE, FALSE);

	if (0 == strcmp(Parent->name, "input")) {
		port->type = IN_PORT;
		port->is_clock = FALSE;
	} else if (0 == strcmp(Parent->name, "output")) {
		port->type = OUT_PORT;
		port->is_clock = FALSE;
	} else if (0 == strcmp(Parent->name, "clock")) {
		port->type = IN_PORT;
		port->is_clock = TRUE;
		if (port->is_non_clock_global == TRUE) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Port %s cannot be both a clock and a non-clock simultaneously\n",
					Parent->line, Parent->name);
		}
	} else {
		vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Unknown port type %s",
				Parent->line, Parent->name);
		exit(1);
	}

	ProcessPb_TypePort_Power(Parent, port, power_method);
}

static void ProcessInterconnect(INOUTP ezxml_t Parent, t_mode * mode) {
	int num_interconnect = 0;
	int i, j, k, L_index, num_annotations;
	const char *Prop;
	ezxml_t Cur, Prev;
	ezxml_t Cur2, Prev2;

	num_interconnect += CountChildren(Parent, "complete", 0);
	num_interconnect += CountChildren(Parent, "direct", 0);
	num_interconnect += CountChildren(Parent, "mux", 0);

	mode->num_interconnect = num_interconnect;
	mode->interconnect = (t_interconnect*) my_calloc(num_interconnect,
			sizeof(t_interconnect));

	i = 0;
	for (L_index = 0; L_index < 3; L_index++) {
		if (L_index == 0) {
			Cur = FindFirstElement(Parent, "complete", FALSE);
		} else if (L_index == 1) {
			Cur = FindFirstElement(Parent, "direct", FALSE);
		} else {
			Cur = FindFirstElement(Parent, "mux", FALSE);
		}
		while (Cur != NULL) {
			if (0 == strcmp(Cur->name, "complete")) {
				mode->interconnect[i].type = COMPLETE_INTERC;
			} else if (0 == strcmp(Cur->name, "direct")) {
				mode->interconnect[i].type = DIRECT_INTERC;
			} else {
				assert(0 == strcmp(Cur->name, "mux"));
				mode->interconnect[i].type = MUX_INTERC;
			}

			mode->interconnect[i].line_num = Cur->line;

			mode->interconnect[i].parent_mode_index = mode->index;
			mode->interconnect[i].parent_mode = mode;

			Prop = FindProperty(Cur, "input", TRUE);
			mode->interconnect[i].input_string = my_strdup(Prop);
			ezxml_set_attr(Cur, "input", NULL);

			Prop = FindProperty(Cur, "output", TRUE);
			mode->interconnect[i].output_string = my_strdup(Prop);
			ezxml_set_attr(Cur, "output", NULL);

			Prop = FindProperty(Cur, "name", TRUE);
			mode->interconnect[i].name = my_strdup(Prop);
			ezxml_set_attr(Cur, "name", NULL);

			/* Process delay and capacitance annotations */
			num_annotations = 0;
			num_annotations += CountChildren(Cur, "delay_constant", 0);
			num_annotations += CountChildren(Cur, "delay_matrix", 0);
			num_annotations += CountChildren(Cur, "C_constant", 0);
			num_annotations += CountChildren(Cur, "C_matrix", 0);
			num_annotations += CountChildren(Cur, "pack_pattern", 0);

			mode->interconnect[i].annotations =
					(t_pin_to_pin_annotation*) my_calloc(num_annotations,
							sizeof(t_pin_to_pin_annotation));
			mode->interconnect[i].num_annotations = num_annotations;

			k = 0;
			Cur2 = NULL;
			for (j = 0; j < 5; j++) {
				if (j == 0) {
					Cur2 = FindFirstElement(Cur, "delay_constant", FALSE);
				} else if (j == 1) {
					Cur2 = FindFirstElement(Cur, "delay_matrix", FALSE);
				} else if (j == 2) {
					Cur2 = FindFirstElement(Cur, "C_constant", FALSE);
				} else if (j == 3) {
					Cur2 = FindFirstElement(Cur, "C_matrix", FALSE);
				} else if (j == 4) {
					Cur2 = FindFirstElement(Cur, "pack_pattern", FALSE);
				}
				while (Cur2 != NULL) {
					ProcessPinToPinAnnotations(Cur2,
							&(mode->interconnect[i].annotations[k]));

					/* get next iteration */
					Prev2 = Cur2;
					Cur2 = Cur2->next;
					k++;
					FreeNode(Prev2);
				}
			}
			assert(k == num_annotations);

			/* Power */
			mode->interconnect[i].interconnect_power =
					(t_interconnect_power*) my_calloc(1,
							sizeof(t_interconnect_power));
			mode->interconnect[i].interconnect_power->port_info_initialized =
					FALSE;

			//ProcessInterconnectMuxArch(Cur, &mode->interconnect[i]);

			/* get next iteration */
			Prev = Cur;
			Cur = Cur->next;
			FreeNode(Prev);
			i++;
		}
	}

	assert(i == num_interconnect);
}

static void ProcessMode(INOUTP ezxml_t Parent, t_mode * mode,
		boolean * default_leakage_mode) {
	int i;
	const char *Prop;
	ezxml_t Cur, Prev;

	if (0 == strcmp(Parent->name, "pb_type")) {
		/* implied mode */
		mode->name = my_strdup(mode->parent_pb_type->name);
	} else {
		Prop = FindProperty(Parent, "name", TRUE);
		mode->name = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
	}

	mode->num_pb_type_children = CountChildren(Parent, "pb_type", 0);
	if (mode->num_pb_type_children > 0) {
		mode->pb_type_children = (t_pb_type*) my_calloc(
				mode->num_pb_type_children, sizeof(t_pb_type));

		i = 0;
		Cur = FindFirstElement(Parent, "pb_type", TRUE);
		while (Cur != NULL) {
			if (0 == strcmp(Cur->name, "pb_type")) {
				ProcessPb_Type(Cur, &mode->pb_type_children[i], mode);

				/* get next iteration */
				Prev = Cur;
				Cur = Cur->next;
				i++;
				FreeNode(Prev);
			}
		}
	} else {
		mode->pb_type_children = NULL;
	}

	/* Allocate power structure */
	mode->mode_power = (t_mode_power*) my_calloc(1, sizeof(t_mode_power));

	Cur = FindElement(Parent, "interconnect", TRUE);
	ProcessInterconnect(Cur, mode);
	FreeNode(Cur);
}

/* Takes in the node ptr for the 'fc_in' and 'fc_out' elements and initializes
 * the appropriate fields of type. Unlinks the contents of the nodes. */
static void Process_Fc(ezxml_t Node, t_type_descriptor * Type) {
	enum Fc_type def_type_in, def_type_out, ovr_type;
	const char *Prop, *Prop2;
	char *port_name;
	float def_in_val, def_out_val, ovr_val;
	int ipin, iclass, end_pin_index, start_pin_index, match_count;
	int iport, iport_pin, curr_pin, port_found;
	ezxml_t Child, Junk;

	def_type_in = FC_FRAC;
	def_type_out = FC_FRAC;
	def_in_val = OPEN;
	def_out_val = OPEN;

	Type->is_Fc_frac = (boolean *) my_malloc(Type->num_pins * sizeof(boolean));
	Type->is_Fc_full_flex = (boolean *) my_malloc(
			Type->num_pins * sizeof(boolean));
	Type->Fc = (float *) my_malloc(Type->num_pins * sizeof(float));

	/* Load the default fc_in, if specified */
	Prop = FindProperty(Node, "default_in_type", FALSE);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_in = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_in = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_in = FC_FULL;
		} else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Invalid type '%s' for Fc. Only abs, frac "
							"and full are allowed.\n", Node->line, Prop);
			exit(1);
		}
		switch (def_type_in) {
		case FC_FULL:
			def_in_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = FindProperty(Node, "default_in_val", TRUE);
			def_in_val = (float) atof(Prop2);
			ezxml_set_attr(Node, "default_in_val", NULL);
			break;
		default:
			def_in_val = -1;
		}
		/* Release the property */
		ezxml_set_attr(Node, "default_in_type", NULL);
	}

	/* Load the default fc_out, if specified */
	Prop = FindProperty(Node, "default_out_type", FALSE);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_out = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_out = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_out = FC_FULL;
		} else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Invalid type '%s' for Fc. Only abs, frac "
							"and full are allowed.\n", Node->line, Prop);
			exit(1);
		}
		switch (def_type_out) {
		case FC_FULL:
			def_out_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = FindProperty(Node, "default_out_val", TRUE);
			def_out_val = (float) atof(Prop2);
			ezxml_set_attr(Node, "default_out_val", NULL);
			break;
		default:
			def_out_val = -1;
		}
		/* Release the property */
		ezxml_set_attr(Node, "default_out_type", NULL);
	}

	/* Go though all the pins in Type, assign def_in_val and def_out_val     *
	 * to entries in Type->Fc array corresponding to input pins and output   *
	 * pins. Also sets up the type of fc of the pin in the boolean arrays    */
	for (ipin = 0; ipin < Type->num_pins; ipin++) {
		iclass = Type->pin_class[ipin];
		if (Type->class_inf[iclass].type == DRIVER) {
			Type->Fc[ipin] = def_out_val;
			Type->is_Fc_full_flex[ipin] =
					(def_type_out == FC_FULL) ? TRUE : FALSE;
			Type->is_Fc_frac[ipin] = (def_type_out == FC_FRAC) ? TRUE : FALSE;
		} else if (Type->class_inf[iclass].type == RECEIVER) {
			Type->Fc[ipin] = def_in_val;
			Type->is_Fc_full_flex[ipin] =
					(def_type_in == FC_FULL) ? TRUE : FALSE;
			Type->is_Fc_frac[ipin] = (def_type_in == FC_FRAC) ? TRUE : FALSE;
		} else {
			Type->Fc[ipin] = -1;
			Type->is_Fc_full_flex[ipin] = FALSE;
			Type->is_Fc_frac[ipin] = FALSE;
		}
	}

	/* Now, check for pin-based fc override - look for pin child. */
	Child = ezxml_child(Node, "pin");
	while (Child != NULL) {
		/* Get all the properties of the child first */
		Prop = FindProperty(Child, "name", TRUE);
		if (Prop == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] Pin child with no name "
					"is not allowed.\n", Child->line);
			exit(1);
		}
		ezxml_set_attr(Child, "name", NULL);

		Prop2 = FindProperty(Child, "fc_type", TRUE);
		if (Prop2 != NULL) {
			if (0 == strcmp(Prop2, "abs")) {
				ovr_type = FC_ABS;
			} else if (0 == strcmp(Prop2, "frac")) {
				ovr_type = FC_FRAC;
			} else if (0 == strcmp(Prop2, "full")) {
				ovr_type = FC_FULL;
			} else {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Invalid type '%s' for Fc. Only abs, frac "
								"and full are allowed.\n", Child->line, Prop2);
				exit(1);
			}
			switch (ovr_type) {
			case FC_FULL:
				ovr_val = 0.0;
				break;
			case FC_ABS:
			case FC_FRAC:
				Prop2 = FindProperty(Child, "fc_val", TRUE);
				if (Prop2 == NULL) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] Pin child with no fc_val specified "
									"is not allowed.\n", Child->line);
					exit(1);
				}
				ovr_val = (float) atof(Prop2);
				ezxml_set_attr(Child, "fc_val", NULL);
				break;
			default:
				ovr_val = -1;
			}
			/* Release the property */
			ezxml_set_attr(Child, "fc_type", NULL);

			port_name = NULL;

			/* Search for the child pin in Type and overwrites the default values     */
			/* Check whether the name is in the format of "<port_name>" or            *
			 * "<port_name> [start_index:end_index]" by looking for the symbol '['    */
			Prop2 = strstr(Prop, "[");
			if (Prop2 == NULL) {
				/* Format "port_name" , Prop stores the port_name */
				end_pin_index = start_pin_index = -1;
			} else {
				/* Format "port_name [start_index:end_index]" */
				match_count = sscanf(Prop, "%s [%d:%d]", port_name,
						&end_pin_index, &start_pin_index);
				Prop = port_name;
				if (match_count != 3
						|| (match_count != 1 && port_name == NULL)) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] Invalid name for pin child, "
									"name should be in the format \"port_name\" or "
									"\"port_name [end_pin_index:start_pin_index]\", "
									" The end_pin_index and start_pin_index can be the same.\n",
							Child->line);
					exit(1);
				}
				if (end_pin_index < 0 || start_pin_index < 0) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] The pin_index should not "
									"be a negative value.\n", Child->line);
					exit(1);
				}
				if (end_pin_index < start_pin_index) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] The end_pin_index should "
									"be not be less than start_pin_index.\n",
							Child->line);
					exit(1);
				}
			}

			/* Find the matching port_name in Type */
			/* TODO: Check for pins assigned more than one override fc's - right now assigning the last value specified. */
			iport_pin = 0;
			port_found = FALSE;
			for (iport = 0;
					((iport < Type->pb_type->num_ports) && (port_found == FALSE));
					iport++) {
				if (strcmp(Prop, Type->pb_type->ports[iport].name) == 0) {
					/* This is the port, the start_pin_index and end_pin_index offset starts
					 * here. The indices are inclusive. */
					port_found = TRUE;
					if (end_pin_index > Type->pb_type->ports[iport].num_pins) {
						vpr_printf(TIO_MESSAGE_ERROR,
								"[LINE %d] The end_pin_index for this port: %d "
										"cannot be greater than the number of pins in this port: %d.\n",
								Child->line, end_pin_index,
								Type->pb_type->ports[iport].num_pins);
						exit(1);
					}

					// The pin indices is not specified - override whole port.
					if (end_pin_index == -1 && start_pin_index == -1) {
						start_pin_index = 0;

						// Minus one since it is going to be assessed inclusively.
						end_pin_index = Type->pb_type->ports[iport].num_pins
								- 1;
					}

					/* Go through the pins in the port from start_pin_index to end_pin_index
					 * and overwrite the default fc_val and fc_type with the values parsed in
					 * from above. */
					for (curr_pin = start_pin_index; curr_pin <= end_pin_index;
							curr_pin++) {

						// Check whether the value had been overwritten
						if (ovr_val != Type->Fc[iport_pin + curr_pin]
									|| Type->is_Fc_full_flex[iport_pin
											+ curr_pin]
											!= (ovr_type == FC_FULL) ? TRUE :
							FALSE
									|| Type->is_Fc_frac[iport_pin + curr_pin]
											!= (ovr_type == FC_FRAC) ?
									TRUE : FALSE) {
							Type->Fc[iport_pin + curr_pin] = ovr_val;
							Type->is_Fc_full_flex[iport_pin + curr_pin] =
									(ovr_type == FC_FULL) ? TRUE : FALSE;
							Type->is_Fc_frac[iport_pin + curr_pin] =
									(ovr_type == FC_FRAC) ? TRUE : FALSE;

						} else {

							vpr_printf(TIO_MESSAGE_ERROR,
									"[LINE %d] Multiple Fc override detected!\n",
									Child->line);
							exit(1);

						}
					}

				} else {
					/* This is not the matching port, move the iport_pin index forward. */
					iport_pin += Type->pb_type->ports[iport].num_pins;
				}
			} /* Finish going through all the ports in pb_type looking for the pin child's port. */

			/* The override pin child is not in any of the ports in pb_type. */
			if (port_found == FALSE) {
				vpr_printf(TIO_MESSAGE_ERROR, "[LINE %d] The port \"%s\" "
						"cannot be found.\n", Child->line);
				exit(1);
			}

			/* End of case where fc_type of pin_child is specified. */
		} else {
			/* fc_type of pin_child is not specified. Error out. */
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Pin child with no fc_type specified "
							"is not allowed.\n", Child->line);
			exit(1);
		}

		/* Find next child and frees up the current child. */
		Junk = Child;
		Child = ezxml_next(Child);
		FreeNode(Junk);

	} /* End of processing pin children */

}

/* Thie processes attributes of the 'type' tag and then unlinks them */
static void ProcessComplexBlockProps(ezxml_t Node, t_type_descriptor * Type) {
	const char *Prop;

	/* Load type name */
	Prop = FindProperty(Node, "name", TRUE);
	Type->name = my_strdup(Prop);
	ezxml_set_attr(Node, "name", NULL);

	/* Load properties */
	Type->capacity = GetIntProperty(Node, "capacity", FALSE, 1); /* TODO: Any block with capacity > 1 that is not I/O has not been tested, must test */
	Type->height = GetIntProperty(Node, "height", FALSE, 1);
	Type->area = GetFloatProperty(Node, "area", FALSE, UNDEFINED);

	if (atof(Prop) < 0) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Area for type %s must be non-negative\n", Node->line,
				Type->name);
		exit(1);
	}
}

/* Takes in node pointing to <models> and loads all the
 * child type objects. Unlinks the entire <models> node
 * when complete. */
static void ProcessModels(INOUTP ezxml_t Node, OUTP struct s_arch *arch) {
	const char *Prop;
	ezxml_t child;
	ezxml_t p;
	ezxml_t junk;
	ezxml_t junkp;
	t_model *temp;
	t_model_ports *tp;
	int L_index;

	L_index = NUM_MODELS_IN_LIBRARY;

	arch->models = NULL;
	child = ezxml_child(Node, "model");
	while (child != NULL) {
		temp = (t_model*) my_calloc(1, sizeof(t_model));
		temp->used = 0;
		temp->inputs = temp->outputs = NULL;
		temp->instances = NULL;
		Prop = FindProperty(child, "name", TRUE);
		temp->name = my_strdup(Prop);
		ezxml_set_attr(child, "name", NULL);
		temp->pb_types = NULL;
		temp->index = L_index;
		L_index++;

		/* Process the inputs */
		p = ezxml_child(child, "input_ports");
		junkp = p;
		if (p == NULL)
			vpr_printf(TIO_MESSAGE_ERROR,
					"Required input ports not found for element '%s'.\n",
					temp->name);

		p = ezxml_child(p, "port");
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) my_calloc(1, sizeof(t_model_ports));
				Prop = FindProperty(p, "name", TRUE);
				tp->name = my_strdup(Prop);
				ezxml_set_attr(p, "name", NULL);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->inputs;
				tp->dir = IN_PORT;
				tp->is_non_clock_global = GetBooleanProperty(p,
						"is_non_clock_global", FALSE, FALSE);
				tp->is_clock = FALSE;
				Prop = FindProperty(p, "is_clock", FALSE);
				if (Prop && my_atoi(Prop) != 0) {
					tp->is_clock = TRUE;
				}
				ezxml_set_attr(p, "is_clock", NULL);
				if (tp->is_clock == TRUE && tp->is_non_clock_global == TRUE) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] Signal cannot be both a clock and a non-clock signal simultaneously\n",
							p->line);
				}
				temp->inputs = tp;
				junk = p;
				p = ezxml_next(p);
				FreeNode(junk);
			}
		} else /* No input ports? */
		{
			vpr_printf(TIO_MESSAGE_ERROR,
					"Required input ports not found for element '%s'.\n",
					temp->name);
		}
		FreeNode(junkp);

		/* Process the outputs */
		p = ezxml_child(child, "output_ports");
		junkp = p;
		if (p == NULL)
			vpr_printf(TIO_MESSAGE_ERROR,
					"Required output ports not found for element '%s'.\n",
					temp->name);

		p = ezxml_child(p, "port");
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) my_calloc(1, sizeof(t_model_ports));
				Prop = FindProperty(p, "name", TRUE);
				tp->name = my_strdup(Prop);
				ezxml_set_attr(p, "name", NULL);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->outputs;
				tp->dir = OUT_PORT;
				temp->outputs = tp;
				junk = p;
				p = ezxml_next(p);
				FreeNode(junk);
			}
		} else /* No output ports? */
		{
			vpr_printf(TIO_MESSAGE_ERROR,
					"Required output ports not found for element '%s'.\n",
					temp->name);
		}
		FreeNode(junkp);

		/* Find the next model */
		temp->next = arch->models;
		arch->models = temp;
		junk = child;
		child = ezxml_next(child);
		FreeNode(junk);
	}

	return;
}

/* Takes in node pointing to <layout> and loads all the
 * child type objects. Unlinks the entire <layout> node
 * when complete. */
static void ProcessLayout(INOUTP ezxml_t Node, OUTP struct s_arch *arch) {
	const char *Prop;

	arch->clb_grid.IsAuto = TRUE;

	/* Load width and height if applicable */
	Prop = FindProperty(Node, "width", FALSE);
	if (Prop != NULL) {
		arch->clb_grid.IsAuto = FALSE;
		arch->clb_grid.W = my_atoi(Prop);
		ezxml_set_attr(Node, "width", NULL);

		arch->clb_grid.H = GetIntProperty(Node, "height", TRUE, UNDEFINED);
	}

	/* Load aspect ratio if applicable */
	Prop = FindProperty(Node, "auto", arch->clb_grid.IsAuto);
	if (Prop != NULL) {
		if (arch->clb_grid.IsAuto == FALSE) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Auto-sizing, width and height cannot be specified\n");
		}
		arch->clb_grid.Aspect = (float) atof(Prop);
		ezxml_set_attr(Node, "auto", NULL);
		if (arch->clb_grid.Aspect <= 0) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"Grid aspect ratio is less than or equal to zero %g\n",
					arch->clb_grid.Aspect);
		}
	}
}

/* Takes in node pointing to <device> and loads all the
 * child type objects. Unlinks the entire <device> node
 * when complete. */
static void ProcessDevice(INOUTP ezxml_t Node, OUTP struct s_arch *arch,
		INP boolean timing_enabled) {
	const char *Prop;
	ezxml_t Cur;

	Cur = FindElement(Node, "sizing", TRUE);
	arch->R_minW_nmos = GetFloatProperty(Cur, "R_minW_nmos", timing_enabled, 0);
	arch->R_minW_pmos = GetFloatProperty(Cur, "R_minW_pmos", timing_enabled, 0);
	arch->ipin_mux_trans_size = GetFloatProperty(Cur, "ipin_mux_trans_size",
			FALSE, 0);
	FreeNode(Cur);

	Cur = FindElement(Node, "timing", timing_enabled);
	if (Cur != NULL) {
		arch->C_ipin_cblock = GetFloatProperty(Cur, "C_ipin_cblock", FALSE, 0);
		arch->T_ipin_cblock = GetFloatProperty(Cur, "T_ipin_cblock", FALSE, 0);
		FreeNode(Cur);
	}

	Cur = FindElement(Node, "area", TRUE);
	arch->grid_logic_tile_area = GetFloatProperty(Cur, "grid_logic_tile_area",
			FALSE, 0);
	FreeNode(Cur);

	Cur = FindElement(Node, "chan_width_distr", FALSE);
	if (Cur != NULL) {
		ProcessChanWidthDistr(Cur, arch);
		FreeNode(Cur);
	}

	Cur = FindElement(Node, "switch_block", TRUE);
	Prop = FindProperty(Cur, "type", TRUE);
	if (strcmp(Prop, "wilton") == 0) {
		arch->SBType = WILTON;
	} else if (strcmp(Prop, "universal") == 0) {
		arch->SBType = UNIVERSAL;
	} else if (strcmp(Prop, "subset") == 0) {
		arch->SBType = SUBSET;
	} else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Unknown property %s for switch block type x\n",
				Cur->line, Prop);
		exit(1);
	}
	ezxml_set_attr(Cur, "type", NULL);

	arch->Fs = GetIntProperty(Cur, "fs", TRUE, 3);

	FreeNode(Cur);
}

/* Takes in node pointing to <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire <chan_width_distr> node
 * when complete. */
static void ProcessChanWidthDistr(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch) {
	ezxml_t Cur;

	Cur = FindElement(Node, "io", TRUE);
	arch->Chans.chan_width_io = GetFloatProperty(Cur, "width", TRUE, UNDEFINED);
	FreeNode(Cur);
	Cur = FindElement(Node, "x", TRUE);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_x_dist);
	FreeNode(Cur);
	Cur = FindElement(Node, "y", TRUE);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_y_dist);
	FreeNode(Cur);
}

/* Takes in node within <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire node when complete. */
static void ProcessChanWidthDistrDir(INOUTP ezxml_t Node, OUTP t_chan * chan) {
	const char *Prop;

	boolean hasXpeak, hasWidth, hasDc;
	hasXpeak = hasWidth = hasDc = FALSE;
	Prop = FindProperty(Node, "distr", TRUE);
	if (strcmp(Prop, "uniform") == 0) {
		chan->type = UNIFORM;
	} else if (strcmp(Prop, "gaussian") == 0) {
		chan->type = GAUSSIAN;
		hasXpeak = hasWidth = hasDc = TRUE;
	} else if (strcmp(Prop, "pulse") == 0) {
		chan->type = PULSE;
		hasXpeak = hasWidth = hasDc = TRUE;
	} else if (strcmp(Prop, "delta") == 0) {
		hasXpeak = hasDc = TRUE;
		chan->type = DELTA;
	} else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] Unknown property %s for chan_width_distr x\n",
				Node->line, Prop);
		exit(1);
	}
	ezxml_set_attr(Node, "distr", NULL);
	chan->peak = GetFloatProperty(Node, "peak", TRUE, UNDEFINED);
	chan->width = GetFloatProperty(Node, "width", hasWidth, 0);
	chan->xpeak = GetFloatProperty(Node, "xpeak", hasXpeak, 0);
	chan->dc = GetFloatProperty(Node, "dc", hasDc, 0);
}

static void SetupEmptyType(void) {
	t_type_descriptor * type;
	type = &cb_type_descriptors[EMPTY_TYPE->index];
	type->name = "<EMPTY>";
	type->num_pins = 0;
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

static void alloc_and_load_default_child_for_pb_type( INOUTP t_pb_type *pb_type,
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
	lut_pb_type->modes[1].interconnect[0].infer_annotations = TRUE;

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
	lut_pb_type->modes[1].interconnect[1].infer_annotations = TRUE;

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
static void ProcessMemoryClass(INOUTP t_pb_type *mem_pb_type) {
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
				vpr_printf(TIO_MESSAGE_ERROR,
						"memory %s has inconsistent number of data bits %d and %d\n",
						mem_pb_type->name, num_pb,
						mem_pb_type->ports[i].num_pins);
				exit(1);
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

/* Takes in node pointing to <typelist> and loads all the
 * child type objects. Unlinks the entire <typelist> node
 * when complete. */
static void ProcessComplexBlocks(INOUTP ezxml_t Node,
		OUTP t_type_descriptor ** Types, OUTP int *NumTypes,
		boolean timing_enabled) {
	ezxml_t CurType, Prev;
	ezxml_t Cur;
	t_type_descriptor * Type;
	int i;

	/* Alloc the type list. Need one additional t_type_desctiptors:
	 * 1: empty psuedo-type
	 */
	*NumTypes = CountChildren(Node, "pb_type", 1) + 1;
	*Types = (t_type_descriptor *) my_malloc(
			sizeof(t_type_descriptor) * (*NumTypes));

	cb_type_descriptors = *Types;

	EMPTY_TYPE = &cb_type_descriptors[EMPTY_TYPE_INDEX];
	IO_TYPE = &cb_type_descriptors[IO_TYPE_INDEX];
	cb_type_descriptors[EMPTY_TYPE_INDEX].index = EMPTY_TYPE_INDEX;
	cb_type_descriptors[IO_TYPE_INDEX].index = IO_TYPE_INDEX;
	SetupEmptyType();

	/* Process the types */
	/* TODO: I should make this more flexible but release is soon and I don't have time so assert values for empty and io types*/
	assert(EMPTY_TYPE_INDEX == 0);
	assert(IO_TYPE_INDEX == 1);
	i = 1; /* Skip over 'empty' type */
	CurType = Node->child;
	while (CurType) {
		CheckElement(CurType, "pb_type");

		/* Alias to current type */
		Type = &(*Types)[i];

		/* Parses the properties fields of the type */
		ProcessComplexBlockProps(CurType, Type);

		/* Load pb_type info */
		Type->pb_type = (t_pb_type*) my_malloc(sizeof(t_pb_type));
		Type->pb_type->name = my_strdup(Type->name);
		if (i == IO_TYPE_INDEX) {
			if (strcmp(Type->name, "io") != 0) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"First complex block must be named \"io\" and define the inputs and outputs for the FPGA");
				exit(1);
			}
		}
		ProcessPb_Type(CurType, Type->pb_type, NULL);
		Type->num_pins = Type->capacity
				* (Type->pb_type->num_input_pins
						+ Type->pb_type->num_output_pins
						+ Type->pb_type->num_clock_pins);
		Type->num_receivers = Type->capacity * Type->pb_type->num_input_pins;
		Type->num_drivers = Type->capacity * Type->pb_type->num_output_pins;

		/* Load pin names and classes and locations */
		Cur = FindElement(CurType, "pinlocations", TRUE);
		SetupPinLocationsAndPinClasses(Cur, Type);
		FreeNode(Cur);
		Cur = FindElement(CurType, "gridlocations", TRUE);
		SetupGridLocations(Cur, Type);
		FreeNode(Cur);

		/* Load Fc */
		Cur = FindElement(CurType, "fc", TRUE);
		Process_Fc(Cur, Type);
		FreeNode(Cur);

#if 0
		Cur = FindElement(CurType, "timing", timing_enabled);
		if (Cur)
		{
			SetupTypeTiming(Cur, Type);
			FreeNode(Cur);
		}
#endif
		Type->index = i;

		/* Type fully read */
		++i;

		/* Free this node and get its next sibling node */
		Prev = CurType;
		CurType = CurType->next;
		FreeNode(Prev);

	}
	if (FILL_TYPE == NULL) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"grid location type 'fill' must be specified.\n");
		exit(1);
	}
}

/* Loads the given architecture file. Currently only
 * handles type information */
void XmlReadArch(INP const char *ArchFile, INP boolean timing_enabled,
		OUTP struct s_arch *arch, OUTP t_type_descriptor ** Types,
		OUTP int *NumTypes) {
	ezxml_t Cur, Next;
	const char *Prop;
	boolean power_reqd;

	/* Parse the file */
	Cur = ezxml_parse_file(ArchFile);
	if (NULL == Cur) {
		vpr_printf(TIO_MESSAGE_ERROR,
				"Unable to load architecture file '%s'.\n", ArchFile);
		exit(1);
	}

	/* Root node should be architecture */
	CheckElement(Cur, "architecture");
	/* TODO: do version processing properly with string delimiting on the . */
	Prop = FindProperty(Cur, "version", FALSE);
	if (Prop != NULL) {
		if (atof(Prop) > atof(VPR_VERSION)) {
			vpr_printf(TIO_MESSAGE_WARNING,
					"This architecture version is for VPR %f while your current VPR version is " VPR_VERSION ", compatability issues may arise\n",
					atof(Prop));
		}
		ezxml_set_attr(Cur, "version", NULL);
	}

	/* Process models */
	Next = FindElement(Cur, "models", TRUE);
	ProcessModels(Next, arch);
	FreeNode(Next);
	CreateModelLibrary(arch);

	/* Process layout */
	Next = FindElement(Cur, "layout", TRUE);
	ProcessLayout(Next, arch);
	FreeNode(Next);

	/* Process device */
	Next = FindElement(Cur, "device", TRUE);
	ProcessDevice(Next, arch, timing_enabled);
	FreeNode(Next);

	/* Process types */
	Next = FindElement(Cur, "complexblocklist", TRUE);
	ProcessComplexBlocks(Next, Types, NumTypes, timing_enabled);
	FreeNode(Next);

	/* Process switches */
	Next = FindElement(Cur, "switchlist", TRUE);
	ProcessSwitches(Next, &(arch->Switches), &(arch->num_switches),
			timing_enabled);
	FreeNode(Next);

	/* Process segments. This depends on switches */
	Next = FindElement(Cur, "segmentlist", TRUE);
	ProcessSegments(Next, &(arch->Segments), &(arch->num_segments),
			arch->Switches, arch->num_switches, timing_enabled);
	FreeNode(Next);

	/* Process directs */
	Next = FindElement(Cur, "directlist", FALSE);
	if (Next) {
		ProcessDirects(Next, &(arch->Directs), &(arch->num_directs),
				timing_enabled);
		FreeNode(Next);
	}

	/* Process architecture power information */

	/* If arch->power has been initialized, meaning the user has requested power estimation,
	 * then the power architecture information is required.
	 */
	if (arch->power) {
		power_reqd = TRUE;
	} else {
		power_reqd = FALSE;
	}

	Next = FindElement(Cur, "power", power_reqd);
	if (Next) {
		if (arch->power) {
			ProcessPower(Next, arch->power, *Types, *NumTypes);
		} else {
			/* This information still needs to be read, even if it is just
			 * thrown away.
			 */
			t_power_arch * power_arch_fake = (t_power_arch*) my_calloc(1,
					sizeof(t_power_arch));
			ProcessPower(Next, power_arch_fake, *Types, *NumTypes);
			free(power_arch_fake);
		}
		FreeNode(Next);
	}

// Process Clocks
	Next = FindElement(Cur, "clocks", power_reqd);
	if (Next) {
		if (arch->clocks) {
			ProcessClocks(Next, arch->clocks);
		} else {
			/* This information still needs to be read, even if it is just
			 * thrown away.
			 */
			t_clock_arch * clocks_fake = (t_clock_arch*) my_calloc(1,
					sizeof(t_clock_arch));
			ProcessClocks(Next, clocks_fake);
			free(clocks_fake->clock_inf);
			free(clocks_fake);
		}
		FreeNode(Next);
	}
	SyncModelsPbTypes(arch, *Types, *NumTypes);
	UpdateAndCheckModels(arch);

	/* Release the full XML tree */
	FreeNode(Cur);
}

static void ProcessSegments(INOUTP ezxml_t Parent,
		OUTP struct s_segment_inf **Segs, OUTP int *NumSegs,
		INP struct s_switch_inf *Switches, INP int NumSwitches,
		INP boolean timing_enabled) {
	int i, j, length;
	const char *tmp;

	ezxml_t SubElem;
	ezxml_t Node;

	/* Count the number of segs and check they are in fact
	 * of segment elements. */
	*NumSegs = CountChildren(Parent, "segment", 1);

	/* Alloc segment list */
	*Segs = NULL;
	if (*NumSegs > 0) {
		*Segs = (struct s_segment_inf *) my_malloc(
				*NumSegs * sizeof(struct s_segment_inf));
		memset(*Segs, 0, (*NumSegs * sizeof(struct s_segment_inf)));
	}

	/* Load the segments. */
	for (i = 0; i < *NumSegs; ++i) {
		Node = ezxml_child(Parent, "segment");

		/* Get segment length */
		length = 1; /* DEFAULT */
		tmp = FindProperty(Node, "length", FALSE);
		if (tmp) {
			if (strcmp(tmp, "longline") == 0) {
				(*Segs)[i].longline = TRUE;
			} else {
				length = my_atoi(tmp);
			}
		}
		(*Segs)[i].length = length;
		ezxml_set_attr(Node, "length", NULL);

		/* Get the frequency */
		(*Segs)[i].frequency = 1; /* DEFAULT */
		tmp = FindProperty(Node, "freq", FALSE);
		if (tmp) {
			(*Segs)[i].frequency = (int) (atof(tmp) * MAX_CHANNEL_WIDTH);
		}
		ezxml_set_attr(Node, "freq", NULL);

		/* Get timing info */
		(*Segs)[i].Rmetal = GetFloatProperty(Node, "Rmetal", timing_enabled, 0);
		(*Segs)[i].Cmetal = GetFloatProperty(Node, "Cmetal", timing_enabled, 0);

		/* Get Power info */
		/*
		(*Segs)[i].Cmetal_per_m = GetFloatProperty(Node, "Cmetal_per_m", FALSE,
				0.);*/

		/* Get the type */
		tmp = FindProperty(Node, "type", TRUE);
		if (0 == strcmp(tmp, "bidir")) {
			(*Segs)[i].directionality = BI_DIRECTIONAL;
		}

		else if (0 == strcmp(tmp, "unidir")) {
			(*Segs)[i].directionality = UNI_DIRECTIONAL;
		}

		else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Invalid switch type '%s'.\n", Node->line, tmp);
			exit(1);
		}
		ezxml_set_attr(Node, "type", NULL);

		/* Get the wire and opin switches, or mux switch if unidir */
		if (UNI_DIRECTIONAL == (*Segs)[i].directionality) {
			SubElem = FindElement(Node, "mux", TRUE);
			tmp = FindProperty(SubElem, "name", TRUE);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] '%s' is not a valid mux name.\n",
						SubElem->line, tmp);
				exit(1);
			}
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);

			/* Unidir muxes must have the same switch
			 * for wire and opin fanin since there is
			 * really only the mux in unidir. */
			(*Segs)[i].wire_switch = j;
			(*Segs)[i].opin_switch = j;
		}

		else {
			assert(BI_DIRECTIONAL == (*Segs)[i].directionality);
			SubElem = FindElement(Node, "wire_switch", TRUE);
			tmp = FindProperty(SubElem, "name", TRUE);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] '%s' is not a valid wire_switch name.\n",
						SubElem->line, tmp);
				exit(1);
			}
			(*Segs)[i].wire_switch = j;
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);
			SubElem = FindElement(Node, "opin_switch", TRUE);
			tmp = FindProperty(SubElem, "name", TRUE);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] '%s' is not a valid opin_switch name.\n",
						SubElem->line, tmp);
				exit(1);
			}
			(*Segs)[i].opin_switch = j;
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);
		}

		/* Setup the CB list if they give one, otherwise use full */
		(*Segs)[i].cb_len = length;
		(*Segs)[i].cb = (boolean *) my_malloc(length * sizeof(boolean));
		for (j = 0; j < length; ++j) {
			(*Segs)[i].cb[j] = TRUE;
		}
		SubElem = FindElement(Node, "cb", FALSE);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].cb, length);
			FreeNode(SubElem);
		}

		/* Setup the SB list if they give one, otherwise use full */
		(*Segs)[i].sb_len = (length + 1);
		(*Segs)[i].sb = (boolean *) my_malloc((length + 1) * sizeof(boolean));
		for (j = 0; j < (length + 1); ++j) {
			(*Segs)[i].sb[j] = TRUE;
		}
		SubElem = FindElement(Node, "sb", FALSE);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].sb, (length + 1));
			FreeNode(SubElem);
		}
		FreeNode(Node);
	}
}

static void ProcessCB_SB(INOUTP ezxml_t Node, INOUTP boolean * list,
		INP int len) {
	const char *tmp = NULL;
	int i;

	/* Check the type. We only support 'pattern' for now.
	 * Should add frac back eventually. */
	tmp = FindProperty(Node, "type", TRUE);
	if (0 == strcmp(tmp, "pattern")) {
		i = 0;

		/* Get the content string */
		tmp = Node->txt;
		while (*tmp) {
			switch (*tmp) {
			case ' ':
			case '\t':
			case '\n':
				break;
			case 'T':
			case '1':
				if (i >= len) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] CB or SB depopulation is too long. It "

									"should be (length) symbols for CBs and (length+1) "
									"symbols for SBs.\n", Node->line);
					exit(1);
				}
				list[i] = TRUE;
				++i;
				break;
			case 'F':
			case '0':
				if (i >= len) {
					vpr_printf(TIO_MESSAGE_ERROR,
							"[LINE %d] CB or SB depopulation is too long. It "

									"should be (length) symbols for CBs and (length+1) "
									"symbols for SBs.\n", Node->line);
					exit(1);
				}
				list[i] = FALSE;
				++i;
				break;
			default:
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Invalid character %c in CB or "
								"SB depopulation list.\n", Node->line, *tmp);
				exit(1);
			}
			++tmp;
		}
		if (i < len) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] CB or SB depopulation is too short. It "
							"should be (length) symbols for CBs and (length+1) "
							"symbols for SBs.\n", Node->line);
			exit(1);
		}

		/* Free content string */
		ezxml_set_txt(Node, "");
	}

	else {
		vpr_printf(TIO_MESSAGE_ERROR,
				"[LINE %d] '%s' is not a valid type for specifying "
						"cb and sb depopulation.\n", Node->line, tmp);
		exit(1);
	}
	ezxml_set_attr(Node, "type", NULL);
}

static void ProcessSwitches(INOUTP ezxml_t Parent,
		OUTP struct s_switch_inf **Switches, OUTP int *NumSwitches,
		INP boolean timing_enabled) {
	int i, j;
	const char *type_name;
	const char *switch_name;
	const char *buf_size;

	boolean has_buf_size;
	ezxml_t Node;
	has_buf_size = FALSE;

	/* Count the children and check they are switches */
	*NumSwitches = CountChildren(Parent, "switch", 1);

	/* Alloc switch list */
	*Switches = NULL;
	if (*NumSwitches > 0) {
		*Switches = (struct s_switch_inf *) my_malloc(
				*NumSwitches * sizeof(struct s_switch_inf));
		memset(*Switches, 0, (*NumSwitches * sizeof(struct s_switch_inf)));
	}

	/* Load the switches. */
	for (i = 0; i < *NumSwitches; ++i) {
		Node = ezxml_child(Parent, "switch");
		switch_name = FindProperty(Node, "name", TRUE);
		type_name = FindProperty(Node, "type", TRUE);

		/* Check for switch name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Switches)[j].name, switch_name)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Two switches with the same name '%s' were "
								"found.\n", Node->line, switch_name);
				exit(1);
			}
		}
		(*Switches)[i].name = my_strdup(switch_name);
		ezxml_set_attr(Node, "name", NULL);

		/* Figure out the type of switch. */
		if (0 == strcmp(type_name, "mux")) {
			(*Switches)[i].buffered = TRUE;
			has_buf_size = TRUE;
		}

		else if (0 == strcmp(type_name, "pass_trans")) {
			(*Switches)[i].buffered = FALSE;
		}

		else if (0 == strcmp(type_name, "buffer")) {
			(*Switches)[i].buffered = TRUE;
		}

		else {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] Invalid switch type '%s'.\n", Node->line,
					type_name);
			exit(1);
		}
		ezxml_set_attr(Node, "type", NULL);
		(*Switches)[i].R = GetFloatProperty(Node, "R", timing_enabled, 0);
		(*Switches)[i].Cin = GetFloatProperty(Node, "Cin", timing_enabled, 0);
		(*Switches)[i].Cout = GetFloatProperty(Node, "Cout", timing_enabled, 0);
		(*Switches)[i].Tdel = GetFloatProperty(Node, "Tdel", timing_enabled, 0);
		(*Switches)[i].buf_size = GetFloatProperty(Node, "buf_size",
				has_buf_size, 0);
		(*Switches)[i].mux_trans_size = GetFloatProperty(Node, "mux_trans_size",
				FALSE, 1);

		buf_size = FindProperty(Node, "power_buf_size", FALSE);
		if (buf_size == NULL) {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_AUTO;
		} else if (strcmp(buf_size, "auto") == 0) {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_AUTO;
		} else {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
			(*Switches)[i].power_buffer_size = (float) atof(buf_size);
		}
		ezxml_set_attr(Node, "power_buf_size", NULL);

		/* Remove the switch element from parse tree */
		FreeNode(Node);
	}
}

static void ProcessDirects(INOUTP ezxml_t Parent, OUTP t_direct_inf **Directs,
		OUTP int *NumDirects, INP boolean timing_enabled) {
	int i, j;
	const char *direct_name;
	const char *from_pin_name;
	const char *to_pin_name;

	ezxml_t Node;

	/* Count the children and check they are direct connections */
	*NumDirects = CountChildren(Parent, "direct", 1);

	/* Alloc direct list */
	*Directs = NULL;
	if (*NumDirects > 0) {
		*Directs = (t_direct_inf *) my_malloc(
				*NumDirects * sizeof(t_direct_inf));
		memset(*Directs, 0, (*NumDirects * sizeof(t_direct_inf)));
	}

	/* Load the directs. */
	for (i = 0; i < *NumDirects; ++i) {
		Node = ezxml_child(Parent, "direct");

		direct_name = FindProperty(Node, "name", TRUE);
		/* Check for direct name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Directs)[j].name, direct_name)) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"[LINE %d] Two directs with the same name '%s' were "
								"found.\n", Node->line, direct_name);
				exit(1);
			}
		}
		(*Directs)[i].name = my_strdup(direct_name);
		ezxml_set_attr(Node, "name", NULL);

		/* Figure out the source pin and sink pin name */
		from_pin_name = FindProperty(Node, "from_pin", TRUE);
		to_pin_name = FindProperty(Node, "to_pin", TRUE);

		/* Check that to_pin and the from_pin are not the same */
		if (0 == strcmp(to_pin_name, from_pin_name)) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] The source pin and sink pin are the same: %s.\n",
					Node->line, to_pin_name);
			exit(1);
		}
		(*Directs)[i].from_pin = my_strdup(from_pin_name);
		(*Directs)[i].to_pin = my_strdup(to_pin_name);
		ezxml_set_attr(Node, "from_pin", NULL);
		ezxml_set_attr(Node, "to_pin", NULL);

		(*Directs)[i].x_offset = GetIntProperty(Node, "x_offset", TRUE, 0);
		(*Directs)[i].y_offset = GetIntProperty(Node, "y_offset", TRUE, 0);
		(*Directs)[i].z_offset = GetIntProperty(Node, "z_offset", TRUE, 0);
		ezxml_set_attr(Node, "x_offset", NULL);
		ezxml_set_attr(Node, "y_offset", NULL);
		ezxml_set_attr(Node, "z_offset", NULL);

		/* Check that the direct chain connection is not zero in both direction */
		if ((*Directs)[i].x_offset == 0 && (*Directs)[i].y_offset == 0) {
			vpr_printf(TIO_MESSAGE_ERROR,
					"[LINE %d] The x_offset and y_offset are both zero, "
							"this is a length 0 direct chain connection.\n",
					Node->line);
			exit(1);
		}

		(*Directs)[i].line = Node->line;
		/* Should I check that the direct chain offset is not greater than the chip? How? */

		/* Remove the direct element from parse tree */
		FreeNode(Node);
	}
}

static void CreateModelLibrary(OUTP struct s_arch *arch) {
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
	model_library[0].outputs->is_clock = FALSE;

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
	model_library[1].inputs->is_clock = FALSE;
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
	model_library[2].inputs[0].is_clock = FALSE;
	model_library[2].inputs[1].dir = IN_PORT;
	model_library[2].inputs[1].name = my_strdup("clk");
	model_library[2].inputs[1].next = NULL;
	model_library[2].inputs[1].size = 1;
	model_library[2].inputs[1].min_size = 1;
	model_library[2].inputs[1].index = 0;
	model_library[2].inputs[1].is_clock = TRUE;
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
	model_library[2].outputs->is_clock = FALSE;

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
	model_library[3].inputs->is_clock = FALSE;
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
	model_library[3].outputs->is_clock = FALSE;

	arch->model_library = model_library;
}

static void SyncModelsPbTypes(INOUTP struct s_arch *arch,
		INP t_type_descriptor * Types, INP int NumTypes) {
	int i;
	for (i = 0; i < NumTypes; i++) {
		if (Types[i].pb_type != NULL) {
			SyncModelsPbTypes_rec(arch, Types[i].pb_type);
		}
	}
}

static void SyncModelsPbTypes_rec(INOUTP struct s_arch *arch,
		INOUTP t_pb_type * pb_type) {
	int i, j, p;
	t_model *model_match_prim, *cur_model;
	t_model_ports *model_port;
	struct s_linked_vptr *old;
	char* blif_model_name;

	boolean found;

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
			vpr_printf(TIO_MESSAGE_ERROR,
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
		found = FALSE;
		model_match_prim = NULL;
		while (cur_model && !found) {
			/* blif model always starts with .subckt so need to skip first 8 characters */
			if (strcmp(blif_model_name, cur_model->name) == 0) {
				found = TRUE;
				model_match_prim = cur_model;
			}
			cur_model = cur_model->next;
		}
		if (found != TRUE) {
			vpr_printf(TIO_MESSAGE_ERROR, "No matching model for pb_type %s\n",
					pb_type->blif_model);
			exit(1);
		}

		pb_type->model = model_match_prim;
		old = model_match_prim->pb_types;
		model_match_prim->pb_types = (struct s_linked_vptr*) my_malloc(
				sizeof(struct s_linked_vptr));
		model_match_prim->pb_types->next = old;
		model_match_prim->pb_types->data_vptr = pb_type;

		for (p = 0; p < pb_type->num_ports; p++) {
			found = FALSE;
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
					found = TRUE;
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
					found = TRUE;
				}
				model_port = model_port->next;
			}
			if (found != TRUE) {
				vpr_printf(TIO_MESSAGE_ERROR,
						"No matching model port for port %s in pb_type %s\n",
						pb_type->ports[p].name, pb_type->name);
				exit(1);
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

static void UpdateAndCheckModels(INOUTP struct s_arch *arch) {
	t_model * cur_model;
	t_model_ports *port;
	int i, j;
	cur_model = arch->models;
	while (cur_model) {
		if (cur_model->pb_types == NULL) {
			vpr_printf(TIO_MESSAGE_ERROR, "No pb_type found for model %s\n",
					cur_model->name);
			exit(1);
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

/* Output the data from architecture data so user can verify it
 * was interpretted correctly. */
void EchoArch(INP const char *EchoFile, INP const t_type_descriptor * Types,
		INP int NumTypes, struct s_arch *arch) {
	int i, j;
	FILE * Echo;
	t_model * cur_model;
	t_model_ports * model_port;
	struct s_linked_vptr *cur_vptr;

	Echo = my_fopen(EchoFile, "w", 0);
	cur_model = NULL;

	for (j = 0; j < 2; j++) {
		if (j == 0) {
			fprintf(Echo, "Printing user models \n");
			cur_model = arch->models;
		} else if (j == 1) {
			fprintf(Echo, "Printing library models \n");
			cur_model = arch->model_library;
		}
		while (cur_model) {
			fprintf(Echo, "Model: \"%s\"\n", cur_model->name);
			model_port = cur_model->inputs;
			while (model_port) {
				fprintf(Echo, "\tInput Ports: \"%s\" \"%d\" min_size=\"%d\"\n",
						model_port->name, model_port->size,
						model_port->min_size);
				model_port = model_port->next;
			}
			model_port = cur_model->outputs;
			while (model_port) {
				fprintf(Echo, "\tOutput Ports: \"%s\" \"%d\" min_size=\"%d\"\n",
						model_port->name, model_port->size,
						model_port->min_size);
				model_port = model_port->next;
			}
			cur_vptr = cur_model->pb_types;
			i = 0;
			while (cur_vptr != NULL) {
				fprintf(Echo, "\tpb_type %d: \"%s\"\n", i,
						((t_pb_type*) cur_vptr->data_vptr)->name);
				cur_vptr = cur_vptr->next;
				i++;
			}

			cur_model = cur_model->next;
		}
	}

	for (i = 0; i < NumTypes; ++i) {
		fprintf(Echo, "Type: \"%s\"\n", Types[i].name);
		fprintf(Echo, "\tcapacity: %d\n", Types[i].capacity);
		fprintf(Echo, "\theight: %d\n", Types[i].height);

		for (j = 0; j < Types[i].num_pins; j++) {
			fprintf(Echo, "\tis_Fc_frac: \n");
			fprintf(Echo, "\t\tPin number %d: %s\n", j,
					(Types[i].is_Fc_frac[j] ? "TRUE" : "FALSE"));
			fprintf(Echo, "\tis_Fc_full_flex: \n");
			fprintf(Echo, "\t\tPin number %d: %s\n", j,
					(Types[i].is_Fc_full_flex[j] ? "TRUE" : "FALSE"));
			fprintf(Echo, "\tFc_val: \n");
			fprintf(Echo, "\tPin number %d: %f\n", j, Types[i].Fc[j]);
		}
		fprintf(Echo, "\tnum_drivers: %d\n", Types[i].num_drivers);
		fprintf(Echo, "\tnum_receivers: %d\n", Types[i].num_receivers);
		fprintf(Echo, "\tindex: %d\n", Types[i].index);
		if (Types[i].pb_type) {
			PrintPb_types_rec(Echo, Types[i].pb_type, 2);
		}
		fprintf(Echo, "\n");
	}
	fclose(Echo);
}

static void PrintPb_types_rec(INP FILE * Echo, INP const t_pb_type * pb_type,
		int level) {
	int i, j, k;
	char *tabs;

	tabs = (char*) my_malloc((level + 1) * sizeof(char));
	for (i = 0; i < level; i++) {
		tabs[i] = '\t';
	}
	tabs[level] = '\0';

	fprintf(Echo, "%spb_type name: %s\n", tabs, pb_type->name);
	fprintf(Echo, "%s\tblif_model: %s\n", tabs, pb_type->blif_model);
	fprintf(Echo, "%s\tclass_type: %d\n", tabs, pb_type->class_type);
	fprintf(Echo, "%s\tnum_modes: %d\n", tabs, pb_type->num_modes);
	fprintf(Echo, "%s\tnum_ports: %d\n", tabs, pb_type->num_ports);
	for (i = 0; i < pb_type->num_ports; i++) {
		fprintf(Echo, "%s\tport %s type %d num_pins %d\n", tabs,
				pb_type->ports[i].name, pb_type->ports[i].type,
				pb_type->ports[i].num_pins);
	}
	for (i = 0; i < pb_type->num_modes; i++) {
		fprintf(Echo, "%s\tmode %s:\n", tabs, pb_type->modes[i].name);
		for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
			PrintPb_types_rec(Echo, &pb_type->modes[i].pb_type_children[j],
					level + 2);
		}
		for (j = 0; j < pb_type->modes[i].num_interconnect; j++) {
			fprintf(Echo, "%s\t\tinterconnect %d %s %s\n", tabs,
					pb_type->modes[i].interconnect[j].type,
					pb_type->modes[i].interconnect[j].input_string,
					pb_type->modes[i].interconnect[j].output_string);
			for (k = 0; k < pb_type->modes[i].interconnect[j].num_annotations;
					k++) {
				fprintf(Echo, "%s\t\t\tannotation %s %s %d: %s\n", tabs,
						pb_type->modes[i].interconnect[j].annotations[k].input_pins,
						pb_type->modes[i].interconnect[j].annotations[k].output_pins,
						pb_type->modes[i].interconnect[j].annotations[k].format,
						pb_type->modes[i].interconnect[j].annotations[k].value[0]);
			}
		}
	}
	free(tabs);
}

static void ProcessPower( INOUTP ezxml_t parent,
		INOUTP t_power_arch * power_arch, INP t_type_descriptor * Types,
		INP int NumTypes) {
	ezxml_t Cur;

	/* Get the local interconnect capacitances */
	power_arch->local_interc_factor = 0.5;
	Cur = FindElement(parent, "local_interconnect", FALSE);
	if (Cur) {
		power_arch->C_wire_local = GetFloatProperty(Cur, "C_wire", FALSE, 0.);
		power_arch->local_interc_factor = GetFloatProperty(Cur, "factor", FALSE,
				0.5);
		FreeNode(Cur);
	}

	/* Get segment split */
	/*
	power_arch->seg_buffer_split = 1;
	Cur = FindElement(parent, "segment_buffer_split", FALSE);
	if (Cur) {
		power_arch->seg_buffer_split = GetIntProperty(Cur, "split_into", TRUE,
				1);
		FreeNode(Cur);
	}*/

	/* Get logical effort factor */
	power_arch->logical_effort_factor = 4.0;
	Cur = FindElement(parent, "buffers", FALSE);
	if (Cur) {
		power_arch->logical_effort_factor = GetFloatProperty(Cur,
				"logical_effort_factor", TRUE, 0);
		FreeNode(Cur);
	}

	/* Get SRAM Size */
	power_arch->transistors_per_SRAM_bit = 6.0;
	Cur = FindElement(parent, "sram", FALSE);
	if (Cur) {
		power_arch->transistors_per_SRAM_bit = GetFloatProperty(Cur,
				"transistors_per_bit", TRUE, 0);
		FreeNode(Cur);
	}

	/* Get Mux transistor size */
	power_arch->mux_transistor_size = 1.0;
	Cur = FindElement(parent, "mux_transistor_size", FALSE);
	if (Cur) {
		power_arch->mux_transistor_size = GetFloatProperty(Cur,
				"mux_transistor_size", TRUE, 0);
		FreeNode(Cur);
	}

	/* Get FF size */
	power_arch->FF_size = 1.0;
	Cur = FindElement(parent, "FF_size", FALSE);
	if (Cur) {
		power_arch->FF_size = GetFloatProperty(Cur, "FF_size", TRUE, 0);
		FreeNode(Cur);
	}

	/* Get LUT transistor size */
	power_arch->LUT_transistor_size = 1.0;
	Cur = FindElement(parent, "LUT_transistor_size", FALSE);
	if (Cur) {
		power_arch->LUT_transistor_size = GetFloatProperty(Cur,
				"LUT_transistor_size", TRUE, 0);
		FreeNode(Cur);
	}
}

/* Get the clock architcture */
static void ProcessClocks(ezxml_t Parent, t_clock_arch * clocks) {
	ezxml_t Node;
	int i;
	const char *tmp;

	clocks->num_global_clocks = CountChildren(Parent, "clock", 0);

	/* Alloc the clockdetails */
	clocks->clock_inf = NULL;
	if (clocks->num_global_clocks > 0) {
		clocks->clock_inf = (t_clock_network *) my_malloc(
				clocks->num_global_clocks * sizeof(t_clock_network));
		memset(clocks->clock_inf, 0,
				clocks->num_global_clocks * sizeof(t_clock_network));
	}

	/* Load the clock info. */
	for (i = 0; i < clocks->num_global_clocks; ++i) {
		/* get the next clock item */
		Node = ezxml_child(Parent, "clock");

		tmp = FindProperty(Node, "buffer_size", TRUE);
		if (strcmp(tmp, "auto") == 0) {
			clocks->clock_inf[i].autosize_buffer = TRUE;
		} else {
			clocks->clock_inf[i].autosize_buffer = FALSE;
			clocks->clock_inf[i].buffer_size = (float) atof(tmp);
		}
		ezxml_set_attr(Node, "buffer_size", NULL);

		clocks->clock_inf[i].C_wire = GetFloatProperty(Node, "C_wire", TRUE, 0);
		FreeNode(Node);
	}
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
      return POWER_METHOD_UNDEFINED;  // Should never get here, but avoids a compiler warning.
	}
}

