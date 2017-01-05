/* The XML parser processes an XML file into a tree data structure composed of  
 * pugi::xml_nodes.  Each node represents an XML element.  For example          
 * <a> <b/> </a> will generate two pugi::xml_nodes.  One called "a" and its      
 * child "b".  Each pugi::xml_node can contain various XML data such as attribute 
 * information and text content.  The XML parser provides several functions to  
 * help the developer build, and traverse tree (this is also somtime referred to
 * as the Document Object Model or DOM).              
 *                                                                              
 * For convenience it often makes sense to use some wraper functions (provided in
 * the pugiutil namespace of libvtrutil) which simplify loading an XML file and 
 * error handling.
 *
 * The function pugiutil::load_xml() reads in an xml file.                 
 *                                                                              
 * The function pugiutil::get_single_child() returns a child xml_node for a given parent
 * xml_node if there is a child which matches the name provided by the developer.                          
 *                                                                              
 * The function pugiutil::get_attribute() is used to extract attributes from an 
 * xml_node, returning a pugi::xml_attribute. xml_attribute objects support accessors
 * such as as_float(), as_int() to retrieve semantic values. See pugixml documentation
 * for more details.
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
#include <map>
#include <string>
#include <sstream>

#include "pugixml.hpp"
#include "pugixml_util.hpp"

#include "vtr_assert.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_matrix.h"

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"

#include "read_xml_arch_file.h"
#include "read_xml_util.h"
#include "parse_switchblocks.h"

using namespace std;
using namespace pugiutil;

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
static void SetupPinLocationsAndPinClasses(pugi::xml_node Locations,
		t_type_descriptor * Type, const pugiutil::loc_data& loc_data);
static void SetupGridLocations(pugi::xml_node Locations, t_type_descriptor * Type, const pugiutil::loc_data& loc_data);
/*    Process XML hiearchy */
static void ProcessPb_Type(pugi::xml_node Parent, t_pb_type * pb_type,
		t_mode * mode, const pugiutil::loc_data& loc_data);
static void ProcessPb_TypePort(pugi::xml_node Parent, t_port * port,
		e_power_estimation_method power_method, const pugiutil::loc_data& loc_data);
static void ProcessPinToPinAnnotations(pugi::xml_node parent,
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type, const pugiutil::loc_data& loc_data);
static void ProcessInterconnect(pugi::xml_node Parent, t_mode * mode, const pugiutil::loc_data& loc_data);
static void ProcessMode(pugi::xml_node Parent, t_mode * mode,
		const pugiutil::loc_data& loc_data);
static void Process_Fc(pugi::xml_node Node, t_type_descriptor * Type, t_segment_inf *segments, int num_segments, const pugiutil::loc_data& loc_data);
static void ProcessComplexBlockProps(pugi::xml_node Node, t_type_descriptor * Type, const pugiutil::loc_data& loc_data);
static void ProcessSizingTimingIpinCblock(pugi::xml_node Node,
		struct s_arch *arch, const bool timing_enabled, const pugiutil::loc_data& loc_data);
static void ProcessChanWidthDistr(pugi::xml_node Node,
		struct s_arch *arch, const pugiutil::loc_data& loc_data);
static void ProcessChanWidthDistrDir(pugi::xml_node Node, t_chan * chan, const pugiutil::loc_data& loc_data);
static void ProcessModels(pugi::xml_node Node, struct s_arch *arch, const pugiutil::loc_data& loc_data);
static void ProcessLayout(pugi::xml_node Node, struct s_arch *arch, const pugiutil::loc_data& loc_data);
static void ProcessDevice(pugi::xml_node Node, struct s_arch *arch,
		const bool timing_enabled, const pugiutil::loc_data& loc_data);
static void ProcessComplexBlocks(pugi::xml_node Node,
		t_type_descriptor ** Types, int *NumTypes,
		const s_arch& arch, const pugiutil::loc_data& loc_data);
static void ProcessSwitches(pugi::xml_node Node,
		struct s_arch_switch_inf **Switches, int *NumSwitches,
		const bool timing_enabled, const pugiutil::loc_data& loc_data);
static void ProcessSwitchTdel(pugi::xml_node Node, const bool timing_enabled,
		const int switch_index, s_arch_switch_inf *Switches, const pugiutil::loc_data& loc_data);
static void ProcessDirects(pugi::xml_node Parent, t_direct_inf **Directs,
		 int *NumDirects, const struct s_arch_switch_inf *Switches, const int NumSwitches,
		 const pugiutil::loc_data& loc_data);
static void ProcessSegments(pugi::xml_node Parent,
		struct s_segment_inf **Segs, int *NumSegs,
		const struct s_arch_switch_inf *Switches, const int NumSwitches,
		const bool timing_enabled, const bool switchblocklist_required, const pugiutil::loc_data& loc_data);
static void ProcessSwitchblocks(pugi::xml_node Parent, vector<t_switchblock_inf> *switchblocks,
				const t_arch_switch_inf *switches, const int num_switches, const pugiutil::loc_data& loc_data);
static void ProcessCB_SB(pugi::xml_node Node, bool * list,
		const int len, const pugiutil::loc_data& loc_data);
static void ProcessPower( pugi::xml_node parent,
		t_power_arch * power_arch,
        const pugiutil::loc_data& loc_data);

static void ProcessClocks(pugi::xml_node Parent, t_clock_arch * clocks, const pugiutil::loc_data& loc_data);

static void ProcessPb_TypePowerEstMethod(pugi::xml_node Parent, t_pb_type * pb_type, const pugiutil::loc_data& loc_data);
static void ProcessPb_TypePort_Power(pugi::xml_node Parent, t_port * port,
		e_power_estimation_method power_method, const pugiutil::loc_data& loc_data);

/*
 *
 *
 * External Function Implementations
 *
 *
 */

/* Loads the given architecture file. */
void XmlReadArch(const char *ArchFile, const bool timing_enabled,
		struct s_arch *arch, t_type_descriptor ** Types,
		int *NumTypes) {

	const char *Prop;
	pugi::xml_node Next;
	ReqOpt POWER_REQD, SWITCHBLOCKLIST_REQD;

	if (vtr::check_file_name_extension(ArchFile, ".xml") == false) {
		vtr::printf_warning(__FILE__, __LINE__,
				"Architecture file '%s' may be in incorrect format. "
						"Expecting .xml format for architecture files.\n",
				ArchFile);
	}

	/* Parse the file */
	pugi::xml_document doc;
	pugiutil::loc_data loc_data;
	try {
		loc_data = pugiutil::load_xml(doc, ArchFile);
	} catch (XmlError& e) {
		archfpga_throw(ArchFile, e.line(),
				"%s", e.what());
	}

	arch_file_name = ArchFile;

	/* Root node should be architecture */
	auto architecture = get_single_child(doc, "architecture", loc_data);

	/* TODO: do version processing properly with string delimiting on the . */
	Prop = get_attribute(architecture, "version", loc_data, OPTIONAL).as_string(NULL);
	if (Prop != NULL) {
		if (atof(Prop) > atof(VPR_VERSION)) {
			vtr::printf_warning(__FILE__, __LINE__,
					"This architecture version is for VPR %f while your current VPR version is " VPR_VERSION ", compatability issues may arise\n",
					atof(Prop));
		}
	}

	/* Process models */
	Next = get_single_child(architecture, "models", loc_data);
	ProcessModels(Next, arch, loc_data);
	CreateModelLibrary(arch);

	/* Process layout */
	Next = get_single_child(architecture, "layout", loc_data);
	ProcessLayout(Next, arch, loc_data);

	/* Process device */
	Next = get_single_child(architecture, "device", loc_data);
	ProcessDevice(Next, arch, timing_enabled, loc_data);

	/* Process switches */
	Next = get_single_child(architecture, "switchlist", loc_data);
	ProcessSwitches(Next, &(arch->Switches), &(arch->num_switches),
			timing_enabled, loc_data);

	/* Process switchblocks. This depends on switches */
	bool switchblocklist_required = (arch->SBType == CUSTOM);	//require this section only if custom switchblocks are used
	SWITCHBLOCKLIST_REQD = BoolToReqOpt(switchblocklist_required);	


	/* Process segments. This depends on switches */
	Next = get_single_child(architecture, "segmentlist", loc_data);
	ProcessSegments(Next, &(arch->Segments), &(arch->num_segments),
			arch->Switches, arch->num_switches, timing_enabled, switchblocklist_required, loc_data);
	

	Next = get_single_child(architecture, "switchblocklist", loc_data, SWITCHBLOCKLIST_REQD);
	if (Next){
		ProcessSwitchblocks(Next, &(arch->switchblocks), arch->Switches, arch->num_switches, loc_data);
	}

	/* Process types */
	Next = get_single_child(architecture, "complexblocklist", loc_data);
	ProcessComplexBlocks(Next, Types, NumTypes, *arch, loc_data);

	/* Process directs */
	Next = get_single_child(architecture, "directlist", loc_data, OPTIONAL);
	if (Next) {
		ProcessDirects(Next, &(arch->Directs), &(arch->num_directs),
                arch->Switches, arch->num_switches,
				loc_data);
	}

	/* Process architecture power information */

	/* If arch->power has been initialized, meaning the user has requested power estimation,
	 * then the power architecture information is required.
	 */
	if (arch->power) {
		POWER_REQD = REQUIRED;
	} else {
		POWER_REQD = OPTIONAL;
	}

	Next = get_single_child(architecture, "power", loc_data, POWER_REQD);
	if (Next) {
		if (arch->power) {
			ProcessPower(Next, arch->power, loc_data);
		} else {
			/* This information still needs to be read, even if it is just
			 * thrown away.
			 */
			t_power_arch * power_arch_fake = (t_power_arch*) vtr::calloc(1,
					sizeof(t_power_arch));
			ProcessPower(Next, power_arch_fake, loc_data);
			free(power_arch_fake);
		}
	}

// Process Clocks
	Next = get_single_child(architecture, "clocks", loc_data, POWER_REQD);
	if (Next) {
		if (arch->clocks) {
			ProcessClocks(Next, arch->clocks, loc_data);
		} else {
			/* This information still needs to be read, even if it is just
			 * thrown away.
			 */
			t_clock_arch * clocks_fake = (t_clock_arch*) vtr::calloc(1,
					sizeof(t_clock_arch));
			ProcessClocks(Next, clocks_fake, loc_data);
			free(clocks_fake->clock_inf);
			free(clocks_fake);
		}
	}
	SyncModelsPbTypes(arch, *Types, *NumTypes);
	UpdateAndCheckModels(arch);

}


/*
 *
 *
 * File-scope function implementations
 *
 *
 */

/* Sets up the pinloc map and pin classes for the type. 
 * Pins and pin classses must already be setup by SetupPinClasses */
static void SetupPinLocationsAndPinClasses(pugi::xml_node Locations,
		t_type_descriptor * Type, const pugiutil::loc_data& loc_data) {
	int i, j, k, Count;
	int capacity, pin_count;
	int num_class;
	const char * Prop;

	pugi::xml_node Cur;

	capacity = Type->capacity;

	Prop = get_attribute(Locations, "pattern", loc_data).value();
	if (strcmp(Prop, "spread") == 0) {
		Type->pin_location_distribution = E_SPREAD_PIN_DISTR;
	} else if (strcmp(Prop, "custom") == 0) {
		Type->pin_location_distribution = E_CUSTOM_PIN_DISTR;
	} else {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Locations),
				"%s is an invalid pin location pattern.\n", Prop);
	}

	/* Alloc and clear pin locations */
	Type->pin_width = (int *) vtr::calloc(Type->num_pins, sizeof(int));
	Type->pin_height = (int *) vtr::calloc(Type->num_pins, sizeof(int));
	Type->pinloc = (int ****) vtr::malloc(Type->width * sizeof(int ***));
	for (int width = 0; width < Type->width; ++width) {
		Type->pinloc[width] = (int ***) vtr::malloc(Type->height * sizeof(int **));
		for (int height = 0; height < Type->height; ++height) {
			Type->pinloc[width][height] = (int **) vtr::malloc(4 * sizeof(int *));
			for (int side = 0; side < 4; ++side) {
				Type->pinloc[width][height][side] = (int *) vtr::malloc(
						Type->num_pins * sizeof(int));
				for (int pin = 0; pin < Type->num_pins; ++pin) {
					Type->pinloc[width][height][side][pin] = 0;
				}
			}
		}
	}

	Type->pin_loc_assignments = (char *****) vtr::malloc(
			Type->width * sizeof(char ****));
	Type->num_pin_loc_assignments = (int ***) vtr::malloc(
			Type->width * sizeof(int **));
	for (int width = 0; width < Type->width; ++width) {
		Type->pin_loc_assignments[width] = (char ****) vtr::calloc(Type->height,
				sizeof(char ***));
		Type->num_pin_loc_assignments[width] = (int **) vtr::calloc(Type->height,
				sizeof(int *));
		for (int height = 0; height < Type->height; ++height) {
			Type->pin_loc_assignments[width][height] = (char ***) vtr::calloc(4,
					sizeof(char **));
			Type->num_pin_loc_assignments[width][height] = (int *) vtr::calloc(4,
					sizeof(int));
		}
	}

	/* Load the pin locations */
	if (Type->pin_location_distribution == E_CUSTOM_PIN_DISTR) {
		Cur = Locations.first_child();
		while (Cur) {
			check_node(Cur, "loc", loc_data);

			/* Get offset (ie. height) */
			int height = get_attribute(Cur, "offset", loc_data, OPTIONAL).as_int(0);
			if ((height < 0) || (height >= Type->height)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"'%d' is an invalid offset for type '%s'.\n", height,
						Type->name);
			}

			/* Get side */
			int side = 0;
			Prop = get_attribute(Cur, "side", loc_data).value();
			if (0 == strcmp(Prop, "left")) {
				side = LEFT;
			} else if (0 == strcmp(Prop, "top")) {
				side = TOP;
			} else if (0 == strcmp(Prop, "right")) {
				side = RIGHT;
			} else if (0 == strcmp(Prop, "bottom")) {
				side = BOTTOM;
			} else {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"'%s' is not a valid side.\n", Prop);
			}

			/* Check location is on perimeter */
			if ((TOP == side) && (height != (Type->height - 1))) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Locations are only allowed on large block perimeter. 'top' side should be at offset %d only.\n",
						(Type->height - 1));
			}
			if ((BOTTOM == side) && (height != 0)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Locations are only allowed on large block perimeter. 'bottom' side should be at offset 0 only.\n");
			}

			/* Go through lists of pins */
			const std::vector<std::string> Tokens = vtr::split(Cur.child_value());
			Count = Tokens.size();
			Type->num_pin_loc_assignments[0][height][side] = Count;
			if (Count > 0) {
				Type->pin_loc_assignments[0][height][side] = (char**) vtr::calloc(
						Count, sizeof(char*));
				for (int pin = 0; pin < Count; ++pin) {
					/* Store location assignment */
					Type->pin_loc_assignments[0][height][side][pin] = vtr::strdup(
							Tokens[pin].c_str());

					/* Advance through list of pins in this location */
				}
			}
			Cur = Cur.next_sibling(Cur.name());
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
	Type->class_inf = (struct s_class*) vtr::calloc(num_class,
			sizeof(struct s_class));
	Type->num_class = num_class;
	Type->pin_class = (int*) vtr::malloc(Type->num_pins * sizeof(int) * capacity);
	Type->is_global_pin = (bool*) vtr::malloc(
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
				Type->class_inf[num_class].pinlist = (int *) vtr::malloc(
						sizeof(int) * Type->pb_type->ports[j].num_pins);
			}

			for (k = 0; k < Type->pb_type->ports[j].num_pins; ++k) {
				if (!Type->pb_type->ports[j].equivalent) {
					Type->class_inf[num_class].num_pins = 1;
					Type->class_inf[num_class].pinlist = (int *) vtr::malloc(
							sizeof(int) * 1);
					Type->class_inf[num_class].pinlist[0] = pin_count;
				} else {
					Type->class_inf[num_class].pinlist[k] = pin_count;
				}

				if (Type->pb_type->ports[j].type == IN_PORT) {
					Type->class_inf[num_class].type = RECEIVER;
				} else {
					VTR_ASSERT(Type->pb_type->ports[j].type == OUT_PORT);
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
	VTR_ASSERT(num_class == Type->num_class);
	VTR_ASSERT(pin_count == Type->num_pins);
}

/* Sets up the grid_loc_def for the type. */
static void SetupGridLocations(pugi::xml_node Locations, t_type_descriptor * Type, const pugiutil::loc_data& loc_data) {
	int i;

	pugi::xml_node Cur;
	const char *Prop;

	Type->num_grid_loc_def = count_children(Locations, "loc", loc_data);
	Type->grid_loc_def = (struct s_grid_loc_def *) vtr::calloc(
			Type->num_grid_loc_def, sizeof(struct s_grid_loc_def));

	/* Load the pin locations */
	Cur = Locations.first_child();
	i = 0;
	while (Cur) {
		check_node(Cur, "loc", loc_data);

		/* loc index */
		Prop = get_attribute(Cur, "type", loc_data).value();
		if (Prop) {
			if (strcmp(Prop, "perimeter") == 0) {
				if (Type->num_grid_loc_def != 1) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
							"Another loc specified for perimeter.\n");
				}
				Type->grid_loc_def[i].grid_loc_type = BOUNDARY;
				VTR_ASSERT(IO_TYPE == Type);
				/* IO goes to boundary */
			} else if (strcmp(Prop, "fill") == 0) {
				if (Type->num_grid_loc_def != 1 || FILL_TYPE != NULL) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
							"Another loc specified for fill.\n");
				}
				Type->grid_loc_def[i].grid_loc_type = FILL;
				FILL_TYPE = Type;
			} else if (strcmp(Prop, "col") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REPEAT;
			} else if (strcmp(Prop, "rel") == 0) {
				Type->grid_loc_def[i].grid_loc_type = COL_REL;
			} else {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Unknown grid location type '%s' for type '%s'.\n",
						Prop, Type->name);
			}
		}
		Prop = get_attribute(Cur, "start", loc_data, OPTIONAL).as_string(NULL);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop == NULL) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"grid location property 'start' must be specified for grid location type 'col'.\n");
			}
			Type->grid_loc_def[i].start_col = vtr::atoi(Prop);
		} else if (Prop != NULL) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
					"grid location property 'start' valid for grid location type 'col' only.\n");
		}
		Prop = get_attribute(Cur, "repeat", loc_data, OPTIONAL).as_string(NULL);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REPEAT) {
			if (Prop != NULL) {
				Type->grid_loc_def[i].repeat = vtr::atoi(Prop);
			}
		} else if (Prop != NULL) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
					"Grid location property 'repeat' valid for grid location type 'col' only.\n");
		}
		Prop = get_attribute(Cur, "pos", loc_data, OPTIONAL).as_string(NULL);
		if (Type->grid_loc_def[i].grid_loc_type == COL_REL) {
			if (Prop == NULL) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Grid location property 'pos' must be specified for grid location type 'rel'.\n");
			}
			Type->grid_loc_def[i].col_rel = (float) atof(Prop);
		} else if (Prop != NULL) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
					"Grid location property 'pos' valid for grid location type 'rel' only.\n");
		}

		Type->grid_loc_def[i].priority = get_attribute(Cur, "priority", loc_data, OPTIONAL). as_int(1);

		Cur = Cur.next_sibling(Cur.name());
		i++;
	}
}

static void ProcessPinToPinAnnotations(pugi::xml_node Parent,
		t_pin_to_pin_annotation *annotation, t_pb_type * parent_pb_type, const pugiutil::loc_data& loc_data) {
	int i = 0;
	const char *Prop;
	
	if ( get_attribute(Parent, "max", loc_data, OPTIONAL).as_string(NULL) ){
		i++;
	}
	if ( get_attribute(Parent, "min", loc_data, OPTIONAL).as_string(NULL) ){
		i++;
	}
	if ( get_attribute(Parent, "type", loc_data, OPTIONAL).as_string(NULL) ){
		i++;
	}
	if ( get_attribute(Parent, "value", loc_data, OPTIONAL).as_string(NULL) ){
		i++;
	}
	if (0 == strcmp(Parent.name(), "C_constant")
			|| 0 == strcmp(Parent.name(), "C_matrix")
			|| 0 == strcmp(Parent.name(), "pack_pattern")) {
		i = 1;
	}

	annotation->num_value_prop_pairs = i;
	annotation->prop = (int*) vtr::calloc(i, sizeof(int));
	annotation->value = (char**) vtr::calloc(i, sizeof(char *));
	annotation->line_num = loc_data.line(Parent);
	/* Todo: This is slow, I should use a case lookup */
	i = 0;
	if (0 == strcmp(Parent.name(), "delay_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "max", loc_data, OPTIONAL).as_string(NULL);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MAX;
			annotation->value[i] = vtr::strdup(Prop);
			i++;
		}
		Prop = get_attribute(Parent, "min",loc_data, OPTIONAL).as_string(NULL);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MIN;
			annotation->value[i] = vtr::strdup(Prop);
			i++;
		}
		Prop = get_attribute(Parent, "in_port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "out_port", loc_data).value();
		annotation->output_pins = vtr::strdup(Prop);

	} else if (0 == strcmp(Parent.name(), "delay_matrix")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
		Prop = get_attribute(Parent, "type", loc_data).value();
		annotation->value[i] = vtr::strdup(Parent.child_value());

		if (0 == strcmp(Prop, "max")) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MAX;
		} else {
			VTR_ASSERT(0 == strcmp(Prop, "min"));
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_MIN;
		}

		i++;
		Prop = get_attribute(Parent, "in_port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "out_port", loc_data).value();
		annotation->output_pins = vtr::strdup(Prop);

	} else if (0 == strcmp(Parent.name(), "C_constant")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "C", loc_data).value();
		annotation->value[i] = vtr::strdup(Prop);
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
		i++;

		Prop = get_attribute(Parent, "in_port", loc_data, OPTIONAL).as_string(NULL);
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "out_port", loc_data,OPTIONAL).as_string(NULL);
		annotation->output_pins = vtr::strdup(Prop);
		VTR_ASSERT(
				annotation->output_pins != NULL || annotation->input_pins != NULL);

	} else if (0 == strcmp(Parent.name(), "C_matrix")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_CAPACITANCE;
		annotation->format = E_ANNOT_PIN_TO_PIN_MATRIX;
		annotation->value[i] = vtr::strdup(Parent.child_value());
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_CAPACITANCE_C;
		i++;

		Prop = get_attribute(Parent, "in_port", loc_data, OPTIONAL).as_string(NULL);
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "out_port", loc_data, OPTIONAL).as_string(NULL);
		annotation->output_pins = vtr::strdup(Prop);
		VTR_ASSERT(
				annotation->output_pins != NULL || annotation->input_pins != NULL);

	} else if (0 == strcmp(Parent.name(), "T_setup")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "value", loc_data).value();
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_TSETUP;
		annotation->value[i] = vtr::strdup(Prop);

		i++;
		Prop = get_attribute(Parent, "port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "clock", loc_data).value();
		annotation->clock = vtr::strdup(Prop);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent.name(), "T_clock_to_Q")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "max", loc_data, OPTIONAL).as_string(NULL);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX;
			annotation->value[i] = vtr::strdup(Prop);
			i++;
		}
		Prop = get_attribute(Parent, "min", loc_data, OPTIONAL).as_string(NULL);
		if (Prop) {
			annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN;
			annotation->value[i] = vtr::strdup(Prop);
			i++;
		}

		Prop = get_attribute(Parent, "port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "clock", loc_data).value();
		annotation->clock = vtr::strdup(Prop);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent.name(), "T_hold")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_DELAY;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "value", loc_data).value();
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_DELAY_THOLD;
		annotation->value[i] = vtr::strdup(Prop);
		i++;

		Prop = get_attribute(Parent, "port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "clock", loc_data).value();
		annotation->clock = vtr::strdup(Prop);

		primitives_annotation_clock_match(annotation, parent_pb_type);

	} else if (0 == strcmp(Parent.name(), "pack_pattern")) {
		annotation->type = E_ANNOT_PIN_TO_PIN_PACK_PATTERN;
		annotation->format = E_ANNOT_PIN_TO_PIN_CONSTANT;
		Prop = get_attribute(Parent, "name", loc_data).value();
		annotation->prop[i] = (int) E_ANNOT_PIN_TO_PIN_PACK_PATTERN_NAME;
		annotation->value[i] = vtr::strdup(Prop);
		i++;

		Prop = get_attribute(Parent, "in_port", loc_data).value();
		annotation->input_pins = vtr::strdup(Prop);

		Prop = get_attribute(Parent, "out_port", loc_data).value();
		annotation->output_pins = vtr::strdup(Prop);

	} else {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
				"Unknown port type %s in %s in %s", Parent.name(),
				Parent.parent().name(), Parent.parent().parent().name() );
	} 
	VTR_ASSERT(i == annotation->num_value_prop_pairs);
}

static void ProcessPb_TypePowerPinToggle(pugi::xml_node parent, t_pb_type * pb_type, const pugiutil::loc_data& loc_data) {
	pugi::xml_node cur;
	const char * prop;
	t_port * port;
	int high, low;

	cur = get_first_child(parent, "port", loc_data, OPTIONAL);
	while (cur) {
		prop = get_attribute(cur, "name", loc_data).value();

		port = findPortByName(prop, pb_type, &high, &low);
		if (!port) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
					"Could not find port '%s' needed for energy per toggle.",
					prop);
		}
		if (high != port->num_pins - 1 || low != 0) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
					"Pin-toggle does not support pin indices (%s)", prop);
		}

		if (port->port_power->pin_toggle_initialized) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
					"Duplicate pin-toggle energy for port '%s'", port->name);
		}
		port->port_power->pin_toggle_initialized = true;

		/* Get energy per toggle */
		port->port_power->energy_per_toggle = get_attribute(cur,
				"energy_per_toggle", loc_data).as_float(0.);

		/* Get scaled by factor */
		bool reverse_scaled = false;
		prop = get_attribute(cur, "scaled_by_static_prob", loc_data, OPTIONAL).as_string(NULL);
		if (!prop) {
			prop = get_attribute(cur, "scaled_by_static_prob_n", loc_data, OPTIONAL).as_string(NULL);
			if (prop) {
				reverse_scaled = true;
			}
		}

		if (prop) {
			port->port_power->scaled_by_port = findPortByName(prop, pb_type,
					&high, &low);
			if (high != low) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Pin-toggle 'scaled_by_static_prob' must be a single pin (%s)",
						prop);
			}
			port->port_power->scaled_by_port_pin_idx = high;
			port->port_power->reverse_scaled = reverse_scaled;
		}

		cur = cur.next_sibling(cur.name());
	}
}

static void ProcessPb_TypePower(pugi::xml_node Parent, t_pb_type * pb_type, const pugiutil::loc_data& loc_data) {
	pugi::xml_node cur, child;
	bool require_dynamic_absolute = false;
	bool require_static_absolute = false;
	bool require_dynamic_C_internal = false;

	cur = get_first_child(Parent, "power", loc_data, OPTIONAL);
	if (!cur) {
		return;
	}

	switch (pb_type->pb_type_power->estimation_method) {
	case POWER_METHOD_TOGGLE_PINS:
		ProcessPb_TypePowerPinToggle(cur, pb_type, loc_data);
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
		child = get_single_child(cur, "static_power", loc_data);
		pb_type->pb_type_power->absolute_power_per_instance.leakage =
				get_attribute(child, "power_per_instance", loc_data).as_float(0.);
	}

	if (require_dynamic_absolute) {
		child = get_single_child(cur, "dynamic_power", loc_data);
		pb_type->pb_type_power->absolute_power_per_instance.dynamic =
				get_attribute(child, "power_per_instance", loc_data).as_float(0.);
	}

	if (require_dynamic_C_internal) {
		child = get_single_child(cur, "dynamic_power", loc_data);
		pb_type->pb_type_power->C_internal = get_attribute(child,
				"C_internal", loc_data).as_float(0.);
	}

}

static void ProcessPb_TypePowerEstMethod(pugi::xml_node Parent, t_pb_type * pb_type, const pugiutil::loc_data& loc_data) {
	pugi::xml_node cur;
	const char * prop;

	e_power_estimation_method parent_power_method;

	prop = NULL;

	cur = get_first_child(Parent, "power", loc_data, OPTIONAL);
	if (cur) {
		prop = get_attribute(cur, "method", loc_data, OPTIONAL).as_string(NULL);
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
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
				"Invalid power estimation method for pb_type '%s'",
				pb_type->name);
	}
}

/* Takes in a pb_type, allocates and loads data for it and recurses downwards */
static void ProcessPb_Type(pugi::xml_node Parent, t_pb_type * pb_type,
		t_mode * mode, const pugiutil::loc_data& loc_data) {
	int num_ports, i, j, k, num_annotations;
	const char *Prop;
	pugi::xml_node Cur;

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
		Prop = get_attribute(Parent, "name", loc_data).value();
		pb_type->name = vtr::strdup(Prop);
	} else {
		pb_type->depth = 0;
		/* same name as type */
	}

	Prop = get_attribute(Parent, "blif_model", loc_data, OPTIONAL).as_string(NULL);
	pb_type->blif_model = vtr::strdup(Prop);

	pb_type->class_type = UNKNOWN_CLASS;
	Prop = get_attribute(Parent, "class", loc_data, OPTIONAL).as_string(NULL);
	class_name = vtr::strdup(Prop);

	if (class_name) {

		if (0 == strcmp(class_name, "lut")) {
			pb_type->class_type = LUT_CLASS;
		} else if (0 == strcmp(class_name, "flipflop")) {
			pb_type->class_type = LATCH_CLASS;
		} else if (0 == strcmp(class_name, "memory")) {
			pb_type->class_type = MEMORY_CLASS;
		} else {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
					"Unknown class '%s' in pb_type '%s'\n", class_name,
					pb_type->name);
		}
		free(class_name);
	}

	if (mode == NULL) {
		pb_type->num_pb = 1;
	} else {
		pb_type->num_pb = get_attribute(Parent, "num_pb", loc_data).as_int(0);
	}

	VTR_ASSERT(pb_type->num_pb > 0);
	num_ports = num_in_ports = num_out_ports = num_clock_ports = 0;
	num_in_ports = count_children(Parent, "input", loc_data, OPTIONAL);
	num_out_ports = count_children(Parent, "output", loc_data, OPTIONAL);
	num_clock_ports = count_children(Parent, "clock", loc_data, OPTIONAL);
	num_ports = num_in_ports + num_out_ports + num_clock_ports;
	pb_type->ports = (t_port*) vtr::calloc(num_ports, sizeof(t_port));
	pb_type->num_ports = num_ports;

	/* Enforce VPR's definition of LUT/FF by checking number of ports */
	if (pb_type->class_type == LUT_CLASS
			|| pb_type->class_type == LATCH_CLASS) {
		if (num_in_ports != 1 || num_out_ports != 1) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
					"%s primitives must contain exactly one input port and one output port."
							"Found '%d' input port(s) and '%d' output port(s) for '%s'",
					(pb_type->class_type == LUT_CLASS) ? "LUT" : "Latch",
					num_in_ports, num_out_ports, pb_type->name);
		}
	}


	/* Initialize Power Structure */
	pb_type->pb_type_power = (t_pb_type_power*) vtr::calloc(1,
			sizeof(t_pb_type_power));
	ProcessPb_TypePowerEstMethod(Parent, pb_type, loc_data);

	/* process ports */
	j = 0;
	for (i = 0; i < 3; i++) {
		if (i == 0) {
			k = 0;
			Cur = get_first_child(Parent, "input", loc_data, OPTIONAL);
		} else if (i == 1) {
			k = 0;
			Cur = get_first_child(Parent, "output", loc_data, OPTIONAL);
		} else {
			k = 0;
			Cur = get_first_child(Parent, "clock", loc_data, OPTIONAL);
		}
		while (Cur) {
			pb_type->ports[j].parent_pb_type = pb_type;
			pb_type->ports[j].index = j;
			pb_type->ports[j].port_index_by_type = k;
			ProcessPb_TypePort(Cur, &pb_type->ports[j],
					pb_type->pb_type_power->estimation_method, loc_data);

			//Check port name duplicates
			ret_pb_ports = pb_port_names.insert(
					pair<string, int>(pb_type->ports[j].name, 0));
			if (!ret_pb_ports.second) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Duplicate port names in pb_type '%s': port '%s'\n",
						pb_type->name, pb_type->ports[j].name);
			}

			/* get next iteration */
			j++;
			k++;
			Cur = Cur.next_sibling(Cur.name()); 
		}
	}

	VTR_ASSERT(j == num_ports);

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
			VTR_ASSERT(
					pb_type->ports[i].is_clock
							&& pb_type->ports[i].type == IN_PORT);
			pb_type->num_clock_pins += pb_type->ports[i].num_pins;
		}
	}

	/* set max_internal_delay if exist */
	pb_type->max_internal_delay = UNDEFINED;
	Cur = get_single_child(Parent, "max_internal_delay", loc_data, OPTIONAL);
	if (Cur) {
		pb_type->max_internal_delay = get_attribute(Cur, "value", loc_data).as_float(UNDEFINED);
	}

	pb_type->annotations = NULL;
	pb_type->num_annotations = 0;
	i = 0;
	/* Determine if this is a leaf or container pb_type */
	if (pb_type->blif_model != NULL) {
		/* Process delay and capacitance annotations */
		num_annotations = 0;
		num_delay_constant = count_children(Parent, "delay_constant", loc_data, OPTIONAL);
		num_delay_matrix = count_children(Parent, "delay_matrix", loc_data, OPTIONAL);
		num_C_constant = count_children(Parent, "C_constant", loc_data, OPTIONAL);
		num_C_matrix = count_children(Parent, "C_matrix", loc_data, OPTIONAL);
		num_T_setup = count_children(Parent, "T_setup", loc_data, OPTIONAL);
		num_T_cq = count_children(Parent, "T_clock_to_Q", loc_data, OPTIONAL);
		num_T_hold = count_children(Parent, "T_hold", loc_data, OPTIONAL);
		num_annotations = num_delay_constant + num_delay_matrix + num_C_constant
				+ num_C_matrix + num_T_setup + num_T_cq + num_T_hold;

		pb_type->annotations = (t_pin_to_pin_annotation*) vtr::calloc(
				num_annotations, sizeof(t_pin_to_pin_annotation));
		pb_type->num_annotations = num_annotations;

		j = 0;
		for (i = 0; i < 7; i++) {
			if (i == 0) {
				Cur = get_first_child(Parent, "delay_constant", loc_data, OPTIONAL);
			} else if (i == 1) {
				Cur = get_first_child(Parent, "delay_matrix", loc_data, OPTIONAL);
			} else if (i == 2) {
				Cur = get_first_child(Parent, "C_constant", loc_data, OPTIONAL);
			} else if (i == 3) {
				Cur = get_first_child(Parent, "C_matrix", loc_data, OPTIONAL);
			} else if (i == 4) {
				Cur = get_first_child(Parent, "T_setup", loc_data, OPTIONAL);
			} else if (i == 5) {
				Cur = get_first_child(Parent, "T_clock_to_Q", loc_data, OPTIONAL);
			} else if (i == 6) {
				Cur = get_first_child(Parent, "T_hold", loc_data, OPTIONAL);
			}
			while (Cur) {
				ProcessPinToPinAnnotations(Cur, &pb_type->annotations[j],
						pb_type, loc_data);

				/* get next iteration */
				j++;
				Cur = Cur.next_sibling(Cur.name());
			}
		}
		VTR_ASSERT(j == num_annotations);

		/* leaf pb_type, if special known class, then read class lib otherwise treat as primitive */
		if (pb_type->class_type == LUT_CLASS) {
			ProcessLutClass(pb_type);
		} else if (pb_type->class_type == MEMORY_CLASS) {
			ProcessMemoryClass(pb_type);
		} else {
			/* other leaf pb_type do not have modes */
			pb_type->num_modes = 0;
			VTR_ASSERT(count_children(Parent, "mode", loc_data, OPTIONAL) == 0);
		}
	} else {
		/* container pb_type, process modes */
		VTR_ASSERT(pb_type->class_type == UNKNOWN_CLASS);
		pb_type->num_modes = count_children(Parent, "mode", loc_data, OPTIONAL);
		pb_type->pb_type_power->leakage_default_mode = 0;

		if (pb_type->num_modes == 0) {
			/* The pb_type operates in an implied one mode */
			pb_type->num_modes = 1;
			pb_type->modes = (t_mode*) vtr::calloc(pb_type->num_modes,
					sizeof(t_mode));
			pb_type->modes[i].parent_pb_type = pb_type;
			pb_type->modes[i].index = i;
			ProcessMode(Parent, &pb_type->modes[i], loc_data);
			i++;
		} else {
			pb_type->modes = (t_mode*) vtr::calloc(pb_type->num_modes,
					sizeof(t_mode));

			Cur = get_first_child(Parent, "mode", loc_data);
			while (Cur != NULL) {
				if (0 == strcmp(Cur.name(), "mode")) {
					pb_type->modes[i].parent_pb_type = pb_type;
					pb_type->modes[i].index = i;
					ProcessMode(Cur, &pb_type->modes[i], loc_data);

					ret_mode_names = mode_names.insert(
							pair<string, int>(pb_type->modes[i].name, 0));
					if (!ret_mode_names.second) {
						archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
								"Duplicate mode name: '%s' in pb_type '%s'.\n",
								pb_type->modes[i].name, pb_type->name);
					}

					/* get next iteration */
					i++;
					Cur = Cur.next_sibling(Cur.name());
				}
			}
		}
		VTR_ASSERT(i == pb_type->num_modes);
	}

	pb_port_names.clear();
	mode_names.clear();
	ProcessPb_TypePower(Parent, pb_type, loc_data);

}

static void ProcessPb_TypePort_Power(pugi::xml_node Parent, t_port * port,
		e_power_estimation_method power_method, const pugiutil::loc_data& loc_data) {
	pugi::xml_node cur;
	const char * prop;
	bool wire_defined = false;

	port->port_power = (t_port_power*) vtr::calloc(1, sizeof(t_port_power));

	//Defaults
	if (power_method == POWER_METHOD_AUTO_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_AUTO;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
	} else if (power_method == POWER_METHOD_SPECIFY_SIZES) {
		port->port_power->wire_type = POWER_WIRE_TYPE_IGNORED;
		port->port_power->buffer_type = POWER_BUFFER_TYPE_NONE;
	}

	cur = get_single_child(Parent, "power", loc_data, OPTIONAL);

	if (cur) {
		/* Wire capacitance */

		/* Absolute C provided */
		prop = get_attribute(cur, "wire_capacitance", loc_data, OPTIONAL).as_string(NULL);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Wire capacitance defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_C;
				port->port_power->wire.C = (float) atof(prop);
			}
		}

		/* Wire absolute length provided */
		prop = get_attribute(cur, "wire_length", loc_data, OPTIONAL).as_string(NULL);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Wire length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
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
		}

		/* Wire relative length provided */
		prop = get_attribute(cur, "wire_relative_length", loc_data, OPTIONAL).as_string(NULL);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Wire relative length defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (wire_defined) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Multiple wire properties defined for port '%s', pb_type '%s'.",
						port->name, port->parent_pb_type->name);
			} else {
				wire_defined = true;
				port->port_power->wire_type = POWER_WIRE_TYPE_RELATIVE_LENGTH;
				port->port_power->wire.relative_length = (float) atof(prop);
			}
		}

		/* Buffer Size */
		prop = get_attribute(cur, "buffer_size", loc_data, OPTIONAL).as_string(NULL);
		if (prop) {
			if (!(power_method == POWER_METHOD_AUTO_SIZES
					|| power_method == POWER_METHOD_SPECIFY_SIZES)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(cur),
						"Buffer size defined for port '%s'.  This is an invalid option for the parent pb_type '%s' power estimation method.",
						port->name, port->parent_pb_type->name);
			} else if (strcmp(prop, "auto") == 0) {
				port->port_power->buffer_type = POWER_BUFFER_TYPE_AUTO;
			} else {
				port->port_power->buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
				port->port_power->buffer_size = (float) atof(prop);
			}
		}
	}
}

static void ProcessPb_TypePort(pugi::xml_node Parent, t_port * port,
		e_power_estimation_method power_method, const pugiutil::loc_data& loc_data) {
	const char *Prop;
	Prop = get_attribute(Parent, "name", loc_data).value();
	port->name = vtr::strdup(Prop);

	Prop = get_attribute(Parent, "port_class", loc_data, OPTIONAL).as_string(NULL);
	port->port_class = vtr::strdup(Prop);

	Prop = get_attribute(Parent, "chain", loc_data, OPTIONAL).as_string(NULL);
	port->chain_name = vtr::strdup(Prop);

	port->equivalent = get_attribute(Parent, "equivalent", loc_data, OPTIONAL).as_bool(false);
	port->num_pins = get_attribute(Parent, "num_pins", loc_data).as_int(0);
	port->is_non_clock_global = get_attribute(Parent,
			"is_non_clock_global", loc_data, OPTIONAL).as_bool(false);

	if (0 == strcmp(Parent.name(), "input")) {
		port->type = IN_PORT;
		port->is_clock = false;

		/* Check if LUT/FF port class is lut_in/D */
		if (port->parent_pb_type->class_type == LUT_CLASS) {
			if ((!port->port_class) || strcmp("lut_in", port->port_class)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Inputs to LUT primitives must have a port class named "
								"as \"lut_in\".");
			}
		} else if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("D", port->port_class)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Input to flipflop primitives must have a port class named "
								"as \"D\".");
			}
			/* Only allow one input pin for FF's */
			if (port->num_pins != 1) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Input port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}

	} else if (0 == strcmp(Parent.name(), "output")) {
		port->type = OUT_PORT;
		port->is_clock = false;

		/* Check if LUT/FF port class is lut_out/Q */
		if (port->parent_pb_type->class_type == LUT_CLASS) {
			if ((!port->port_class) || strcmp("lut_out", port->port_class)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Output to LUT primitives must have a port class named "
								"as \"lut_in\".");
			}
			/* Only allow one output pin for LUT's */
			if (port->num_pins != 1) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Output port of LUT primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		} else if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("Q", port->port_class)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Output to flipflop primitives must have a port class named "
								"as \"D\".");
			}
			/* Only allow one output pin for FF's */
			if (port->num_pins != 1) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Output port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}
	} else if (0 == strcmp(Parent.name(), "clock")) {
		port->type = IN_PORT;
		port->is_clock = true;
		if (port->is_non_clock_global == true) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
					"Port %s cannot be both a clock and a non-clock simultaneously\n",
					Parent.name());
		}

		if (port->parent_pb_type->class_type == LATCH_CLASS) {
			if ((!port->port_class) || strcmp("clock", port->port_class)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Clock to flipflop primitives must have a port class named "
								"as \"clock\".");
			}
			/* Only allow one output pin for FF's */
			if (port->num_pins != 1) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
						"Clock port of flipflop primitives must have exactly one pin. "
								"Found %d.", port->num_pins);
			}
		}
	} else {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Parent),
				"Unknown port type %s", Parent.name());
	}

	ProcessPb_TypePort_Power(Parent, port, power_method, loc_data);
}

static void ProcessInterconnect(pugi::xml_node Parent, t_mode * mode, const pugiutil::loc_data& loc_data) {
	int num_interconnect = 0;
	int num_complete, num_direct, num_mux;
	int i, j, k, L_index, num_annotations;
	int num_delay_constant, num_delay_matrix, num_C_constant, num_C_matrix,
			num_pack_pattern;
	const char *Prop;
	pugi::xml_node Cur;
	pugi::xml_node Cur2;

	map<string, int> interc_names;
	pair<map<string, int>::iterator, bool> ret_interc_names;

	num_complete = num_direct = num_mux = 0;
	num_complete = count_children(Parent, "complete", loc_data, OPTIONAL);
	num_direct = count_children(Parent, "direct", loc_data, OPTIONAL);
	num_mux = count_children(Parent, "mux", loc_data, OPTIONAL);
	num_interconnect = num_complete + num_direct + num_mux;

	mode->num_interconnect = num_interconnect;
	mode->interconnect = (t_interconnect*) vtr::calloc(num_interconnect,
			sizeof(t_interconnect));

	i = 0;
	for (L_index = 0; L_index < 3; L_index++) {
		if (L_index == 0) {
			Cur = get_first_child(Parent, "complete", loc_data, OPTIONAL);
		} else if (L_index == 1) {
			Cur = get_first_child(Parent, "direct", loc_data, OPTIONAL);
		} else {
			Cur = get_first_child(Parent, "mux", loc_data, OPTIONAL);
		}
		while (Cur != NULL) {
			if (0 == strcmp(Cur.name(), "complete")) {
				mode->interconnect[i].type = COMPLETE_INTERC;
			} else if (0 == strcmp(Cur.name(), "direct")) {
				mode->interconnect[i].type = DIRECT_INTERC;
			} else {
				VTR_ASSERT(0 == strcmp(Cur.name(), "mux"));
				mode->interconnect[i].type = MUX_INTERC;
			}

			mode->interconnect[i].line_num = loc_data.line(Cur);

			mode->interconnect[i].parent_mode_index = mode->index;
			mode->interconnect[i].parent_mode = mode;

			Prop = get_attribute(Cur, "input", loc_data).value();
			mode->interconnect[i].input_string = vtr::strdup(Prop);

			Prop = get_attribute(Cur, "output", loc_data).value();
			mode->interconnect[i].output_string = vtr::strdup(Prop);

			Prop = get_attribute(Cur, "name", loc_data).value();
			mode->interconnect[i].name = vtr::strdup(Prop);

			ret_interc_names = interc_names.insert(
					pair<string, int>(mode->interconnect[i].name, 0));
			if (!ret_interc_names.second) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
						"Duplicate interconnect name: '%s' in mode: '%s'.\n",
						mode->interconnect[i].name, mode->name);
			}

			/* Process delay and capacitance annotations */
			num_annotations = 0;
			num_delay_constant = count_children(Cur, "delay_constant", loc_data, OPTIONAL);
			num_delay_matrix = count_children(Cur, "delay_matrix", loc_data, OPTIONAL);
			num_C_constant = count_children(Cur, "C_constant", loc_data, OPTIONAL);
			num_C_matrix = count_children(Cur, "C_matrix", loc_data, OPTIONAL);
			num_pack_pattern = count_children(Cur, "pack_pattern", loc_data, OPTIONAL);
			num_annotations = num_delay_constant + num_delay_matrix
					+ num_C_constant + num_C_matrix + num_pack_pattern;


			mode->interconnect[i].annotations =
					(t_pin_to_pin_annotation*) vtr::calloc(num_annotations,
							sizeof(t_pin_to_pin_annotation));
			mode->interconnect[i].num_annotations = num_annotations;

			k = 0;
			for (j = 0; j < 5; j++) {
				if (j == 0) {
					Cur2 = get_first_child(Cur, "delay_constant", loc_data, OPTIONAL);
				} else if (j == 1) {
					Cur2 = get_first_child(Cur, "delay_matrix", loc_data, OPTIONAL);
				} else if (j == 2) {
					Cur2 = get_first_child(Cur, "C_constant", loc_data, OPTIONAL);
				} else if (j == 3) {
					Cur2 = get_first_child(Cur, "C_matrix", loc_data, OPTIONAL);
				} else if (j == 4) {
					Cur2 = get_first_child(Cur, "pack_pattern", loc_data, OPTIONAL);
				}
				while (Cur2 != NULL) {
					ProcessPinToPinAnnotations(Cur2,
							&(mode->interconnect[i].annotations[k]), NULL, loc_data);

					/* get next iteration */
					k++;
					Cur2 = Cur2.next_sibling(Cur2.name());
				}
			}
			VTR_ASSERT(k == num_annotations);

			/* Power */
			mode->interconnect[i].interconnect_power =
					(t_interconnect_power*) vtr::calloc(1,
							sizeof(t_interconnect_power));
			mode->interconnect[i].interconnect_power->port_info_initialized =
					false;

			/* get next iteration */
			Cur = Cur.next_sibling(Cur.name());			
			i++;
		}
	}

	interc_names.clear();
	VTR_ASSERT(i == num_interconnect);
}

static void ProcessMode(pugi::xml_node Parent, t_mode * mode,
		const pugiutil::loc_data& loc_data) {
	int i;
	const char *Prop;
	pugi::xml_node Cur;
	map<string, int> pb_type_names;
	pair<map<string, int>::iterator, bool> ret_pb_types;

	if (0 == strcmp(Parent.name(), "pb_type")) {
		/* implied mode */
		mode->name = vtr::strdup(mode->parent_pb_type->name);
	} else {
		Prop = get_attribute(Parent, "name", loc_data).value();
		mode->name = vtr::strdup(Prop);
	}

	mode->num_pb_type_children = count_children(Parent, "pb_type", loc_data, OPTIONAL);
	if (mode->num_pb_type_children > 0) {
		mode->pb_type_children = (t_pb_type*) vtr::calloc(
				mode->num_pb_type_children, sizeof(t_pb_type));

		i = 0;
		Cur = get_first_child(Parent, "pb_type", loc_data);
		while (Cur != NULL) {
			if (0 == strcmp(Cur.name(), "pb_type")) {
				ProcessPb_Type(Cur, &mode->pb_type_children[i], mode, loc_data);

				ret_pb_types = pb_type_names.insert(
						pair<string, int>(mode->pb_type_children[i].name, 0));
				if (!ret_pb_types.second) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
							"Duplicate pb_type name: '%s' in mode: '%s'.\n",
							mode->pb_type_children[i].name, mode->name);
				}

				/* get next iteration */
				i++;
				Cur = Cur.next_sibling(Cur.name());
			}
		}
	} else {
		mode->pb_type_children = NULL;
	}

	/* Allocate power structure */
	mode->mode_power = (t_mode_power*) vtr::calloc(1, sizeof(t_mode_power));

	/* Clear STL map used for duplicate checks */
	pb_type_names.clear();

	Cur = get_single_child(Parent, "interconnect", loc_data);
	ProcessInterconnect(Cur, mode, loc_data);
}

/* Takes in the node ptr for the 'fc_in' and 'fc_out' elements and initializes
 * the appropriate fields of type. */
static void Process_Fc(pugi::xml_node Node, t_type_descriptor * Type, t_segment_inf *segments, int num_segments, const pugiutil::loc_data& loc_data) {
	enum Fc_type def_type_in, def_type_out, ovr_type;
	const char *Prop, *Prop2;
	char *port_name;
	float def_in_val, def_out_val, ovr_val;
	int ipin, iclass, end_pin_index, start_pin_index, match_count;
	int iport, iport_pin, curr_pin, port_found;
	pugi::xml_node Child;

	def_type_in = FC_FRAC;
	def_type_out = FC_FRAC;
    ovr_type = FC_FRAC; //Remove uninitialized warning
	def_in_val = OPEN;
	def_out_val = OPEN;

	Type->is_Fc_frac = (bool *) vtr::malloc(Type->num_pins * sizeof(bool));
	Type->is_Fc_full_flex = (bool *) vtr::malloc(
			Type->num_pins * sizeof(bool));
	Type->Fc = vtr::alloc_matrix<float>(0, Type->num_pins-1, 0, num_segments-1);

	/* Load the default fc_in, if specified */
	Prop = get_attribute(Node, "default_in_type", loc_data, OPTIONAL).as_string(NULL);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_in = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_in = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_in = FC_FULL;
		} else {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
					Prop);
		}
		switch (def_type_in) {
		case FC_FULL:
			def_in_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = get_attribute(Node, "default_in_val", loc_data).value();
			def_in_val = (float) atof(Prop2);
			break;
		default:
			def_in_val = -1;
		}
	}

	/* Load the default fc_out, if specified */
	Prop = get_attribute(Node, "default_out_type", loc_data, OPTIONAL).as_string(NULL);
	if (Prop != NULL) {
		if (0 == strcmp(Prop, "abs")) {
			def_type_out = FC_ABS;
		} else if (0 == strcmp(Prop, "frac")) {
			def_type_out = FC_FRAC;
		} else if (0 == strcmp(Prop, "full")) {
			def_type_out = FC_FULL;
		} else {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
					Prop);
		}
		switch (def_type_out) {
		case FC_FULL:
			def_out_val = 0.0;
			break;
		case FC_ABS:
		case FC_FRAC:
			Prop2 = get_attribute(Node, "default_out_val", loc_data).value();
			def_out_val = (float) atof(Prop2);
			break;
		default:
			def_out_val = -1;
		}
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
	if (get_first_child(Node, "pin", loc_data, OPTIONAL) != NULL && get_first_child(Node, "segment", loc_data, OPTIONAL) != NULL){
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
				"Complex block 'fc' is allowed to specify pin-based fc overrides OR segment-based fc overrides, not both.\n");
	}

	/* Now, check for pin-based fc override - look for pin child. */
	Child = get_first_child(Node, "pin", loc_data, OPTIONAL);
	while (Child != NULL) {
		/* Get all the properties of the child first */
		Prop = get_attribute(Child, "name", loc_data).as_string(NULL);

		Prop2 = get_attribute(Child, "fc_type", loc_data).as_string(NULL);
		if (Prop2 != NULL) {
			if (0 == strcmp(Prop2, "abs")) {
				ovr_type = FC_ABS;
			} else if (0 == strcmp(Prop2, "frac")) {
				ovr_type = FC_FRAC;
			} else if (0 == strcmp(Prop2, "full")) {
				ovr_type = FC_FULL;
			} else {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
						"Invalid type '%s' for Fc. Only abs, frac and full are allowed.\n",
						Prop2);
			}
			switch (ovr_type) {
			case FC_FULL:
				ovr_val = 0.0;
				break;
			case FC_ABS:
			case FC_FRAC:
				Prop2 = get_attribute(Child, "fc_val", loc_data).value();
				ovr_val = (float) atof(Prop2);
				break;
			default:
				ovr_val = -1;
			}

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
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
							"Invalid name for pin child, name should be in the format \"port_name\" or \"port_name [end_pin_index:start_pin_index]\","
									"The end_pin_index and start_pin_index can be the same.\n");
				}
				if (end_pin_index < 0 || start_pin_index < 0) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
							"The pin_index should not be a negative value.\n");
				}
				if (end_pin_index < start_pin_index) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
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
						archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
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

							archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
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
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
						"The port \"%s\" cannot be found.\n", Prop);
			}

			/* End of case where fc_type of pin_child is specified. */
		} else {
			/* fc_type of pin_child is not specified. Error out. */
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
					"Pin child with no fc_type specified is not allowed.\n");
		}

		/* Find next child and frees up the current child. */
		Child = Child.next_sibling(Child.name());

	} /* End of processing pin children */

	/* now check for segment-based overrides. earlier in this function we already checked that both kinds of
	   overrides haven't been specified */
	Child = get_first_child(Node, "segment", loc_data, OPTIONAL);
	while (Child != NULL){
		const char *segment_name;
		int seg_ind;
		float in_val;
		float out_val;

		/* get name */
		Prop = get_attribute(Child, "name", loc_data).value();
		segment_name = Prop;

		/* get fc_in */
		Prop = get_attribute(Child, "in_val", loc_data).value();
		in_val = (float) atof(Prop);


		/* get fc_out */
		Prop = get_attribute(Child, "out_val", loc_data).value();
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
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Child),
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


		/* Find next child*/
		Child = Child.next_sibling(Child.name());
	}

}

/* Thie processes attributes of the 'type' tag */
static void ProcessComplexBlockProps(pugi::xml_node Node, t_type_descriptor * Type, const pugiutil::loc_data& loc_data) {
	const char *Prop;

	/* Load type name */
	Prop = get_attribute(Node, "name", loc_data).value();
	Type->name = vtr::strdup(Prop);

	/* Load properties */
	Type->capacity = get_attribute(Node, "capacity", loc_data, OPTIONAL).as_int(1); /* TODO: Any block with capacity > 1 that is not I/O has not been tested, must test */
	Type->width = get_attribute(Node, "width", loc_data, OPTIONAL).as_int(1);
	Type->height = get_attribute(Node, "height", loc_data, OPTIONAL).as_int(1);
	Type->area = get_attribute(Node, "area", loc_data, OPTIONAL).as_float(UNDEFINED);

	if (atof(Prop) < 0) {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
				"Area for type %s must be non-negative\n", Type->name);
	}
}

/* Takes in node pointing to <models> and loads all the
 * child type objects.  */
static void ProcessModels(pugi::xml_node Node, struct s_arch *arch, const pugiutil::loc_data& loc_data) {
	const char *Prop;
	pugi::xml_node child;
	pugi::xml_node p;
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
	child = get_first_child(Node, "model", loc_data, OPTIONAL);
	while (child != NULL) {
		temp = (t_model*) vtr::calloc(1, sizeof(t_model));
		temp->used = 0;
		temp->inputs = temp->outputs = NULL;
		temp->instances = NULL;
		Prop = get_attribute(child, "name", loc_data).value();
		temp->name = vtr::strdup(Prop);
		temp->pb_types = NULL;
		temp->index = L_index;
		L_index++;

		/* Try insert new model, check if already exist at the same time */
		ret_map_name = model_name_map.insert(pair<string, int>(temp->name, 0));
		if (!ret_map_name.second) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(child),
					"Duplicate model name: '%s'.\n", temp->name);
		}

		/* Process the inputs */
		p = get_first_child(child, "input_ports", loc_data);

		p = get_first_child(p, "port", loc_data);
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) vtr::calloc(1, sizeof(t_model_ports));
				Prop = get_attribute(p, "name", loc_data).value();
				tp->name = vtr::strdup(Prop);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->inputs;
				tp->dir = IN_PORT;
				tp->is_non_clock_global = get_attribute(p,
						"is_non_clock_global", loc_data, OPTIONAL).as_bool(false);
				tp->is_clock = false;
				Prop = get_attribute(p, "is_clock", loc_data, OPTIONAL).as_string(NULL);
				if (Prop && vtr::atoi(Prop) != 0) {
					tp->is_clock = true;
				}

				if (tp->is_clock == true && tp->is_non_clock_global == true) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(p),
							"Signal cannot be both a clock and a non-clock signal simultaneously\n");
				}

				/* Try insert new port, check if already exist at the same time */
				ret_map_port = model_port_map.insert(
						pair<string, int>(tp->name, 0));
				if (!ret_map_port.second) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(p),
							"Duplicate model input port name: '%s'.\n",
							tp->name);
				}

				temp->inputs = tp;
				p = p.next_sibling(p.name());
			}
		}

		/* Process the outputs */
		p = get_first_child(child, "output_ports", loc_data);
		p = get_first_child(p, "port", loc_data);
		if (p != NULL) {
			while (p != NULL) {
				tp = (t_model_ports*) vtr::calloc(1, sizeof(t_model_ports));
				Prop = get_attribute(p, "name", loc_data).value();
				tp->name = vtr::strdup(Prop);
				tp->size = -1; /* determined later by pb_types */
				tp->min_size = -1; /* determined later by pb_types */
				tp->next = temp->outputs;
				tp->dir = OUT_PORT;
				Prop = get_attribute(p, "is_clock", loc_data, OPTIONAL).as_string(NULL);
				if (Prop && vtr::atoi(Prop) != 0) {
					tp->is_clock = true;
				}

				if (tp->is_clock == true && tp->is_non_clock_global == true) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(p),
							"Signal cannot be both a clock and a non-clock signal simultaneously\n");
				}

				/* Try insert new output port, check if already exist at the same time */
				ret_map_port = model_port_map.insert(
						pair<string, int>(tp->name, 0));
				if (!ret_map_port.second) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(p),
							"Duplicate model output port name: '%s'.\n",
							tp->name);
				}

				temp->outputs = tp;
				p = p.next_sibling(p.name());
			}
		} 

		/* Clear port map for next model */
		model_port_map.clear();
		/* Push new model onto model stack */
		temp->next = arch->models;
		arch->models = temp;
		/* Find next model */
		child = child.next_sibling(child.name());
	}
	model_port_map.clear();
	model_name_map.clear();
	return;
}

/* Takes in node pointing to <layout> and loads all the
 * child type objects. */
static void ProcessLayout(pugi::xml_node Node, struct s_arch *arch, const pugiutil::loc_data& loc_data) {
	const char *Prop;

	arch->clb_grid.IsAuto = true;
	ReqOpt CLB_GRID_ISAUTO;

	/* Load width and height if applicable */
	Prop = get_attribute(Node, "width", loc_data, OPTIONAL).as_string(NULL);
	if (Prop != NULL) {
		arch->clb_grid.IsAuto = false;
		arch->clb_grid.W = vtr::atoi(Prop);

		arch->clb_grid.H = get_attribute(Node, "height", loc_data).as_int(UNDEFINED);
	}

	/* Load aspect ratio if applicable */
	CLB_GRID_ISAUTO = BoolToReqOpt(arch->clb_grid.IsAuto);
	Prop = get_attribute(Node, "auto", loc_data, CLB_GRID_ISAUTO).as_string(NULL);
	if (Prop != NULL) {
		if (arch->clb_grid.IsAuto == false) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Auto-sizing, width and height cannot be specified\n");
		}
		arch->clb_grid.Aspect = (float) atof(Prop);
		if (arch->clb_grid.Aspect <= 0) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Grid aspect ratio is less than or equal to zero %g\n",
					arch->clb_grid.Aspect);
		}
	}
}

/* Takes in node pointing to <device> and loads all the
 * child type objects. */
static void ProcessDevice(pugi::xml_node Node, struct s_arch *arch,
		const bool timing_enabled, const pugiutil::loc_data& loc_data) {
	const char *Prop;
	pugi::xml_node Cur;
	bool custom_switch_block = false;

	ProcessSizingTimingIpinCblock(Node, arch, timing_enabled, loc_data);

	Cur = get_single_child(Node, "area", loc_data);
	arch->grid_logic_tile_area = get_attribute(Cur, "grid_logic_tile_area",
			loc_data, OPTIONAL).as_float(0);

	Cur = get_single_child(Node, "chan_width_distr", loc_data, OPTIONAL);
	if (Cur != NULL) {
		ProcessChanWidthDistr(Cur, arch, loc_data);
	}

	Cur = get_single_child(Node, "switch_block", loc_data);
	Prop = get_attribute(Cur, "type", loc_data).value();
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
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Cur),
				"Unknown property %s for switch block type x\n", Prop);
	}
	
	ReqOpt CUSTOM_SWITCHBLOCK_REQD = BoolToReqOpt(!custom_switch_block);
	arch->Fs = get_attribute(Cur, "fs", loc_data, CUSTOM_SWITCHBLOCK_REQD).as_int(3);

}

/* Processes the sizing, timing, and ipin_cblock child objects of the 'device' node.
   We can specify an ipin cblock's info through the sizing/timing nodes (legacy),
   OR through the ipin_cblock node which specifies the info using the index of a switch. */
static void ProcessSizingTimingIpinCblock(pugi::xml_node Node,
		struct s_arch *arch, const bool timing_enabled, const pugiutil::loc_data& loc_data) {

	pugi::xml_node Cur;

	arch->ipin_mux_trans_size = UNDEFINED;
	arch->C_ipin_cblock = UNDEFINED;
	arch->T_ipin_cblock = UNDEFINED;
	ReqOpt TIMING_ENABLE_REQD;	
	

	TIMING_ENABLE_REQD = BoolToReqOpt(timing_enabled);

	Cur = get_single_child(Node, "sizing", loc_data);
	arch->R_minW_nmos = get_attribute(Cur, "R_minW_nmos", loc_data, TIMING_ENABLE_REQD).as_float(0);
	arch->R_minW_pmos = get_attribute(Cur, "R_minW_pmos", loc_data, TIMING_ENABLE_REQD).as_float(0);
	arch->ipin_mux_trans_size = get_attribute(Cur, "ipin_mux_trans_size",
			loc_data, OPTIONAL).as_float(0);

	/* currently only ipin cblock info is specified in the timing node */
	TIMING_ENABLE_REQD = BoolToReqOpt(timing_enabled);
	Cur = get_single_child(Node, "timing", loc_data, TIMING_ENABLE_REQD);
	if (Cur != NULL) {
		arch->C_ipin_cblock = get_attribute(Cur, "C_ipin_cblock", loc_data, OPTIONAL).as_float(0);
		arch->T_ipin_cblock = get_attribute(Cur, "T_ipin_cblock", loc_data, OPTIONAL).as_float(0);

	}
}

/* Takes in node pointing to <chan_width_distr> and loads all the
 * child type objects. */
static void ProcessChanWidthDistr(pugi::xml_node Node,
		struct s_arch *arch, const pugiutil::loc_data& loc_data) {
	pugi::xml_node Cur;

	Cur = get_single_child(Node, "io", loc_data);
	arch->Chans.chan_width_io = get_attribute(Cur, "width", loc_data).as_float(UNDEFINED);

	Cur = get_single_child(Node, "x", loc_data);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_x_dist, loc_data);

	Cur = get_single_child(Node, "y", loc_data);
	ProcessChanWidthDistrDir(Cur, &arch->Chans.chan_y_dist, loc_data);
}

/* Takes in node within <chan_width_distr> and loads all the
 * child type objects. */
static void ProcessChanWidthDistrDir(pugi::xml_node Node, t_chan * chan, const pugiutil::loc_data& loc_data) {
	const char *Prop;

	ReqOpt hasXpeak, hasWidth, hasDc;
	hasXpeak = hasWidth = hasDc = OPTIONAL;

	Prop = get_attribute(Node, "distr", loc_data).value();
	if (strcmp(Prop, "uniform") == 0) {
		chan->type = UNIFORM;
	} else if (strcmp(Prop, "gaussian") == 0) {
		chan->type = GAUSSIAN;
		hasXpeak = hasWidth = hasDc = REQUIRED;
	} else if (strcmp(Prop, "pulse") == 0) {
		chan->type = PULSE;
		hasXpeak = hasWidth = hasDc = REQUIRED;
	} else if (strcmp(Prop, "delta") == 0) {
		hasXpeak = hasDc = REQUIRED;
		chan->type = DELTA;
	} else {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
				"Unknown property %s for chan_width_distr x\n", Prop);
	}

	chan->peak = get_attribute(Node, "peak", loc_data).as_float(UNDEFINED);
	chan->width = get_attribute(Node, "width", loc_data, hasWidth).as_float(0);
	chan->xpeak = get_attribute(Node, "xpeak", loc_data, hasXpeak).as_float(0);
	chan->dc = get_attribute(Node, "dc", loc_data, hasDc).as_float(0);
}



/* Takes in node pointing to <typelist> and loads all the
 * child type objects. */
static void ProcessComplexBlocks(pugi::xml_node Node,
		t_type_descriptor ** Types, int *NumTypes,
		const s_arch& arch, const pugiutil::loc_data& loc_data) {
	pugi::xml_node CurType, Prev;
	pugi::xml_node Cur;
	t_type_descriptor * Type;
	int i;
	map<string, int> pb_type_descriptors;
	pair<map<string, int>::iterator, bool> ret_pb_type_descriptors;
	/* Alloc the type list. Need one additional t_type_desctiptors:
	 * 1: empty psuedo-type
	 */
	*NumTypes = count_children(Node, "pb_type", loc_data) + 1;
	*Types = (t_type_descriptor *) vtr::malloc(
			sizeof(t_type_descriptor) * (*NumTypes));

	cb_type_descriptors = *Types;

	EMPTY_TYPE = &cb_type_descriptors[EMPTY_TYPE_INDEX];
	IO_TYPE = &cb_type_descriptors[IO_TYPE_INDEX];
	cb_type_descriptors[EMPTY_TYPE_INDEX].index = EMPTY_TYPE_INDEX;
	cb_type_descriptors[IO_TYPE_INDEX].index = IO_TYPE_INDEX;
	SetupEmptyType(cb_type_descriptors, EMPTY_TYPE);

	/* Process the types */
	/* TODO: I should make this more flexible but release is soon and I don't have time so assert values for empty and io types*/
	VTR_ASSERT(EMPTY_TYPE_INDEX == 0);
	VTR_ASSERT(IO_TYPE_INDEX == 1);
	i = 1; /* Skip over 'empty' type */

	CurType = Node.first_child();
	while (CurType) {
		check_node(CurType, "pb_type", loc_data);

		/* Alias to current type */
		Type = &(*Types)[i];

		/* Parses the properties fields of the type */
		ProcessComplexBlockProps(CurType, Type, loc_data);

		ret_pb_type_descriptors = pb_type_descriptors.insert(
				pair<string, int>(Type->name, 0));
		if (!ret_pb_type_descriptors.second) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(CurType),
					"Duplicate pb_type descriptor name: '%s'.\n", Type->name);
		}

		/* Load pb_type info */
		Type->pb_type = (t_pb_type*) vtr::malloc(sizeof(t_pb_type));
		Type->pb_type->name = vtr::strdup(Type->name);
		if (i == IO_TYPE_INDEX) {
			if (strcmp(Type->name, "io") != 0) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(CurType),
						"First complex block must be named \"io\" and define the inputs and outputs for the FPGA");
			}
		}
		ProcessPb_Type(CurType, Type->pb_type, NULL, loc_data);
		Type->num_pins = Type->capacity
				* (Type->pb_type->num_input_pins
						+ Type->pb_type->num_output_pins
						+ Type->pb_type->num_clock_pins);
		Type->num_receivers = Type->capacity * Type->pb_type->num_input_pins;
		Type->num_drivers = Type->capacity * Type->pb_type->num_output_pins;

		/* Load pin names and classes and locations */
		Cur = get_single_child(CurType, "pinlocations", loc_data);
		SetupPinLocationsAndPinClasses(Cur, Type, loc_data);

		Cur = get_single_child(CurType, "gridlocations", loc_data);
		SetupGridLocations(Cur, Type, loc_data);

		/* Load Fc */
		Cur = get_single_child(CurType, "fc", loc_data);
		Process_Fc(Cur, Type, arch.Segments, arch.num_segments, loc_data);


		Type->index = i;

		/* Type fully read */
		++i;

		/* Free this node and get its next sibling node */
		CurType = CurType.next_sibling (CurType.name());

	}
	if (FILL_TYPE == NULL) {
		archfpga_throw(arch_file_name, 0,
				"grid location type 'fill' must be specified.\n");
	}
	pb_type_descriptors.clear();
}


static void ProcessSegments(pugi::xml_node Parent,
		struct s_segment_inf **Segs, int *NumSegs,
		const struct s_arch_switch_inf *Switches, const int NumSwitches,
		const bool timing_enabled, const bool switchblocklist_required, const pugiutil::loc_data& loc_data) {
	int i, j, length;
	const char *tmp;

	pugi::xml_node SubElem;
	pugi::xml_node Node;

	/* Count the number of segs and check they are in fact
	 * of segment elements. */
	*NumSegs = count_children(Parent, "segment", loc_data);

	/* Alloc segment list */
	*Segs = NULL;
	if (*NumSegs > 0) {
		*Segs = (struct s_segment_inf *) vtr::malloc(
				*NumSegs * sizeof(struct s_segment_inf));
		memset(*Segs, 0, (*NumSegs * sizeof(struct s_segment_inf)));
	}

	/* Load the segments. */
	Node = get_first_child(Parent, "segment", loc_data);
	for (i = 0; i < *NumSegs; ++i) {
		
		/* Get segment name */
		tmp = get_attribute(Node, "name", loc_data, OPTIONAL).as_string(NULL);
		if (tmp) {
			(*Segs)[i].name = vtr::strdup(tmp);
		} else {
			/* if swich block is "custom", then you have to provide a name for segment */
			if (switchblocklist_required) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"No name specified for the segment #%d.\n", i);

			}
			/* set name to default: "unnamed_segment_<segment_index>" */
			stringstream ss;
			ss << "unnamed_segment_" << i;
			string dummy = ss.str();
			tmp = dummy.c_str();
			(*Segs)[i].name = vtr::strdup(tmp);
		}

		/* Get segment length */
		length = 1; /* DEFAULT */
		tmp = get_attribute(Node, "length", loc_data, OPTIONAL).as_string(NULL);
		if (tmp) {
			if (strcmp(tmp, "longline") == 0) {
				(*Segs)[i].longline = true;
			} else {
				length = vtr::atoi(tmp);
			}
		}
		(*Segs)[i].length = length;

		/* Get the frequency */
		(*Segs)[i].frequency = 1; /* DEFAULT */
		tmp = get_attribute(Node, "freq", loc_data, OPTIONAL).as_string(NULL);
		if (tmp) {
			(*Segs)[i].frequency = (int) (atof(tmp) * MAX_CHANNEL_WIDTH);
		}

		/* Get timing info */
		ReqOpt TIMING_ENABLE_REQD = BoolToReqOpt(timing_enabled);
		(*Segs)[i].Rmetal = get_attribute(Node, "Rmetal", loc_data, TIMING_ENABLE_REQD).as_float(0);
		(*Segs)[i].Cmetal = get_attribute(Node, "Cmetal", loc_data, TIMING_ENABLE_REQD).as_float(0);

		/* Get Power info */
		/*
		 (*Segs)[i].Cmetal_per_m = get_attribute(Node, "Cmetal_per_m", false,
		 0.);*/

		/* Get the type */
		tmp = get_attribute(Node, "type", loc_data).value();
		if (0 == strcmp(tmp, "bidir")) {
			(*Segs)[i].directionality = BI_DIRECTIONAL;
		}

		else if (0 == strcmp(tmp, "unidir")) {
			(*Segs)[i].directionality = UNI_DIRECTIONAL;
		}

		else {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Invalid switch type '%s'.\n", tmp);
		}

		/* Get the wire and opin switches, or mux switch if unidir */
		if (UNI_DIRECTIONAL == (*Segs)[i].directionality) {
			SubElem = get_single_child(Node, "mux", loc_data);
			tmp = get_attribute(SubElem, "name", loc_data).value();

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(SubElem),
						"'%s' is not a valid mux name.\n", tmp);
			}

			/* Unidir muxes must have the same switch
			 * for wire and opin fanin since there is
			 * really only the mux in unidir. */
			(*Segs)[i].arch_wire_switch = j;
			(*Segs)[i].arch_opin_switch = j;
		}

		else {
			VTR_ASSERT(BI_DIRECTIONAL == (*Segs)[i].directionality);
			SubElem = get_single_child(Node, "wire_switch", loc_data);
			tmp = get_attribute(SubElem, "name", loc_data).value();

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(SubElem),
						"'%s' is not a valid wire_switch name.\n", tmp);
			}
			(*Segs)[i].arch_wire_switch = j;
			SubElem = get_single_child(Node, "opin_switch", loc_data);
			tmp = get_attribute(SubElem, "name", loc_data).value();

			/* Match names */
			for (j = 0; j < NumSwitches; ++j) {
				if (0 == strcmp(tmp, Switches[j].name)) {
					break; /* End loop so j is where we want it */
				}
			}
			if (j >= NumSwitches) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(SubElem),
						"'%s' is not a valid opin_switch name.\n", tmp);
			}
			(*Segs)[i].arch_opin_switch = j;
		}

		/* Setup the CB list if they give one, otherwise use full */
		(*Segs)[i].cb_len = length;
		(*Segs)[i].cb = (bool *) vtr::malloc(length * sizeof(bool));
		for (j = 0; j < length; ++j) {
			(*Segs)[i].cb[j] = true;
		}
		SubElem = get_single_child(Node, "cb", loc_data, OPTIONAL);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].cb, length, loc_data);
		}

		/* Setup the SB list if they give one, otherwise use full */
		(*Segs)[i].sb_len = (length + 1);
		(*Segs)[i].sb = (bool *) vtr::malloc((length + 1) * sizeof(bool));
		for (j = 0; j < (length + 1); ++j) {
			(*Segs)[i].sb[j] = true;
		}
		SubElem = get_single_child(Node, "sb", loc_data, OPTIONAL);
		if (SubElem) {
			ProcessCB_SB(SubElem, (*Segs)[i].sb, (length + 1), loc_data);
		}
		
		/* Get next Node */
		Node = Node.next_sibling(Node.name());
	}
}

/* Processes the switchblocklist section from the xml architecture file. 
   See vpr/SRC/route/build_switchblocks.c for a detailed description of this 
   switch block format */
static void ProcessSwitchblocks(pugi::xml_node Parent, vector<t_switchblock_inf> *switchblocks,
				const t_arch_switch_inf *switches, const int num_switches, const pugiutil::loc_data& loc_data){

	pugi::xml_node Node;
	pugi::xml_node SubElem;
	const char *tmp;

	/* get the number of switchblocks */
	int num_switchblocks = count_children(Parent, "switchblock", loc_data);
	switchblocks->reserve(num_switchblocks);

	
	/* read-in all switchblock data */
	Node = get_first_child(Parent, "switchblock", loc_data);
	for (int i_sb = 0; i_sb < num_switchblocks; i_sb++){
		/* use a temp variable which will be assigned to switchblocks later */
		t_switchblock_inf sb;

		/* get name */
		tmp = get_attribute(Node, "name", loc_data).as_string(NULL);
		if (tmp){
			sb.name = tmp;
		}

		/* get type */
		tmp = get_attribute(Node, "type", loc_data).as_string(NULL);
		if (tmp){
			if (0 == strcmp(tmp, "bidir")){
				sb.directionality = BI_DIRECTIONAL;
			} else if (0 == strcmp(tmp, "unidir")){
				sb.directionality = UNI_DIRECTIONAL;
			} else {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node), "Unsopported switchblock type: %s\n", tmp);
			}
		}

		/* get the switchblock location */
		SubElem = get_single_child(Node, "switchblock_location", loc_data);
		tmp = get_attribute(SubElem, "type", loc_data).as_string(NULL);
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
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(SubElem), "unrecognized switchblock location: %s\n", tmp);
			}
		}

		/* get switchblock permutation functions */
		SubElem = get_first_child(Node, "switchfuncs", loc_data);
		read_sb_switchfuncs(SubElem, &(sb), loc_data);
		
		read_sb_wireconns(switches, num_switches, Node, &(sb), loc_data);

		/* assign the sb to the switchblocks vector */		
		switchblocks->push_back(sb);

		/* run error checks on switch blocks */
		check_switchblock(&sb);

		Node = Node.next_sibling(Node.name());
	}

	return;
}


static void ProcessCB_SB(pugi::xml_node Node, bool * list,
		const int len, const pugiutil::loc_data& loc_data) {
	const char *tmp = NULL;
	int i;

	/* Check the type. We only support 'pattern' for now.
	 * Should add frac back eventually. */
	tmp = get_attribute(Node, "type", loc_data).value();
	if (0 == strcmp(tmp, "pattern")) {
		i = 0;

		/* Get the content string */
		tmp = Node.child_value();
		while (*tmp) {
			switch (*tmp) {
			case ' ':
			case '\t':
			case '\n':
				break;
			case 'T':
			case '1':
				if (i >= len) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
							"CB or SB depopulation is too long. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
				}
				list[i] = true;
				++i;
				break;
			case 'F':
			case '0':
				if (i >= len) {
					archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
							"CB or SB depopulation is too long. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
				}
				list[i] = false;
				++i;
				break;
			default:
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
						"Invalid character %c in CB or SB depopulation list.\n",
						*tmp);
			}
			++tmp;
		}
		if (i < len) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"CB or SB depopulation is too short. It should be (length) symbols for CBs and (length+1) symbols for SBs.\n");
		}
	}

	else {
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
				"'%s' is not a valid type for specifying cb and sb depopulation.\n",
				tmp);
	}
}

static void ProcessSwitches(pugi::xml_node Parent,
		struct s_arch_switch_inf **Switches, int *NumSwitches,
		const bool timing_enabled, const pugiutil::loc_data& loc_data) {
	int i, j;
	const char *type_name;
	const char *switch_name;
	const char *buf_size;

	ReqOpt has_buf_size;
	pugi::xml_node Node;
	has_buf_size = OPTIONAL;

	/* Count the children and check they are switches */
	*NumSwitches = count_children(Parent, "switch", loc_data);

	/* Alloc switch list */
	*Switches = NULL;
	if (*NumSwitches > 0) {
		(*Switches) = new s_arch_switch_inf[(*NumSwitches)];
	}

	/* Load the switches. */
	Node = get_first_child(Parent, "switch", loc_data);
	for (i = 0; i < *NumSwitches; ++i) {

		switch_name = get_attribute(Node, "name", loc_data).value();
		type_name = get_attribute(Node, "type", loc_data).value();

		/* Check for switch name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Switches)[j].name, switch_name)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
						"Two switches with the same name '%s' were found.\n",
						switch_name);
			}
		}
		(*Switches)[i].name = vtr::strdup(switch_name);

		/* Figure out the type of switch. */
		if (0 == strcmp(type_name, "mux")) {
			(*Switches)[i].buffered = true;
			has_buf_size = REQUIRED;
		}

		else if (0 == strcmp(type_name, "pass_trans")) {
			(*Switches)[i].buffered = false;
		}

		else if (0 == strcmp(type_name, "buffer")) {
			(*Switches)[i].buffered = true;
		}

		else {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Invalid switch type '%s'.\n", type_name);
		}
		
		ReqOpt TIMING_ENABLE_REQD = BoolToReqOpt(timing_enabled);
		(*Switches)[i].R = get_attribute(Node, "R", loc_data,TIMING_ENABLE_REQD).as_float(0);
		(*Switches)[i].Cin = get_attribute(Node, "Cin", loc_data, TIMING_ENABLE_REQD).as_float(0);
		(*Switches)[i].Cout = get_attribute(Node, "Cout", loc_data, TIMING_ENABLE_REQD).as_float(0);
		ProcessSwitchTdel(Node, timing_enabled, i, (*Switches), loc_data);
		(*Switches)[i].buf_size = get_attribute(Node, "buf_size", loc_data,
				has_buf_size).as_float(0);
		(*Switches)[i].mux_trans_size = get_attribute(Node, "mux_trans_size", loc_data, OPTIONAL).as_float(1);
				
		buf_size = get_attribute(Node, "power_buf_size", loc_data, OPTIONAL).as_string(NULL);
		if (buf_size == NULL) {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_AUTO;
		} else if (strcmp(buf_size, "auto") == 0) {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_AUTO;
		} else {
			(*Switches)[i].power_buffer_type = POWER_BUFFER_TYPE_ABSOLUTE_SIZE;
			(*Switches)[i].power_buffer_size = (float) atof(buf_size);
		}

		/* Get next switch element */
		Node = Node.next_sibling(Node.name());
		
	}
}

/* Processes the switch delay. Switch delay can be specified in two ways. 
   First way: switch delay is specified as a constant via the property Tdel in the switch node. 
   Second way: switch delay is specified as a function of the switch fan-in. In this 
               case, multiple nodes in the form

               <Tdel num_inputs="1" delay="3e-11"/>

               are specified as children of the switch node. In this case, Tdel
               is not included as a property of the switch node (first way). */
static void ProcessSwitchTdel(pugi::xml_node Node, const bool timing_enabled,
		const int switch_index, s_arch_switch_inf *Switches, const pugiutil::loc_data& loc_data){

	float Tdel_prop_value;
	int num_Tdel_children;

	/* check if switch node has the Tdel property */
	bool has_Tdel_prop = false;
	Tdel_prop_value = get_attribute(Node, "Tdel", loc_data, OPTIONAL).as_float(UNDEFINED);
	if (Tdel_prop_value != UNDEFINED){
		has_Tdel_prop = true;
	}

	/* check if switch node has Tdel children */
	bool has_Tdel_children = false;
	num_Tdel_children = count_children(Node, "Tdel", loc_data, OPTIONAL);
	if (num_Tdel_children != 0){
		has_Tdel_children = true;
	}

	/* delay should not be specified as a Tdel property AND a Tdel child */
	if (has_Tdel_prop && has_Tdel_children){
		archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
				"Switch delay should be specified as EITHER a Tdel property OR as a child of the switch node, not both");
	}

	/* get pointer to the switch's Tdel map, then read-in delay data into this map */
	std::map<int, double> *Tdel_map = &Switches[switch_index].Tdel_map;
	if (has_Tdel_prop){
		/* delay specified as a constant */
		if (Tdel_map->count(UNDEFINED)){
            VTR_ASSERT(false);
		} else {
			(*Tdel_map)[UNDEFINED] = Tdel_prop_value;
		}
	} else if (has_Tdel_children) {
		/* Delay specified as a function of switch fan-in. 
		   Go through each Tdel child, read-in num_inputs and the delay value.
		   Insert this info into the switch delay map */
		pugi::xml_node Tdel_child = get_first_child(Node, "Tdel", loc_data);
		for (int ichild = 0; ichild < num_Tdel_children; ichild++){

			int num_inputs = get_attribute(Tdel_child, "num_inputs", loc_data).as_int(0);
			float Tdel_value = get_attribute(Tdel_child, "delay", loc_data).as_float(0.);

			if (Tdel_map->count( num_inputs ) ){
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Tdel_child),
					"Tdel node specified num_inputs (%d) that has already been specified by another Tdel node", num_inputs);
			} else {
				(*Tdel_map)[num_inputs] = Tdel_value;
			}
			Tdel_child = Tdel_child.next_sibling(Tdel_child.name());

		}
	} else {
		/* No delay info specified for switch */
		if (timing_enabled){
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"Switch should contain intrinsic delay information if timing is enabled");
		} else {
			/* set a default value */
			(*Tdel_map)[UNDEFINED] = 0;
		}
	}
}

static void ProcessDirects(pugi::xml_node Parent, t_direct_inf **Directs,
		 int *NumDirects, const struct s_arch_switch_inf *Switches, const int NumSwitches,
		 const pugiutil::loc_data& loc_data) {
	int i, j;
	const char *direct_name;
	const char *from_pin_name;
	const char *to_pin_name;
	const char *switch_name;

	pugi::xml_node Node;

	/* Count the children and check they are direct connections */
	*NumDirects = count_children(Parent, "direct", loc_data);

	/* Alloc direct list */
	*Directs = NULL;
	if (*NumDirects > 0) {
		*Directs = (t_direct_inf *) vtr::malloc(
				*NumDirects * sizeof(t_direct_inf));
		memset(*Directs, 0, (*NumDirects * sizeof(t_direct_inf)));
	}

	/* Load the directs. */
	Node = get_first_child(Parent, "direct", loc_data);
	for (i = 0; i < *NumDirects; ++i) {
		
		direct_name = get_attribute(Node, "name", loc_data).value();
		/* Check for direct name collisions */
		for (j = 0; j < i; ++j) {
			if (0 == strcmp((*Directs)[j].name, direct_name)) {
				archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
						"Two directs with the same name '%s' were found.\n",
						direct_name);
			}
		}
		(*Directs)[i].name = vtr::strdup(direct_name);

		/* Figure out the source pin and sink pin name */
		from_pin_name = get_attribute(Node, "from_pin",  loc_data).value();
		to_pin_name = get_attribute(Node, "to_pin", loc_data).value();

		/* Check that to_pin and the from_pin are not the same */
		if (0 == strcmp(to_pin_name, from_pin_name)) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"The source pin and sink pin are the same: %s.\n",
					to_pin_name);
		}
		(*Directs)[i].from_pin = vtr::strdup(from_pin_name);
		(*Directs)[i].to_pin = vtr::strdup(to_pin_name);

		(*Directs)[i].x_offset = get_attribute(Node, "x_offset", loc_data).as_int(0);
		(*Directs)[i].y_offset = get_attribute(Node, "y_offset", loc_data).as_int(0);
		(*Directs)[i].z_offset = get_attribute(Node, "z_offset", loc_data).as_int(0);

        //Set the optional switch type
        switch_name = get_attribute(Node, "switch_name", loc_data, OPTIONAL).as_string(NULL);
        if(switch_name != NULL) {
            //Look-up the user defined switch
            for(j = 0; j < NumSwitches; j++) {
                if(0 == strcmp(switch_name, Switches[j].name)) {
                    break; //Found the switch
                }
            }
            if(j >= NumSwitches) {
                archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
                        "Could not find switch named '%s' in switch list.\n", switch_name);

            }
            (*Directs)[i].switch_type = j; //Save the correct switch index
        } else {
            //If not defined, use the delayless switch by default
            //TODO: find a better way of indicating this.  Ideally, we would
            //specify the delayless switch index here, but it does not appear
            //to be defined at this point.
            (*Directs)[i].switch_type = -1; 
        }

		/* Check that the direct chain connection is not zero in both direction */
		if ((*Directs)[i].x_offset == 0 && (*Directs)[i].y_offset == 0) {
			archfpga_throw(loc_data.filename_c_str(), loc_data.line(Node),
					"The x_offset and y_offset are both zero, this is a length 0 direct chain connection.\n");
		}

		(*Directs)[i].line = loc_data.line(Node);
		/* Should I check that the direct chain offset is not greater than the chip? How? */

		/* Get next direct element */
		Node = Node.next_sibling(Node.name());
	}
}

static void ProcessPower( pugi::xml_node parent,
		t_power_arch * power_arch,
		const pugiutil::loc_data& loc_data) {
	pugi::xml_node Cur;

	/* Get the local interconnect capacitances */
	power_arch->local_interc_factor = 0.5;
	Cur = get_single_child(parent, "local_interconnect", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->C_wire_local = get_attribute(Cur, "C_wire", loc_data, OPTIONAL).as_float(0.);
		power_arch->local_interc_factor = get_attribute(Cur, "factor", loc_data, OPTIONAL).as_float(0.5);

	}

	/* Get logical effort factor */
	power_arch->logical_effort_factor = 4.0;
	Cur = get_single_child(parent, "buffers", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->logical_effort_factor = get_attribute(Cur,
				"logical_effort_factor", loc_data).as_float(0);;
	}

	/* Get SRAM Size */
	power_arch->transistors_per_SRAM_bit = 6.0;
	Cur = get_single_child(parent, "sram", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->transistors_per_SRAM_bit = get_attribute(Cur,
				"transistors_per_bit", loc_data).as_float(0);
	}

	/* Get Mux transistor size */
	power_arch->mux_transistor_size = 1.0;
	Cur = get_single_child(parent, "mux_transistor_size", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->mux_transistor_size = get_attribute(Cur,
				"mux_transistor_size", loc_data).as_float(0);
	}

	/* Get FF size */
	power_arch->FF_size = 1.0;
	Cur = get_single_child(parent, "FF_size", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->FF_size = get_attribute(Cur, "FF_size", loc_data).as_float(0);
	}

	/* Get LUT transistor size */
	power_arch->LUT_transistor_size = 1.0;
	Cur = get_single_child(parent, "LUT_transistor_size", loc_data, OPTIONAL);
	if (Cur) {
		power_arch->LUT_transistor_size = get_attribute(Cur,
				"LUT_transistor_size", loc_data).as_float(0);
	}
}

/* Get the clock architcture */
static void ProcessClocks(pugi::xml_node Parent, t_clock_arch * clocks, const pugiutil::loc_data& loc_data) {
	pugi::xml_node Node;
	int i;
	const char *tmp;

	clocks->num_global_clocks = count_children(Parent, "clock", loc_data, OPTIONAL);

	/* Alloc the clockdetails */
	clocks->clock_inf = NULL;
	if (clocks->num_global_clocks > 0) {
		clocks->clock_inf = (t_clock_network *) vtr::malloc(
				clocks->num_global_clocks * sizeof(t_clock_network));
		memset(clocks->clock_inf, 0,
				clocks->num_global_clocks * sizeof(t_clock_network));
	}

	/* Load the clock info. */
	Node = get_first_child(Parent, "clock", loc_data);
	for (i = 0; i < clocks->num_global_clocks; ++i) {

		tmp = get_attribute(Node, "buffer_size", loc_data).value();
		if (strcmp(tmp, "auto") == 0) {
			clocks->clock_inf[i].autosize_buffer = true;
		} else {
			clocks->clock_inf[i].autosize_buffer = false;
			clocks->clock_inf[i].buffer_size = (float) atof(tmp);
		}

		clocks->clock_inf[i].C_wire = get_attribute(Node, "C_wire", loc_data).as_float(0);

		/* get the next clock item */
		Node = Node.next_sibling(Node.name());
	}
}


/* Used by functions outside read_xml_util.c to gain access to arch filename */
const char* get_arch_file_name() {
	return arch_file_name;
}

