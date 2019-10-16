#include <cstring>
#include <cstdlib>
#include <vector>

#include "echo_arch.h"
#include "arch_types.h"
#include "vtr_list.h"
#include "vtr_util.h"
#include "vtr_memory.h"
#include "vtr_assert.h"

using vtr::t_linked_vptr;

void PrintArchInfo(FILE* Echo, const t_arch* arch);
static void PrintPb_types_rec(FILE* Echo, const t_pb_type* pb_type, int level);
static void PrintPb_types_recPower(FILE* Echo,
                                   const t_pb_type* pb_type,
                                   const char* tabs);

/* Output the data from architecture data so user can verify it
 * was interpretted correctly. */
void EchoArch(const char* EchoFile,
              const std::vector<t_physical_tile_type>& PhysicalTileTypes,
              const std::vector<t_logical_block_type>& LogicalBlockTypes,
              const t_arch* arch) {
    int i, j;
    FILE* Echo;
    t_model* cur_model;
    t_model_ports* model_port;
    t_linked_vptr* cur_vptr;

    Echo = vtr::fopen(EchoFile, "w");
    cur_model = nullptr;

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
            while (cur_vptr != nullptr) {
                fprintf(Echo, "\tpb_type %d: \"%s\"\n", i,
                        ((t_pb_type*)cur_vptr->data_vptr)->name);
                cur_vptr = cur_vptr->next;
                i++;
            }

            cur_model = cur_model->next;
        }
    }
    fprintf(Echo, "*************************************************\n\n");
    fprintf(Echo, "*************************************************\n");
    for (auto& Type : PhysicalTileTypes) {
        fprintf(Echo, "Type: \"%s\"\n", Type.name);
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

        for (auto LogicalBlock : Type.equivalent_sites) {
            fprintf(Echo, "\nEquivalent Site: %s\n", LogicalBlock->name);
        }
        fprintf(Echo, "\n");
    }

    fprintf(Echo, "*************************************************\n\n");
    fprintf(Echo, "*************************************************\n");

    for (auto& LogicalBlock : LogicalBlockTypes) {
        if (LogicalBlock.pb_type) {
            PrintPb_types_rec(Echo, LogicalBlock.pb_type, 2);
        }
        fprintf(Echo, "\n");
    }

    fclose(Echo);
}

//Added May 2013 Daniel Chen, help dump arch info after loading from XML
void PrintArchInfo(FILE* Echo, const t_arch* arch) {
    int i, j;

    fprintf(Echo, "Printing architecture... \n\n");
    //Layout
    fprintf(Echo, "*************************************************\n");
    for (const auto& grid_layout : arch->grid_layouts) {
        if (grid_layout.grid_type == GridDefType::AUTO) {
            fprintf(Echo, "Layout: '%s' Type: auto Aspect_Ratio: %f\n", grid_layout.name.c_str(), grid_layout.aspect_ratio);
        } else {
            VTR_ASSERT(grid_layout.grid_type == GridDefType::FIXED);
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

    fprintf(Echo, "\tInput Connect Block Switch Name: %s\n", arch->ipin_cblock_switch_name.c_str());

    fprintf(Echo, "*************************************************\n\n");
    //Switch list
    fprintf(Echo, "*************************************************\n");
    fprintf(Echo, "Switch List:\n");

    //13 is hard coded because format of %e is always 1.123456e+12
    //It always consists of 10 alphanumeric digits, a decimal
    //and a sign
    for (i = 0; i < arch->num_switches; i++) {
        if (arch->Switches[i].type() == SwitchType::MUX) {
            fprintf(Echo, "\tSwitch[%d]: name %s type mux\n", i + 1, arch->Switches[i].name);
        } else if (arch->Switches[i].type() == SwitchType::TRISTATE) {
            fprintf(Echo, "\tSwitch[%d]: name %s type tristate\n", i + 1, arch->Switches[i].name);
        } else if (arch->Switches[i].type() == SwitchType::SHORT) {
            fprintf(Echo, "\tSwitch[%d]: name %s type short\n", i + 1, arch->Switches[i].name);
        } else if (arch->Switches[i].type() == SwitchType::BUFFER) {
            fprintf(Echo, "\tSwitch[%d]: name %s type buffer\n", i + 1, arch->Switches[i].name);
        } else {
            VTR_ASSERT(arch->Switches[i].type() == SwitchType::PASS_GATE);
            fprintf(Echo, "\tSwitch[%d]: name %s type pass_gate\n", i + 1, arch->Switches[i].name);
        }
        fprintf(Echo, "\t\t\t\tR %e Cin %e Cout %e\n", arch->Switches[i].R,
                arch->Switches[i].Cin, arch->Switches[i].Cout);
        fprintf(Echo, "\t\t\t\t#Tdel values %d buf_size %e mux_trans_size %e\n",
                (int)arch->Switches[i].Tdel_map_.size(), arch->Switches[i].buf_size,
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
    for (i = 0; i < (int)(arch->Segments).size(); i++) {
        const struct t_segment_inf& seg = arch->Segments[i];
        fprintf(Echo,
                "\tSegment[%d]: frequency %d length %d R_metal %e C_metal %e\n",
                i + 1, seg.frequency, seg.length,
                seg.Rmetal, seg.Cmetal);

        if (seg.directionality == UNI_DIRECTIONAL) {
            //wire_switch == arch_opin_switch
            fprintf(Echo, "\t\t\t\ttype unidir mux_name %s\n",
                    arch->Switches[seg.arch_wire_switch].name);
        } else { //Should be bidir
            fprintf(Echo, "\t\t\t\ttype bidir wire_switch %s arch_opin_switch %s\n",
                    arch->Switches[seg.arch_wire_switch].name,
                    arch->Switches[seg.arch_opin_switch].name);
        }

        fprintf(Echo, "\t\t\t\tcb ");
        for (j = 0; j < (int)seg.cb.size(); j++) {
            if (seg.cb[j]) {
                fprintf(Echo, "1 ");
            } else {
                fprintf(Echo, "0 ");
            }
        }
        fprintf(Echo, "\n");

        fprintf(Echo, "\t\t\t\tsb ");
        for (j = 0; j < (int)seg.sb.size(); j++) {
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

static void PrintPb_types_rec(FILE* Echo, const t_pb_type* pb_type, int level) {
    int i, j, k;
    char* tabs;

    tabs = (char*)vtr::malloc((level + 1) * sizeof(char));
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

    if (pb_type->num_modes > 0) { /*one or more modes*/
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
    } else { /*leaf pb with unknown model*/
        /*LUT(names) already handled, it naturally has 2 modes.
         * I/O has no annotations to be displayed
         * All other library or user models may have delays specificied, e.g. Tsetup and Tcq
         * Display the additional information*/
        if (strcmp(pb_type->model->name, MODEL_NAMES)
            && strcmp(pb_type->model->name, MODEL_INPUT)
            && strcmp(pb_type->model->name, MODEL_OUTPUT)) {
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
