#include <cstring>
#include <cstdlib>
#include <vector>

#include "echo_arch.h"
#include "arch_util.h"
#include "logic_types.h"
#include "physical_types.h"
#include "vtr_list.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_assert.h"

/// @brief indices to lookup IPIN connection block switch name
constexpr int ipin_cblock_switch_index_within_die = 0;
constexpr int ipin_cblock_switch_index_between_dice = 1;

void PrintArchInfo(FILE* Echo, const t_arch* arch);
static void print_model(FILE* echo, const t_model& model);
static void PrintPb_types_rec(FILE* Echo, const t_pb_type* pb_type, int level, const LogicalModels& models);
static void PrintPb_types_recPower(FILE* Echo,
                                   const t_pb_type* pb_type,
                                   const char* tabs);

/* Output the data from architecture data so user can verify it
 * was interpretted correctly. */
void EchoArch(const char* EchoFile,
              const std::vector<t_physical_tile_type>& PhysicalTileTypes,
              const std::vector<t_logical_block_type>& LogicalBlockTypes,
              const t_arch* arch) {

    FILE* Echo = vtr::fopen(EchoFile, "w");

    //Print all layout device switch/segment list info first
    PrintArchInfo(Echo, arch);

    //Models
    fprintf(Echo, "*************************************************\n");
    fprintf(Echo, "Printing library models \n");
    for (LogicalModelId model_id : arch->models.library_models()) {
        print_model(Echo, arch->models.get_model(model_id));
    }
    fprintf(Echo, "Printing user models \n");
    for (LogicalModelId model_id : arch->models.user_models()) {
        print_model(Echo, arch->models.get_model(model_id));
    }
    fprintf(Echo, "*************************************************\n\n");
    fprintf(Echo, "*************************************************\n");
    for (const t_physical_tile_type& Type : PhysicalTileTypes) {
        fprintf(Echo, "Type: \"%s\"\n", Type.name.c_str());
        fprintf(Echo, "\tcapacity: %d\n", Type.capacity);
        fprintf(Echo, "\twidth: %d\n", Type.width);
        fprintf(Echo, "\theight: %d\n", Type.height);
        for (const t_fc_specification& fc_spec : Type.fc_specs) {
            fprintf(Echo, "fc_value_type: ");
            if (fc_spec.fc_value_type == e_fc_value_type::ABSOLUTE) {
                fprintf(Echo, "ABSOLUTE");
            } else if (fc_spec.fc_value_type == e_fc_value_type::FRACTIONAL) {
                fprintf(Echo, "FRACTIONAL");
            } else {
                VTR_ASSERT(false);
            }
            fprintf(Echo, " fc_value: %f", fc_spec.fc_value);
            fprintf(Echo, " segment: %s", arch->Segments[fc_spec.seg_index].name.c_str());
            fprintf(Echo, " pins:");
            for (int pin : fc_spec.pins) {
                fprintf(Echo, " %d", pin);
            }
            fprintf(Echo, "\n");
        }
        fprintf(Echo, "\tnum_drivers: %d\n", Type.num_drivers);
        fprintf(Echo, "\tnum_receivers: %d\n", Type.num_receivers);

        int index = Type.index;
        fprintf(Echo, "\tindex: %d\n", index);

        auto equivalent_sites = get_equivalent_sites_set(&Type);

        for (auto LogicalBlock : equivalent_sites) {
            fprintf(Echo, "\nEquivalent Site: %s\n", LogicalBlock->name.c_str());
        }
        fprintf(Echo, "\n");
    }

    fprintf(Echo, "*************************************************\n\n");
    fprintf(Echo, "*************************************************\n");

    for (auto& LogicalBlock : LogicalBlockTypes) {
        if (LogicalBlock.pb_type) {
            PrintPb_types_rec(Echo, LogicalBlock.pb_type, 2, arch->models);
        }
        fprintf(Echo, "\n");
    }

    fclose(Echo);
}

//Added May 2013 Daniel Chen, help dump arch info after loading from XML
void PrintArchInfo(FILE* Echo, const t_arch* arch) {
    fprintf(Echo, "Printing architecture... \n\n");
    //Layout
    fprintf(Echo, "*************************************************\n");
    for (const auto& grid_layout : arch->grid_layouts) {
        if (grid_layout.grid_type == e_grid_def_type::AUTO) {
            fprintf(Echo, "Layout: '%s' Type: auto Aspect_Ratio: %f\n", grid_layout.name.c_str(), grid_layout.aspect_ratio);
        } else {
            VTR_ASSERT(grid_layout.grid_type == e_grid_def_type::FIXED);
            fprintf(Echo, "Layout: '%s' Type: fixed Width: %d Height %d\n", grid_layout.name.c_str(), grid_layout.width, grid_layout.height);
        }
    }
    fprintf(Echo, "*************************************************\n\n");
    //Device
    fprintf(Echo, "*************************************************\n");
    fprintf(Echo, "Device Info:\n");

    fprintf(Echo,
            "\tSizing: R_minW_nmos %e R_minW_pmos %e\n",
            arch->R_minW_nmos, arch->R_minW_pmos);

    fprintf(Echo, "\tArea: grid_logic_tile_area %e\n",
            arch->grid_logic_tile_area);

    fprintf(Echo, "\tChannel Width Distribution:\n");

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
            fprintf(Echo,
                    "\t\tx: distr dleta peak %e \
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
            fprintf(Echo,
                    "\t\ty: distr dleta peak %e \
						  Xpeak %e dc %e\n",
                    arch->Chans.chan_y_dist.peak, arch->Chans.chan_y_dist.xpeak,
                    arch->Chans.chan_y_dist.dc);
            break;
        default:
            fprintf(Echo, "\t\tInvalid Distribution!\n");
            break;
    }

    switch (arch->sb_type) {
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

    fprintf(Echo, "\tInput Connect Block Switch Name Within a Same Die: %s\n", arch->ipin_cblock_switch_name[ipin_cblock_switch_index_within_die].c_str());

    //if there is more than one layer available, print the connection block switch name that is used for connection between two dice
    for (const auto& layout : arch->grid_layouts) {
        int num_layers = (int)layout.layers.size();
        if (num_layers > 1) {
            fprintf(Echo, "\tInput Connect Block Switch Name Between Two Dice: %s\n", arch->ipin_cblock_switch_name[ipin_cblock_switch_index_between_dice].c_str());
        }
    }

    fprintf(Echo, "*************************************************\n\n");
    //Switch list
    fprintf(Echo, "*************************************************\n");
    fprintf(Echo, "Switch List:\n");

    //13 is hard coded because format of %e is always 1.123456e+12
    //It always consists of 10 alphanumeric digits, a decimal
    //and a sign
    for (int i = 0; i < (int)arch->switches.size(); i++) {
        if (arch->switches[i].type() == SwitchType::MUX) {
            fprintf(Echo, "\tSwitch[%d]: name %s type mux\n", i + 1, arch->switches[i].name.c_str());
        } else if (arch->switches[i].type() == SwitchType::TRISTATE) {
            fprintf(Echo, "\tSwitch[%d]: name %s type tristate\n", i + 1, arch->switches[i].name.c_str());
        } else if (arch->switches[i].type() == SwitchType::SHORT) {
            fprintf(Echo, "\tSwitch[%d]: name %s type short\n", i + 1, arch->switches[i].name.c_str());
        } else if (arch->switches[i].type() == SwitchType::BUFFER) {
            fprintf(Echo, "\tSwitch[%d]: name %s type buffer\n", i + 1, arch->switches[i].name.c_str());
        } else {
            VTR_ASSERT(arch->switches[i].type() == SwitchType::PASS_GATE);
            fprintf(Echo, "\tSwitch[%d]: name %s type pass_gate\n", i + 1, arch->switches[i].name.c_str());
        }
        fprintf(Echo, "\t\t\t\tR %e Cin %e Cout %e\n", arch->switches[i].R,
                arch->switches[i].Cin, arch->switches[i].Cout);
        fprintf(Echo, "\t\t\t\t#Tdel values %d buf_size %e mux_trans_size %e\n",
                (int)arch->switches[i].Tdel_map_.size(), arch->switches[i].buf_size,
                arch->switches[i].mux_trans_size);
        if (arch->switches[i].power_buffer_type == POWER_BUFFER_TYPE_AUTO) {
            fprintf(Echo, "\t\t\t\tpower_buffer_size auto\n");
        } else {
            fprintf(Echo, "\t\t\t\tpower_buffer_size %e\n",
                    arch->switches[i].power_buffer_size);
        }
    }

    fprintf(Echo, "*************************************************\n\n");
    //Segment List
    fprintf(Echo, "*************************************************\n");
    fprintf(Echo, "Segment List:\n");
    for (int i = 0; i < (int)(arch->Segments).size(); i++) {
        const struct t_segment_inf& seg = arch->Segments[i];
        fprintf(Echo,
                "\tSegment[%d]: frequency %d length %d R_metal %e C_metal %e\n",
                i + 1, seg.frequency, seg.length,
                seg.Rmetal, seg.Cmetal);

        if (seg.directionality == UNI_DIRECTIONAL) {
            //wire_switch == arch_opin_switch
            fprintf(Echo, "\t\t\t\ttype unidir mux_name for within die connections: %s\n",
                    arch->switches[seg.arch_wire_switch].name.c_str());
            //if there is more than one layer available, print the segment switch name that is used for connection between two dice
            for (const auto& layout : arch->grid_layouts) {
                int num_layers = (int)layout.layers.size();
                if (num_layers > 1) {
                    fprintf(Echo, "\t\t\t\ttype unidir mux_name for between two dice connections: %s\n",
                            arch->switches[seg.arch_inter_die_switch].name.c_str());
                }
            }
        } else { //Should be bidir
            fprintf(Echo, "\t\t\t\ttype bidir wire_switch %s arch_opin_switch %s\n",
                    arch->switches[seg.arch_wire_switch].name.c_str(),
                    arch->switches[seg.arch_opin_switch].name.c_str());
        }

        fprintf(Echo, "\t\t\t\tcb ");
        for (int j = 0; j < (int)seg.cb.size(); j++) {
            if (seg.cb[j]) {
                fprintf(Echo, "1 ");
            } else {
                fprintf(Echo, "0 ");
            }
        }
        fprintf(Echo, "\n");

        fprintf(Echo, "\t\t\t\tsb ");
        for (int j = 0; j < (int)seg.sb.size(); j++) {
            if (seg.sb[j]) {
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
    for (int i = 0; i < (int)arch->directs.size(); i++) {
        fprintf(Echo, "\tDirect[%d]: name %s from_pin %s to_pin %s\n", i + 1,
                arch->directs[i].name.c_str(), arch->directs[i].from_pin.c_str(),
                arch->directs[i].to_pin.c_str());
        fprintf(Echo, "\t\t\t\t x_offset %d y_offset %d z_offset %d\n",
                arch->directs[i].x_offset, arch->directs[i].y_offset,
                arch->directs[i].sub_tile_offset);
    }
    fprintf(Echo, "*************************************************\n\n");

    //Router Connection List
    if (arch->noc != nullptr) {
        fprintf(Echo, "*************************************************\n");
        fprintf(Echo, "NoC Router Connection List:\n");

        for (const auto& noc_router : arch->noc->router_list) {
            fprintf(Echo, "NoC router %d is connected to:\t", noc_router.id);
            for (auto noc_conn_id : noc_router.connection_list) {
                fprintf(Echo, "%d\t", noc_conn_id);
            }
            fprintf(Echo, "\n");
        }
        fprintf(Echo, "*************************************************\n\n");
    }

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
        for (size_t i = 0; i < arch->clocks->size(); i++) {
            if ((*arch->clocks)[i].autosize_buffer) {
                fprintf(Echo, "\tClock[%zu]: buffer_size auto C_wire %e", i + 1,
                        (*arch->clocks)[i].C_wire);
            } else {
                fprintf(Echo, "\tClock[%zu]: buffer_size %e C_wire %e", i + 1,
                        (*arch->clocks)[i].buffer_size,
                        (*arch->clocks)[i].C_wire);
            }
            fprintf(Echo, "\t\t\t\tstat_prob %f switch_density %f period %e",
                    (*arch->clocks)[i].prob,
                    (*arch->clocks)[i].dens,
                    (*arch->clocks)[i].period);
        }
    }

    fprintf(Echo, "*************************************************\n\n");
}

static void print_model(FILE* echo, const t_model& model) {
    fprintf(echo, "Model: \"%s\"\n", model.name);
    t_model_ports* input_model_port = model.inputs;
    while (input_model_port) {
        fprintf(echo, "\tInput Ports: \"%s\" \"%d\" min_size=\"%d\"\n",
                input_model_port->name, input_model_port->size,
                input_model_port->min_size);
        input_model_port = input_model_port->next;
    }
    t_model_ports* output_model_port = model.outputs;
    while (output_model_port) {
        fprintf(echo, "\tOutput Ports: \"%s\" \"%d\" min_size=\"%d\"\n",
                output_model_port->name, output_model_port->size,
                output_model_port->min_size);
        output_model_port = output_model_port->next;
    }
    vtr::t_linked_vptr* cur_vptr = model.pb_types;
    int i = 0;
    while (cur_vptr != nullptr) {
        fprintf(echo, "\tpb_type %d: \"%s\"\n", i,
                ((t_pb_type*)cur_vptr->data_vptr)->name);
        cur_vptr = cur_vptr->next;
        i++;
    }
}

static void PrintPb_types_rec(FILE* Echo, const t_pb_type* pb_type, int level, const LogicalModels& models) {
    std::string tabs = std::string(level, '\t');

    fprintf(Echo, "%spb_type name: %s\n", tabs.c_str(), pb_type->name);
    fprintf(Echo, "%s\tblif_model: %s\n", tabs.c_str(), pb_type->blif_model);
    fprintf(Echo, "%s\tclass_type: %d\n", tabs.c_str(), pb_type->class_type);
    fprintf(Echo, "%s\tnum_modes: %d\n", tabs.c_str(), pb_type->num_modes);
    fprintf(Echo, "%s\tnum_ports: %d\n", tabs.c_str(), pb_type->num_ports);
    for (int i = 0; i < pb_type->num_ports; i++) {
        fprintf(Echo, "%s\tport %s type %d num_pins %d\n", tabs.c_str(),
                pb_type->ports[i].name, pb_type->ports[i].type,
                pb_type->ports[i].num_pins);
    }

    if (pb_type->num_modes > 0) { /*one or more modes*/
        for (int i = 0; i < pb_type->num_modes; i++) {
            fprintf(Echo, "%s\tmode %s:\n", tabs.c_str(), pb_type->modes[i].name);
            for (int j = 0; j < pb_type->modes[i].num_pb_type_children; j++) {
                PrintPb_types_rec(Echo, &pb_type->modes[i].pb_type_children[j],
                                  level + 2, models);
            }
            for (int j = 0; j < pb_type->modes[i].num_interconnect; j++) {
                fprintf(Echo, "%s\t\tinterconnect %d %s %s\n", tabs.c_str(),
                        pb_type->modes[i].interconnect[j].type,
                        pb_type->modes[i].interconnect[j].input_string,
                        pb_type->modes[i].interconnect[j].output_string);
                for (const t_pin_to_pin_annotation& annotation : pb_type->modes[i].interconnect[j].annotations) {
                    fprintf(Echo, "%s\t\t\tannotation %s %s %d: %s\n", tabs.c_str(),
                            annotation.input_pins,
                            annotation.output_pins,
                            annotation.format,
                            annotation.annotation_entries[0].second.c_str());
                }
                //Print power info for interconnects
                if (pb_type->modes[i].interconnect[j].interconnect_power) {
                    if (pb_type->modes[i].interconnect[j].interconnect_power->power_usage.dynamic
                        || pb_type->modes[i].interconnect[j].interconnect_power->power_usage.leakage) {
                        fprintf(Echo, "%s\t\t\tpower %e %e\n", tabs.c_str(),
                                pb_type->modes[i].interconnect[j].interconnect_power->power_usage.dynamic,
                                pb_type->modes[i].interconnect[j].interconnect_power->power_usage.leakage);
                    }
                }
            }
        }
    } else { /*leaf pb with unknown model*/
        /*LUT(names) already handled, it naturally has 2 modes.
         * I/O has no annotations to be displayed
         * All other library or user models may have delays specificied, e.g. Tsetup and Tcq
         * Display the additional information*/
        std::string pb_type_model_name = models.get_model(pb_type->model_id).name;
        if (pb_type_model_name != LogicalModels::MODEL_NAMES
            && pb_type_model_name != LogicalModels::MODEL_INPUT
            && pb_type_model_name != LogicalModels::MODEL_OUTPUT) {
            for (const t_pin_to_pin_annotation& annotation : pb_type->annotations) {
                fprintf(Echo, "%s\t\t\tannotation %s %s %s %d: %s\n", tabs.c_str(),
                        annotation.clock,
                        annotation.input_pins,
                        annotation.output_pins,
                        annotation.format,
                        annotation.annotation_entries[0].second.c_str());
            }
        }
    }

    if (pb_type->pb_type_power) {
        PrintPb_types_recPower(Echo, pb_type, tabs.c_str());
    }
}

//Added May 2013 Daniel Chen, help dump arch info after loading from XML
static void PrintPb_types_recPower(FILE* Echo,
                                   const t_pb_type* pb_type,
                                   const char* tabs) {
    int i = 0;
    /*Print power information for each pb if available*/
    switch (pb_type->pb_type_power->estimation_method) {
        case POWER_METHOD_UNDEFINED:
            fprintf(Echo, "%s\tpower method: undefined\n", tabs);
            break;
        case POWER_METHOD_IGNORE:
            if (pb_type->parent_mode) {
                /*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
                 * This is because of the inheritance property of auto-size*/
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
                 * This is because of the inheritance property of auto-size*/
                if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
                    == POWER_METHOD_AUTO_SIZES)
                    break;
            }
            fprintf(Echo, "%s\tpower method: auto-size\n", tabs);
            break;
        case POWER_METHOD_SPECIFY_SIZES:
            if (pb_type->parent_mode) {
                /*if NOT top-level pb (all top-level pb has NULL parent_mode, check parent's power method
                 * This is because of the inheritance property of specify-size*/
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
                 * This is because once energy_per_toggle is specified at one level,
                 * all children pb's are energy_per_toggle and only want to display once*/
                if (pb_type->parent_mode->parent_pb_type->pb_type_power->estimation_method
                    == POWER_METHOD_TOGGLE_PINS)
                    break;
            }

            fprintf(Echo, "%s\tpower method: pin-toggle\n", tabs);
            for (i = 0; i < pb_type->num_ports; i++) {
                /*Print all the power information on each port, only if available,
                 * will not print if value is 0 or NULL*/
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
                                    (pb_type->ports[i].port_power->reverse_scaled ? "%s\t\t\tscaled_by_static_prob_n: %s[%d]\n" : "%s\t\t\tscaled_by_static_prob: %s[%d]\n"),
                                    tabs,
                                    pb_type->ports[i].port_power->scaled_by_port->name,
                                    pb_type->ports[i].port_power->scaled_by_port_pin_idx);
                        } else {
                            fprintf(Echo,
                                    (pb_type->ports[i].port_power->reverse_scaled ? "%s\t\t\tscaled_by_static_prob_n: %s\n" : "%s\t\t\tscaled_by_static_prob: %s\n"),
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
                 * This is because of values at this level includes all children pb's*/
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
                 * This is because of values at this level includes all children pb's*/
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
