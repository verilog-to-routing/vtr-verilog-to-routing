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
 * of the architecture file that got missed. 
 * 
 * 
 * Architecture file checks already implemented (Daniel Chen):
 *		- Duplicate pb_types, pb_type ports, models, model ports,
 *			interconnects, interconnect annotations.
 *		- Port and pin range checking (port with 4 pins can only be
 *			accessed within [0:3].
 *		- LUT delay matrix size matches # of LUT inputs
 *		- Ensures XML tags are ordered.
 *		- Clocked primitives that have timing annotations must have a clock 
 *			name matching the primitive.
 *		- Enforced VPR definition of LUT and FF must have one input port (n pins) 
 *			and one output port(1 pin).
 *		- Checks file extension for blif and architecture xml file, avoid crashes if
 *			the two files are swapped on command line.
 *			
 */

#include <string.h>
#include <assert.h>
#include <map>
#include <string>
#include <sstream>
#include "util.h"
#include "arch_types.h"
#include "ReadLine.h"
#include "ezxml.h"
#include "read_xml_arch_file.h"
#include "read_xml_util.h"
#include "parse_switchblocks.h"

using namespace std;

enum Fc_type {
	FC_ABS, FC_FRAC, FC_FULL
};

/* This gives access to the architecture file name to 
 all architecture-parser functions       */
static const char* arch_file_name = NULL;

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
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type);
static void ProcessInterconnect(INOUTP ezxml_t Parent, t_mode * mode);
static void ProcessMode(INOUTP ezxml_t Parent, t_mode * mode,
		bool * default_leakage_mode);
static void Process_Fc(ezxml_t Node, t_type_descriptor * Type, t_segment_inf *segments, int num_segments);
static void ProcessComplexBlockProps(ezxml_t Node, t_type_descriptor * Type);
static void ProcessSizingTimingIpinCblock(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch, INP bool timing_enabled);
static void ProcessChanWidthDistr(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch);
static void ProcessChanWidthDistrDir(INOUTP ezxml_t Node, OUTP t_chan * chan);
static void ProcessModels(INOUTP ezxml_t Node, OUTP struct s_arch *arch);
static void ProcessLayout(INOUTP ezxml_t Node, OUTP struct s_arch *arch);
static void ProcessDevice(INOUTP ezxml_t Node, OUTP struct s_arch *arch,
		INP bool timing_enabled);
static void alloc_and_load_default_child_for_pb_type(INOUTP t_pb_type *pb_type,
		char *new_name, t_pb_type *copy);
static void ProcessLutClass(INOUTP t_pb_type *lut_pb_type);
static void ProcessMemoryClass(INOUTP t_pb_type *mem_pb_type);
static void ProcessComplexBlocks(INOUTP ezxml_t Node,
		OUTP t_type_descriptor ** Types, OUTP int *NumTypes,
		INP bool timing_enabled, s_arch arch);
static void ProcessSwitches(INOUTP ezxml_t Node,
		OUTP struct s_arch_switch_inf **Switches, OUTP int *NumSwitches,
		INP bool timing_enabled);
static void ProcessSwitchTdel(INOUTP ezxml_t Node, INP bool timing_enabled,
		INP int switch_index, OUTP s_arch_switch_inf *Switches);
static void ProcessDirects(INOUTP ezxml_t Parent, OUTP t_direct_inf **Directs,
		 OUTP int *NumDirects, INP struct s_arch_switch_inf *Switches, INP int NumSwitches,
		 INP bool timing_enabled);
static void ProcessSegments(INOUTP ezxml_t Parent,
		OUTP struct s_segment_inf **Segs, OUTP int *NumSegs,
		INP struct s_arch_switch_inf *Switches, INP int NumSwitches,
		INP bool timing_enabled, INP bool switchblocklist_required);
static void ProcessSwitchblocks(INOUTP ezxml_t Parent, OUTP vector<t_switchblock_inf> *switchblocks,
				INP t_arch_switch_inf *switches, INP int num_switches);
static void ProcessCB_SB(INOUTP ezxml_t Node, INOUTP bool * list,
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
static void PrintPb_types_recPower(INP FILE * Echo,
		INP const t_pb_type * pb_type, const char* tabs);
static void PrintArchInfo(INP FILE * Echo, struct s_arch *arch);
static void ProcessPb_TypePowerEstMethod(ezxml_t Parent, t_pb_type * pb_type);
static void ProcessPb_TypePort_Power(ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method);
e_power_estimation_method power_method_inherited(
		e_power_estimation_method parent_power_method);
static void CheckXMLTagOrder(ezxml_t Parent);
static void primitives_annotation_clock_match(
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type);

#ifdef INTERPOSER_BASED_ARCHITECTURE
int gcd(int a, int b)
{
	if (b == 0)
		return a;
	else
		return gcd(b, a%b);
}
int lcm(int a, int b)
{
	int mygcd = gcd(a,b);

	if(mygcd==0)
	{
		return -1;
	}
	else
	{
		return (a*b)/mygcd;
	}
}
#endif

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

	Prop = FindProperty(Locations, "pattern", true);
	if (strcmp(Prop, "spread") == 0) {
		Type->pin_location_distribution = E_SPREAD_PIN_DISTR;
	} else if (strcmp(Prop, "custom") == 0) {
		Type->pin_location_distribution = E_CUSTOM_PIN_DISTR;
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Locations->line,
				"%s is an invalid pin location pattern.\n", Prop);
	}
	ezxml_set_attr(Locations, "pattern", NULL);

	/* Alloc and clear pin locations */
	Type->pin_width = (int *) my_calloc(Type->num_pins, sizeof(int));
	Type->pin_height = (int *) my_calloc(Type->num_pins, sizeof(int));
	Type->pinloc = (int ****) my_malloc(Type->width * sizeof(int **));
	for (int width = 0; width < Type->width; ++width) {
		Type->pinloc[width] = (int ***) my_malloc(Type->height * sizeof(int *));
		for (int height = 0; height < Type->height; ++height) {
			Type->pinloc[width][height] = (int **) my_malloc(4 * sizeof(int *));
			for (int side = 0; side < 4; ++side) {
				Type->pinloc[width][height][side] = (int *) my_malloc(
						Type->num_pins * sizeof(int));
				for (int pin = 0; pin < Type->num_pins; ++pin) {
					Type->pinloc[width][height][side][pin] = 0;
				}
			}
		}
	}

	Type->pin_loc_assignments = (char *****) my_malloc(
			Type->width * sizeof(char ****));
	Type->num_pin_loc_assignments = (int ***) my_malloc(
			Type->width * sizeof(int **));
	for (int width = 0; width < Type->width; ++width) {
		Type->pin_loc_assignments[width] = (char ****) my_calloc(Type->height,
				sizeof(char ***));
		Type->num_pin_loc_assignments[width] = (int **) my_calloc(Type->height,
				sizeof(int *));
		for (int height = 0; height < Type->height; ++height) {
			Type->pin_loc_assignments[width][height] = (char ***) my_calloc(4,
					sizeof(char **));
			Type->num_pin_loc_assignments[width][height] = (int *) my_calloc(4,
					sizeof(int));
		}
	}

	/* Load the pin locations */
	if (Type->pin_location_distribution == E_CUSTOM_PIN_DISTR) {
		Cur = Locations->child;
		while (Cur) {
			CheckElement(Cur, "loc");

			/* Get offset (ie. height) */
			int height = GetIntProperty(Cur, "offset", false, 0);
			if ((height < 0) || (height >= Type->height)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"'%d' is an invalid offset for type '%s'.\n", height,
						Type->name);
			}

			/* Get side */
			int side = 0;
			Prop = FindProperty(Cur, "side", true);
			if (0 == strcmp(Prop, "left")) {
				side = LEFT;
			} else if (0 == strcmp(Prop, "top")) {
				side = TOP;
			} else if (0 == strcmp(Prop, "right")) {
				side = RIGHT;
			} else if (0 == strcmp(Prop, "bottom")) {
				side = BOTTOM;
			} else {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"'%s' is not a valid side.\n", Prop);
			}
			ezxml_set_attr(Cur, "side", NULL);

			/* Check location is on perimeter */
			if ((TOP == side) && (height != (Type->height - 1))) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Locations are only allowed on large block perimeter. 'top' side should be at offset %d only.\n",
						(Type->height - 1));
			}
			if ((BOTTOM == side) && (height != 0)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Locations are only allowed on large block perimeter. 'bottom' side should be at offset 0 only.\n");
			}

			/* Go through lists of pins */
			CountTokensInString(Cur->txt, &Count, &Len);
			Type->num_pin_loc_assignments[0][height][side] = Count;
			if (Count > 0) {
				Tokens = GetNodeTokens(Cur);
				CurTokens = Tokens;
				Type->pin_loc_assignments[0][height][side] = (char**) my_calloc(
						Count, sizeof(char*));
				for (int pin = 0; pin < Count; ++pin) {
					/* Store location assignment */
					Type->pin_loc_assignments[0][height][side][pin] = my_strdup(
							*CurTokens);

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
	Type->is_global_pin = (bool*) my_malloc(
        Type->num_pins * sizeof(bool)* capacity);
	for (i = 0; i < Type->num_pins * capacity; i++) {
		Type->pin_class[i] = OPEN;
		Type->is_global_pin[i] = true;
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
				Type->is_global_pin[pin_count] = Type->pb_type->ports[j].is_clock || 
                    Type->pb_type->ports[j].is_non_clock_global;
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
		Prop = FindProperty(Cur, "type", true);
		if (Prop) {
			if (strcmp(Prop, "perimeter") == 0) {
				if (Type->num_grid_loc_def != 1) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
							"Another loc specified for perimeter.\n");
				}
				Type->grid_loc_def[i].grid_loc_type = BOUNDARY;
				assert(IO_TYPE == Type);
				/* IO goes to boundary */
			} else if (strcmp(Prop, "fill") == 0) {
				if (Type->num_grid_loc_def != 1 || FILL_TYPE != NULL) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
							"Another loc specified for fill.\n");
				}
				Type->grid_loc_def[i].grid_loc_type = FILL;
				FILL_TYPE = Type;
			} else if (strcmp(Prop, "col") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REPEAT;
			} else if (strcmp(Prop, "rel") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REL;
			} else {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Unknown grid location type '%s' for type '%s'.\n",
						Prop, Type->name);
			}
			ezxml_set_attr(Cur, "type", NULL);
		}
		Prop = FindProperty(Cur, "start", false);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop == NULL) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"grid location property 'start' must be specified for grid location type 'col'.\n");
			}
			Type->grid_loc_def[i].start_col = my_atoi(Prop);
			ezxml_set_attr(Cur, "start", NULL);
		} else if (Prop != NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
					"grid location property 'start' valid for grid location type 'col' only.\n");
		}
		Prop = FindProperty(Cur, "repeat", false);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop != NULL) {
				Type->grid_loc_def[i].repeat = my_atoi(Prop);
				ezxml_set_attr(Cur, "repeat", NULL);
			}
		} else if (Prop != NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
					"Grid location property 'repeat' valid for grid location type 'col' only.\n");
		}
		Prop = FindProperty(Cur, "pos", false);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REL) {
			if (Prop == NULL) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Grid location property 'pos' must be specified for grid location type 'rel'.\n");
			}
			Type->grid_loc_def[i].col_rel = (float) atof(Prop);
			ezxml_set_attr(Cur, "pos", NULL);
		} else if (Prop != NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
					"Grid location property 'pos' valid for grid location type 'rel' only.\n");
		}

		Type->grid_loc_def[i].priority = GetIntProperty(Cur, "priority", false,
				1);

		Prev = Cur;
		Cur = Cur->next;
		FreeNode(Prev);
		i++;
	}
}

static void ProcessPinToPinAnnotations(ezxml_t Parent,
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type) {
	int i = 0;
	const char *Prop;

	if (FindProperty(Parent, "max", false)) {
		i++;
	}
	if (FindProperty(Parent, "min", false)) {
		i++;
	}
	if (FindProperty(Parent, "type", false)) {
		i++;
	}
	if (FindProperty(Parent, "value", false)) {
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
	annotation->line_num = Parent->line;
	/* Todo: This is slow, I should use a case lookup */
	i = 0;
	if (0 == strcmp(Parent->name, "delay_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "max", false);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MAX;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "max", NULL);
			i++;
		}
		Prop = FindProperty(Parent, "min", false);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MIN;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "min", NULL);
			i++;
		}
		Prop = FindProperty(Parent, "in_port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", true);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else if (0 == strcmp(Parent->name, "delay_matrix")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
		Prop = FindProperty(Parent, "type", true);
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
		Prop = FindProperty(Parent, "in_port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", true);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else if (0 == strcmp(Parent->name, "C_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "C", true);
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "C", NULL);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
		i++;

		Prop = FindProperty(Parent, "in_port", false);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", false);
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
		Prop = FindProperty(Parent, "in_port", false);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", false);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
		assert(
				annotation->output_pins != NULL || annotation->input_pins != NULL);
	} else if (0 == strcmp(Parent->name, "T_setup")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "value", true);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_TSETUP;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "value", NULL);
		i++;
		Prop = FindProperty(Parent, "port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", true);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent->name, "T_clock_to_Q")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "max", false);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "max", NULL);
			i++;
		}
		Prop = FindProperty(Parent, "min", false);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN;
			annotation->value[i] = my_strdup(Prop);
			ezxml_set_attr(Parent, "min", NULL);
			i++;
		}

		Prop = FindProperty(Parent, "port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", true);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent->name, "T_hold")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "value", true);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_THOLD;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "value", NULL);
		i++;

		Prop = FindProperty(Parent, "port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "port", NULL);
		Prop = FindProperty(Parent, "clock", true);
		annotation->clock = my_strdup(Prop);
		ezxml_set_attr(Parent, "clock", NULL);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent->name, "pack_pattern")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = FindProperty(Parent, "name", true);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
		annotation->value[i] = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
		i++;

		Prop = FindProperty(Parent, "in_port", true);
		annotation->input_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "in_port", NULL);
		Prop = FindProperty(Parent, "out_port", true);
		annotation->output_pins = my_strdup(Prop);
		ezxml_set_attr(Parent, "out_port", NULL);
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
				"Unknown port type %s in %s in %s", Parent->name,
				Parent->parent->name, Parent->parent->parent->name);
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

static void ProcessPb_TypePowerPinToggle(ezxml_t parent, t_pb_type * pb_type) {
	ezxml_t cur, prev;
	const char * prop;
	t_port * port;
	int high, low;

	cur = FindFirstElement(parent, "port", false);
	while (cur) {
		prop = FindProperty(cur, "name", true);

		port = findPortByName(prop, pb_type, &high, &low);
		if (!port) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
					"Could not find port '%s' needed for energy per toggle.",
					prop);
		}
		if (high != port->num_pins - 1 || low != 0) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
					"Pin-toggle does not support pin indices (%s)", prop);
		}

		if (port->port_power->pin_toggle_initialized) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
					"Duplicate pin-toggle energy for port '%s'", port->name);
		}
		port->port_power->pin_toggle_initialized = true;
		ezxml_set_attr(cur, "name", NULL);

		/* Get energy per toggle */
		port->port_power->energy_per_toggle = GetFloatProperty(cur,
				"energy_per_toggle", true, 0.);

		/* Get scaled by factor */
		bool reverse_scaled = false;
		prop = FindProperty(cur, "scaled_by_static_prob", false);
		if (!prop) {
			prop = FindProperty(cur, "scaled_by_static_prob_n", false);
			if (prop) {
				reverse_scaled = true;
			}
		}

		if (prop) {
			port->port_power->scaled_by_port = findPortByName(prop, pb_type,
					&high, &low);
			if (high != low) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Pin-toggle 'scaled_by_static_prob' must be a single pin (%s)",
						prop);
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
	bool require_dynamic_absolute = false;
	bool require_static_absolute = false;
	bool require_dynamic_C_internal = false;

	cur = FindFirstElement(Parent, "power", false);
	if (!cur) {
		return;
	}

	switch (pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_TOGGLE_PINS:
		ProcessPb_TypePowerPinToggle(cur, pb_type);
		require_static_absolute = true;
		break;
	case POWER_METHOD_C_INTERNAL:
		require_dynamic_C_internal = true;
		require_static_absolute = true;
		break;
	case POWER_METHOD_ABSOLUTE:
		require_dynamic_absolute = true;
		require_static_absolute = true;
		break;
	default:
		break;
	}

	if (require_static_absolute) {
		child = FindElement(cur, "static_power", true);
		pb_type->pb_type_power->absolute_power_per_instance.leakage =
				GetFloatProperty(child, "power_per_instance", true, 0.);
		FreeNode(child);
	}

	if (require_dynamic_absolute) {
		child = FindElement(cur, "dynamic_power", true);
		pb_type->pb_type_power->absolute_power_per_instance.dynamic =
				GetFloatProperty(child, "power_per_instance", true, 0.);
		FreeNode(child);
	}

	if (require_dynamic_C_internal) {
		child = FindElement(cur, "dynamic_power", true);
		pb_type->pb_type_power->C_internal = GetFloatProperty(child,
				"C_internal", true, 0.);
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

	cur = FindFirstElement(Parent, "power", false);
	if (cur) {
		prop = FindProperty(cur, "method", false);
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
		pb_type->pb_type_power->estimation_method =
				POWER_METHOD_SUM_OF_CHILDREN;
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
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
	ezxml_t Cur = NULL;
	ezxml_t Prev = NULL;
	char* class_name;
	/* STL maps for checking various duplicate names */
	map<string, int> pb_port_names;
	map<string, int> mode_names;
	pair<map<string, int>::iterator, bool> ret_pb_ports;
	pair<map<string, int>::iterator, bool> ret_mode_names;
	int num_in_ports, num_out_ports, num_clock_ports;
	int num_delay_constant, num_delay_matrix, num_C_constant, num_C_matrix,
			num_T_setup, num_T_cq, num_T_hold;

	pb_type->parent_mode = mode;
	if (mode != NULL && mode->parent_pb_type != NULL) {
		pb_type->depth = mode->parent_pb_type->depth + 1;
		Prop = FindProperty(Parent, "name", true);
		pb_type->name = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
	} else {
		pb_type->depth = 0;
		/* same name as type */
	}

	Prop = FindProperty(Parent, "blif_model", false);
	pb_type->blif_model = my_strdup(Prop);
	ezxml_set_attr(Parent, "blif_model", NULL);

	pb_type->class_type = UNKNOWN_CLASS;
	Prop = FindProperty(Parent, "class", false);
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
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
					"Unknown class '%s' in pb_type '%s'\n", class_name,
					pb_type->name);
		}
		free(class_name);
	}

	if (mode == NULL) {
		pb_type->num_pb = 1;
	} else {
		pb_type->num_pb = GetIntProperty(Parent, "num_pb", true, 0);
	}

	assert(pb_type->num_pb > 0);
	num_ports = num_in_ports = num_out_ports = num_clock_ports = 0;
	num_in_ports = CountChildren(Parent, "input", 0);
	num_out_ports = CountChildren(Parent, "output", 0);
	num_clock_ports = CountChildren(Parent, "clock", 0);
	num_ports = num_in_ports + num_out_ports + num_clock_ports;
	pb_type->ports = (t_port*) my_calloc(num_ports, sizeof(t_port));
	pb_type->num_ports = num_ports;

	/* Enforce VPR's definition of LUT/FF by checking number of ports */
	if (pb_type->class_type == LUT_CLASS
			|| pb_type->class_type == LATCH_CLASS) {
		if (num_in_ports != 1 || num_out_ports != 1) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
					"%s primitives must contain exactly one input port and one output port."
							"Found '%d' input port(s) and '%d' output port(s) for '%s'",
					(pb_type->class_type == LUT_CLASS) ? "LUT" : "Latch",
					num_in_ports, num_out_ports, pb_type->name);
		}
	}

	/* Check the XML tag ordering of pb_type */
	CheckXMLTagOrder(Parent);

	/* Initialize Power Structure */
	pb_type->pb_type_power = (t_pb_type_power*) my_calloc(1,
			sizeof(t_pb_type_power));
	ProcessPb_TypePowerEstMethod(Parent, pb_type);

	/* process ports */
	j = 0;
	for (i = 0; i < 3; i++) {
		if (i == 0) {
			k = 0;
			Cur = FindFirstElement(Parent, "input", false);
		} else if (i == 1) {
			k = 0;
			Cur = FindFirstElement(Parent, "output", false);
		} else {
			k = 0;
			Cur = FindFirstElement(Parent, "clock", false);
		}
		while (Cur != NULL) {
			pb_type->ports[j].parent_pb_type = pb_type;
			pb_type->ports[j].index = j;
			pb_type->ports[j].port_index_by_type = k;
			ProcessPb_TypePort(Cur, &pb_type->ports[j],
					pb_type->pb_type_power->estimation_method);

			//Check port name duplicates
			ret_pb_ports = pb_port_names.insert(
					pair<string, int>(pb_type->ports[j].name, 0));
			if (!ret_pb_ports.second) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Duplicate port names in pb_type '%s': port '%s'\n",
						pb_type->name, pb_type->ports[j].name);
			}

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
				&& pb_type->ports[i].is_clock == false) {
			pb_type->num_input_pins += pb_type->ports[i].num_pins;
		} else if (pb_type->ports[i].type == OUT_PORT) {
			pb_type->num_output_pins += pb_type->ports[i].num_pins;
		} else {
			assert(
					pb_type->ports[i].is_clock
							&& pb_type->ports[i].type == IN_PORT);
			pb_type->num_clock_pins += pb_type->ports[i].num_pins;
		}
	}

	/* set max_internal_delay if exist */
	pb_type->max_internal_delay = UNDEFINED;
	Cur = FindElement(Parent, "max_internal_delay", false);
	if (Cur) {
		pb_type->max_internal_delay = GetFloatProperty(Cur, "value", true,
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
		num_delay_constant = CountChildren(Parent, "delay_constant", 0);
		num_delay_matrix = CountChildren(Parent, "delay_matrix", 0);
		num_C_constant = CountChildren(Parent, "C_constant", 0);
		num_C_matrix = CountChildren(Parent, "C_matrix", 0);
		num_T_setup = CountChildren(Parent, "T_setup", 0);
		num_T_cq = CountChildren(Parent, "T_clock_to_Q", 0);
		num_T_hold = CountChildren(Parent, "T_hold", 0);
		num_annotations = num_delay_constant + num_delay_matrix + num_C_constant
				+ num_C_matrix + num_T_setup + num_T_cq + num_T_hold;

		CheckXMLTagOrder(Parent);

		pb_type->annotations = (t_pin_to_pin_annotation*) my_calloc(
				num_annotations, sizeof(t_pin_to_pin_annotation));
		pb_type->num_annotations = num_annotations;

		j = 0;
		Cur = NULL;
		for (i = 0; i < 7; i++) {
			if (i == 0) {
				Cur = FindFirstElement(Parent, "delay_constant", false);
			} else if (i == 1) {
				Cur = FindFirstElement(Parent, "delay_matrix", false);
			} else if (i == 2) {
				Cur = FindFirstElement(Parent, "C_constant", false);
			} else if (i == 3) {
				Cur = FindFirstElement(Parent, "C_matrix", false);
			} else if (i == 4) {
				Cur = FindFirstElement(Parent, "T_setup", false);
			} else if (i == 5) {
				Cur = FindFirstElement(Parent, "T_clock_to_Q", false);
			} else if (i == 6) {
				Cur = FindFirstElement(Parent, "T_hold", false);
			}
			while (Cur != NULL) {
				ProcessPinToPinAnnotations(Cur, &pb_type->annotations[j],
						pb_type);

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
		bool default_leakage_mode = false;

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

			Cur = FindFirstElement(Parent, "mode", true);
			while (Cur != NULL) {
				if (0 == strcmp(Cur->name, "mode")) {
					pb_type->modes[i].parent_pb_type = pb_type;
					pb_type->modes[i].index = i;
					ProcessMode(Cur, &pb_type->modes[i], &default_leakage_mode);
					if (default_leakage_mode) {
						pb_type->pb_type_power->leakage_default_mode = i;
					}

					ret_mode_names = mode_names.insert(
							pair<string, int>(pb_type->modes[i].name, 0));
					if (!ret_mode_names.second) {
						vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
								"Duplicate mode name: '%s' in pb_type '%s'.\n",
								pb_type->modes[i].name, pb_type->name);
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

	pb_port_names.clear();
	mode_names.clear();
	ProcessPb_TypePower(Parent, pb_type);

}

static void ProcessPb_TypePort_Power(ezxml_t Parent, t_port * port,
		e_power_estimation_method power_method) {
	ezxml_t cur;
	const char * prop;
	bool wire_defined = false;

	port->port_power = (t_port_power*) my_calloc(1, sizeof(t_port_power));

	//Defaults
	if (power_method == POWER_METHOD_AUTO_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_AUTO;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
	} else if (power_method == POWER_METHOD_SPECIFY_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_IGNORED;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_NONE;
	}

	cur = FindElement(Parent, "power", false);

	if (cur) {
		/* Wire capacitance */

		/* Absolute C provided */
		prop = FindProperty(cur, "wire_capacitance", false);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Wire capacitance defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_C;
				port->port_power->wire.C = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_capacitance", NULL);
		}

		/* Wire absolute length provided */
		prop = FindProperty(cur, "wire_length", false);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Wire length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Multiple wire properties defined for port '%s', pb_type '%s'.",
						port->name, port->parent_pb_type->name);
			} else if (strcmp(prop, "auto") == 0) {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_AUTO;
			} else {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_ABSOLUTE_LENGTH;
				port->port_power->wire.absolute_length = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_length", NULL);
		}

		/* Wire relative length provided */
		prop = FindProperty(cur, "wire_relative_length", false);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Wire relative length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
						"Multiple wire properties defined for port '%s', pb_type '%s'.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_RELATIVE_LENGTH;
				port->port_power->wire.relative_length = (float) atof(prop);
			}
			ezxml_set_attr(cur, "wire_relative_length", NULL);
		}

		/* Buffer Size */
		prop = FindProperty(cur, "buffer_size", false);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, cur->line,
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
	Prop = FindProperty(Parent, "name", true);
	port->name = my_strdup(Prop);
	ezxml_set_attr(Parent, "name", NULL);

	Prop = FindProperty(Parent, "port_class", false);
	port->port_class = my_strdup(Prop);
	ezxml_set_attr(Parent, "port_class", NULL);

	Prop = FindProperty(Parent, "chain", false);
	port->chain_name = my_strdup(Prop);
	ezxml_set_attr(Parent, "chain", NULL);

	port->equivalent = GetboolProperty(Parent, "equivalent", false, false);
	port->num_pins = GetIntProperty(Parent, "num_pins", true, 0);
	port->is_non_clock_global = GetboolProperty(Parent,
			"is_non_clock_global", false, false);

	if (0 == strcmp(Parent->name, "input")) {
		port->type = IN_PORT;
		port->is_clock = false;

		/* Check if LUT/FF port class is lut_in/D */
		if (port->parent_pb_type->class_type == LUT_CLASS) {
			if ((!port->port_class) || strcmp("lut_in", port->port_class)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Inputs to LUT primitives must have a port class named "
								"as \"lut_in\".");
			}
		} else if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("D", port->port_class)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Input to flipflop primitives must have a port class named "
								"as \"D\".");
			}
			/* Only allow one input pin for FF's */
			if (port->num_pins != 1) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Input port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}

	} else if (0 == strcmp(Parent->name, "output")) {
		port->type = OUT_PORT;
		port->is_clock = false;

		/* Check if LUT/FF port class is lut_out/Q */
		if (port->parent_pb_type->class_type == LUT_CLASS) {
			if ((!port->port_class) || strcmp("lut_out", port->port_class)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Output to LUT primitives must have a port class named "
								"as \"lut_in\".");
			}
			/* Only allow one output pin for LUT's */
			if (port->num_pins != 1) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Output port of LUT primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		} else if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("Q", port->port_class)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Output to flipflop primitives must have a port class named "
								"as \"D\".");
			}
			/* Only allow one output pin for FF's */
			if (port->num_pins != 1) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Output port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}
	} else if (0 == strcmp(Parent->name, "clock")) {
		port->type = IN_PORT;
		port->is_clock = true;
		if (port->is_non_clock_global == true) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
					"Port %s cannot be both a clock and a non-clock simultaneously\n",
					Parent->name);
		}

		if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("clock", port->port_class)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Clock to flipflop primitives must have a port class named "
								"as \"clock\".");
			}
			/* Only allow one output pin for FF's */
			if (port->num_pins != 1) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
						"Clock port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Parent->line,
				"Unknown port type %s", Parent->name);
	}

	ProcessPb_TypePort_Power(Parent, port, power_method);
}

static void ProcessInterconnect(INOUTP ezxml_t Parent, t_mode * mode) {
	int num_interconnect = 0;
	int num_complete, num_direct, num_mux;
	int i, j, k, L_index, num_annotations;
	int num_delay_constant, num_delay_matrix, num_C_constant, num_C_matrix,
			num_pack_pattern;
	const char *Prop;
	ezxml_t Cur, Prev;
	ezxml_t Cur2, Prev2;
	Cur = Cur2 = Prev = Prev2 = NULL;

	map<string, int> interc_names;
	pair<map<string, int>::iterator, bool> ret_interc_names;

	num_complete = num_direct = num_mux = 0;
	num_complete = CountChildren(Parent, "complete", 0);
	num_direct = CountChildren(Parent, "direct", 0);
	num_mux = CountChildren(Parent, "mux", 0);
	num_interconnect = num_complete + num_direct + num_mux;

	CheckXMLTagOrder(Parent);

	mode->num_interconnect = num_interconnect;
	mode->interconnect = (t_interconnect*) my_calloc(num_interconnect,
			sizeof(t_interconnect));

	i = 0;
	for (L_index = 0; L_index < 3; L_index++) {
		if (L_index == 0) {
			Cur = FindFirstElement(Parent, "complete", false);
		} else if (L_index == 1) {
			Cur = FindFirstElement(Parent, "direct", false);
		} else {
			Cur = FindFirstElement(Parent, "mux", false);
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

			Prop = FindProperty(Cur, "input", true);
			mode->interconnect[i].input_string = my_strdup(Prop);
			ezxml_set_attr(Cur, "input", NULL);

			Prop = FindProperty(Cur, "output", true);
			mode->interconnect[i].output_string = my_strdup(Prop);
			ezxml_set_attr(Cur, "output", NULL);

			Prop = FindProperty(Cur, "name", true);
			mode->interconnect[i].name = my_strdup(Prop);
			ezxml_set_attr(Cur, "name", NULL);

			ret_interc_names = interc_names.insert(
					pair<string, int>(mode->interconnect[i].name, 0));
			if (!ret_interc_names.second) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
						"Duplicate interconnect name: '%s' in mode: '%s'.\n",
						mode->interconnect[i].name, mode->name);
			}

			/* Process delay and capacitance annotations */
			num_annotations = 0;
			num_delay_constant = CountChildren(Cur, "delay_constant", 0);
			num_delay_matrix = CountChildren(Cur, "delay_matrix", 0);
			num_C_constant = CountChildren(Cur, "C_constant", 0);
			num_C_matrix = CountChildren(Cur, "C_matrix", 0);
			num_pack_pattern = CountChildren(Cur, "pack_pattern", 0);
			num_annotations = num_delay_constant + num_delay_matrix
					+ num_C_constant + num_C_matrix + num_pack_pattern;

			CheckXMLTagOrder(Cur);

			mode->interconnect[i].annotations =
					(t_pin_to_pin_annotation*) my_calloc(num_annotations,
							sizeof(t_pin_to_pin_annotation));
			mode->interconnect[i].num_annotations = num_annotations;

			k = 0;
			Cur2 = NULL;
			for (j = 0; j < 5; j++) {
				if (j == 0) {
					Cur2 = FindFirstElement(Cur, "delay_constant", false);
				} else if (j == 1) {
					Cur2 = FindFirstElement(Cur, "delay_matrix", false);
				} else if (j == 2) {
					Cur2 = FindFirstElement(Cur, "C_constant", false);
				} else if (j == 3) {
					Cur2 = FindFirstElement(Cur, "C_matrix", false);
				} else if (j == 4) {
					Cur2 = FindFirstElement(Cur, "pack_pattern", false);
				}
				while (Cur2 != NULL) {
					ProcessPinToPinAnnotations(Cur2,
							&(mode->interconnect[i].annotations[k]), NULL);

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
					false;

			//ProcessInterconnectMuxArch(Cur, &mode->interconnect[i]);

			/* get next iteration */
			Prev = Cur;
			Cur = Cur->next;
			FreeNode(Prev);
			i++;
		}
	}

	interc_names.clear();
	assert(i == num_interconnect);
}

static void ProcessMode(INOUTP ezxml_t Parent, t_mode * mode,
		bool * default_leakage_mode) {
	int i;
	const char *Prop;
	ezxml_t Cur, Prev;
	map<string, int> pb_type_names;
	pair<map<string, int>::iterator, bool> ret_pb_types;

	if (0 == strcmp(Parent->name, "pb_type")) {
		/* implied mode */
		mode->name = my_strdup(mode->parent_pb_type->name);
	} else {
		Prop = FindProperty(Parent, "name", true);
		mode->name = my_strdup(Prop);
		ezxml_set_attr(Parent, "name", NULL);
	}

	mode->num_pb_type_children = CountChildren(Parent, "pb_type", 0);
	if (mode->num_pb_type_children > 0) {
		mode->pb_type_children = (t_pb_type*) my_calloc(
				mode->num_pb_type_children, sizeof(t_pb_type));

		i = 0;
		Cur = FindFirstElement(Parent, "pb_type", true);
		while (Cur != NULL) {
			if (0 == strcmp(Cur->name, "pb_type")) {
				ProcessPb_Type(Cur, &mode->pb_type_children[i], mode);

				ret_pb_types = pb_type_names.insert(
						pair<string, int>(mode->pb_type_children[i].name, 0));
				if (!ret_pb_types.second) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
							"Duplicate pb_type name: '%s' in mode: '%s'.\n",
							mode->pb_type_children[i].name, mode->name);
				}

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

	/* Clear STL map used for duplicate checks */
	pb_type_names.clear();

	Cur = FindElement(Parent, "interconnect", true);
	ProcessInterconnect(Cur, mode);
	FreeNode(Cur);

}

/* Takes in the node ptr for the 'fc_in' and 'fc_out' elements and initializes
 * the appropriate fields of type. Unlinks the contents of the nodes. */
static void Process_Fc(ezxml_t Node, t_type_descriptor * Type, t_segment_inf *segments, int num_segments) {
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

	Type->is_Fc_frac = (bool *) my_malloc(Type->num_pins * sizeof(bool));
	Type->is_Fc_full_flex = (bool *) my_malloc(
			Type->num_pins * sizeof(bool));
	Type->Fc = (float **) alloc_matrix(0, Type->num_pins-1, 0, num_segments-1, sizeof(float));

	/* Load the default fc_in, if specified */
	Prop = FindProperty(Node, "default_in_type", false);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_in = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_in = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_in = FC_FULL;
		} else {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
					Prop);
		}
		switch (def_type_in) {
		case FC_FULL:
			def_in_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = FindProperty(Node, "default_in_val", true);
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
	Prop = FindProperty(Node, "default_out_type", false);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_out = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_out = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_out = FC_FULL;
		} else {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
					Prop);
		}
		switch (def_type_out) {
		case FC_FULL:
			def_out_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = FindProperty(Node, "default_out_val", true);
			def_out_val = (float) atof(Prop2);
			ezxml_set_attr(Node, "default_out_val", NULL);
			break;
		default:
			def_out_val = -1;
		}
		/* Release the property */
		ezxml_set_attr(Node, "default_out_type", NULL);
	}

	/* Go though all the pins and segments in Type, assign def_in_val and def_out_val
	 * to entries in Type->Fc array corresponding to input pins and output
	 * pins. Also sets up the type of fc of the pin in the bool arrays    */
	for (ipin = 0; ipin < Type->num_pins; ipin++) {
		iclass = Type->pin_class[ipin];
		for (int iseg = 0; iseg < num_segments; iseg++){
			if (Type->class_inf[iclass].type == DRIVER) {
				Type->Fc[ipin][iseg] = def_out_val;
				Type->is_Fc_full_flex[ipin] =
						(def_type_out == FC_FULL) ? true : false;
				Type->is_Fc_frac[ipin] = (def_type_out == FC_FRAC) ? true : false;
			} else if (Type->class_inf[iclass].type == RECEIVER) {
				Type->Fc[ipin][iseg] = def_in_val;
				Type->is_Fc_full_flex[ipin] =
						(def_type_in == FC_FULL) ? true : false;
				Type->is_Fc_frac[ipin] = (def_type_in == FC_FRAC) ? true : false;
			} else {
				Type->Fc[ipin][iseg] = -1;
				Type->is_Fc_full_flex[ipin] = false;
				Type->is_Fc_frac[ipin] = false;
			}
		}
	}

	/* pin-based fc overrides and segment-based fc overrides should not exist at the same time */
	if (ezxml_child(Node, "pin") != NULL && ezxml_child(Node, "segment") != NULL){
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
				"Complex block 'fc' is allowed to specify pin-based fc overrides OR segment-based fc overrides, not both.\n");
	}

	/* Now, check for pin-based fc override - look for pin child. */
	Child = ezxml_child(Node, "pin");
	while (Child != NULL) {
		/* Get all the properties of the child first */
		Prop = FindProperty(Child, "name", true);
		if (Prop == NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
					"Pin child with no name is not allowed.\n");
		}
		ezxml_set_attr(Child, "name", NULL);

		Prop2 = FindProperty(Child, "fc_type", true);
		if (Prop2 != NULL) {
			if (0 == strcmp(Prop2, "abs")) {
				ovr_type = FC_ABS;
			} else if (0 == strcmp(Prop2, "frac")) {
				ovr_type = FC_FRAC;
			} else if (0 == strcmp(Prop2, "full")) {
				ovr_type = FC_FULL;
			} else {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
						"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
						Prop2);
			}
			switch (ovr_type) {
			case FC_FULL:
				ovr_val = 0.0;
				break;
			case FC_ABS:
			case FC_FRAC:
				Prop2 = FindProperty(Child, "fc_val", true);
				if (Prop2 == NULL) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
							"Pin child with no fc_val specified is not allowed.\n");
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
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
							"Invalid name for pin child, name should be in the format \"port_name\" or \"port_name [end_pin_index:start_pin_index]\","
									"The end_pin_index and start_pin_index can be the same.\n");
				}
				if (end_pin_index < 0 || start_pin_index < 0) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
							"The pin_index should not be a negative value.\n");
				}
				if (end_pin_index < start_pin_index) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
							"The end_pin_index should be not be less than start_pin_index.\n");
				}
			}

			/* Find the matching port_name in Type */
			/* TODO: Check for pins assigned more than one override fc's - right now assigning the last value specified. */
			iport_pin = 0;
			port_found = false;
			for (iport = 0;
					((iport < Type->pb_type->num_ports) && (port_found == false));
					iport++) {
				if (strcmp(Prop, Type->pb_type->ports[iport].name) == 0) {
					/* This is the port, the start_pin_index and end_pin_index offset starts
					 * here. The indices are inclusive. */
					port_found = true;
					if (end_pin_index > Type->pb_type->ports[iport].num_pins) {
						vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
								"The end_pin_index for this port: %d cannot be greater than the number of pins in this port: %d.\n",
								end_pin_index,
								Type->pb_type->ports[iport].num_pins);
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
						if (ovr_val != Type->Fc[iport_pin + curr_pin][0]
									|| Type->is_Fc_full_flex[iport_pin
											+ curr_pin]
											!= (ovr_type == FC_FULL) ? true :
							false
									|| Type->is_Fc_frac[iport_pin + curr_pin]
											!= (ovr_type == FC_FRAC) ?
									true : false) {

							for (int iseg = 0; iseg < num_segments; iseg++){
								Type->Fc[iport_pin + curr_pin][iseg] = ovr_val;
							}
							Type->is_Fc_full_flex[iport_pin + curr_pin] =
									(ovr_type == FC_FULL) ? true : false;
							Type->is_Fc_frac[iport_pin + curr_pin] =
									(ovr_type == FC_FRAC) ? true : false;

						} else {

							vpr_throw(VPR_ERROR_ARCH, arch_file_name,
									Child->line,
									"Multiple Fc override detected!\n");
						}
					}

				} else {
					/* This is not the matching port, move the iport_pin index forward. */
					iport_pin += Type->pb_type->ports[iport].num_pins;
				}
			} /* Finish going through all the ports in pb_type looking for the pin child's port. */

			/* The override pin child is not in any of the ports in pb_type. */
			if (port_found == false) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
						"The port \"%s\" cannot be found.\n", Prop);
			}

			/* End of case where fc_type of pin_child is specified. */
		} else {
			/* fc_type of pin_child is not specified. Error out. */
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
					"Pin child with no fc_type specified is not allowed.\n");
		}

		/* Find next child and frees up the current child. */
		Junk = Child;
		Child = ezxml_next(Child);
		FreeNode(Junk);

	} /* End of processing pin children */

	/* now check for segment-based overrides. earlier in this function we already checked that both kinds of
	   overrides haven't been specified */
	Child = ezxml_child(Node, "segment");
	while (Child != NULL){
		const char *segment_name;
		int seg_ind;
		float in_val;
		float out_val;

		/* get name */
		Prop = FindProperty(Child, "name", true);
		ezxml_set_attr(Child, "name", NULL);
		segment_name = Prop;

		/* get fc_in */
		Prop = FindProperty(Child, "in_val", true);
		ezxml_set_attr(Child, "in_val", NULL);
		in_val = (float) atof(Prop);


		/* get fc_out */
		Prop = FindProperty(Child, "out_val", true);
		ezxml_set_attr(Child, "out_val", NULL);
		out_val = (float) atof(Prop);


		/* get segment index for which Fc should be updated */
		seg_ind = UNDEFINED;
		for (int iseg = 0; iseg < num_segments; iseg++){
			if ( strcmp(segment_name, segments[iseg].name) == 0 ){
				seg_ind = iseg;
				break;
			}
		}
		if (seg_ind == UNDEFINED){
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Child->line,
				"Segment-based Fc override specified segment name that cannot be found in segment list: %s\n", segment_name);
		}

		/* update Fc for this segment across all driver/receiver pins */
		for (ipin = 0; ipin < Type->num_pins; ipin++){
			iclass = Type->pin_class[ipin];

			if (Type->class_inf[iclass].type == DRIVER){
				Type->Fc[ipin][seg_ind] = out_val;
			} else if (Type->class_inf[iclass].type == RECEIVER){
				Type->Fc[ipin][seg_ind] = in_val;
			} else {
				/* do nothing */
			}
		}


		/* Find next child and frees up the current child. */
		Junk = Child;
		Child = ezxml_next(Child);
		FreeNode(Junk);
	}

}

/* Thie processes attributes of the 'type' tag and then unlinks them */
static void ProcessComplexBlockProps(ezxml_t Node, t_type_descriptor * Type) {
	const char *Prop;

	/* Load type name */
	Prop = FindProperty(Node, "name", true);
	Type->name = my_strdup(Prop);
	ezxml_set_attr(Node, "name", NULL);

	/* Load properties */
	Type->capacity = GetIntProperty(Node, "capacity", false, 1); /* TODO: Any block with capacity > 1 that is not I/O has not been tested, must test */
	Type->width = GetIntProperty(Node, "width", false, 1);
	Type->height = GetIntProperty(Node, "height", false, 1);
	Type->area = GetFloatProperty(Node, "area", false, UNDEFINED);

	if (atof(Prop) < 0) {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
				"Area for type %s must be non-negative\n", Type->name);
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
	/* std::maps for checking duplicates */
	map<string, int> model_name_map;
	map<string, int> model_port_map;
	pair<map<string, int>::iterator, bool> ret_map_name;
	pair<map<string, int>::iterator, bool> ret_map_port;

	L_index = NUM_MODELS_IN_LIBRARY;

	arch->models = NULL;
	child = ezxml_child(Node, "model");
	while (child != NULL) {
		temp = (t_model*) my_calloc(1, sizeof(t_model));
		temp->used = 0;
		temp->inputs = temp->outputs = NULL;
		temp->instances = NULL;
		Prop = FindProperty(child, "name", true);
		temp->name = my_strdup(Prop);
		ezxml_set_attr(child, "name", NULL);
		temp->pb_types = NULL;
		temp->index = L_index;
		L_index++;

		/* Try insert new model, check if already exist at the same time */
		ret_map_name = model_name_map.insert(pair<string, int>(temp->name, 0));
		if (!ret_map_name.second) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, child->line,
					"Duplicate model name: '%s'.\n", temp->name);
		}

		/* Process the inputs */
		p = ezxml_child(child, "input_ports");
		junkp = p;
		if (p == NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, child->line,
					"Required input ports not found for element '%s'.\n",
					temp->name);
		}
		p = ezxml_child(p, "port");
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) my_calloc(1, sizeof(t_model_ports));
				Prop = FindProperty(p, "name", true);
				tp->name = my_strdup(Prop);
				ezxml_set_attr(p, "name", NULL);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->inputs;
				tp->dir = IN_PORT;
				tp->is_non_clock_global = GetboolProperty(p,
						"is_non_clock_global", false, false);
				tp->is_clock = false;
				Prop = FindProperty(p, "is_clock", false);
				if (Prop && my_atoi(Prop) != 0) {
					tp->is_clock = true;
				}
				ezxml_set_attr(p, "is_clock", NULL);
				if (tp->is_clock == true && tp->is_non_clock_global == true) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, p->line,
							"Signal cannot be both a clock and a non-clock signal simultaneously\n");
				}

				/* Try insert new port, check if already exist at the same time */
				ret_map_port = model_port_map.insert(
						pair<string, int>(tp->name, 0));
				if (!ret_map_port.second) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, p->line,
							"Duplicate model input port name: '%s'.\n",
							tp->name);
				}

				temp->inputs = tp;
				junk = p;
				p = ezxml_next(p);
				FreeNode(junk);
			}
		} else /* No input ports? */
		{
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, child->line,
					"Required input ports not found for element '%s'.\n",
					temp->name);
		}
		FreeNode(junkp);

		/* Process the outputs */
		p = ezxml_child(child, "output_ports");
		junkp = p;
		if (p == NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, child->line,
					"Required output ports not found for element '%s'.\n",
					temp->name);
		}
		p = ezxml_child(p, "port");
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) my_calloc(1, sizeof(t_model_ports));
				Prop = FindProperty(p, "name", true);
				tp->name = my_strdup(Prop);
				ezxml_set_attr(p, "name", NULL);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->outputs;
				tp->dir = OUT_PORT;
				Prop = FindProperty(p, "is_clock", false);
				if (Prop && my_atoi(Prop) != 0) {
					tp->is_clock = true;
				}
				ezxml_set_attr(p, "is_clock", NULL);
				if (tp->is_clock == true && tp->is_non_clock_global == true) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, p->line,
							"Signal cannot be both a clock and a non-clock signal simultaneously\n");
				}

				/* Try insert new output port, check if already exist at the same time */
				ret_map_port = model_port_map.insert(
						pair<string, int>(tp->name, 0));
				if (!ret_map_port.second) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, p->line,
							"Duplicate model output port name: '%s'.\n",
							tp->name);
				}

				temp->outputs = tp;
				junk = p;
				p = ezxml_next(p);
				FreeNode(junk);
			}
		} else /* No output ports? */
		{
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, child->line,
					"Required output ports not found for element '%s'.\n",
					temp->name);
		}

		FreeNode(junkp);

		/* Clear port map for next model */
		model_port_map.clear();
		/* Push new model onto model stack */
		temp->next = arch->models;
		arch->models = temp;
		/* Find next model */
		junk = child;
		child = ezxml_next(child);
		FreeNode(junk);
	}
	model_port_map.clear();
	model_name_map.clear();
	return;
}

/* Takes in node pointing to <layout> and loads all the
 * child type objects. Unlinks the entire <layout> node
 * when complete. */
static void ProcessLayout(INOUTP ezxml_t Node, OUTP struct s_arch *arch) {
	const char *Prop;

	arch->clb_grid.IsAuto = true;

	/* Load width and height if applicable */
	Prop = FindProperty(Node, "width", false);
	if (Prop != NULL) {
		arch->clb_grid.IsAuto = false;
		arch->clb_grid.W = my_atoi(Prop);
		ezxml_set_attr(Node, "width", NULL);

		arch->clb_grid.H = GetIntProperty(Node, "height", true, UNDEFINED);
	}

	/* Load aspect ratio if applicable */
	Prop = FindProperty(Node, "auto", arch->clb_grid.IsAuto);
	if (Prop != NULL) {
		if (arch->clb_grid.IsAuto == false) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Auto-sizing, width and height cannot be specified\n");
		}
		arch->clb_grid.Aspect = (float) atof(Prop);
		ezxml_set_attr(Node, "auto", NULL);
		if (arch->clb_grid.Aspect <= 0) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Grid aspect ratio is less than or equal to zero %g\n",
					arch->clb_grid.Aspect);
		}
	}
}

/* Takes in node pointing to <device> and loads all the
 * child type objects. Unlinks the entire <device> node
 * when complete. */
static void ProcessDevice(INOUTP ezxml_t Node, OUTP struct s_arch *arch,
		INP bool timing_enabled) {
	const char *Prop;
	ezxml_t Cur;
	bool custom_switch_block = false;

	ProcessSizingTimingIpinCblock(Node, arch, timing_enabled);

	Cur = FindElement(Node, "area", true);
	arch->grid_logic_tile_area = GetFloatProperty(Cur, "grid_logic_tile_area",
			false, 0);
	FreeNode(Cur);

	Cur = FindElement(Node, "chan_width_distr", false);
	if (Cur != NULL) {
		ProcessChanWidthDistr(Cur, arch);
		FreeNode(Cur);
	}

	Cur = FindElement(Node, "switch_block", true);
	Prop = FindProperty(Cur, "type", true);
	if (strcmp(Prop, "wilton") == 0) {
		arch->SBType = WILTON;
	} else if (strcmp(Prop, "universal") == 0) {
		arch->SBType = UNIVERSAL;
	} else if (strcmp(Prop, "subset") == 0) {
		arch->SBType = SUBSET;
	} else if (strcmp(Prop, "custom") == 0) {
		arch->SBType = CUSTOM;
		custom_switch_block = true;
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
				"Unknown property %s for switch block type x\n", Prop);
	}
	ezxml_set_attr(Cur, "type", NULL);

	arch->Fs = GetIntProperty(Cur, "fs", !custom_switch_block, 3);

	FreeNode(Cur);
}

/* Processes the sizing, timing, and ipin_cblock child objects of the 'device' node.
   We can specify an ipin cblock's info through the sizing/timing nodes (legacy),
   OR through the ipin_cblock node which specifies the info using the index of a switch. */
static void ProcessSizingTimingIpinCblock(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch, INP bool timing_enabled) {

	ezxml_t Cur;

	bool ipin_cblock_info_as_switch = false;
	arch->ipin_cblock_switch_name = NULL;
	arch->ipin_mux_trans_size = UNDEFINED;
	arch->C_ipin_cblock = UNDEFINED;
	arch->T_ipin_cblock = UNDEFINED;

	Cur = FindElement(Node, "ipin_cblock", false);
	if (Cur) {
		const char *switch_name;
		/* if an ipin_cblock node exists, then ipin cblock delay/capacitance/area are specified
		   through a corresponding switch. in this case, ipin cblock info cannot be specified 
		   through the timing/sizing nodes */
		switch_name = FindProperty(Cur, "switch", true);
		arch->ipin_cblock_switch_name = my_strdup(switch_name);
		ezxml_set_attr(Cur, "switch", NULL);
		ipin_cblock_info_as_switch = true;
		FreeNode(Cur);
	}

	Cur = FindElement(Node, "sizing", true);
	arch->R_minW_nmos = GetFloatProperty(Cur, "R_minW_nmos", timing_enabled, 0);
	arch->R_minW_pmos = GetFloatProperty(Cur, "R_minW_pmos", timing_enabled, 0);
	arch->ipin_mux_trans_size = GetFloatProperty(Cur, "ipin_mux_trans_size",
			false, 0);
	if (arch->ipin_mux_trans_size && ipin_cblock_info_as_switch){
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
				"If ipin cblock mux trans size is specified via a switch, it should not be specified again via sizing/timing nodes\n");
	}
	FreeNode(Cur);

	/* currently only ipin cblock info is specified in the timing node */
	Cur = FindElement(Node, "timing", (bool)(timing_enabled && !ipin_cblock_info_as_switch));
	if (Cur != NULL) {
		arch->C_ipin_cblock = GetFloatProperty(Cur, "C_ipin_cblock", false, 0);
		arch->T_ipin_cblock = GetFloatProperty(Cur, "T_ipin_cblock", false, 0);

		if (arch->C_ipin_cblock && ipin_cblock_info_as_switch){
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
					"If ipin cblock C is specified via a switch, it should not be specified again via sizing/timing nodes\n");
		}
		if (arch->T_ipin_cblock && ipin_cblock_info_as_switch){
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Cur->line,
					"If ipin cblock T is specified via a switch, it should not be specified again via sizing/timing nodes\n");
		}
		FreeNode(Cur);
	}
}

/* Takes in node pointing to <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire <chan_width_distr> node
 * when complete. */
static void ProcessChanWidthDistr(INOUTP ezxml_t Node,
		OUTP struct s_arch *arch) {
	ezxml_t Cur;

	Cur = FindElement(Node, "io", true);
	arch->Chans.chan_width_io = GetFloatProperty(Cur, "width", true, UNDEFINED);
	FreeNode(Cur);
	Cur = FindElement(Node, "x", true);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_x_dist);
	FreeNode(Cur);
	Cur = FindElement(Node, "y", true);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_y_dist);
	FreeNode(Cur);
}

/* Takes in node within <chan_width_distr> and loads all the
 * child type objects. Unlinks the entire node when complete. */
static void ProcessChanWidthDistrDir(INOUTP ezxml_t Node, OUTP t_chan * chan) {
	const char *Prop;

	bool hasXpeak, hasWidth, hasDc;
	hasXpeak = hasWidth = hasDc = false;
	Prop = FindProperty(Node, "distr", true);
	if (strcmp(Prop, "uniform") == 0) {
		chan->type = UNIFORM;
	} else if (strcmp(Prop, "gaussian") == 0) {
		chan->type = GAUSSIAN;
		hasXpeak = hasWidth = hasDc = true;
	} else if (strcmp(Prop, "pulse") == 0) {
		chan->type = PULSE;
		hasXpeak = hasWidth = hasDc = true;
	} else if (strcmp(Prop, "delta") == 0) {
		hasXpeak = hasDc = true;
		chan->type = DELTA;
	} else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
				"Unknown property %s for chan_width_distr x\n", Prop);
	}
	ezxml_set_attr(Node, "distr", NULL);
	chan->peak = GetFloatProperty(Node, "peak", true, UNDEFINED);
	chan->width = GetFloatProperty(Node, "width", hasWidth, 0);
	chan->xpeak = GetFloatProperty(Node, "xpeak", hasXpeak, 0);
	chan->dc = GetFloatProperty(Node, "dc", hasDc, 0);
}

static void SetupEmptyType(void) {
	t_type_descriptor * type;
	type = &cb_type_descriptors[EMPTY_TYPE->index];
	type->name = "<EMPTY>";
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
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
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

/* Takes in node pointing to <typelist> and loads all the
 * child type objects. Unlinks the entire <typelist> node
 * when complete. */
static void ProcessComplexBlocks(INOUTP ezxml_t Node,
		OUTP t_type_descriptor ** Types, OUTP int *NumTypes,
		bool timing_enabled, s_arch arch) {
	ezxml_t CurType, Prev;
	ezxml_t Cur;
	t_type_descriptor * Type;
	int i;
	map<string, int> pb_type_descriptors;
	pair<map<string, int>::iterator, bool> ret_pb_type_descriptors;
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

		ret_pb_type_descriptors = pb_type_descriptors.insert(
				pair<string, int>(Type->name, 0));
		if (!ret_pb_type_descriptors.second) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, CurType->line,
					"Duplicate pb_type descriptor name: '%s'.\n", Type->name);
		}

		/* Load pb_type info */
		Type->pb_type = (t_pb_type*) my_malloc(sizeof(t_pb_type));
		Type->pb_type->name = my_strdup(Type->name);
		if (i == IO_TYPE_INDEX) {
			if (strcmp(Type->name, "io") != 0) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, CurType->line,
						"First complex block must be named \"io\" and define the inputs and outputs for the FPGA");
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
		Cur = FindElement(CurType, "pinlocations", true);
		SetupPinLocationsAndPinClasses(Cur, Type);
		FreeNode(Cur);
		Cur = FindElement(CurType, "gridlocations", true);
		SetupGridLocations(Cur, Type);
		FreeNode(Cur);

		/* Load Fc */
		Cur = FindElement(CurType, "fc", true);
		Process_Fc(Cur, Type, arch.Segments, arch.num_segments);
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
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
				"grid location type 'fill' must be specified.\n");
	}
	pb_type_descriptors.clear();
}

/* Loads the given architecture file. Currently only
 * handles type information */
void XmlReadArch(INP const char *ArchFile, INP bool timing_enabled,
		OUTP struct s_arch *arch, OUTP t_type_descriptor ** Types,
		OUTP int *NumTypes) {
	ezxml_t Cur = NULL, Next;
	const char *Prop;
	bool power_reqd;

	if (check_file_name_extension(ArchFile, ".xml") == false) {
		vpr_printf_warning(__FILE__, __LINE__,
				"Architecture file '%s' may be in incorrect format. "
						"Expecting .xml format for architecture files.\n",
				ArchFile);
	}

	/* Parse the file */
	Cur = ezxml_parse_file(ArchFile);
	if (NULL == Cur) {
		vpr_throw(VPR_ERROR_ARCH, ArchFile, 0,
				"Unable to find/load architecture file '%s'.\n", ArchFile);
	}

	arch_file_name = ArchFile;

	/* Root node should be architecture */
	CheckElement(Cur, "architecture");
	/* TODO: do version processing properly with string delimiting on the . */
	Prop = FindProperty(Cur, "version", false);
	if (Prop != NULL) {
		if (atof(Prop) > atof(VPR_VERSION)) {
			vpr_printf_warning(__FILE__, __LINE__,
					"This architecture version is for VPR %f while your current VPR version is " VPR_VERSION ", compatability issues may arise\n",
					atof(Prop));
		}
		ezxml_set_attr(Cur, "version", NULL);
	}

	/* Process models */
	Next = FindElement(Cur, "models", true);
	ProcessModels(Next, arch);
	FreeNode(Next);
	CreateModelLibrary(arch);

	/* Process layout */
	Next = FindElement(Cur, "layout", true);
	ProcessLayout(Next, arch);
	FreeNode(Next);

	/* Process device */
	Next = FindElement(Cur, "device", true);
	ProcessDevice(Next, arch, timing_enabled);
	FreeNode(Next);

	/* Process switches */
	Next = FindElement(Cur, "switchlist", true);
	ProcessSwitches(Next, &(arch->Switches), &(arch->num_switches),
			timing_enabled);
	FreeNode(Next);

	/* Process switchblocks. This depends on switches */
	bool switchblocklist_required = (arch->SBType == CUSTOM);	//require this section only if custom switchblocks are used

	/* Process segments. This depends on switches */
	Next = FindElement(Cur, "segmentlist", true);
	ProcessSegments(Next, &(arch->Segments), &(arch->num_segments),
			arch->Switches, arch->num_switches, timing_enabled, switchblocklist_required);
	FreeNode(Next);

	Next = FindElement(Cur, "switchblocklist", switchblocklist_required);
	if (Next){
		ProcessSwitchblocks(Next, &(arch->switchblocks), arch->Switches, arch->num_switches);
		FreeNode(Next);
	}

	/* Process types */
	Next = FindElement(Cur, "complexblocklist", true);
	ProcessComplexBlocks(Next, Types, NumTypes, timing_enabled, *arch);
	FreeNode(Next);

	#ifdef INTERPOSER_BASED_ARCHITECTURE	
	/* find the least common multiple of block heights
	 * for interposer based architectures, a culine cannot go through a block */
	arch->lcm_of_block_heights = 1;
	for(int i=0; i < *NumTypes; ++i)
	{
		t_type_descriptor * Type = &(*Types)[i];
		if(Type!=0)
		{
			arch->lcm_of_block_heights = lcm(arch->lcm_of_block_heights, Type->height);
		}
	}
	#endif

	/* Process directs */
	Next = FindElement(Cur, "directlist", false);
	if (Next) {
		ProcessDirects(Next, &(arch->Directs), &(arch->num_directs),
                arch->Switches, arch->num_switches,
				timing_enabled);
		FreeNode(Next);
	}

	/* Process architecture power information */

	/* If arch->power has been initialized, meaning the user has requested power estimation,
	 * then the power architecture information is required.
	 */
	if (arch->power) {
		power_reqd = true;
	} else {
		power_reqd = false;
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
		INP struct s_arch_switch_inf *Switches, INP int NumSwitches,
		INP bool timing_enabled, INP bool switchblocklist_required) {
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

		/* Get segment name */
		tmp = FindProperty(Node, "name", false);
		if (tmp) {
			(*Segs)[i].name = my_strdup(tmp);
		} else {
			/* if swich block is "custom", then you have to provide a name for segment */
			if (switchblocklist_required) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"No name specified for the segment #%d.\n", i);

			}
			/* set name to default: "unnamed_segment_<segment_index>" */
			stringstream ss;
			ss << "unnamed_segment_" << i;
			string dummy = ss.str();
			tmp = dummy.c_str();
			(*Segs)[i].name = my_strdup(tmp);
		}
		ezxml_set_attr(Node, "name", NULL);

		/* Get segment length */
		length = 1; /* DEFAULT */
		tmp = FindProperty(Node, "length", false);
		if (tmp) {
			if (strcmp(tmp, "longline") == 0) {
				(*Segs)[i].longline = true;
			} else {
				length = my_atoi(tmp);
			}
		}
		(*Segs)[i].length = length;
		ezxml_set_attr(Node, "length", NULL);

		/* Get the frequency */
		(*Segs)[i].frequency = 1; /* DEFAULT */
		tmp = FindProperty(Node, "freq", false);
		if (tmp) {
			(*Segs)[i].frequency = (int) (atof(tmp) * MAX_CHANNEL_WIDTH);
		}
		ezxml_set_attr(Node, "freq", NULL);

		/* Get timing info */
		(*Segs)[i].Rmetal = GetFloatProperty(Node, "Rmetal", timing_enabled, 0);
		(*Segs)[i].Cmetal = GetFloatProperty(Node, "Cmetal", timing_enabled, 0);

		/* Get Power info */
		/*
		 (*Segs)[i].Cmetal_per_m = GetFloatProperty(Node, "Cmetal_per_m", false,
		 0.);*/

		/* Get the type */
		tmp = FindProperty(Node, "type", true);
		if (0 == strcmp(tmp, "bidir")) {
			(*Segs)[i].directionality = BI_DIRECTIONAL;
		}

		else if (0 == strcmp(tmp, "unidir")) {
			(*Segs)[i].directionality = UNI_DIRECTIONAL;
		}

		else {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Invalid switch type '%s'.\n", tmp);
		}
		ezxml_set_attr(Node, "type", NULL);

		/* Get the wire and opin switches, or mux switch if unidir */
		if (UNI_DIRECTIONAL == (*Segs)[i].directionality) {
			SubElem = FindElement(Node, "mux", true);
			tmp = FindProperty(SubElem, "name", true);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, SubElem->line,
						"'%s' is not a valid mux name.\n", tmp);
			}
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);

			/* Unidir muxes must have the same switch
			 * for wire and opin fanin since there is
			 * really only the mux in unidir. */
			(*Segs)[i].arch_wire_switch = j;
			(*Segs)[i].arch_opin_switch = j;
		}

		else {
			assert(BI_DIRECTIONAL == (*Segs)[i].directionality);
			SubElem = FindElement(Node, "wire_switch", true);
			tmp = FindProperty(SubElem, "name", true);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, SubElem->line,
						"'%s' is not a valid wire_switch name.\n", tmp);
			}
			(*Segs)[i].arch_wire_switch = j;
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);
			SubElem = FindElement(Node, "opin_switch", true);
			tmp = FindProperty(SubElem, "name", true);

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, SubElem->line,
						"'%s' is not a valid opin_switch name.\n", tmp);
			}
			(*Segs)[i].arch_opin_switch = j;
			ezxml_set_attr(SubElem, "name", NULL);
			FreeNode(SubElem);
		}

		/* Setup the CB list if they give one, otherwise use full */
		(*Segs)[i].cb_len = length;
		(*Segs)[i].cb = (bool *) my_malloc(length * sizeof(bool));
		for (j = 0; j < length; ++j) {
			(*Segs)[i].cb[j] = true;
		}
		SubElem = FindElement(Node, "cb", false);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].cb, length);
			FreeNode(SubElem);
		}

		/* Setup the SB list if they give one, otherwise use full */
		(*Segs)[i].sb_len = (length + 1);
		(*Segs)[i].sb = (bool *) my_malloc((length + 1) * sizeof(bool));
		for (j = 0; j < (length + 1); ++j) {
			(*Segs)[i].sb[j] = true;
		}
		SubElem = FindElement(Node, "sb", false);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].sb, (length + 1));
			FreeNode(SubElem);
		}
		FreeNode(Node);
	}
}

/* Processes the switchblocklist section from the xml architecture file. 
   See vpr/SRC/route/build_switchblocks.c for a detailed description of this 
   switch block format */
static void ProcessSwitchblocks(INOUTP ezxml_t Parent, OUTP vector<t_switchblock_inf> *switchblocks,
				INP t_arch_switch_inf *switches, INP int num_switches){

	ezxml_t Node;
	ezxml_t SubElem;
	const char *tmp;

	/* get the number of switchblocks */
	int num_switchblocks = CountChildren(Parent, "switchblock", 1);
	switchblocks->reserve(num_switchblocks);

	/* read-in all switchblock data */
	for (int i_sb = 0; i_sb < num_switchblocks; i_sb++){
		/* use a temp variable which will be assigned to switchblocks later */
		t_switchblock_inf sb;

		Node = ezxml_child(Parent, "switchblock");

		/* get name */
		tmp = FindProperty(Node, "name", true);
		if (tmp){
			sb.name = tmp;
		}
		ezxml_set_attr(Node, "name", NULL);

		/* get type */
		tmp = FindProperty(Node, "type", true);
		if (tmp){
			if (0 == strcmp(tmp, "bidir")){
				sb.directionality = BI_DIRECTIONAL;
			} else if (0 == strcmp(tmp, "unidir")){
				sb.directionality = UNI_DIRECTIONAL;
			} else {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line, "Unsopported switchblock type: %s\n", tmp);
			}
		}
		ezxml_set_attr(Node, "type", NULL);

		/* get the switchblock location */
		SubElem = ezxml_child(Node, "switchblock_location");
		tmp = FindProperty(SubElem, "type", true);
		if (tmp){
			if (strcmp(tmp, "EVERYWHERE") == 0){
				sb.location = E_EVERYWHERE;
			} else if (strcmp(tmp, "PERIMETER") == 0){
				sb.location = E_PERIMETER;
			} else if (strcmp(tmp, "CORE") == 0){
				sb.location = E_CORE;
			} else if (strcmp(tmp, "CORNER") == 0){
				sb.location = E_CORNER;
			} else if (strcmp(tmp, "FRINGE") == 0){
				sb.location = E_FRINGE;
			} else {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line, "unrecognized switchblock location: %s\n", tmp);
			}
		}
		ezxml_set_attr(SubElem, "type", NULL);
		FreeNode(SubElem);

		/* get switchblock permutation functions */
		SubElem = ezxml_child(Node, "switchfuncs");
		read_sb_switchfuncs(SubElem, &(sb));
		FreeNode(SubElem);
		
		read_sb_wireconns(switches, num_switches, Node, &(sb));

		/* assign the sb to the switchblocks vector */		
		switchblocks->push_back(sb);

		/* run error checks on switch blocks */
		check_switchblock(&sb);

		FreeNode(Node);
	}

	return;
}


static void ProcessCB_SB(INOUTP ezxml_t Node, INOUTP bool * list,
		INP int len) {
	const char *tmp = NULL;
	int i;

	/* Check the type. We only support 'pattern' for now.
	 * Should add frac back eventually. */
	tmp = FindProperty(Node, "type", true);
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
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
							"CB or SB depopulation is too long. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
				}
				list[i] = true;
				++i;
				break;
			case 'F':
			case '0':
				if (i >= len) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
							"CB or SB depopulation is too long. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
				}
				list[i] = false;
				++i;
				break;
			default:
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
						"Invalid character %c in CB or SB depopulation list.\n",
						*tmp);
			}
			++tmp;
		}
		if (i < len) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"CB or SB depopulation is too short. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
		}

		/* Free content string */
		ezxml_set_txt(Node, "");
	}

	else {
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
				"'%s' is not a valid type for specifying cb and sb depopulation.\n",
				tmp);
	}
	ezxml_set_attr(Node, "type", NULL);
}

static void ProcessSwitches(INOUTP ezxml_t Parent,
		OUTP struct s_arch_switch_inf **Switches, OUTP int *NumSwitches,
		INP bool timing_enabled) {
	int i, j;
	const char *type_name;
	const char *switch_name;
	const char *buf_size;

	bool has_buf_size;
	ezxml_t Node;
	has_buf_size = false;

	/* Count the children and check they are switches */
	*NumSwitches = CountChildren(Parent, "switch", 1);

	/* Alloc switch list */
	*Switches = NULL;
	if (*NumSwitches > 0) {
		(*Switches) = new s_arch_switch_inf[(*NumSwitches)];
	}

	/* Load the switches. */
	for (i = 0; i < *NumSwitches; ++i) {
		Node = ezxml_child(Parent, "switch");
		switch_name = FindProperty(Node, "name", true);
		type_name = FindProperty(Node, "type", true);

		/* Check for switch name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Switches)[j].name, switch_name)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
						"Two switches with the same name '%s' were found.\n",
						switch_name);
			}
		}
		(*Switches)[i].name = my_strdup(switch_name);
		ezxml_set_attr(Node, "name", NULL);

		/* Figure out the type of switch. */
		if (0 == strcmp(type_name, "mux")) {
			(*Switches)[i].buffered = true;
			has_buf_size = true;
		}

		else if (0 == strcmp(type_name, "pass_trans")) {
			(*Switches)[i].buffered = false;
		}

		else if (0 == strcmp(type_name, "buffer")) {
			(*Switches)[i].buffered = true;
		}

		else {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Invalid switch type '%s'.\n", type_name);
		}
		ezxml_set_attr(Node, "type", NULL);
		(*Switches)[i].R = GetFloatProperty(Node, "R", timing_enabled, 0);
		(*Switches)[i].Cin = GetFloatProperty(Node, "Cin", timing_enabled, 0);
		(*Switches)[i].Cout = GetFloatProperty(Node, "Cout", timing_enabled, 0);
		//(*Switches)[i].Tdel = GetFloatProperty(Node, "Tdel", timing_enabled, 0);
		ProcessSwitchTdel(Node, timing_enabled, i, (*Switches));
		(*Switches)[i].buf_size = GetFloatProperty(Node, "buf_size",
				has_buf_size, 0);
		(*Switches)[i].mux_trans_size = GetFloatProperty(Node, "mux_trans_size",
				false, 1);

		buf_size = FindProperty(Node, "power_buf_size", false);
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

/* Processes the switch delay. Switch delay can be specified in two ways. 
   First way: switch delay is specified as a constant via the property Tdel in the switch node. 
   Second way: switch delay is specified as a function of the switch fan-in. In this 
               case, multiple nodes in the form

               <Tdel num_inputs="1" delay="3e-11"/>

               are specified as children of the switch node. In this case, Tdel
               is not included as a property of the switch node (first way). */
static void ProcessSwitchTdel(INOUTP ezxml_t Node, INP bool timing_enabled,
		INP int switch_index, OUTP s_arch_switch_inf *Switches){

	float Tdel_prop_value;
	int num_Tdel_children;

	/* check if switch node has the Tdel property */
	bool has_Tdel_prop = false;
	Tdel_prop_value = GetFloatProperty(Node, "Tdel", false, UNDEFINED);
	if (Tdel_prop_value != UNDEFINED){
		has_Tdel_prop = true;
	}

	/* check if switch node has Tdel children */
	bool has_Tdel_children = false;
	num_Tdel_children = CountChildren(Node, "Tdel", 0);
	if (num_Tdel_children != 0){
		has_Tdel_children = true;
	}

	/* delay should not be specified as a Tdel property AND a Tdel child */
	if (has_Tdel_prop && has_Tdel_children){
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
				"Switch delay should be specified as EITHER a Tdel property OR as a child of the switch node, not both");
	}

	/* get pointer to the switch's Tdel map, then read-in delay data into this map */
	std::map<int, double> *Tdel_map = &Switches[switch_index].Tdel_map;
	if (has_Tdel_prop){
		/* delay specified as a constant */
		if (Tdel_map->count(UNDEFINED)){
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"what the fuck");
		} else {
			(*Tdel_map)[UNDEFINED] = Tdel_prop_value;
		}
	} else if (has_Tdel_children) {
		/* Delay specified as a function of switch fan-in. 
		   Go through each Tdel child, read-in num_inputs and the delay value.
		   Insert this info into the switch delay map */
		for (int ichild = 0; ichild < num_Tdel_children; ichild++){
			ezxml_t Tdel_child = ezxml_child(Node, "Tdel");

			int num_inputs = GetIntProperty(Tdel_child, "num_inputs", true, 0);
			float Tdel_value = GetFloatProperty(Tdel_child, "delay", true, 0.);

			if (Tdel_map->count( num_inputs ) ){
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Tdel_child->line,
					"Tdel node specified num_inputs (%d) that has already been specified by another Tdel node", num_inputs);
			} else {
				(*Tdel_map)[num_inputs] = Tdel_value;
			}

			FreeNode(Tdel_child);
		}
	} else {
		/* No delay info specified for switch */
		if (timing_enabled){
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"Switch should contain intrinsic delay information if timing is enabled");
		} else {
			/* set a default value */
			(*Tdel_map)[UNDEFINED] = 0;
		}
	}
}

static void ProcessDirects(INOUTP ezxml_t Parent, OUTP t_direct_inf **Directs,
		 OUTP int *NumDirects, INP struct s_arch_switch_inf *Switches, INP int NumSwitches,
		 INP bool timing_enabled) {
	int i, j;
	const char *direct_name;
	const char *from_pin_name;
	const char *to_pin_name;
	const char *switch_name;

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

		direct_name = FindProperty(Node, "name", true);
		/* Check for direct name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Directs)[j].name, direct_name)) {
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
						"Two directs with the same name '%s' were found.\n",
						direct_name);
			}
		}
		(*Directs)[i].name = my_strdup(direct_name);
		ezxml_set_attr(Node, "name", NULL);

		/* Figure out the source pin and sink pin name */
		from_pin_name = FindProperty(Node, "from_pin", true);
		to_pin_name = FindProperty(Node, "to_pin", true);

		/* Check that to_pin and the from_pin are not the same */
		if (0 == strcmp(to_pin_name, from_pin_name)) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"The source pin and sink pin are the same: %s.\n",
					to_pin_name);
		}
		(*Directs)[i].from_pin = my_strdup(from_pin_name);
		(*Directs)[i].to_pin = my_strdup(to_pin_name);
		ezxml_set_attr(Node, "from_pin", NULL);
		ezxml_set_attr(Node, "to_pin", NULL);

		(*Directs)[i].x_offset = GetIntProperty(Node, "x_offset", true, 0);
		(*Directs)[i].y_offset = GetIntProperty(Node, "y_offset", true, 0);
		(*Directs)[i].z_offset = GetIntProperty(Node, "z_offset", true, 0);
		ezxml_set_attr(Node, "x_offset", NULL);
		ezxml_set_attr(Node, "y_offset", NULL);
		ezxml_set_attr(Node, "z_offset", NULL);

        //Set the optional switch type
        switch_name = FindProperty(Node, "switch_name", false);
        if(switch_name != NULL) {
            //Look-up the user defined switch
            for(j = 0; j < NumSwitches; j++) {
                if(0 == strcmp(switch_name, Switches[j].name)) {
                    break; //Found the switch
                }
            }
            if(j >= NumSwitches) {
                vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
                        "Could not find switch named '%s' in switch list.\n", switch_name);

            }
            (*Directs)[i].switch_type = j; //Save the correct switch index
            ezxml_set_attr(Node, "switch_name", NULL);
        } else {
            //If not defined, use the delayless switch by default
            //TODO: find a better way of indicating this.  Ideally, we would
            //specify the delayless switch index here, but it does not appear
            //to be defined at this point.
            (*Directs)[i].switch_type = -1; 
        }

		/* Check that the direct chain connection is not zero in both direction */
		if ((*Directs)[i].x_offset == 0 && (*Directs)[i].y_offset == 0) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, Node->line,
					"The x_offset and y_offset are both zero, this is a length 0 direct chain connection.\n");
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
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
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
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
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
				vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
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

static void UpdateAndCheckModels(INOUTP struct s_arch *arch) {
	t_model * cur_model;
	t_model_ports *port;
	int i, j;
	cur_model = arch->models;
	while (cur_model) {
		if (cur_model->pb_types == NULL) {
			vpr_throw(VPR_ERROR_ARCH, arch_file_name, 0,
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

	//Print all layout device switch/segment list info first
	PrintArchInfo(Echo, arch);

	//Models
	fprintf(Echo, "*************************************************\n");
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
	fprintf(Echo, "*************************************************\n\n");
	fprintf(Echo, "*************************************************\n");
	for (i = 0; i < NumTypes; ++i) {
		fprintf(Echo, "Type: \"%s\"\n", Types[i].name);
		fprintf(Echo, "\tcapacity: %d\n", Types[i].capacity);
		fprintf(Echo, "\twidth: %d\n", Types[i].width);
		fprintf(Echo, "\theight: %d\n", Types[i].height);
		for (j = 0; j < Types[i].num_pins; j++) {
			fprintf(Echo, "\tis_Fc_frac: \n");
			fprintf(Echo, "\t\tPin number %d: %s\n", j,
					(Types[i].is_Fc_frac[j] ? "true" : "false"));
			fprintf(Echo, "\tis_Fc_full_flex: \n");
			fprintf(Echo, "\t\tPin number %d: %s\n", j,
					(Types[i].is_Fc_full_flex[j] ? "true" : "false"));
			for (int iseg = 0; iseg < arch->num_segments; iseg++){
				fprintf(Echo, "\tPin: %d  Segment: %d  Fc: %f\n", j, iseg, Types[i].Fc[j][iseg]);
			}
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

	if (pb_type->num_modes > 0) {/*one or more modes*/
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
				for (k = 0;
						k < pb_type->modes[i].interconnect[j].num_annotations;
						k++) {
					fprintf(Echo, "%s\t\t\tannotation %s %s %d: %s\n", tabs,
							pb_type->modes[i].interconnect[j].annotations[k].input_pins,
							pb_type->modes[i].interconnect[j].annotations[k].output_pins,
							pb_type->modes[i].interconnect[j].annotations[k].format,
							pb_type->modes[i].interconnect[j].annotations[k].value[0]);
				}
				//Print power info for interconnects
				if (pb_type->modes[i].interconnect[j].interconnect_power) {
					if (pb_type->modes[i].interconnect[j].interconnect_power->power_usage.dynamic
							|| pb_type->modes[i].interconnect[j].interconnect_power->power_usage.leakage) {
						fprintf(Echo, "%s\t\t\tpower %e %e\n", tabs,
								pb_type->modes[i].interconnect[j].interconnect_power->power_usage.dynamic,
								pb_type->modes[i].interconnect[j].interconnect_power->power_usage.leakage);
					}
				}
			}
		}
	} else {/*leaf pb with unknown model*/
		/*LUT(names) already handled, it naturally has 2 modes.
		 I/O has no annotations to be displayed
		 All other library or user models may have delays specificied, e.g. Tsetup and Tcq
		 Display the additional information*/
		if (strcmp(pb_type->model->name, "names")
				&& strcmp(pb_type->model->name, "input")
				&& strcmp(pb_type->model->name, "output")) {
			for (k = 0; k < pb_type->num_annotations; k++) {
				fprintf(Echo, "%s\t\t\tannotation %s %s %s %d: %s\n", tabs,
						pb_type->annotations[k].clock,
						pb_type->annotations[k].input_pins,
						pb_type->annotations[k].output_pins,
						pb_type->annotations[k].format,
						pb_type->annotations[k].value[0]);
			}
		}
	}

	if (pb_type->pb_type_power) {
		PrintPb_types_recPower(Echo, pb_type, tabs);
	}
	free(tabs);
}

//Added May 2013 Daniel Chen, help dump arch info after loading from XML
static void PrintPb_types_recPower(INP FILE * Echo,
		INP const t_pb_type * pb_type, const char* tabs) {

	int i = 0;
	/*Print power information for each pb if available*/
	switch (pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_UNDEFINED:
		fprintf(Echo, "%s\tpower method: undefined\n", tabs);
		break;
	case POWER_METHOD_IGNORE:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because of the inheritance property of auto-size*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_IGNORE)
				break;
		}
		fprintf(Echo, "%s\tpower method: ignore\n", tabs);
		break;
	case POWER_METHOD_SUM_OF_CHILDREN:
		fprintf(Echo, "%s\tpower method: sum-of-children\n", tabs);
		break;
	case POWER_METHOD_AUTO_SIZES:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because of the inheritance property of auto-size*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_AUTO_SIZES)
				break;
		}
		fprintf(Echo, "%s\tpower method: auto-size\n", tabs);
		break;
	case POWER_METHOD_SPECIFY_SIZES:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because of the inheritance property of specify-size*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_SPECIFY_SIZES)
				break;
		}

		fprintf(Echo, "%s\tpower method: specify-size\n", tabs);
		for (i = 0; i < pb_type->num_ports; i++) {
			//Print all the power information on each port, only if available,
			//will not print if value is 0 or NULL
			if (pb_type->ports[i].port_power->buffer_type
					|| pb_type->ports[i].port_power->wire_type
					|| pb_type->pb_type_power->absolute_power_per_instance.leakage
					|| pb_type->pb_type_power->absolute_power_per_instance.dynamic) {

				fprintf(Echo, "%s\t\tport %s type %d num_pins %d\n", tabs,
						pb_type->ports[i].name, pb_type->ports[i].type,
						pb_type->ports[i].num_pins);
				//Buffer size
				switch (pb_type->ports[i].port_power->buffer_type) {
				case (POWER_BUFFER_TYPE_UNDEFINED):
				case (POWER_BUFFER_TYPE_NONE):
					break;
				case (POWER_BUFFER_TYPE_AUTO):
					fprintf(Echo, "%s\t\t\tbuffer_size %s\n", tabs, "auto");
					break;
				case (POWER_BUFFER_TYPE_ABSOLUTE_SIZE):
					fprintf(Echo, "%s\t\t\tbuffer_size %f\n", tabs,
							pb_type->ports[i].port_power->buffer_size);
					break;
				default:
					break;
				}
				switch (pb_type->ports[i].port_power->wire_type) {
				case (POWER_WIRE_TYPE_UNDEFINED):
				case (POWER_WIRE_TYPE_IGNORED):
					break;
				case (POWER_WIRE_TYPE_C):
					fprintf(Echo, "%s\t\t\twire_cap: %e\n", tabs,
							pb_type->ports[i].port_power->wire.C);
					break;
				case (POWER_WIRE_TYPE_ABSOLUTE_LENGTH):
					fprintf(Echo, "%s\t\t\twire_len(abs): %e\n", tabs,
							pb_type->ports[i].port_power->wire.absolute_length);
					break;
				case (POWER_WIRE_TYPE_RELATIVE_LENGTH):
					fprintf(Echo, "%s\t\t\twire_len(rel): %f\n", tabs,
							pb_type->ports[i].port_power->wire.relative_length);
					break;
				case (POWER_WIRE_TYPE_AUTO):
					fprintf(Echo, "%s\t\t\twire_len: %s\n", tabs, "auto");
					break;
				default:
					break;
				}

			}
		}
		//Output static power even if non zero
		if (pb_type->pb_type_power->absolute_power_per_instance.leakage)
			fprintf(Echo, "%s\t\tstatic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.leakage);

		if (pb_type->pb_type_power->absolute_power_per_instance.dynamic)
			fprintf(Echo, "%s\t\tdynamic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.dynamic);
		break;
	case POWER_METHOD_TOGGLE_PINS:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because once energy_per_toggle is specified at one level,
			 all children pb's are energy_per_toggle and only want to display once*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_TOGGLE_PINS)
				break;
		}

		fprintf(Echo, "%s\tpower method: pin-toggle\n", tabs);
		for (i = 0; i < pb_type->num_ports; i++) {
			/*Print all the power information on each port, only if available,
			 will not print if value is 0 or NULL*/
			if (pb_type->ports[i].port_power->energy_per_toggle
					|| pb_type->ports[i].port_power->scaled_by_port
					|| pb_type->pb_type_power->absolute_power_per_instance.leakage
					|| pb_type->pb_type_power->absolute_power_per_instance.dynamic) {

				fprintf(Echo, "%s\t\tport %s type %d num_pins %d\n", tabs,
						pb_type->ports[i].name, pb_type->ports[i].type,
						pb_type->ports[i].num_pins);
				//Toggle Energy
				if (pb_type->ports[i].port_power->energy_per_toggle) {
					fprintf(Echo, "%s\t\t\tenergy_per_toggle %e\n", tabs,
							pb_type->ports[i].port_power->energy_per_toggle);
				}
				//Scaled by port (could be reversed)
				if (pb_type->ports[i].port_power->scaled_by_port) {
					if (pb_type->ports[i].port_power->scaled_by_port->num_pins
							> 1) {
						fprintf(Echo,
								(pb_type->ports[i].port_power->reverse_scaled ?
										"%s\t\t\tscaled_by_static_prob_n: %s[%d]\n" :
										"%s\t\t\tscaled_by_static_prob: %s[%d]\n"),
								tabs,
								pb_type->ports[i].port_power->scaled_by_port->name,
								pb_type->ports[i].port_power->scaled_by_port_pin_idx);
					} else {
						fprintf(Echo,
								(pb_type->ports[i].port_power->reverse_scaled ?
										"%s\t\t\tscaled_by_static_prob_n: %s\n" :
										"%s\t\t\tscaled_by_static_prob: %s\n"),
								tabs,
								pb_type->ports[i].port_power->scaled_by_port->name);
					}
				}
			}
		}
		//Output static power even if non zero
		if (pb_type->pb_type_power->absolute_power_per_instance.leakage)
			fprintf(Echo, "%s\t\tstatic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.leakage);

		if (pb_type->pb_type_power->absolute_power_per_instance.dynamic)
			fprintf(Echo, "%s\t\tdynamic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.dynamic);

		break;
	case POWER_METHOD_C_INTERNAL:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because of values at this level includes all children pb's*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_C_INTERNAL)
				break;
		}
		fprintf(Echo, "%s\tpower method: C-internal\n", tabs);

		if (pb_type->pb_type_power->absolute_power_per_instance.leakage)
			fprintf(Echo, "%s\t\tstatic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.leakage);

		if (pb_type->pb_type_power->C_internal)
			fprintf(Echo, "%s\t\tdynamic c-internal: %e \n", tabs,
					pb_type->pb_type_power->C_internal);
		break;
	case POWER_METHOD_ABSOLUTE:
		if (pb_type->parent_mode) {
			/*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
			 This is because of values at this level includes all children pb's*/
			if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
					== POWER_METHOD_ABSOLUTE)
				break;
		}
		fprintf(Echo, "%s\tpower method: absolute\n", tabs);
		if (pb_type->pb_type_power->absolute_power_per_instance.leakage)
			fprintf(Echo, "%s\t\tstatic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.leakage);

		if (pb_type->pb_type_power->absolute_power_per_instance.dynamic)
			fprintf(Echo, "%s\t\tdynamic power_per_instance: %e \n", tabs,
					pb_type->pb_type_power->absolute_power_per_instance.dynamic);
		break;
	default:
		fprintf(Echo, "%s\tpower method: error has occcured\n", tabs);
		break;
	}
}
//Added May 2013 Daniel Chen, help dump arch info after loading from XML
static void PrintArchInfo(INP FILE * Echo, struct s_arch *arch) {
	int i, j;

	fprintf(Echo, "Printing architecture... \n\n");
	//Layout
	fprintf(Echo, "*************************************************\n");
	if (arch->clb_grid.IsAuto) {
		fprintf(Echo, "Layout: auto %f\n", arch->clb_grid.Aspect);
	} else {
		fprintf(Echo, "Layout: width %d height %d\n", arch->clb_grid.W,
				arch->clb_grid.H);
	}
	fprintf(Echo, "*************************************************\n\n");
	//Device
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Device Info:\n");

	fprintf(Echo,
			"\tSizing: R_minW_nmos %e R_minW_pmos %e ipin_mux_trans_size %e\n",
			arch->R_minW_nmos, arch->R_minW_pmos, arch->ipin_mux_trans_size);

	fprintf(Echo, "\tTiming: C_ipin_cblock %e T_ipin_cblock %e\n",
			arch->C_ipin_cblock, arch->T_ipin_cblock);

	fprintf(Echo, "\tArea: grid_logic_tile_area %e\n",
			arch->grid_logic_tile_area);

	fprintf(Echo, "\tChannel Width Distribution:\n");
	fprintf(Echo, "\t\tio: width %e\n", arch->Chans.chan_width_io);

	switch (arch->Chans.chan_x_dist.type) {
	case (UNIFORM):
		fprintf(Echo, "\t\tx: type uniform peak %e\n",
				arch->Chans.chan_x_dist.peak);
		break;
	case (GAUSSIAN):
		fprintf(Echo,
				"\t\tx: type gaussian peak %e \
						  width %e Xpeak %e dc %e\n",
				arch->Chans.chan_x_dist.peak, arch->Chans.chan_x_dist.width,
				arch->Chans.chan_x_dist.xpeak, arch->Chans.chan_x_dist.dc);
		break;
	case (PULSE):
		fprintf(Echo,
				"\t\tx: type pulse peak %e \
						  width %e Xpeak %e dc %e\n",
				arch->Chans.chan_x_dist.peak, arch->Chans.chan_x_dist.width,
				arch->Chans.chan_x_dist.xpeak, arch->Chans.chan_x_dist.dc);
		break;
	case (DELTA):
		fprintf(Echo, "\t\tx: distr dleta peak %e \
						  Xpeak %e dc %e\n",
				arch->Chans.chan_x_dist.peak, arch->Chans.chan_x_dist.xpeak,
				arch->Chans.chan_x_dist.dc);
		break;
	default:
		fprintf(Echo, "\t\tInvalid Distribution!\n");
		break;
	}

	switch (arch->Chans.chan_y_dist.type) {
	case (UNIFORM):
		fprintf(Echo, "\t\ty: type uniform peak %e\n",
				arch->Chans.chan_y_dist.peak);
		break;
	case (GAUSSIAN):
		fprintf(Echo,
				"\t\ty: type gaussian peak %e \
						  width %e Xpeak %e dc %e\n",
				arch->Chans.chan_y_dist.peak, arch->Chans.chan_y_dist.width,
				arch->Chans.chan_y_dist.xpeak, arch->Chans.chan_y_dist.dc);
		break;
	case (PULSE):
		fprintf(Echo,
				"\t\ty: type pulse peak %e \
						  width %e Xpeak %e dc %e\n",
				arch->Chans.chan_y_dist.peak, arch->Chans.chan_y_dist.width,
				arch->Chans.chan_y_dist.xpeak, arch->Chans.chan_y_dist.dc);
		break;
	case (DELTA):
		fprintf(Echo, "\t\ty: distr dleta peak %e \
						  Xpeak %e dc %e\n",
				arch->Chans.chan_y_dist.peak, arch->Chans.chan_y_dist.xpeak,
				arch->Chans.chan_y_dist.dc);
		break;
	default:
		fprintf(Echo, "\t\tInvalid Distribution!\n");
		break;
	}

	switch (arch->SBType) {
	case (WILTON):
		fprintf(Echo, "\tSwitch Block: type wilton fs %d\n", arch->Fs);
		break;
	case (UNIVERSAL):
		fprintf(Echo, "\tSwitch Block: type universal fs %d\n", arch->Fs);
		break;
	case (SUBSET):
		fprintf(Echo, "\tSwitch Block: type subset fs %d\n", arch->Fs);
		break;
	default:
		break;
	}
	fprintf(Echo, "*************************************************\n\n");
	//Switch list
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Switch List:\n");

	//13 is hard coded because format of %e is always 1.123456e+12
	//It always consists of 10 alphanumeric digits, a decimal
	//and a sign
	for (i = 0; i < arch->num_switches; i++) {

		if (arch->Switches[i].buffered) {
			fprintf(Echo, "\tSwitch[%d]: name %s type mux/buffer\n", i + 1,
					arch->Switches[i].name);
		} else {
			fprintf(Echo, "\tSwitch[%d]: name %s type pass_trans\n", i + 1,
					arch->Switches[i].name);
		}
		fprintf(Echo, "\t\t\t\tR %e Cin %e Cout %e\n", arch->Switches[i].R,
				arch->Switches[i].Cin, arch->Switches[i].Cout);
		fprintf(Echo, "\t\t\t\t#Tdel values %d buf_size %e mux_trans_size %e\n",
				(int)arch->Switches[i].Tdel_map.size(), arch->Switches[i].buf_size,
				arch->Switches[i].mux_trans_size);
		if (arch->Switches[i].power_buffer_type == POWER_BUFFER_TYPE_AUTO) {
			fprintf(Echo, "\t\t\t\tpower_buffer_size auto\n");
		} else {
			fprintf(Echo, "\t\t\t\tpower_buffer_size %e\n",
					arch->Switches[i].power_buffer_size);
		}
	}

	fprintf(Echo, "*************************************************\n\n");
	//Segment List
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Segment List:\n");
	for (i = 0; i < arch->num_segments; i++) {
		fprintf(Echo,
				"\tSegment[%d]: frequency %d length %d R_metal %e C_metal %e\n",
				i + 1, arch->Segments[i].frequency, arch->Segments[i].length,
				arch->Segments[i].Rmetal, arch->Segments[i].Cmetal);

		if (arch->Segments[i].directionality == UNI_DIRECTIONAL) {
			//wire_switch == arch_opin_switch
			fprintf(Echo, "\t\t\t\ttype unidir mux_name %s\n",
					arch->Switches[arch->Segments[i].arch_wire_switch].name);
		} else { //Should be bidir
			fprintf(Echo, "\t\t\t\ttype bidir wire_switch %s arch_opin_switch %s\n",
					arch->Switches[arch->Segments[i].arch_wire_switch].name,
					arch->Switches[arch->Segments[i].arch_opin_switch].name);
		}

		fprintf(Echo, "\t\t\t\tcb ");
		for (j = 0; j < arch->Segments->cb_len; j++) {
			if (arch->Segments->cb[j]) {
				fprintf(Echo, "1 ");
			} else {
				fprintf(Echo, "0 ");
			}
		}
		fprintf(Echo, "\n");

		fprintf(Echo, "\t\t\t\tsb ");
		for (j = 0; j < arch->Segments->sb_len; j++) {
			if (arch->Segments->sb[j]) {
				fprintf(Echo, "1 ");
			} else {
				fprintf(Echo, "0 ");
			}
		}
		fprintf(Echo, "\n");
	}
	fprintf(Echo, "*************************************************\n\n");
	//Direct List
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Direct List:\n");
	for (i = 0; i < arch->num_directs; i++) {
		fprintf(Echo, "\tDirect[%d]: name %s from_pin %s to_pin %s\n", i + 1,
				arch->Directs[i].name, arch->Directs[i].from_pin,
				arch->Directs[i].to_pin);
		fprintf(Echo, "\t\t\t\t x_offset %d y_offset %d z_offset %d\n",
				arch->Directs[i].x_offset, arch->Directs[i].y_offset,
				arch->Directs[i].z_offset);
	}
	fprintf(Echo, "*************************************************\n\n");

	//Architecture Power
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Power:\n");
	if (arch->power) {
		fprintf(Echo, "\tlocal_interconnect C_wire %e factor %f\n",
				arch->power->C_wire_local, arch->power->local_interc_factor);
		fprintf(Echo, "\tlogical_effort_factor %f trans_per_sram_bit %f\n",
				arch->power->logical_effort_factor,
				arch->power->transistors_per_SRAM_bit);

	}

	fprintf(Echo, "*************************************************\n\n");
	//Architecture Clock
	fprintf(Echo, "*************************************************\n");
	fprintf(Echo, "Clock:\n");
	if (arch->clocks) {
		for (i = 0; i < arch->clocks->num_global_clocks; i++) {
			if (arch->clocks->clock_inf[i].autosize_buffer) {
				fprintf(Echo, "\tClock[%d]: buffer_size auto C_wire %e", i + 1,
						arch->clocks->clock_inf->C_wire);
			} else {
				fprintf(Echo, "\tClock[%d]: buffer_size %e C_wire %e", i + 1,
						arch->clocks->clock_inf[i].buffer_size,
						arch->clocks->clock_inf[i].C_wire);
			}
			fprintf(Echo, "\t\t\t\tstat_prob %f switch_density %f period %e",
					arch->clocks->clock_inf[i].prob,
					arch->clocks->clock_inf[i].dens,
					arch->clocks->clock_inf[i].period);
		}
	}

	fprintf(Echo, "*************************************************\n\n");
}
static void ProcessPower( INOUTP ezxml_t parent,
		INOUTP t_power_arch * power_arch, INP t_type_descriptor * Types,
		INP int NumTypes) {
	ezxml_t Cur;

	/* Get the local interconnect capacitances */
	power_arch->local_interc_factor = 0.5;
	Cur = FindElement(parent, "local_interconnect", false);
	if (Cur) {
		power_arch->C_wire_local = GetFloatProperty(Cur, "C_wire", false, 0.);
		power_arch->local_interc_factor = GetFloatProperty(Cur, "factor", false,
				0.5);
		FreeNode(Cur);
	}

	/* Get segment split */
	/*
	 power_arch->seg_buffer_split = 1;
	 Cur = FindElement(parent, "segment_buffer_split", false);
	 if (Cur) {
	 power_arch->seg_buffer_split = GetIntProperty(Cur, "split_into", true,
	 1);
	 FreeNode(Cur);
	 }*/

	/* Get logical effort factor */
	power_arch->logical_effort_factor = 4.0;
	Cur = FindElement(parent, "buffers", false);
	if (Cur) {
		power_arch->logical_effort_factor = GetFloatProperty(Cur,
				"logical_effort_factor", true, 0);
		FreeNode(Cur);
	}

	/* Get SRAM Size */
	power_arch->transistors_per_SRAM_bit = 6.0;
	Cur = FindElement(parent, "sram", false);
	if (Cur) {
		power_arch->transistors_per_SRAM_bit = GetFloatProperty(Cur,
				"transistors_per_bit", true, 0);
		FreeNode(Cur);
	}

	/* Get Mux transistor size */
	power_arch->mux_transistor_size = 1.0;
	Cur = FindElement(parent, "mux_transistor_size", false);
	if (Cur) {
		power_arch->mux_transistor_size = GetFloatProperty(Cur,
				"mux_transistor_size", true, 0);
		FreeNode(Cur);
	}

	/* Get FF size */
	power_arch->FF_size = 1.0;
	Cur = FindElement(parent, "FF_size", false);
	if (Cur) {
		power_arch->FF_size = GetFloatProperty(Cur, "FF_size", true, 0);
		FreeNode(Cur);
	}

	/* Get LUT transistor size */
	power_arch->LUT_transistor_size = 1.0;
	Cur = FindElement(parent, "LUT_transistor_size", false);
	if (Cur) {
		power_arch->LUT_transistor_size = GetFloatProperty(Cur,
				"LUT_transistor_size", true, 0);
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

		tmp = FindProperty(Node, "buffer_size", true);
		if (strcmp(tmp, "auto") == 0) {
			clocks->clock_inf[i].autosize_buffer = true;
		} else {
			clocks->clock_inf[i].autosize_buffer = false;
			clocks->clock_inf[i].buffer_size = (float) atof(tmp);
		}
		ezxml_set_attr(Node, "buffer_size", NULL);

		clocks->clock_inf[i].C_wire = GetFloatProperty(Node, "C_wire", true, 0);
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
		return POWER_METHOD_UNDEFINED; // Should never get here, but avoids a compiler warning.
	}
}

/* Date:June 28th, 2013								
 * Author: Daniel Chen								
 * Purpose: Checks for correctly grouped XML tag	
 *	       ordering, vpr_throws if incorrect		
 * Note: Used this because there is a				
 *			limitation in ezxml_cut in the ezxml library	
 *			which seg faults with incorrectly	
 *			grouped tags		
 */
static void CheckXMLTagOrder(ezxml_t Parent) {

	int i, num_tags;
	ezxml_t Cur, Trace;
	char* tag_name;
	i = num_tags = 0;

	/* Start with first subtag */
	Cur = Parent->child;

	while (Cur) {
		tag_name = my_strdup(Cur->name);
		num_tags = CountChildren(Parent, tag_name, 0);
		Trace = Cur->ordered;
		for (i = 1; i < num_tags; i++) { //Check the next num_tags-1 tags see if grouped
			if (Trace) {
				if (strcmp(Trace->name, tag_name)) {
					vpr_throw(VPR_ERROR_ARCH, arch_file_name, Trace->line,
							"XML tags of type '%s' must be specified right after/before each other\n",
							tag_name);
				}
				Trace = Trace->ordered;
			}
		}
		Cur = Cur->sibling; // Go to next tag with a different name on the same level 
		free(tag_name); // Free the copied tag name
	}
}

/* Date:July 10th, 2013								
 * Author: Daniel Chen								
 * Purpose: Attempts to match a clock_name specified in an 
 *			timing annotation (Tsetup, Thold, Tc_to_q) with the
 *			clock_name specified in the primitive. Applies
 *			to flipflop/memory right now.
 */
static void primitives_annotation_clock_match(
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
		vpr_throw(VPR_ERROR_ARCH, arch_file_name, annotation->line_num,
				"Clock '%s' does not match any clock defined in pb_type '%s'.\n",
				annotation->clock, parent_pb_type->name);
	}

}

/* Used by functions outside read_xml_util.c to gain access to arch filename */
const char* get_arch_file_name() {
	return arch_file_name;
}

