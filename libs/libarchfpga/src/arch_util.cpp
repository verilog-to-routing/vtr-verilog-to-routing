#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "logic_types.h"
#include "vtr_assert.h"
#include "vtr_list.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"

#include "read_xml_arch_file.h"

/******************** Subroutine declarations ********************************/

static void free_all_pb_graph_nodes(std::vector<t_logical_block_type>& type_descriptors);
static void free_pb_graph(t_pb_graph_node* pb_graph_node);
static void free_pb_type(t_pb_type* pb_type);

/******************** End Subroutine declarations ****************************/

/* This gives access to the architecture file name to
 * all architecture-parser functions       */
static const char* arch_file_name = nullptr;

void set_arch_file_name(const char* arch) {
    arch_file_name = arch;
}

/* Used by functions outside read_xml_util.c to gain access to arch filename */
const char* get_arch_file_name() {
    VTR_ASSERT(arch_file_name != nullptr);

    return arch_file_name;
}

InstPort::InstPort(const std::string& str) {
    std::vector<std::string> inst_port = vtr::StringToken(str).split(".");

    if (inst_port.size() == 1) {
        instance_ = name_index();
        port_ = parse_name_index(inst_port[0]);

    } else if (inst_port.size() == 2) {
        instance_ = parse_name_index(inst_port[0]);
        port_ = parse_name_index(inst_port[1]);
    } else {
        std::string msg = vtr::string_fmt("Failed to parse instance port specification '%s'",
                                          str.c_str());
        throw ArchFpgaError(msg);
    }
}

InstPort::name_index InstPort::parse_name_index(const std::string& str) {
    auto open_bracket_pos = str.find('[');
    auto close_bracket_pos = str.find(']');
    auto colon_pos = str.find(':');

    //Parse checks
    if (open_bracket_pos == std::string::npos && close_bracket_pos != std::string::npos) {
        //Close brace only
        std::string msg = "near '" + str + "', missing '['";
        throw ArchFpgaError(msg);
    }

    if (open_bracket_pos != std::string::npos && close_bracket_pos == std::string::npos) {
        //Open brace only
        std::string msg = "near '" + str + "', missing ']'";
        throw ArchFpgaError(msg);
    }

    if (open_bracket_pos != std::string::npos && close_bracket_pos != std::string::npos) {
        //Have open and close braces, close must be after open
        if (open_bracket_pos > close_bracket_pos) {
            std::string msg = "near '" + str + "', '[' after ']'";
            throw ArchFpgaError(msg);
        }
    }

    if (colon_pos != std::string::npos) {
        //Have a colon, it must be between open/close braces
        if (colon_pos > close_bracket_pos || colon_pos < open_bracket_pos) {
            std::string msg = "near '" + str + "', found ':' but not between '[' and ']'";
            throw ArchFpgaError(msg);
        }
    }

    //Extract the name and index info
    std::string name = str.substr(0, open_bracket_pos);
    std::string first_idx_str;
    std::string second_idx_str;

    if (colon_pos == std::string::npos && open_bracket_pos == std::string::npos && close_bracket_pos == std::string::npos) {
    } else if (colon_pos == std::string::npos) {
        //No colon, implies a single element
        first_idx_str = str.substr(open_bracket_pos + 1, close_bracket_pos);
        second_idx_str = first_idx_str;
    } else {
        //Colon, implies a range
        first_idx_str = str.substr(open_bracket_pos + 1, colon_pos);
        second_idx_str = str.substr(colon_pos + 1, close_bracket_pos);
    }

    int first_idx = UNSPECIFIED;
    if (!first_idx_str.empty()) {
        std::stringstream ss(first_idx_str);
        size_t idx;
        ss >> idx;
        if (!ss.good()) {
            std::string msg = "near '" + str + "', expected positive integer";
            throw ArchFpgaError(msg);
        }
        first_idx = idx;
    }

    int second_idx = UNSPECIFIED;
    if (!second_idx_str.empty()) {
        std::stringstream ss(second_idx_str);
        size_t idx;
        ss >> idx;
        if (!ss.good()) {
            std::string msg = "near '" + str + "', expected positive integer";
            throw ArchFpgaError(msg);
        }
        second_idx = idx;
    }

    name_index value;
    value.name = name;
    value.low_idx = std::min(first_idx, second_idx);
    value.high_idx = std::max(first_idx, second_idx);
    return value;
}

int InstPort::num_instances() const {
    if (instance_high_index() == UNSPECIFIED || instance_low_index() == UNSPECIFIED) {
        throw ArchFpgaError("Unspecified instance indicies");
    }
    return instance_high_index() - instance_low_index() + 1;
}

int InstPort::num_pins() const {
    if (port_high_index() == UNSPECIFIED || port_low_index() == UNSPECIFIED) {
        throw ArchFpgaError("Unspecified port indicies");
    }
    return port_high_index() - port_low_index() + 1;
}

void free_arch(t_arch* arch) {
    if (arch == nullptr) {
        return;
    }

    arch->models.clear_models();

    vtr::release_memory(arch->switches);

    vtr::release_memory(arch->directs);

    vtr::free(arch->architecture_id);

    delete (arch->noc);
}

void free_type_descriptors(std::vector<t_physical_tile_type>& type_descriptors) {
    for (t_physical_tile_type& type : type_descriptors) {
        vtr::release_memory(type.name);

        if (type.index == EMPTY_TYPE_INDEX) {
            continue;
        }

        for (t_sub_tile& sub_tile : type.sub_tiles) {
            vtr::release_memory(sub_tile.name);

            for (t_physical_tile_port& port : sub_tile.ports) {
                vtr::free(port.name);
            }
        }
    }
    type_descriptors.clear();
}

void free_type_descriptors(std::vector<t_logical_block_type>& type_descriptors) {
    free_all_pb_graph_nodes(type_descriptors);

    for (t_logical_block_type& type : type_descriptors) {
        vtr::release_memory(type.name);
        if (type.index == EMPTY_TYPE_INDEX) {
            continue;
        }

        free_pb_type(type.pb_type);
        delete type.pb_type;
    }
    type_descriptors.clear();
}

static void free_all_pb_graph_nodes(std::vector<t_logical_block_type>& type_descriptors) {
    for (t_logical_block_type& type : type_descriptors) {
        if (type.pb_type) {
            if (type.pb_graph_head) {
                free_pb_graph(type.pb_graph_head);
                delete type.pb_graph_head;
            }
        }
    }
}

static void free_pb_graph(t_pb_graph_node* pb_graph_node) {
    int i, j, k;
    const t_pb_type* pb_type;

    pb_type = pb_graph_node->pb_type;

    /*free all lists of connectable input pin pointer of pb_graph_node and it's children*/
    /*free_list_of_connectable_input_pin_ptrs (pb_graph_node);*/

    /* Free ports for pb graph node */
    for (i = 0; i < pb_graph_node->num_input_ports; i++) {
        for (j = 0; j < pb_graph_node->num_input_pins[i]; j++) {
            if (pb_graph_node->input_pins[i][j].parent_pin_class)
                delete[] pb_graph_node->input_pins[i][j].parent_pin_class;
        }
        delete[] pb_graph_node->input_pins[i];
    }
    for (i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            if (pb_graph_node->output_pins[i][j].parent_pin_class)
                delete[] pb_graph_node->output_pins[i][j].parent_pin_class;

            if (pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs) {
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    delete[] pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k];
                }
                delete[] pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs;
            }

            if (pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins)
                delete[] pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins;
        }
        delete[] pb_graph_node->output_pins[i];
    }
    for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            if (pb_graph_node->clock_pins[i][j].parent_pin_class)
                delete[] pb_graph_node->clock_pins[i][j].parent_pin_class;
        }
        delete[] pb_graph_node->clock_pins[i];
    }

    delete[] pb_graph_node->input_pins;
    delete[] pb_graph_node->output_pins;
    delete[] pb_graph_node->clock_pins;

    delete[] pb_graph_node->num_input_pins;
    delete[] pb_graph_node->num_output_pins;
    delete[] pb_graph_node->num_clock_pins;

    delete[] pb_graph_node->input_pin_class_size;
    delete[] pb_graph_node->output_pin_class_size;

    if (pb_graph_node->interconnect_pins) {
        for (i = 0; i < pb_graph_node->pb_type->num_modes; i++) {
            if (pb_graph_node->interconnect_pins[i] == nullptr) continue;

            t_mode* mode = &pb_graph_node->pb_type->modes[i];

            for (j = 0; j < mode->num_interconnect; ++j) {
                //The interconnect_pins data structures are only initialized for power analysis and
                //are bizarrely baroque...
                t_interconnect* interconn = pb_graph_node->interconnect_pins[i][j].interconnect;
                VTR_ASSERT(interconn == &mode->interconnect[j]);

                t_interconnect_power* interconn_power = interconn->interconnect_power;
                for (int iport = 0; iport < interconn_power->num_input_ports; ++iport) {
                    delete[] pb_graph_node->interconnect_pins[i][j].input_pins[iport];
                }
                for (int iport = 0; iport < interconn_power->num_output_ports; ++iport) {
                    delete[] pb_graph_node->interconnect_pins[i][j].output_pins[iport];
                }
                delete[] pb_graph_node->interconnect_pins[i][j].input_pins;
                delete[] pb_graph_node->interconnect_pins[i][j].output_pins;
            }
            delete[] pb_graph_node->interconnect_pins[i];
        }
    }
    delete[] pb_graph_node->interconnect_pins;
    delete pb_graph_node->pb_node_power;

    for (i = 0; i < pb_type->num_modes; i++) {
        for (j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
            for (k = 0; k < pb_type->modes[i].pb_type_children[j].num_pb; k++) {
                free_pb_graph(&pb_graph_node->child_pb_graph_nodes[i][j][k]);
            }
            vtr::free(pb_graph_node->child_pb_graph_nodes[i][j]);
        }
        vtr::free(pb_graph_node->child_pb_graph_nodes[i]);
    }
    vtr::free(pb_graph_node->child_pb_graph_nodes);
}

static void free_pb_type(t_pb_type* pb_type) {
    vtr::free(pb_type->name);
    vtr::free(pb_type->blif_model);

    for (int i = 0; i < pb_type->num_modes; ++i) {
        for (int j = 0; j < pb_type->modes[i].num_pb_type_children; ++j) {
            free_pb_type(&pb_type->modes[i].pb_type_children[j]);
        }
        delete[] pb_type->modes[i].pb_type_children;
        vtr::free(pb_type->modes[i].name);
        for (int j = 0; j < pb_type->modes[i].num_interconnect; ++j) {
            vtr::free(pb_type->modes[i].interconnect[j].input_string);
            vtr::free(pb_type->modes[i].interconnect[j].output_string);
            vtr::free(pb_type->modes[i].interconnect[j].name);

            for (t_pin_to_pin_annotation& annotation : pb_type->modes[i].interconnect[j].annotations) {
                vtr::free(annotation.clock);
                vtr::free(annotation.input_pins);
                vtr::free(annotation.output_pins);
            }
            pb_type->modes[i].interconnect[j].annotations.clear();
            delete pb_type->modes[i].interconnect[j].interconnect_power;
        }
        delete[] pb_type->modes[i].interconnect;
        delete (pb_type->modes[i].mode_power);
    }

    delete[] pb_type->modes;

    for (t_pin_to_pin_annotation& annotation : pb_type->annotations) {
        vtr::free(annotation.input_pins);
        vtr::free(annotation.output_pins);
        vtr::free(annotation.clock);
    }
    pb_type->annotations.clear();

    delete pb_type->pb_type_power;

    for (int i = 0; i < pb_type->num_ports; ++i) {
        vtr::free(pb_type->ports[i].name);
        vtr::free(pb_type->ports[i].port_class);
        delete pb_type->ports[i].port_power;
    }
    delete[] pb_type->ports;
}

t_port* findPortByName(const char* name, t_pb_type* pb_type, int* high_index, int* low_index) {
    t_port* port;
    int i;
    unsigned int high;
    unsigned int low;
    unsigned int bracket_pos;
    unsigned int colon_pos;

    bracket_pos = strcspn(name, "[");

    /* Find port by name */
    port = nullptr;
    for (i = 0; i < pb_type->num_ports; i++) {
        char* compare_to = pb_type->ports[i].name;

        if (strlen(compare_to) == bracket_pos
            && strncmp(name, compare_to, bracket_pos) == 0) {
            port = &pb_type->ports[i];
            break;
        }
    }
    if (i >= pb_type->num_ports) {
        return nullptr;
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

t_physical_tile_type get_empty_physical_type(const char* name /*= EMPTY_BLOCK_NAME*/) {
    t_physical_tile_type type;
    type.name = name;
    type.num_pins = 0;
    type.width = 1;
    type.height = 1;
    type.capacity = 0;
    type.num_drivers = 0;
    type.num_receivers = 0;
    type.area = ARCH_FPGA_UNDEFINED_VAL;
    type.switchblock_locations = vtr::Matrix<e_sb_type>({{size_t(type.width), size_t(type.height)}}, e_sb_type::FULL);
    type.switchblock_switch_overrides = vtr::Matrix<int>({{size_t(type.width), size_t(type.height)}}, DEFAULT_SWITCH);
    type.is_input_type = false;
    type.is_output_type = false;

    return type;
}

t_logical_block_type get_empty_logical_type(const char* name /*=EMPTY_BLOCK_NAME*/) {
    t_logical_block_type type;
    type.name = name;
    type.pb_type = nullptr;

    return type;
}

std::unordered_set<t_logical_block_type_ptr> get_equivalent_sites_set(t_physical_tile_type_ptr type) {
    std::unordered_set<t_logical_block_type_ptr> equivalent_sites;

    for (const t_sub_tile& sub_tile : type->sub_tiles) {
        for (t_logical_block_type_ptr logical_block : sub_tile.equivalent_sites) {
            equivalent_sites.insert(logical_block);
        }
    }

    return equivalent_sites;
}

void alloc_and_load_default_child_for_pb_type(t_pb_type* pb_type,
                                              char* new_name,
                                              t_pb_type* copy) {
    char* dot;

    VTR_ASSERT(pb_type->blif_model != nullptr);

    copy->name = vtr::strdup(new_name);
    copy->blif_model = vtr::strdup(pb_type->blif_model);
    copy->class_type = pb_type->class_type;
    copy->depth = pb_type->depth;
    copy->model_id = pb_type->model_id;
    copy->modes = nullptr;
    copy->num_modes = 0;
    copy->num_clock_pins = pb_type->num_clock_pins;
    copy->num_input_pins = pb_type->num_input_pins;
    copy->num_output_pins = pb_type->num_output_pins;
    copy->num_pins = pb_type->num_pins;
    copy->num_pb = 1;

    /* Power */
    copy->pb_type_power = new t_pb_type_power();
    copy->pb_type_power->estimation_method = power_method_inherited(pb_type->pb_type_power->estimation_method);

    /* Ports */
    copy->num_ports = pb_type->num_ports;
    copy->ports = new t_port[pb_type->num_ports]();
    for (int i = 0; i < pb_type->num_ports; i++) {
        copy->ports[i].is_clock = pb_type->ports[i].is_clock;
        copy->ports[i].model_port = pb_type->ports[i].model_port;
        copy->ports[i].type = pb_type->ports[i].type;
        copy->ports[i].num_pins = pb_type->ports[i].num_pins;
        copy->ports[i].parent_pb_type = copy;
        copy->ports[i].name = vtr::strdup(pb_type->ports[i].name);
        copy->ports[i].port_class = vtr::strdup(pb_type->ports[i].port_class);
        copy->ports[i].port_index_by_type = pb_type->ports[i].port_index_by_type;
        copy->ports[i].index = pb_type->ports[i].index;
        copy->ports[i].absolute_first_pin_index = pb_type->ports[i].absolute_first_pin_index;

        copy->ports[i].port_power = new t_port_power();
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

    size_t num_annotations = pb_type->annotations.size();
    copy->annotations.resize(num_annotations);
    for (size_t i = 0; i < num_annotations; i++) {
        copy->annotations[i].clock = vtr::strdup(pb_type->annotations[i].clock);
        dot = strstr(pb_type->annotations[i].input_pins, ".");
        copy->annotations[i].input_pins = (char*)vtr::malloc(sizeof(char) * (strlen(new_name) + strlen(dot) + 1));
        copy->annotations[i].input_pins[0] = '\0';
        strcat(copy->annotations[i].input_pins, new_name);
        strcat(copy->annotations[i].input_pins, dot);
        if (pb_type->annotations[i].output_pins != nullptr) {
            dot = strstr(pb_type->annotations[i].output_pins, ".");
            copy->annotations[i].output_pins = (char*)vtr::malloc(sizeof(char) * (strlen(new_name) + strlen(dot) + 1));
            copy->annotations[i].output_pins[0] = '\0';
            strcat(copy->annotations[i].output_pins, new_name);
            strcat(copy->annotations[i].output_pins, dot);
        } else {
            copy->annotations[i].output_pins = nullptr;
        }
        copy->annotations[i].line_num = pb_type->annotations[i].line_num;
        copy->annotations[i].format = pb_type->annotations[i].format;
        copy->annotations[i].type = pb_type->annotations[i].type;
        copy->annotations[i].annotation_entries = pb_type->annotations[i].annotation_entries;
    }
}

/* populate special lut class */
void ProcessLutClass(t_pb_type* lut_pb_type) {
    char* default_name;
    t_port* in_port;
    t_port* out_port;

    if (strcmp(lut_pb_type->name, "lut") != 0) {
        default_name = vtr::strdup("lut");
    } else {
        default_name = vtr::strdup("lut_child");
    }

    lut_pb_type->num_modes = 2;
    lut_pb_type->pb_type_power->leakage_default_mode = 1;
    lut_pb_type->modes = new t_mode[lut_pb_type->num_modes];

    /* First mode, route_through */
    lut_pb_type->modes[0].name = vtr::strdup("wire");
    lut_pb_type->modes[0].parent_pb_type = lut_pb_type;
    lut_pb_type->modes[0].index = 0;
    lut_pb_type->modes[0].num_pb_type_children = 0;
    lut_pb_type->modes[0].mode_power = new t_mode_power();

    /* Process interconnect */
    /* TODO: add timing annotations to route-through */
    VTR_ASSERT(lut_pb_type->num_ports == 2);
    if (strcmp(lut_pb_type->ports[0].port_class, "lut_in") == 0) {
        VTR_ASSERT(strcmp(lut_pb_type->ports[1].port_class, "lut_out") == 0);
        in_port = &lut_pb_type->ports[0];
        out_port = &lut_pb_type->ports[1];
    } else {
        VTR_ASSERT(strcmp(lut_pb_type->ports[0].port_class, "lut_out") == 0);
        VTR_ASSERT(strcmp(lut_pb_type->ports[1].port_class, "lut_in") == 0);
        out_port = &lut_pb_type->ports[0];
        in_port = &lut_pb_type->ports[1];
    }
    lut_pb_type->modes[0].num_interconnect = 1;
    lut_pb_type->modes[0].interconnect = new t_interconnect[1];
    lut_pb_type->modes[0].interconnect[0].name = (char*)vtr::calloc(strlen(lut_pb_type->name) + 10, sizeof(char));
    sprintf(lut_pb_type->modes[0].interconnect[0].name, "complete:%s",
            lut_pb_type->name);
    lut_pb_type->modes[0].interconnect[0].type = COMPLETE_INTERC;
    lut_pb_type->modes[0].interconnect[0].input_string = (char*)vtr::calloc(strlen(lut_pb_type->name) + strlen(in_port->name) + 2,
                                                                            sizeof(char));
    sprintf(lut_pb_type->modes[0].interconnect[0].input_string, "%s.%s",
            lut_pb_type->name, in_port->name);
    lut_pb_type->modes[0].interconnect[0].output_string = (char*)vtr::calloc(strlen(lut_pb_type->name) + strlen(out_port->name) + 2,
                                                                             sizeof(char));
    sprintf(lut_pb_type->modes[0].interconnect[0].output_string, "%s.%s",
            lut_pb_type->name, out_port->name);

    lut_pb_type->modes[0].interconnect[0].parent_mode_index = 0;
    lut_pb_type->modes[0].interconnect[0].parent_mode = &lut_pb_type->modes[0];
    lut_pb_type->modes[0].interconnect[0].interconnect_power = new t_interconnect_power();

    size_t num_annotations = lut_pb_type->annotations.size();
    lut_pb_type->modes[0].interconnect[0].annotations.resize(num_annotations);
    for (size_t i = 0; i < num_annotations; i++) {
        lut_pb_type->modes[0].interconnect[0].annotations[i].clock = vtr::strdup(lut_pb_type->annotations[i].clock);
        lut_pb_type->modes[0].interconnect[0].annotations[i].input_pins = vtr::strdup(lut_pb_type->annotations[i].input_pins);
        lut_pb_type->modes[0].interconnect[0].annotations[i].output_pins = vtr::strdup(lut_pb_type->annotations[i].output_pins);
        lut_pb_type->modes[0].interconnect[0].annotations[i].line_num = lut_pb_type->annotations[i].line_num;
        lut_pb_type->modes[0].interconnect[0].annotations[i].format = lut_pb_type->annotations[i].format;
        lut_pb_type->modes[0].interconnect[0].annotations[i].type = lut_pb_type->annotations[i].type;

        lut_pb_type->modes[0].interconnect[0].annotations[i].annotation_entries = lut_pb_type->annotations[i].annotation_entries;
    }

    /* Second mode, LUT */

    lut_pb_type->modes[1].name = vtr::strdup(lut_pb_type->name);
    lut_pb_type->modes[1].parent_pb_type = lut_pb_type;
    lut_pb_type->modes[1].index = 1;
    lut_pb_type->modes[1].num_pb_type_children = 1;
    lut_pb_type->modes[1].mode_power = new t_mode_power();
    lut_pb_type->modes[1].pb_type_children = new t_pb_type[1];
    alloc_and_load_default_child_for_pb_type(lut_pb_type, default_name,
                                             lut_pb_type->modes[1].pb_type_children);
    /* moved annotations to child so delete old annotations */
    for (size_t i = 0; i < num_annotations; i++) {
        vtr::free(lut_pb_type->annotations[i].input_pins);
        vtr::free(lut_pb_type->annotations[i].output_pins);
        vtr::free(lut_pb_type->annotations[i].clock);
    }
    lut_pb_type->annotations.clear();
    lut_pb_type->modes[1].pb_type_children[0].depth = lut_pb_type->depth + 1;
    lut_pb_type->modes[1].pb_type_children[0].parent_mode = &lut_pb_type->modes[1];
    for (int i = 0; i < lut_pb_type->modes[1].pb_type_children[0].num_ports; i++) {
        if (lut_pb_type->modes[1].pb_type_children[0].ports[i].type == IN_PORT) {
            lut_pb_type->modes[1].pb_type_children[0].ports[i].equivalent = PortEquivalence::FULL;
        }
    }

    /* Process interconnect */
    lut_pb_type->modes[1].num_interconnect = 2;
    lut_pb_type->modes[1].interconnect = new t_interconnect[lut_pb_type->modes[1].num_interconnect];
    lut_pb_type->modes[1].interconnect[0].name = (char*)vtr::calloc(strlen(lut_pb_type->name) + 10, sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[0].name, "direct:%s",
            lut_pb_type->name);
    lut_pb_type->modes[1].interconnect[0].type = DIRECT_INTERC;
    lut_pb_type->modes[1].interconnect[0].input_string = (char*)vtr::calloc(strlen(lut_pb_type->name) + strlen(in_port->name) + 2,
                                                                            sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[0].input_string, "%s.%s",
            lut_pb_type->name, in_port->name);
    lut_pb_type->modes[1].interconnect[0].output_string = (char*)vtr::calloc(strlen(default_name) + strlen(in_port->name) + 2, sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[0].output_string, "%s.%s",
            default_name, in_port->name);
    lut_pb_type->modes[1].interconnect[0].infer_annotations = true;

    lut_pb_type->modes[1].interconnect[0].parent_mode_index = 1;
    lut_pb_type->modes[1].interconnect[0].parent_mode = &lut_pb_type->modes[1];
    lut_pb_type->modes[1].interconnect[0].interconnect_power = new t_interconnect_power();

    lut_pb_type->modes[1].interconnect[1].name = (char*)vtr::calloc(strlen(lut_pb_type->name) + 11, sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[1].name, "direct:%s",
            lut_pb_type->name);

    lut_pb_type->modes[1].interconnect[1].type = DIRECT_INTERC;
    lut_pb_type->modes[1].interconnect[1].input_string = (char*)vtr::calloc(strlen(default_name) + strlen(out_port->name) + 4, sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[1].input_string, "%s.%s",
            default_name, out_port->name);
    lut_pb_type->modes[1].interconnect[1].output_string = (char*)vtr::calloc(strlen(lut_pb_type->name) + strlen(out_port->name)
                                                                                 + strlen(in_port->name) + 2,
                                                                             sizeof(char));
    sprintf(lut_pb_type->modes[1].interconnect[1].output_string, "%s.%s",
            lut_pb_type->name, out_port->name);
    lut_pb_type->modes[1].interconnect[1].infer_annotations = true;

    lut_pb_type->modes[1].interconnect[1].parent_mode_index = 1;
    lut_pb_type->modes[1].interconnect[1].parent_mode = &lut_pb_type->modes[1];
    lut_pb_type->modes[1].interconnect[1].interconnect_power = new t_interconnect_power();

    free(default_name);

    free(lut_pb_type->blif_model);
    lut_pb_type->blif_model = nullptr;
    lut_pb_type->model_id = LogicalModelId::INVALID();
}

/* populate special memory class */
void ProcessMemoryClass(t_pb_type* mem_pb_type) {
    char* default_name;
    char *input_name, *input_port_name, *output_name, *output_port_name;
    int i, j, i_inter, num_pb;

    if (strcmp(mem_pb_type->name, "memory_slice") != 0) {
        default_name = vtr::strdup("memory_slice");
    } else {
        default_name = vtr::strdup("memory_slice_1bit");
    }

    mem_pb_type->modes = new t_mode[1];
    mem_pb_type->modes[0].name = vtr::strdup(default_name);
    mem_pb_type->modes[0].parent_pb_type = mem_pb_type;
    mem_pb_type->modes[0].index = 0;
    mem_pb_type->modes[0].mode_power = new t_mode_power();
    num_pb = OPEN;
    for (i = 0; i < mem_pb_type->num_ports; i++) {
        if (mem_pb_type->ports[i].port_class != nullptr
            && strstr(mem_pb_type->ports[i].port_class, "data")
                   == mem_pb_type->ports[i].port_class) {
            if (num_pb == OPEN) {
                num_pb = mem_pb_type->ports[i].num_pins;
            } else if (num_pb != mem_pb_type->ports[i].num_pins) {
                archfpga_throw(get_arch_file_name(), 0,
                               "memory %s has inconsistent number of data bits %d and %d\n",
                               mem_pb_type->name, num_pb,
                               mem_pb_type->ports[i].num_pins);
            }
        }
    }

    mem_pb_type->modes[0].num_pb_type_children = 1;
    mem_pb_type->modes[0].pb_type_children = new t_pb_type[1];
    alloc_and_load_default_child_for_pb_type(mem_pb_type, default_name,
                                             &mem_pb_type->modes[0].pb_type_children[0]);
    mem_pb_type->modes[0].pb_type_children[0].depth = mem_pb_type->depth + 1;
    mem_pb_type->modes[0].pb_type_children[0].parent_mode = &mem_pb_type->modes[0];
    mem_pb_type->modes[0].pb_type_children[0].num_pb = num_pb;

    mem_pb_type->num_modes = 1;

    free(mem_pb_type->blif_model);
    mem_pb_type->blif_model = nullptr;
    mem_pb_type->model_id = LogicalModelId::INVALID();

    mem_pb_type->modes[0].num_interconnect = mem_pb_type->num_ports * num_pb;

    std::string error_msg = (std::stringstream() << "Memory pb_type " << mem_pb_type->name << " has no interconnect").str();
    VTR_ASSERT_MSG(mem_pb_type->modes[0].num_interconnect > 0, error_msg.c_str());
    mem_pb_type->modes[0].interconnect = new t_interconnect[mem_pb_type->modes[0].num_interconnect];

    for (i = 0; i < mem_pb_type->modes[0].num_interconnect; i++) {
        mem_pb_type->modes[0].interconnect[i].parent_mode_index = 0;
        mem_pb_type->modes[0].interconnect[i].parent_mode = &mem_pb_type->modes[0];
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

        if (mem_pb_type->ports[i].port_class != nullptr
            && strstr(mem_pb_type->ports[i].port_class, "data")
                   == mem_pb_type->ports[i].port_class) {
            mem_pb_type->modes[0].interconnect[i_inter].name = (char*)vtr::calloc(i_inter / 10 + 8, sizeof(char));
            sprintf(mem_pb_type->modes[0].interconnect[i_inter].name,
                    "direct%d", i_inter);
            mem_pb_type->modes[0].interconnect[i_inter].infer_annotations = true;

            if (mem_pb_type->ports[i].type == IN_PORT) {
                /* force data pins to be one bit wide and update stats */
                mem_pb_type->modes[0].pb_type_children[0].ports[i].num_pins = 1;
                mem_pb_type->modes[0].pb_type_children[0].num_input_pins -= (mem_pb_type->ports[i].num_pins - 1);

                mem_pb_type->modes[0].interconnect[i_inter].input_string = (char*)vtr::calloc(strlen(input_name) + strlen(input_port_name)
                                                                                                  + 2,
                                                                                              sizeof(char));
                sprintf(mem_pb_type->modes[0].interconnect[i_inter].input_string,
                        "%s.%s", input_name, input_port_name);
                mem_pb_type->modes[0].interconnect[i_inter].output_string = (char*)vtr::calloc(strlen(output_name) + strlen(output_port_name)
                                                                                                   + 2 * (6 + num_pb / 10),
                                                                                               sizeof(char));
                sprintf(mem_pb_type->modes[0].interconnect[i_inter].output_string,
                        "%s[%d:0].%s", output_name, num_pb - 1,
                        output_port_name);
            } else {
                /* force data pins to be one bit wide and update stats */
                mem_pb_type->modes[0].pb_type_children[0].ports[i].num_pins = 1;
                mem_pb_type->modes[0].pb_type_children[0].num_output_pins -= (mem_pb_type->ports[i].num_pins - 1);

                mem_pb_type->modes[0].interconnect[i_inter].input_string = (char*)vtr::calloc(strlen(input_name) + strlen(input_port_name)
                                                                                                  + 2 * (6 + num_pb / 10),
                                                                                              sizeof(char));
                sprintf(mem_pb_type->modes[0].interconnect[i_inter].input_string,
                        "%s[%d:0].%s", input_name, num_pb - 1, input_port_name);
                mem_pb_type->modes[0].interconnect[i_inter].output_string = (char*)vtr::calloc(strlen(output_name) + strlen(output_port_name)
                                                                                                   + 2,
                                                                                               sizeof(char));
                sprintf(mem_pb_type->modes[0].interconnect[i_inter].output_string,
                        "%s.%s", output_name, output_port_name);
            }

            /* Allocate interconnect power structures */
            mem_pb_type->modes[0].interconnect[i_inter].interconnect_power = new t_interconnect_power();
            i_inter++;
        } else {
            for (j = 0; j < num_pb; j++) {
                /* Anything that is not data must be an input */
                mem_pb_type->modes[0].interconnect[i_inter].name = (char*)vtr::calloc(i_inter / 10 + j / 10 + 10,
                                                                                      sizeof(char));
                sprintf(mem_pb_type->modes[0].interconnect[i_inter].name,
                        "direct%d_%d", i_inter, j);
                mem_pb_type->modes[0].interconnect[i_inter].infer_annotations = true;

                if (mem_pb_type->ports[i].type == IN_PORT) {
                    mem_pb_type->modes[0].interconnect[i_inter].type = DIRECT_INTERC;
                    mem_pb_type->modes[0].interconnect[i_inter].input_string = (char*)vtr::calloc(strlen(input_name) + strlen(input_port_name)
                                                                                                      + 2,
                                                                                                  sizeof(char));
                    sprintf(mem_pb_type->modes[0].interconnect[i_inter].input_string,
                            "%s.%s", input_name, input_port_name);
                    mem_pb_type->modes[0].interconnect[i_inter].output_string = (char*)vtr::calloc(strlen(output_name)
                                                                                                       + strlen(output_port_name)
                                                                                                       + 2 * (6 + num_pb / 10),
                                                                                                   sizeof(char));
                    sprintf(mem_pb_type->modes[0].interconnect[i_inter].output_string,
                            "%s[%d:%d].%s", output_name, j, j,
                            output_port_name);
                } else {
                    mem_pb_type->modes[0].interconnect[i_inter].type = DIRECT_INTERC;
                    mem_pb_type->modes[0].interconnect[i_inter].input_string = (char*)vtr::calloc(strlen(input_name) + strlen(input_port_name)
                                                                                                      + 2 * (6 + num_pb / 10),
                                                                                                  sizeof(char));
                    sprintf(mem_pb_type->modes[0].interconnect[i_inter].input_string,
                            "%s[%d:%d].%s", input_name, j, j, input_port_name);
                    mem_pb_type->modes[0].interconnect[i_inter].output_string = (char*)vtr::calloc(strlen(output_name)
                                                                                                       + strlen(output_port_name) + 2,
                                                                                                   sizeof(char));
                    sprintf(mem_pb_type->modes[0].interconnect[i_inter].output_string,
                            "%s.%s", output_name, output_port_name);
                }

                /* Allocate interconnect power structures */
                mem_pb_type->modes[0].interconnect[i_inter].interconnect_power = new t_interconnect_power();
                i_inter++;
            }
        }
    }

    mem_pb_type->modes[0].num_interconnect = i_inter;

    free(default_name);
}

e_power_estimation_method power_method_inherited(e_power_estimation_method parent_power_method) {
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
            VTR_ASSERT(0);
            return POWER_METHOD_UNDEFINED; // Should never get here, but avoids a compiler warning.
    }
}

void SyncModelsPbTypes(t_arch* arch,
                       const std::vector<t_logical_block_type>& Types) {
    for (const t_logical_block_type& Type : Types) {
        if (Type.pb_type != nullptr) {
            SyncModelsPbTypes_rec(arch, Type.pb_type);
        }
    }
}

void SyncModelsPbTypes_rec(t_arch* arch,
                           t_pb_type* pb_type) {

    if (pb_type->blif_model != nullptr) {
        /* get actual name of subckt */
        char* blif_model_name = pb_type->blif_model;
        if (strstr(blif_model_name, ".subckt ") == blif_model_name) {
            blif_model_name = strchr(blif_model_name, ' ');
            ++blif_model_name; //Advance past space
        }
        if (!blif_model_name) {
            archfpga_throw(get_arch_file_name(), 0,
                           "Unknown blif model %s in pb_type %s\n",
                           pb_type->blif_model, pb_type->name);
        }

        /* Determine the logical model to use */
        LogicalModelId model_match_prim_id = arch->models.get_model_by_name(blif_model_name);
        if (!model_match_prim_id.is_valid()) {
            archfpga_throw(get_arch_file_name(), 0,
                           "No matching model for pb_type %s\n", pb_type->blif_model);
        }
        t_model& model_match_prim = arch->models.get_model(model_match_prim_id);

        pb_type->model_id = model_match_prim_id;
        vtr::t_linked_vptr* old = model_match_prim.pb_types;
        model_match_prim.pb_types = new vtr::t_linked_vptr;
        model_match_prim.pb_types->next = old;
        model_match_prim.pb_types->data_vptr = pb_type;

        for (int p = 0; p < pb_type->num_ports; p++) {
            bool found = false;
            /* TODO: Parse error checking - check if INPUT matches INPUT and OUTPUT matches OUTPUT (not yet done) */
            t_model_ports* model_port = model_match_prim.inputs;
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
                    if (pb_type->ports[p].type != model_port->dir) {
                        archfpga_throw(get_arch_file_name(), 0,
                                       "Direction for port '%s' on model does not match port direction in pb_type '%s'\n",
                                       pb_type->ports[p].name, pb_type->name);
                    }
                    if (pb_type->ports[p].is_clock != model_port->is_clock) {
                        archfpga_throw(get_arch_file_name(), 0,
                                       "Port '%s' on model does not match is_clock in pb_type '%s'\n",
                                       pb_type->ports[p].name, pb_type->name);
                    }
                    found = true;
                }
                model_port = model_port->next;
            }
            model_port = model_match_prim.outputs;
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
                    if (pb_type->ports[p].type != model_port->dir) {
                        archfpga_throw(get_arch_file_name(), 0,
                                       "Direction for port '%s' on model does not match port direction in pb_type '%s'\n",
                                       pb_type->ports[p].name, pb_type->name);
                    }
                    found = true;
                }
                model_port = model_port->next;
            }
            if (!found) {
                archfpga_throw(get_arch_file_name(), 0,
                               "No matching model port for port %s in pb_type %s\n",
                               pb_type->ports[p].name, pb_type->name);
            }
        }
    } else {
        for (int i = 0; i < pb_type->num_modes; i++) {
            for (int j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
                SyncModelsPbTypes_rec(arch, &(pb_type->modes[i].pb_type_children[j]));
            }
        }
    }
}

/* Date:July 10th, 2013
 * Author: Daniel Chen
 * Purpose: Attempts to match a clock_name specified in an
 *			timing annotation (Tsetup, Thold, Tc_to_q) with the
 *			clock_name specified in the primitive. Applies
 *			to flipflop/memory right now.
 */
void primitives_annotation_clock_match(t_pin_to_pin_annotation* annotation,
                                       t_pb_type* parent_pb_type) {
    int i_port;
    bool clock_valid = false; //Determine if annotation's clock is same as primitive's clock

    if (!parent_pb_type || !annotation) {
        archfpga_throw(__FILE__, __LINE__,
                       "Annotation_clock check encounters invalid annotation or primitive.\n");
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
        archfpga_throw(get_arch_file_name(), annotation->line_num,
                       "Clock '%s' does not match any clock defined in pb_type '%s'.\n",
                       annotation->clock, parent_pb_type->name);
    }
}

const t_segment_inf* find_segment(const t_arch* arch, std::string_view name) {
    for (const t_segment_inf& segment : arch->Segments) {
        if (segment.name == name) {
            return &segment;
        }
    }

    return nullptr;
}

bool segment_exists(const t_arch* arch, std::string_view name) {
    return find_segment(arch, name) != nullptr;
}

//Returns true if the specified block type contains the specified blif model name
//
// TODO: Remove block_type_contains_blif_model / pb_type_contains_blif_model
// as part of
// https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1193
bool block_type_contains_blif_model(t_logical_block_type_ptr type, const std::string& blif_model_name) {
    return pb_type_contains_blif_model(type->pb_type, blif_model_name);
}

//Returns true of a pb_type (or it's children) contain the specified blif model name
bool pb_type_contains_blif_model(const t_pb_type* pb_type, const std::string& blif_model_name) {
    if (!pb_type) {
        return false;
    }

    if (pb_type->blif_model != nullptr) {
        //Leaf pb_type
        VTR_ASSERT(pb_type->is_primitive());
        if (blif_model_name == pb_type->blif_model
            || ".subckt " + blif_model_name == pb_type->blif_model) {
            return true;
        } else {
            return false;
        }
    } else {
        for (int imode = 0; imode < pb_type->num_modes; ++imode) {
            const t_mode* mode = &pb_type->modes[imode];

            for (int ichild = 0; ichild < mode->num_pb_type_children; ++ichild) {
                const t_pb_type* pb_type_child = &mode->pb_type_children[ichild];
                if (pb_type_contains_blif_model(pb_type_child, blif_model_name)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool has_sequential_annotation(const t_pb_type* pb_type, const t_model_ports* port, enum e_pin_to_pin_delay_annotations annot_type) {
    VTR_ASSERT(annot_type == E_ANNOT_PIN_TO_PIN_DELAY_TSETUP
               || annot_type == E_ANNOT_PIN_TO_PIN_DELAY_THOLD
               || annot_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MAX
               || annot_type == E_ANNOT_PIN_TO_PIN_DELAY_CLOCK_TO_Q_MIN);

    for (const t_pin_to_pin_annotation& annotation : pb_type->annotations) {
        InstPort annot_in(annotation.input_pins);
        if (annot_in.port_name() == port->name) {
            for (const auto& [key, _] : annotation.annotation_entries) {
                if (key == annot_type) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool has_combinational_annotation(const t_pb_type* pb_type, std::string_view in_port, std::string_view out_port) {
    for (const t_pin_to_pin_annotation& annotation : pb_type->annotations) {
        for (const std::string& annot_in_str : vtr::StringToken(annotation.input_pins).split(" \t\n")) {
            InstPort in_pins(annot_in_str);
            for (const std::string& annot_out_str : vtr::StringToken(annotation.output_pins).split(" \t\n")) {
                InstPort out_pins(annot_out_str);
                if (in_pins.port_name() == in_port && out_pins.port_name() == out_port) {
                    for (const auto& [key, _] : annotation.annotation_entries) {
                        if (key == E_ANNOT_PIN_TO_PIN_DELAY_MAX
                            || key == E_ANNOT_PIN_TO_PIN_DELAY_MIN) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

void link_physical_logical_types(std::vector<t_physical_tile_type>& PhysicalTileTypes,
                                 std::vector<t_logical_block_type>& LogicalBlockTypes) {
    for (t_physical_tile_type& physical_tile : PhysicalTileTypes) {
        if (physical_tile.index == EMPTY_TYPE_INDEX) continue;

        auto eq_sites_set = get_equivalent_sites_set(&physical_tile);
        auto equivalent_sites = std::vector<t_logical_block_type_ptr>(eq_sites_set.begin(), eq_sites_set.end());

        auto criteria = [&physical_tile](const t_logical_block_type* lhs, const t_logical_block_type* rhs) {
            int num_pins = physical_tile.num_inst_pins;

            int lhs_num_logical_pins = lhs->pb_type->num_pins;
            int rhs_num_logical_pins = rhs->pb_type->num_pins;

            int lhs_diff_num_pins = num_pins - lhs_num_logical_pins;
            int rhs_diff_num_pins = num_pins - rhs_num_logical_pins;

            return lhs_diff_num_pins < rhs_diff_num_pins;
        };

        std::sort(equivalent_sites.begin(), equivalent_sites.end(), criteria);

        for (t_logical_block_type& logical_block : LogicalBlockTypes) {
            for (t_logical_block_type_ptr site : equivalent_sites) {
                if (logical_block.name == site->pb_type->name) {
                    logical_block.equivalent_tiles.push_back(&physical_tile);
                    break;
                }
            }
        }
    }

    for (t_logical_block_type& logical_block : LogicalBlockTypes) {
        if (logical_block.index == EMPTY_TYPE_INDEX) continue;

        auto& equivalent_tiles = logical_block.equivalent_tiles;

        if ((int)equivalent_tiles.size() <= 0) {
            archfpga_throw(__FILE__, __LINE__,
                           "Logical Block %s does not have any equivalent tiles.\n", logical_block.name.c_str());
        }

        std::unordered_map<int, bool> ignored_pins_check_map;
        std::unordered_map<int, bool> global_pins_check_map;

        auto criteria = [&logical_block](const t_physical_tile_type* lhs, const t_physical_tile_type* rhs) {
            int num_logical_pins = logical_block.pb_type->num_pins;

            int lhs_num_pins = lhs->num_inst_pins;
            int rhs_num_pins = rhs->num_inst_pins;

            int lhs_diff_num_pins = lhs_num_pins - num_logical_pins;
            int rhs_diff_num_pins = rhs_num_pins - num_logical_pins;

            return lhs_diff_num_pins < rhs_diff_num_pins;
        };

        std::sort(equivalent_tiles.begin(), equivalent_tiles.end(), criteria);

        for (int pin = 0; pin < logical_block.pb_type->num_pins; pin++) {
            for (auto& tile : equivalent_tiles) {
                auto direct_maps = tile->tile_block_pin_directs_map.at(logical_block.index);

                for (auto& sub_tile : tile->sub_tiles) {
                    auto equiv_sites = sub_tile.equivalent_sites;
                    if (std::find(equiv_sites.begin(), equiv_sites.end(), &logical_block) == equiv_sites.end()) {
                        continue;
                    }

                    auto direct_map = direct_maps.at(sub_tile.index);

                    auto result = direct_map.find(t_logical_pin(pin));
                    if (result == direct_map.end()) {
                        archfpga_throw(__FILE__, __LINE__,
                                       "Logical pin %d not present in pin mapping between Tile %s and Block %s.\n",
                                       pin, tile->name.c_str(), logical_block.name.c_str());
                    }

                    int sub_tile_pin_index = result->second.pin;
                    int phy_index = sub_tile.sub_tile_to_tile_pin_indices[sub_tile_pin_index];

                    bool is_ignored = tile->is_ignored_pin[phy_index];
                    bool is_global = tile->is_pin_global[phy_index];

                    auto ignored_result = ignored_pins_check_map.insert(std::pair<int, bool>(pin, is_ignored));
                    if (!ignored_result.second && ignored_result.first->second != is_ignored) {
                        archfpga_throw(__FILE__, __LINE__,
                                       "Physical Tile %s has a different value for the ignored pin (physical pin: %d, logical pin: %d) "
                                       "different from the corresponding pins of the other equivalent site %s\n.",
                                       tile->name.c_str(), phy_index, pin, logical_block.name.c_str());
                    }

                    auto global_result = global_pins_check_map.insert(std::pair<int, bool>(pin, is_global));
                    if (!global_result.second && global_result.first->second != is_global) {
                        archfpga_throw(__FILE__, __LINE__,
                                       "Physical Tile %s has a different value for the global pin (physical pin: %d, logical pin: %d) "
                                       "different from the corresponding pins of the other equivalent sites\n.",
                                       tile->name.c_str(), phy_index, pin);
                    }
                }
            }
        }
    }
}

/* Sets up the pin classes for the type. */
void setup_pin_classes(t_physical_tile_type* type) {
    int num_class;

    for (int i = 0; i < type->num_pins; i++) {
        type->pin_class.push_back(OPEN);
        type->is_ignored_pin.push_back(true);
        type->is_pin_global.push_back(true);
    }

    int pin_count = 0;

    t_class_range class_range;

    /* Equivalent pins share the same class, non-equivalent pins belong to different pin classes */
    for (const t_sub_tile& sub_tile : type->sub_tiles) {
        int capacity = sub_tile.capacity.total();
        class_range.low = type->class_inf.size();
        class_range.high = class_range.low - 1;
        for (int i = 0; i < capacity; ++i) {
            for (const t_physical_tile_port& port : sub_tile.ports) {
                if (port.equivalent != PortEquivalence::NONE) {
                    t_class class_inf;
                    num_class = (int)type->class_inf.size();
                    class_inf.num_pins = port.num_pins;
                    class_inf.equivalence = port.equivalent;

                    if (port.type == IN_PORT) {
                        class_inf.type = RECEIVER;
                    } else {
                        VTR_ASSERT(port.type == OUT_PORT);
                        class_inf.type = DRIVER;
                    }

                    for (int k = 0; k < port.num_pins; ++k) {
                        class_inf.pinlist.push_back(pin_count);
                        type->pin_class[pin_count] = num_class;
                        // clock pins and other specified global ports are initially specified
                        // as ignored pins (i.e. connections are not created in the rr_graph and
                        // nets connected to the port are ignored as well).
                        type->is_ignored_pin[pin_count] = port.is_clock || port.is_non_clock_global;
                        // clock pins and other specified global ports are flaged as global
                        type->is_pin_global[pin_count] = port.is_clock || port.is_non_clock_global;

                        if (port.is_clock) {
                            type->clock_pin_indices.push_back(pin_count);
                        }

                        pin_count++;
                    }

                    type->class_inf.push_back(class_inf);
                    class_range.high++;
                } else if (port.equivalent == PortEquivalence::NONE) {
                    for (int k = 0; k < port.num_pins; ++k) {
                        t_class class_inf;
                        num_class = (int)type->class_inf.size();
                        class_inf.num_pins = 1;
                        class_inf.pinlist.push_back(pin_count);
                        class_inf.equivalence = port.equivalent;

                        if (port.type == IN_PORT) {
                            class_inf.type = RECEIVER;
                        } else {
                            VTR_ASSERT(port.type == OUT_PORT);
                            class_inf.type = DRIVER;
                        }

                        type->pin_class[pin_count] = num_class;
                        // clock pins and other specified global ports are initially specified
                        // as ignored pins (i.e. connections are not created in the rr_graph and
                        // nets connected to the port are ignored as well).
                        type->is_ignored_pin[pin_count] = port.is_clock || port.is_non_clock_global;
                        // clock pins and other specified global ports are flagged as global
                        type->is_pin_global[pin_count] = port.is_clock || port.is_non_clock_global;

                        if (port.is_clock) {
                            type->clock_pin_indices.push_back(pin_count);
                        }

                        pin_count++;

                        type->class_inf.push_back(class_inf);
                        class_range.high++;
                    }
                }
            }
        }

        type->sub_tiles[sub_tile.index].class_range = class_range;
    }

    VTR_ASSERT(pin_count == type->num_pins);
}
