#include <cstring>
#include <sstream>

#include "vtr_assert.h"
#include "vtr_memory.h"
#include "vtr_util.h"

#include "arch_types.h"
#include "arch_util.h"
#include "arch_error.h"

#include "read_xml_arch_file.h"
#include "read_xml_util.h"

struct t_pin_inst_port {
    int sub_tile_index;    // Sub Tile index
    int capacity_instance; // within capacity
    int port_index;        // Port index
    int pin_index_in_port; // Pin's index within the port
};

/******************** Subroutine declarations ********************************/

static void free_all_pb_graph_nodes(std::vector<t_logical_block_type>& type_descriptors);
static void free_pb_graph(t_pb_graph_node* pb_graph_node);
static void free_pb_type(t_pb_type* pb_type);

static std::tuple<int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_index);
static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_index);

/******************** End Subroutine declarations ****************************/

InstPort::InstPort(std::string str) {
    std::vector<std::string> inst_port = vtr::split(str, ".");

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
    auto open_bracket_pos = str.find("[");
    auto close_bracket_pos = str.find("]");
    auto colon_pos = str.find(":");

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

    for (int i = 0; i < arch->num_switches; ++i) {
        if (arch->Switches->name != nullptr) {
            vtr::free(arch->Switches[i].name);
        }
    }
    delete[] arch->Switches;
    arch->Switches = nullptr;

    free_arch_models(arch->models);

    for (int i = 0; i < arch->num_directs; ++i) {
        vtr::free(arch->Directs[i].name);
        vtr::free(arch->Directs[i].from_pin);
        vtr::free(arch->Directs[i].to_pin);
    }
    vtr::free(arch->Directs);

    vtr::free(arch->architecture_id);

    if (arch->model_library) {
        for (int i = 0; i < 4; ++i) {
            vtr::t_linked_vptr* vptr = arch->model_library[i].pb_types;
            while (vptr) {
                vtr::t_linked_vptr* vptr_prev = vptr;
                vptr = vptr->next;
                vtr::free(vptr_prev);
            }
        }

        vtr::free(arch->model_library[0].name);
        vtr::free(arch->model_library[0].outputs->name);
        delete[] arch->model_library[0].outputs;
        vtr::free(arch->model_library[1].inputs->name);
        delete[] arch->model_library[1].inputs;
        vtr::free(arch->model_library[1].name);
        vtr::free(arch->model_library[2].name);
        vtr::free(arch->model_library[2].inputs[0].name);
        vtr::free(arch->model_library[2].inputs[1].name);
        delete[] arch->model_library[2].inputs;
        vtr::free(arch->model_library[2].outputs->name);
        delete[] arch->model_library[2].outputs;
        vtr::free(arch->model_library[3].name);
        vtr::free(arch->model_library[3].inputs->name);
        delete[] arch->model_library[3].inputs;
        vtr::free(arch->model_library[3].outputs->name);
        delete[] arch->model_library[3].outputs;
        delete[] arch->model_library;
    }

    if (arch->clocks) {
        vtr::free(arch->clocks->clock_inf);
    }
}

//Frees all models in the linked list
void free_arch_models(t_model* models) {
    t_model* model = models;
    while (model) {
        model = free_arch_model(model);
    }
}

//Frees the specified model, and returns the next model (if any) in the linked list
t_model* free_arch_model(t_model* model) {
    if (!model) return nullptr;

    t_model* next_model = model->next;

    free_arch_model_ports(model->inputs);
    free_arch_model_ports(model->outputs);

    vtr::t_linked_vptr* vptr = model->pb_types;
    while (vptr) {
        vtr::t_linked_vptr* vptr_prev = vptr;
        vptr = vptr->next;
        vtr::free(vptr_prev);
    }

    if (model->instances)
        vtr::free(model->instances);
    vtr::free(model->name);
    delete model;

    return next_model;
}

//Frees all the model portss in a linked list
void free_arch_model_ports(t_model_ports* model_ports) {
    t_model_ports* model_port = model_ports;
    while (model_port) {
        model_port = free_arch_model_port(model_port);
    }
}

//Frees the specified model_port, and returns the next model_port (if any) in the linked list
t_model_ports* free_arch_model_port(t_model_ports* model_port) {
    if (!model_port) return nullptr;

    t_model_ports* next_port = model_port->next;

    vtr::free(model_port->name);
    delete model_port;

    return next_port;
}

void free_type_descriptors(std::vector<t_physical_tile_type>& type_descriptors) {
    for (auto& type : type_descriptors) {
        vtr::free(type.name);
        if (type.index == EMPTY_TYPE_INDEX) {
            continue;
        }

        for (auto& sub_tile : type.sub_tiles) {
            vtr::free(sub_tile.name);

            for (auto port : sub_tile.ports) {
                vtr::free(port.name);
            }
        }
    }
    type_descriptors.clear();
}

void free_type_descriptors(std::vector<t_logical_block_type>& type_descriptors) {
    free_all_pb_graph_nodes(type_descriptors);

    for (auto& type : type_descriptors) {
        vtr::free(type.name);
        if (type.index == EMPTY_TYPE_INDEX) {
            continue;
        }

        free_pb_type(type.pb_type);
        delete type.pb_type;
    }
    type_descriptors.clear();
}

static void free_all_pb_graph_nodes(std::vector<t_logical_block_type>& type_descriptors) {
    for (auto& type : type_descriptors) {
        if (type.pb_type) {
            if (type.pb_graph_head) {
                free_pb_graph(type.pb_graph_head);
                vtr::free(type.pb_graph_head);
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
            if (pb_graph_node->input_pins[i][j].pin_timing)
                vtr::free(pb_graph_node->input_pins[i][j].pin_timing);
            if (pb_graph_node->input_pins[i][j].pin_timing_del_max)
                vtr::free(pb_graph_node->input_pins[i][j].pin_timing_del_max);
            if (pb_graph_node->input_pins[i][j].pin_timing_del_min)
                vtr::free(pb_graph_node->input_pins[i][j].pin_timing_del_min);
            if (pb_graph_node->input_pins[i][j].input_edges)
                vtr::free(pb_graph_node->input_pins[i][j].input_edges);
            if (pb_graph_node->input_pins[i][j].output_edges)
                vtr::free(pb_graph_node->input_pins[i][j].output_edges);
            if (pb_graph_node->input_pins[i][j].parent_pin_class)
                vtr::free(pb_graph_node->input_pins[i][j].parent_pin_class);
        }
        delete[] pb_graph_node->input_pins[i];
    }
    for (i = 0; i < pb_graph_node->num_output_ports; i++) {
        for (j = 0; j < pb_graph_node->num_output_pins[i]; j++) {
            if (pb_graph_node->output_pins[i][j].pin_timing)
                vtr::free(pb_graph_node->output_pins[i][j].pin_timing);
            if (pb_graph_node->output_pins[i][j].pin_timing_del_max)
                vtr::free(pb_graph_node->output_pins[i][j].pin_timing_del_max);
            if (pb_graph_node->output_pins[i][j].pin_timing_del_min)
                vtr::free(pb_graph_node->output_pins[i][j].pin_timing_del_min);
            if (pb_graph_node->output_pins[i][j].input_edges)
                vtr::free(pb_graph_node->output_pins[i][j].input_edges);
            if (pb_graph_node->output_pins[i][j].output_edges)
                vtr::free(pb_graph_node->output_pins[i][j].output_edges);
            if (pb_graph_node->output_pins[i][j].parent_pin_class)
                vtr::free(pb_graph_node->output_pins[i][j].parent_pin_class);

            if (pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs) {
                for (k = 0; k < pb_graph_node->pb_type->depth; k++) {
                    if (pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k]) {
                        vtr::free(pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs[k]);
                    }
                }
                vtr::free(pb_graph_node->output_pins[i][j].list_of_connectable_input_pin_ptrs);
            }

            if (pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins)
                vtr::free(pb_graph_node->output_pins[i][j].num_connectable_primitive_input_pins);
        }
        delete[] pb_graph_node->output_pins[i];
    }
    for (i = 0; i < pb_graph_node->num_clock_ports; i++) {
        for (j = 0; j < pb_graph_node->num_clock_pins[i]; j++) {
            if (pb_graph_node->clock_pins[i][j].pin_timing)
                vtr::free(pb_graph_node->clock_pins[i][j].pin_timing);
            if (pb_graph_node->clock_pins[i][j].pin_timing_del_max)
                vtr::free(pb_graph_node->clock_pins[i][j].pin_timing_del_max);
            if (pb_graph_node->clock_pins[i][j].pin_timing_del_min)
                vtr::free(pb_graph_node->clock_pins[i][j].pin_timing_del_min);
            if (pb_graph_node->clock_pins[i][j].input_edges)
                vtr::free(pb_graph_node->clock_pins[i][j].input_edges);
            if (pb_graph_node->clock_pins[i][j].output_edges)
                vtr::free(pb_graph_node->clock_pins[i][j].output_edges);
            if (pb_graph_node->clock_pins[i][j].parent_pin_class)
                vtr::free(pb_graph_node->clock_pins[i][j].parent_pin_class);
        }
        delete[] pb_graph_node->clock_pins[i];
    }

    vtr::free(pb_graph_node->input_pins);
    vtr::free(pb_graph_node->output_pins);
    vtr::free(pb_graph_node->clock_pins);

    vtr::free(pb_graph_node->num_input_pins);
    vtr::free(pb_graph_node->num_output_pins);
    vtr::free(pb_graph_node->num_clock_pins);

    vtr::free(pb_graph_node->input_pin_class_size);
    vtr::free(pb_graph_node->output_pin_class_size);

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
                    vtr::free(pb_graph_node->interconnect_pins[i][j].input_pins[iport]);
                }
                for (int iport = 0; iport < interconn_power->num_output_ports; ++iport) {
                    vtr::free(pb_graph_node->interconnect_pins[i][j].output_pins[iport]);
                }
                vtr::free(pb_graph_node->interconnect_pins[i][j].input_pins);
                vtr::free(pb_graph_node->interconnect_pins[i][j].output_pins);
            }
            vtr::free(pb_graph_node->interconnect_pins[i]);
        }
    }
    vtr::free(pb_graph_node->interconnect_pins);
    vtr::free(pb_graph_node->pb_node_power);

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
    if (pb_type->blif_model)
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

            for (int k = 0; k < pb_type->modes[i].interconnect[j].num_annotations; ++k) {
                if (pb_type->modes[i].interconnect[j].annotations[k].clock)
                    vtr::free(pb_type->modes[i].interconnect[j].annotations[k].clock);
                if (pb_type->modes[i].interconnect[j].annotations[k].input_pins) {
                    vtr::free(pb_type->modes[i].interconnect[j].annotations[k].input_pins);
                }
                if (pb_type->modes[i].interconnect[j].annotations[k].output_pins) {
                    vtr::free(pb_type->modes[i].interconnect[j].annotations[k].output_pins);
                }
                for (int m = 0; m < pb_type->modes[i].interconnect[j].annotations[k].num_value_prop_pairs; ++m) {
                    vtr::free(pb_type->modes[i].interconnect[j].annotations[k].value[m]);
                }
                vtr::free(pb_type->modes[i].interconnect[j].annotations[k].prop);
                vtr::free(pb_type->modes[i].interconnect[j].annotations[k].value);
            }
            vtr::free(pb_type->modes[i].interconnect[j].annotations);
            if (pb_type->modes[i].interconnect[j].interconnect_power)
                vtr::free(pb_type->modes[i].interconnect[j].interconnect_power);
        }
        if (pb_type->modes[i].interconnect)
            delete[] pb_type->modes[i].interconnect;
        if (pb_type->modes[i].mode_power)
            vtr::free(pb_type->modes[i].mode_power);
    }
    if (pb_type->modes)
        delete[] pb_type->modes;

    for (int i = 0; i < pb_type->num_annotations; ++i) {
        for (int j = 0; j < pb_type->annotations[i].num_value_prop_pairs; ++j) {
            vtr::free(pb_type->annotations[i].value[j]);
        }
        vtr::free(pb_type->annotations[i].value);
        vtr::free(pb_type->annotations[i].prop);
        if (pb_type->annotations[i].input_pins) {
            vtr::free(pb_type->annotations[i].input_pins);
        }
        if (pb_type->annotations[i].output_pins) {
            vtr::free(pb_type->annotations[i].output_pins);
        }
        if (pb_type->annotations[i].clock) {
            vtr::free(pb_type->annotations[i].clock);
        }
    }
    if (pb_type->num_annotations > 0) {
        vtr::free(pb_type->annotations);
    }

    if (pb_type->pb_type_power) {
        vtr::free(pb_type->pb_type_power);
    }

    for (int i = 0; i < pb_type->num_ports; ++i) {
        vtr::free(pb_type->ports[i].name);
        if (pb_type->ports[i].port_class) {
            vtr::free(pb_type->ports[i].port_class);
        }
        if (pb_type->ports[i].port_power) {
            vtr::free(pb_type->ports[i].port_power);
        }
    }
    vtr::free(pb_type->ports);
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

t_physical_tile_type SetupEmptyPhysicalType() {
    t_physical_tile_type type;
    type.name = vtr::strdup("EMPTY");
    type.num_pins = 0;
    type.width = 1;
    type.height = 1;
    type.capacity = 0;
    type.num_drivers = 0;
    type.num_receivers = 0;
    type.area = UNDEFINED;
    type.switchblock_locations = vtr::Matrix<e_sb_type>({{size_t(type.width), size_t(type.height)}}, e_sb_type::FULL);
    type.switchblock_switch_overrides = vtr::Matrix<int>({{size_t(type.width), size_t(type.height)}}, DEFAULT_SWITCH);
    type.is_input_type = false;
    type.is_output_type = false;

    return type;
}

t_logical_block_type SetupEmptyLogicalType() {
    t_logical_block_type type;
    type.name = vtr::strdup("EMPTY");
    type.pb_type = nullptr;

    return type;
}

std::unordered_set<t_logical_block_type_ptr> get_equivalent_sites_set(t_physical_tile_type_ptr type) {
    std::unordered_set<t_logical_block_type_ptr> equivalent_sites;

    for (auto& sub_tile : type->sub_tiles) {
        for (auto& logical_block : sub_tile.equivalent_sites) {
            equivalent_sites.insert(logical_block);
        }
    }

    return equivalent_sites;
}

void alloc_and_load_default_child_for_pb_type(t_pb_type* pb_type,
                                              char* new_name,
                                              t_pb_type* copy) {
    int i, j;
    char* dot;

    VTR_ASSERT(pb_type->blif_model != nullptr);

    copy->name = vtr::strdup(new_name);
    copy->blif_model = vtr::strdup(pb_type->blif_model);
    copy->class_type = pb_type->class_type;
    copy->depth = pb_type->depth;
    copy->model = pb_type->model;
    copy->modes = nullptr;
    copy->num_modes = 0;
    copy->num_clock_pins = pb_type->num_clock_pins;
    copy->num_input_pins = pb_type->num_input_pins;
    copy->num_output_pins = pb_type->num_output_pins;
    copy->num_pb = 1;

    /* Power */
    copy->pb_type_power = (t_pb_type_power*)vtr::calloc(1,
                                                        sizeof(t_pb_type_power));
    copy->pb_type_power->estimation_method = power_method_inherited(pb_type->pb_type_power->estimation_method);

    /* Ports */
    copy->num_ports = pb_type->num_ports;
    copy->ports = (t_port*)vtr::calloc(pb_type->num_ports, sizeof(t_port));
    for (i = 0; i < pb_type->num_ports; i++) {
        copy->ports[i].is_clock = pb_type->ports[i].is_clock;
        copy->ports[i].model_port = pb_type->ports[i].model_port;
        copy->ports[i].type = pb_type->ports[i].type;
        copy->ports[i].num_pins = pb_type->ports[i].num_pins;
        copy->ports[i].parent_pb_type = copy;
        copy->ports[i].name = vtr::strdup(pb_type->ports[i].name);
        copy->ports[i].port_class = vtr::strdup(pb_type->ports[i].port_class);
        copy->ports[i].port_index_by_type = pb_type->ports[i].port_index_by_type;

        copy->ports[i].port_power = (t_port_power*)vtr::calloc(1,
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

    copy->annotations = (t_pin_to_pin_annotation*)vtr::calloc(pb_type->num_annotations, sizeof(t_pin_to_pin_annotation));
    copy->num_annotations = pb_type->num_annotations;
    for (i = 0; i < copy->num_annotations; i++) {
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
        copy->annotations[i].num_value_prop_pairs = pb_type->annotations[i].num_value_prop_pairs;
        copy->annotations[i].prop = (int*)vtr::malloc(sizeof(int) * pb_type->annotations[i].num_value_prop_pairs);
        copy->annotations[i].value = (char**)vtr::malloc(sizeof(char*) * pb_type->annotations[i].num_value_prop_pairs);
        for (j = 0; j < pb_type->annotations[i].num_value_prop_pairs; j++) {
            copy->annotations[i].prop[j] = pb_type->annotations[i].prop[j];
            copy->annotations[i].value[j] = vtr::strdup(pb_type->annotations[i].value[j]);
        }
    }
}

/* populate special lut class */
void ProcessLutClass(t_pb_type* lut_pb_type) {
    char* default_name;
    t_port* in_port;
    t_port* out_port;
    int i, j;

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
    lut_pb_type->modes[0].mode_power = (t_mode_power*)vtr::calloc(1,
                                                                  sizeof(t_mode_power));

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
    lut_pb_type->modes[0].interconnect[0].interconnect_power = (t_interconnect_power*)vtr::calloc(1, sizeof(t_interconnect_power));

    lut_pb_type->modes[0].interconnect[0].annotations = (t_pin_to_pin_annotation*)vtr::calloc(lut_pb_type->num_annotations,
                                                                                              sizeof(t_pin_to_pin_annotation));
    lut_pb_type->modes[0].interconnect[0].num_annotations = lut_pb_type->num_annotations;
    for (i = 0; i < lut_pb_type->modes[0].interconnect[0].num_annotations;
         i++) {
        lut_pb_type->modes[0].interconnect[0].annotations[i].clock = vtr::strdup(lut_pb_type->annotations[i].clock);
        lut_pb_type->modes[0].interconnect[0].annotations[i].input_pins = vtr::strdup(lut_pb_type->annotations[i].input_pins);
        lut_pb_type->modes[0].interconnect[0].annotations[i].output_pins = vtr::strdup(lut_pb_type->annotations[i].output_pins);
        lut_pb_type->modes[0].interconnect[0].annotations[i].line_num = lut_pb_type->annotations[i].line_num;
        lut_pb_type->modes[0].interconnect[0].annotations[i].format = lut_pb_type->annotations[i].format;
        lut_pb_type->modes[0].interconnect[0].annotations[i].type = lut_pb_type->annotations[i].type;
        lut_pb_type->modes[0].interconnect[0].annotations[i].num_value_prop_pairs = lut_pb_type->annotations[i].num_value_prop_pairs;
        lut_pb_type->modes[0].interconnect[0].annotations[i].prop = (int*)vtr::malloc(sizeof(int)
                                                                                      * lut_pb_type->annotations[i].num_value_prop_pairs);
        lut_pb_type->modes[0].interconnect[0].annotations[i].value = (char**)vtr::malloc(sizeof(char*)
                                                                                         * lut_pb_type->annotations[i].num_value_prop_pairs);
        for (j = 0; j < lut_pb_type->annotations[i].num_value_prop_pairs; j++) {
            lut_pb_type->modes[0].interconnect[0].annotations[i].prop[j] = lut_pb_type->annotations[i].prop[j];
            lut_pb_type->modes[0].interconnect[0].annotations[i].value[j] = vtr::strdup(lut_pb_type->annotations[i].value[j]);
        }
    }

    /* Second mode, LUT */

    lut_pb_type->modes[1].name = vtr::strdup(lut_pb_type->name);
    lut_pb_type->modes[1].parent_pb_type = lut_pb_type;
    lut_pb_type->modes[1].index = 1;
    lut_pb_type->modes[1].num_pb_type_children = 1;
    lut_pb_type->modes[1].mode_power = (t_mode_power*)vtr::calloc(1,
                                                                  sizeof(t_mode_power));
    lut_pb_type->modes[1].pb_type_children = new t_pb_type[1];
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
    lut_pb_type->annotations = nullptr;
    lut_pb_type->modes[1].pb_type_children[0].depth = lut_pb_type->depth + 1;
    lut_pb_type->modes[1].pb_type_children[0].parent_mode = &lut_pb_type->modes[1];
    for (i = 0; i < lut_pb_type->modes[1].pb_type_children[0].num_ports; i++) {
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
    lut_pb_type->modes[1].interconnect[0].interconnect_power = (t_interconnect_power*)vtr::calloc(1, sizeof(t_interconnect_power));

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
    lut_pb_type->modes[1].interconnect[1].interconnect_power = (t_interconnect_power*)vtr::calloc(1, sizeof(t_interconnect_power));

    free(default_name);

    free(lut_pb_type->blif_model);
    lut_pb_type->blif_model = nullptr;
    lut_pb_type->model = nullptr;
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
    mem_pb_type->modes[0].mode_power = (t_mode_power*)vtr::calloc(1,
                                                                  sizeof(t_mode_power));
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
    mem_pb_type->model = nullptr;

    mem_pb_type->modes[0].num_interconnect = mem_pb_type->num_ports * num_pb;
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
            mem_pb_type->modes[0].interconnect[i_inter].interconnect_power = (t_interconnect_power*)vtr::calloc(1,
                                                                                                                sizeof(t_interconnect_power));
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
                mem_pb_type->modes[0].interconnect[i_inter].interconnect_power = (t_interconnect_power*)vtr::calloc(1,
                                                                                                                    sizeof(t_interconnect_power));
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

void CreateModelLibrary(t_arch* arch) {
    t_model* model_library;

    model_library = new t_model[4];

    //INPAD
    model_library[0].name = vtr::strdup(MODEL_INPUT);
    model_library[0].index = 0;
    model_library[0].inputs = nullptr;
    model_library[0].instances = nullptr;
    model_library[0].next = &model_library[1];
    model_library[0].outputs = new t_model_ports[1];
    model_library[0].outputs->dir = OUT_PORT;
    model_library[0].outputs->name = vtr::strdup("inpad");
    model_library[0].outputs->next = nullptr;
    model_library[0].outputs->size = 1;
    model_library[0].outputs->min_size = 1;
    model_library[0].outputs->index = 0;
    model_library[0].outputs->is_clock = false;

    //OUTPAD
    model_library[1].name = vtr::strdup(MODEL_OUTPUT);
    model_library[1].index = 1;
    model_library[1].inputs = new t_model_ports[1];
    model_library[1].inputs->dir = IN_PORT;
    model_library[1].inputs->name = vtr::strdup("outpad");
    model_library[1].inputs->next = nullptr;
    model_library[1].inputs->size = 1;
    model_library[1].inputs->min_size = 1;
    model_library[1].inputs->index = 0;
    model_library[1].inputs->is_clock = false;
    model_library[1].instances = nullptr;
    model_library[1].next = &model_library[2];
    model_library[1].outputs = nullptr;

    //LATCH
    model_library[2].name = vtr::strdup(MODEL_LATCH);
    model_library[2].index = 2;
    model_library[2].inputs = new t_model_ports[2];

    model_library[2].inputs[0].dir = IN_PORT;
    model_library[2].inputs[0].name = vtr::strdup("D");
    model_library[2].inputs[0].next = &model_library[2].inputs[1];
    model_library[2].inputs[0].size = 1;
    model_library[2].inputs[0].min_size = 1;
    model_library[2].inputs[0].index = 0;
    model_library[2].inputs[0].is_clock = false;
    model_library[2].inputs[0].clock = "clk";

    model_library[2].inputs[1].dir = IN_PORT;
    model_library[2].inputs[1].name = vtr::strdup("clk");
    model_library[2].inputs[1].next = nullptr;
    model_library[2].inputs[1].size = 1;
    model_library[2].inputs[1].min_size = 1;
    model_library[2].inputs[1].index = 0;
    model_library[2].inputs[1].is_clock = true;

    model_library[2].instances = nullptr;
    model_library[2].next = &model_library[3];

    model_library[2].outputs = new t_model_ports[1];
    model_library[2].outputs[0].dir = OUT_PORT;
    model_library[2].outputs[0].name = vtr::strdup("Q");
    model_library[2].outputs[0].next = nullptr;
    model_library[2].outputs[0].size = 1;
    model_library[2].outputs[0].min_size = 1;
    model_library[2].outputs[0].index = 0;
    model_library[2].outputs[0].is_clock = false;
    model_library[2].outputs[0].clock = "clk";

    //NAMES
    model_library[3].name = vtr::strdup(MODEL_NAMES);
    model_library[3].index = 3;

    model_library[3].inputs = new t_model_ports[1];
    model_library[3].inputs[0].dir = IN_PORT;
    model_library[3].inputs[0].name = vtr::strdup("in");
    model_library[3].inputs[0].next = nullptr;
    model_library[3].inputs[0].size = 1;
    model_library[3].inputs[0].min_size = 1;
    model_library[3].inputs[0].index = 0;
    model_library[3].inputs[0].is_clock = false;
    model_library[3].inputs[0].combinational_sink_ports = {"out"};

    model_library[3].instances = nullptr;
    model_library[3].next = nullptr;

    model_library[3].outputs = new t_model_ports[1];
    model_library[3].outputs[0].dir = OUT_PORT;
    model_library[3].outputs[0].name = vtr::strdup("out");
    model_library[3].outputs[0].next = nullptr;
    model_library[3].outputs[0].size = 1;
    model_library[3].outputs[0].min_size = 1;
    model_library[3].outputs[0].index = 0;
    model_library[3].outputs[0].is_clock = false;

    arch->model_library = model_library;
}

void SyncModelsPbTypes(t_arch* arch,
                       const std::vector<t_logical_block_type>& Types) {
    for (auto& Type : Types) {
        if (Type.pb_type != nullptr) {
            SyncModelsPbTypes_rec(arch, Type.pb_type);
        }
    }
}

void SyncModelsPbTypes_rec(t_arch* arch,
                           t_pb_type* pb_type) {
    int i, j, p;
    t_model *model_match_prim, *cur_model;
    t_model_ports* model_port;
    vtr::t_linked_vptr* old;
    char* blif_model_name = nullptr;

    bool found;

    if (pb_type->blif_model != nullptr) {
        /* get actual name of subckt */
        blif_model_name = pb_type->blif_model;
        if (strstr(blif_model_name, ".subckt ") == blif_model_name) {
            blif_model_name = strchr(blif_model_name, ' ');
            ++blif_model_name; //Advance past space
        }
        if (!blif_model_name) {
            archfpga_throw(get_arch_file_name(), 0,
                           "Unknown blif model %s in pb_type %s\n",
                           pb_type->blif_model, pb_type->name);
        }

        /* There are two sets of models to consider, the standard library of models and the user defined models */
        if (is_library_model(blif_model_name)) {
            cur_model = arch->model_library;
        } else {
            cur_model = arch->models;
        }

        /* Determine the logical model to use */
        found = false;
        model_match_prim = nullptr;
        while (cur_model && !found) {
            /* blif model always starts with .subckt so need to skip first 8 characters */
            if (strcmp(blif_model_name, cur_model->name) == 0) {
                found = true;
                model_match_prim = cur_model;
            }
            cur_model = cur_model->next;
        }
        if (found != true) {
            archfpga_throw(get_arch_file_name(), 0,
                           "No matching model for pb_type %s\n", pb_type->blif_model);
        }

        pb_type->model = model_match_prim;
        old = model_match_prim->pb_types;
        model_match_prim->pb_types = (vtr::t_linked_vptr*)vtr::malloc(sizeof(vtr::t_linked_vptr));
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
                    if (pb_type->ports[p].type != model_port->dir) {
                        archfpga_throw(get_arch_file_name(), 0,
                                       "Direction for port '%s' on model does not match port direction in pb_type '%s'\n",
                                       pb_type->ports[p].name, pb_type->name);
                    }
                    found = true;
                }
                model_port = model_port->next;
            }
            if (found != true) {
                archfpga_throw(get_arch_file_name(), 0,
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

void UpdateAndCheckModels(t_arch* arch) {
    t_model* cur_model;
    t_model_ports* port;
    int i, j;
    cur_model = arch->models;
    while (cur_model) {
        if (cur_model->pb_types == nullptr) {
            archfpga_throw(get_arch_file_name(), 0,
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
void primitives_annotation_clock_match(t_pin_to_pin_annotation* annotation,
                                       t_pb_type* parent_pb_type) {
    int i_port;
    bool clock_valid = false; //Determine if annotation's clock is same as primtive's clock

    if (!parent_pb_type || !annotation) {
        archfpga_throw(__FILE__, __LINE__,
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
        archfpga_throw(get_arch_file_name(), annotation->line_num,
                       "Clock '%s' does not match any clock defined in pb_type '%s'.\n",
                       annotation->clock, parent_pb_type->name);
    }
}

const t_segment_inf* find_segment(const t_arch* arch, std::string name) {
    for (size_t i = 0; i < (arch->Segments).size(); ++i) {
        const t_segment_inf* seg = &arch->Segments[i];
        if (seg->name == name) {
            return seg;
        }
    }

    return nullptr;
}

bool segment_exists(const t_arch* arch, std::string name) {
    return find_segment(arch, name) != nullptr;
}

bool is_library_model(const char* model_name) {
    if (model_name == std::string(MODEL_NAMES)
        || model_name == std::string(MODEL_LATCH)
        || model_name == std::string(MODEL_INPUT)
        || model_name == std::string(MODEL_OUTPUT)) {
        return true;
    }
    return false;
}

bool is_library_model(const t_model* model) {
    return is_library_model(model->name);
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
        VTR_ASSERT(pb_type->num_modes == 0);
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

static std::tuple<int, int, int> get_pin_index_for_inst(t_physical_tile_type_ptr type, int pin_index) {
    VTR_ASSERT(pin_index < type->num_pins);

    int total_pin_counts = 0;
    int pin_offset = 0;
    for (auto& sub_tile : type->sub_tiles) {
        total_pin_counts += sub_tile.num_phy_pins;

        if (pin_index < total_pin_counts) {
            int pins_per_inst = sub_tile.num_phy_pins / sub_tile.capacity.total();
            int inst_num = (pin_index - pin_offset) / pins_per_inst;
            int inst_index = (pin_index - pin_offset) % pins_per_inst;

            return std::make_tuple(inst_index, inst_num, sub_tile.index);
        }

        pin_offset += sub_tile.num_phy_pins;
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Could not infer the correct pin instance index for %s (pin index: %d)", type->name, pin_index);
}

static t_pin_inst_port block_type_pin_index_to_pin_inst(t_physical_tile_type_ptr type, int pin_index) {
    int sub_tile_index, inst_num;
    std::tie<int, int, int>(pin_index, inst_num, sub_tile_index) = get_pin_index_for_inst(type, pin_index);

    t_pin_inst_port pin_inst_port;
    pin_inst_port.sub_tile_index = sub_tile_index;
    pin_inst_port.capacity_instance = inst_num;
    pin_inst_port.port_index = OPEN;
    pin_inst_port.pin_index_in_port = OPEN;

    for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
        if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
            pin_inst_port.port_index = port.index;
            pin_inst_port.pin_index_in_port = pin_index - port.absolute_first_pin_index;
            break;
        }
    }
    return pin_inst_port;
}

int get_sub_tile_physical_pin(int sub_tile_index,
                              t_physical_tile_type_ptr physical_tile,
                              t_logical_block_type_ptr logical_block,
                              int pin) {
    t_logical_pin logical_pin(pin);

    const auto& direct_map = physical_tile->tile_block_pin_directs_map.at(logical_block->index).at(sub_tile_index);
    auto result = direct_map.find(logical_pin);

    if (result == direct_map.end()) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile pin of the logical block pin %d."
                       "Physical Tile Type: %s, Logical Block Type: %s.\n",
                       pin, physical_tile->name, logical_block->name);
    }

    return result->second.pin;
}

int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block) {
    int sub_tile_index = OPEN;
    for (const auto& sub_tile : physical_tile->sub_tiles) {
        auto eq_sites = sub_tile.equivalent_sites;
        auto it = std::find(eq_sites.begin(), eq_sites.end(), logical_block);
        if (it != eq_sites.end()) {
            sub_tile_index = sub_tile.index;
        }
    }

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Found no instances of logical block type '%s' within physical tile type '%s'. ",
                       logical_block->name, physical_tile->name);
    }

    return sub_tile_index;
}

int get_physical_pin(t_physical_tile_type_ptr physical_tile,
                     t_logical_block_type_ptr logical_block,
                     int pin) {
    int sub_tile_index = get_logical_block_physical_sub_tile_index(physical_tile, logical_block);

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile type pin of the logical block type pin %d.",
                       pin);
    }

    int sub_tile_physical_pin = get_sub_tile_physical_pin(sub_tile_index, physical_tile, logical_block, pin);
    return physical_tile->sub_tiles[sub_tile_index].sub_tile_to_tile_pin_indices[sub_tile_physical_pin];
}

int get_logical_block_physical_sub_tile_index(t_physical_tile_type_ptr physical_tile,
                                              t_logical_block_type_ptr logical_block,
                                              int sub_tile_capacity) {
    int sub_tile_index = OPEN;
    for (const auto& sub_tile : physical_tile->sub_tiles) {
        auto eq_sites = sub_tile.equivalent_sites;
        auto it = std::find(eq_sites.begin(), eq_sites.end(), logical_block);
        if (it != eq_sites.end()
            && (sub_tile.capacity.is_in_range(sub_tile_capacity))) {
            sub_tile_index = sub_tile.index;
            break;
        }
    }

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Found no instances of logical block type '%s' within physical tile type '%s'. ",
                       logical_block->name, physical_tile->name);
    }

    return sub_tile_index;
}

/**
 * This function returns the most common physical tile type given a logical block
 */
t_physical_tile_type_ptr pick_physical_type(t_logical_block_type_ptr logical_block) {
    return logical_block->equivalent_tiles[0];
}

t_logical_block_type_ptr pick_logical_type(t_physical_tile_type_ptr physical_tile) {
    return physical_tile->sub_tiles[0].equivalent_sites[0];
}

bool is_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block) {
    const auto& equivalent_tiles = logical_block->equivalent_tiles;
    return std::find(equivalent_tiles.begin(), equivalent_tiles.end(), physical_tile) != equivalent_tiles.end();
}

bool is_sub_tile_compatible(t_physical_tile_type_ptr physical_tile, t_logical_block_type_ptr logical_block, int sub_tile_loc) {
    bool capacity_compatible = false;
    for (auto& sub_tile : physical_tile->sub_tiles) {
        auto result = std::find(sub_tile.equivalent_sites.begin(), sub_tile.equivalent_sites.end(), logical_block);

        if (sub_tile.capacity.is_in_range(sub_tile_loc) && result != sub_tile.equivalent_sites.end()) {
            capacity_compatible = true;
            break;
        }
    }

    return capacity_compatible && is_tile_compatible(physical_tile, logical_block);
}

int get_physical_pin_at_sub_tile_location(t_physical_tile_type_ptr physical_tile,
                                          t_logical_block_type_ptr logical_block,
                                          int sub_tile_capacity,
                                          int pin) {
    int sub_tile_index = get_logical_block_physical_sub_tile_index(physical_tile, logical_block, sub_tile_capacity);

    if (sub_tile_index == OPEN) {
        archfpga_throw(__FILE__, __LINE__,
                       "Couldn't find the corresponding physical tile type pin of the logical block type pin %d.",
                       pin);
    }

    int sub_tile_physical_pin = get_sub_tile_physical_pin(sub_tile_index, physical_tile, logical_block, pin);

    /* Find the relative capacity of the logical_block in this sub tile */
    int relative_capacity = sub_tile_capacity - physical_tile->sub_tiles[sub_tile_index].capacity.low;

    /* Find the number of pins per block in the equivalent site list
     * of the sub tile. Otherwise, the current logical block may have smaller/larger number of pins
     * than other logical blocks that can be placed in the sub-tile. This will lead to an error
     * when computing the pin index!
     */
    int block_num_pins = physical_tile->sub_tiles[sub_tile_index].num_phy_pins / physical_tile->sub_tiles[sub_tile_index].capacity.total();

    return relative_capacity * block_num_pins
           + physical_tile->sub_tiles[sub_tile_index].sub_tile_to_tile_pin_indices[sub_tile_physical_pin];
}

int get_max_num_pins(t_logical_block_type_ptr logical_block) {
    int max_num_pins = 0;

    for (auto physical_tile : logical_block->equivalent_tiles) {
        max_num_pins = std::max(max_num_pins, physical_tile->num_pins);
    }

    return max_num_pins;
}

//Returns the pin class associated with the specified pin_index_in_port within the port port_name on type
int find_pin_class(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port, e_pin_type pin_type) {
    int iclass = OPEN;

    int ipin = find_pin(type, port_name, pin_index_in_port);

    if (ipin != OPEN) {
        iclass = type->pin_class[ipin];

        if (iclass != OPEN) {
            VTR_ASSERT(type->class_inf[iclass].type == pin_type);
        }
    }
    return iclass;
}

int find_pin(t_physical_tile_type_ptr type, std::string port_name, int pin_index_in_port) {
    int ipin = OPEN;
    int port_base_ipin = 0;
    int num_pins = OPEN;

    bool port_found = false;
    for (const auto& sub_tile : type->sub_tiles) {
        for (const auto& port : sub_tile.ports) {
            if (0 == strcmp(port.name, port_name.c_str())) {
                port_found = true;
                num_pins = port.num_pins;
                break;
            }

            port_base_ipin += port.num_pins;
        }

        if (port_found) {
            break;
        }

        port_base_ipin = 0;
    }

    if (num_pins != OPEN) {
        VTR_ASSERT(pin_index_in_port < num_pins);

        ipin = port_base_ipin + pin_index_in_port;
    }

    return ipin;
}

std::pair<int, int> get_capacity_location_from_physical_pin(t_physical_tile_type_ptr physical_tile, int pin) {
    int pins_to_remove = 0;
    for (auto sub_tile : physical_tile->sub_tiles) {
        auto capacity = sub_tile.capacity;
        int sub_tile_num_pins = sub_tile.num_phy_pins;
        int sub_tile_pin = pin - pins_to_remove;

        if (sub_tile_pin < sub_tile_num_pins) {
            int rel_capacity = sub_tile_pin / (sub_tile_num_pins / capacity.total());
            int rel_pin = sub_tile_pin % (sub_tile_num_pins / capacity.total());

            return std::pair<int, int>(rel_capacity + capacity.low, rel_pin);
        }

        pins_to_remove += sub_tile_num_pins;
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Couldn't find sub tile that contains the pin %d in physical tile %s.\n",
                   pin, physical_tile->name);
}

int get_physical_pin_from_capacity_location(t_physical_tile_type_ptr physical_tile, int relative_pin, int capacity_location) {
    int pins_to_add = 0;
    for (auto sub_tile : physical_tile->sub_tiles) {
        auto capacity = sub_tile.capacity;
        int rel_capacity = capacity_location - capacity.low;
        int num_inst_pins = sub_tile.num_phy_pins / capacity.total();

        if (capacity.is_in_range(capacity_location)) {
            return pins_to_add + num_inst_pins * rel_capacity + relative_pin;
        }

        pins_to_add += sub_tile.num_phy_pins;
    }

    archfpga_throw(__FILE__, __LINE__,
                   "Couldn't find sub tile that contains the relative pin %d at the capacity location %d in physical tile %s.\n",
                   relative_pin, capacity_location, physical_tile->name);
}
bool is_opin(int ipin, t_physical_tile_type_ptr type) {
    /* Returns true if this clb pin is an output, false otherwise. */

    if (ipin > type->num_pins) {
        //Not a top level pin
        return false;
    }

    int iclass = type->pin_class[ipin];

    if (type->class_inf[iclass].type == DRIVER)
        return true;
    else
        return false;
}

// TODO: Remove is_input_type / is_output_type / is_io_type as part of
// https://github.com/verilog-to-routing/vtr-verilog-to-routing/issues/1193
bool is_input_type(t_physical_tile_type_ptr type) {
    return type->is_input_type;
}

bool is_output_type(t_physical_tile_type_ptr type) {
    return type->is_output_type;
}

bool is_io_type(t_physical_tile_type_ptr type) {
    return is_input_type(type)
           || is_output_type(type);
}

std::string block_type_pin_index_to_name(t_physical_tile_type_ptr type, int pin_index) {
    VTR_ASSERT(pin_index < type->num_pins);

    std::string pin_name = type->name;

    int sub_tile_index, inst_num;
    std::tie<int, int, int>(pin_index, inst_num, sub_tile_index) = get_pin_index_for_inst(type, pin_index);

    if (type->sub_tiles[sub_tile_index].capacity.total() > 1) {
        pin_name += "[" + std::to_string(inst_num) + "]";
    }

    pin_name += ".";

    for (auto const& port : type->sub_tiles[sub_tile_index].ports) {
        if (pin_index >= port.absolute_first_pin_index && pin_index < port.absolute_first_pin_index + port.num_pins) {
            //This port contains the desired pin index
            int index_in_port = pin_index - port.absolute_first_pin_index;
            pin_name += port.name;
            pin_name += "[" + std::to_string(index_in_port) + "]";
            return pin_name;
        }
    }

    return "<UNKOWN>";
}

std::vector<std::string> block_type_class_index_to_pin_names(t_physical_tile_type_ptr type, int class_index) {
    VTR_ASSERT(class_index < (int)type->class_inf.size());

    auto class_inf = type->class_inf[class_index];

    std::vector<t_pin_inst_port> pin_info;
    for (int ipin = 0; ipin < class_inf.num_pins; ++ipin) {
        int pin_index = class_inf.pinlist[ipin];
        pin_info.push_back(block_type_pin_index_to_pin_inst(type, pin_index));
    }

    auto cmp = [](const t_pin_inst_port& lhs, const t_pin_inst_port& rhs) {
        return std::tie(lhs.capacity_instance, lhs.port_index, lhs.pin_index_in_port)
               < std::tie(rhs.capacity_instance, rhs.port_index, rhs.pin_index_in_port);
    };

    //Ensure all the pins are in order
    std::sort(pin_info.begin(), pin_info.end(), cmp);

    //Determine ranges for each capacity instance and port pair
    std::map<std::tuple<int, int, int>, std::pair<int, int>> pin_ranges;
    for (const auto& pin_inf : pin_info) {
        auto key = std::make_tuple(pin_inf.sub_tile_index, pin_inf.capacity_instance, pin_inf.port_index);
        if (!pin_ranges.count(key)) {
            pin_ranges[key].first = pin_inf.pin_index_in_port;
            pin_ranges[key].second = pin_inf.pin_index_in_port;
        } else {
            VTR_ASSERT(pin_ranges[key].second == pin_inf.pin_index_in_port - 1);
            pin_ranges[key].second = pin_inf.pin_index_in_port;
        }
    }

    //Format pin ranges
    std::vector<std::string> pin_names;
    for (auto kv : pin_ranges) {
        auto type_port = kv.first;
        auto pins = kv.second;

        int isub_tile, icapacity, iport;
        std::tie<int, int, int>(isub_tile, icapacity, iport) = type_port;

        int ipin_start = pins.first;
        int ipin_end = pins.second;

        auto& sub_tile = type->sub_tiles[isub_tile];

        std::string pin_name;
        if (ipin_start == ipin_end) {
            pin_name = vtr::string_fmt("%s[%d].%s[%d]",
                                       type->name,
                                       icapacity,
                                       sub_tile.ports[iport].name,
                                       ipin_start);
        } else {
            pin_name = vtr::string_fmt("%s[%d].%s[%d:%d]",
                                       type->name,
                                       icapacity,
                                       sub_tile.ports[iport].name,
                                       ipin_start,
                                       ipin_end);
        }

        pin_names.push_back(pin_name);
    }

    return pin_names;
}
