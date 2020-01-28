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

static void free_all_pb_graph_nodes(std::vector<t_logical_block_type>& type_descriptors);
static void free_pb_graph(t_pb_graph_node* pb_graph_node);
static void free_pb_type(t_pb_type* pb_type);

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

InstPort::name_index InstPort::parse_name_index(std::string str) {
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
    t_model* model = arch->models;
    while (model) {
        t_model_ports* input_port = model->inputs;
        while (input_port) {
            t_model_ports* prev_port = input_port;
            input_port = input_port->next;
            vtr::free(prev_port->name);
            delete prev_port;
        }
        t_model_ports* output_port = model->outputs;
        while (output_port) {
            t_model_ports* prev_port = output_port;
            output_port = output_port->next;
            vtr::free(prev_port->name);
            delete prev_port;
        }
        vtr::t_linked_vptr* vptr = model->pb_types;
        while (vptr) {
            vtr::t_linked_vptr* vptr_prev = vptr;
            vptr = vptr->next;
            vtr::free(vptr_prev);
        }
        t_model* prev_model = model;

        model = model->next;
        if (prev_model->instances)
            vtr::free(prev_model->instances);
        vtr::free(prev_model->name);
        delete prev_model;
    }

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

void free_type_descriptors(std::vector<t_physical_tile_type>& type_descriptors) {
    for (auto& type : type_descriptors) {
        vtr::free(type.name);
        if (type.index == EMPTY_TYPE_INDEX) {
            continue;
        }

        for (int width = 0; width < type.width; ++width) {
            for (int height = 0; height < type.height; ++height) {
                for (int side = 0; side < 4; ++side) {
                    for (int pin = 0; pin < type.num_pin_loc_assignments[width][height][side]; ++pin) {
                        if (type.pin_loc_assignments[width][height][side][pin])
                            vtr::free(type.pin_loc_assignments[width][height][side][pin]);
                    }
                    vtr::free(type.pinloc[width][height][side]);
                    vtr::free(type.pin_loc_assignments[width][height][side]);
                }
                vtr::free(type.pinloc[width][height]);
                vtr::free(type.pin_loc_assignments[width][height]);
                vtr::free(type.num_pin_loc_assignments[width][height]);
            }
            vtr::free(type.pinloc[width]);
            vtr::free(type.pin_loc_assignments[width]);
            vtr::free(type.num_pin_loc_assignments[width]);
        }
        vtr::free(type.pinloc);
        vtr::free(type.pin_loc_assignments);
        vtr::free(type.num_pin_loc_assignments);

        for (int j = 0; j < type.num_class; ++j) {
            vtr::free(type.class_inf[j].pinlist);
        }
        vtr::free(type.class_inf);
        vtr::free(type.is_ignored_pin);
        vtr::free(type.is_pin_global);
        vtr::free(type.pin_class);

        for (auto port : type.ports) {
            vtr::free(port.name);
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
    type.pinloc = nullptr;
    type.num_class = 0;
    type.class_inf = nullptr;
    type.pin_class = nullptr;
    type.is_ignored_pin = nullptr;
    type.area = UNDEFINED;
    type.switchblock_locations = vtr::Matrix<e_sb_type>({{size_t(type.width), size_t(type.height)}}, e_sb_type::FULL);
    type.switchblock_switch_overrides = vtr::Matrix<int>({{size_t(type.width), size_t(type.height)}}, DEFAULT_SWITCH);

    return type;
}

t_logical_block_type SetupEmptyLogicalType() {
    t_logical_block_type type;
    type.name = vtr::strdup("EMPTY");
    type.pb_type = nullptr;

    return type;
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
