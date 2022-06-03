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
 *  This file offers functions to estimate power of major components
 * within the FPGA (flip-flops, LUTs, interconnect structures, etc).
 */

/************************* INCLUDES *********************************/
#include <cstring>
#include <cmath>

#include "vtr_math.h"
#include "vtr_assert.h"
#include "vtr_memory.h"

#include "power_components.h"
#include "power_lowlevel.h"
#include "power_util.h"
#include "power_callibrate.h"
#include "globals.h"

/************************* STRUCTS **********************************/

/************************* FUNCTION DECLARATIONS ********************/
static void power_usage_mux_rec(t_power_usage* power_usage, float* out_prob, float* out_dens, float* v_out, t_mux_node* mux_node, t_mux_arch* mux_arch, int* selector_values, float* primary_input_prob, float* primary_input_dens, bool v_out_restored, float period);

/************************* FUNCTION DEFINITIONS *********************/

/**
 * Module initializer function, called by power_init. 
 * Zero initializes via power_zero_usage.
 */
void power_components_init() {
    int i;
    auto& power_ctx = g_vpr_ctx.mutable_power();

    power_ctx.by_component.components = new t_power_usage[POWER_COMPONENT_MAX_NUM];
    for (i = 0; i < POWER_COMPONENT_MAX_NUM; i++) {
        power_zero_usage(&power_ctx.by_component.components[i]);
    }
}

/**
 * Module un-initializer function, called by power_uninit
 */
void power_components_uninit() {
    auto& power_ctx = g_vpr_ctx.mutable_power();
    delete[](power_ctx.by_component.components);
}

/**
 * Adds power usage for a component to the global component tracker
 * - power_usage: Power usage to add
 * - component_idx: Type of component
 */
void power_component_add_usage(t_power_usage* power_usage,
                               e_power_component_type component_idx) {
    auto& power_ctx = g_vpr_ctx.power();
    power_add_usage(&power_ctx.by_component.components[component_idx],
                    power_usage);
}

/**
 * Gets power usage for a component
 * - power_usage: (Return value) Power usage for the given component
 * - component_idx: Type of component
 */
void power_component_get_usage(t_power_usage* power_usage,
                               e_power_component_type component_idx) {
    auto& power_ctx = g_vpr_ctx.power();
    memcpy(power_usage, &power_ctx.by_component.components[component_idx],
           sizeof(t_power_usage));
}

/**
 * Returns total power for a given component
 * - component_idx: Type of component
 */
float power_component_get_usage_sum(e_power_component_type component_idx) {
    auto& power_ctx = g_vpr_ctx.power();
    return power_sum_usage(&power_ctx.by_component.components[component_idx]);
}

/**
 * Calculates power of a D flip-flop
 * - power_usage: (Return value) power usage of the flip-flop
 * - D_prob: Signal probability of the input
 * - D_dens: Transition density of the input
 * - Q_prob: Signal probability of the output
 * - Q_dens: Transition density of the output
 * - clk_prob: Signal probability of the clock
 * - clk_dens: Transition density of the clock
 */
void power_usage_ff(t_power_usage* power_usage, float size, float D_prob, float D_dens, float Q_prob, float Q_dens, float clk_prob, float clk_dens, float period) {
    t_power_usage sub_power_usage;
    float mux_in_dens[2];
    float mux_in_prob[2];
    PowerSpicedComponent* callibration;
    float scale_factor;

    power_zero_usage(power_usage);

    /* DFF is build using a master loop and slave loop.
     * Each loop begins with a MUX and contains 2 inverters
     * in a feedback loop to the mux.
     * Each mux is built using two transmission gates.
     */

    /* Master */
    mux_in_dens[0] = D_dens;
    mux_in_dens[1] = (1 - clk_prob) * D_dens;
    mux_in_prob[0] = D_prob;
    mux_in_prob[1] = D_prob;
    power_usage_MUX2_transmission(&sub_power_usage, size, mux_in_dens,
                                  mux_in_prob, clk_dens, (1 - clk_prob) * D_dens, period);
    power_add_usage(power_usage, &sub_power_usage);

    power_usage_inverter(&sub_power_usage, (1 - clk_prob) * D_dens, D_prob,
                         size, period);
    power_add_usage(power_usage, &sub_power_usage);

    power_usage_inverter(&sub_power_usage, (1 - clk_prob) * D_dens, 1 - D_prob,
                         size, period);
    power_add_usage(power_usage, &sub_power_usage);

    /* Slave */
    mux_in_dens[0] = Q_dens;
    mux_in_dens[1] = (1 - clk_prob) * D_dens;
    mux_in_prob[0] = (1 - Q_prob);
    mux_in_prob[1] = (1 - D_prob);
    power_usage_MUX2_transmission(&sub_power_usage, size, mux_in_dens,
                                  mux_in_prob, clk_dens, Q_dens, period);
    power_add_usage(power_usage, &sub_power_usage);

    power_usage_inverter(&sub_power_usage, Q_dens, 1 - Q_prob, size, period);
    power_add_usage(power_usage, &sub_power_usage);

    power_usage_inverter(&sub_power_usage, Q_dens, Q_prob, size, period);
    power_add_usage(power_usage, &sub_power_usage);

    /* Callibration */
    auto& power_ctx = g_vpr_ctx.power();
    callibration = power_ctx.commonly_used->component_callibration[POWER_CALLIB_COMPONENT_FF];
    if (callibration->is_done_callibration()) {
        scale_factor = callibration->scale_factor(1, size);
        power_scale_usage(power_usage, scale_factor);
    }

    return;
}

/**
 * Calculated power of a look-up table (LUT)
 * - power_usage: (Return value) The power usage of the LUT
 * - LUT_size: Number of LUT inputs
 * - SRAM_values: The 2^(LUT_size) truth table values.  String of '0' and '1' characters.
 * 		First characters is for all inputs = 0 and last characters is for all inputs = 1.
 * - input_prob: Array of input signal probabilities
 * - input_dens: Array of input transition densities
 *
 * NOTE: The following provides a diagram of a 3-LUT, the sram bit ordering, and
 *  the input array ordering.
 *
 *	X - NMOS gate controlled by complement of input
 *	Z - NMOS gate controlled by input
 *
 * S
 * R  I   I   I
 * A  N   N   N
 * M  2   1   0
 * |  |   |   |
 * v  v   |   |
 *        v   |
 * 0 _X_      |
 *      |_X_  v
 * 1 _Z_|   |
 *          |_X_
 * 2 _X_    |   |
 *      |_Z_|   |
 * 3 _Z_|       |
 *              |------ out
 * 4 _X_        |
 *      |_X_    |
 * 5 _Z_|   |   |
 *          |_Z_|
 * 6 _X_    |
 *      |_Z_|
 * 7 _Z_|
 *
 */
void power_usage_lut(t_power_usage* power_usage, int lut_size, float transistor_size, char* SRAM_values, float* input_prob, float* input_dens, float period) {
    float** internal_prob;
    float** internal_dens;
    float** internal_v;
    int i;
    int level_idx;
    PowerSpicedComponent* callibration;
    float scale_factor;

    int num_SRAM_bits;

    bool level_restorer_this_level = false;

    auto& power_ctx = g_vpr_ctx.power();

    power_zero_usage(power_usage);

    num_SRAM_bits = 1 << lut_size;

    /* Initialize internal node data */
    internal_prob = new float*[lut_size + 1];
    internal_dens = new float*[lut_size + 1];
    internal_v = new float*[lut_size + 1];
    for (i = 0; i <= lut_size; i++) {
        internal_prob[i] = new float[1 << (lut_size - i)];
        internal_dens[i] = new float[1 << (lut_size - i)];
        internal_v[i] = new float[1 << (lut_size - i)];
        for (int j = 0; j < (1 << (lut_size - i)); j++) {
            internal_prob[i][j] = 0;
            internal_dens[i][j] = 0;
            internal_v[i][j] = 0;
        }
    }

    /* Initialize internal probabilities/densities from SRAM bits */
    for (i = 0; i < num_SRAM_bits; i++) {
        if (SRAM_values[i] == '0') {
            internal_prob[0][i] = 0.;
        } else {
            internal_prob[0][i] = 1.;
        }
        internal_dens[0][i] = 0.;
        internal_v[0][i] = power_ctx.tech->Vdd;
    }

    for (level_idx = 0; level_idx < lut_size; level_idx++) {
        t_power_usage driver_power_usage;
        int MUXs_this_level;
        int MUX_idx;
        int reverse_idx = lut_size - level_idx - 1;

        MUXs_this_level = 1 << (reverse_idx);

        /* Power of input drivers */
        power_usage_inverter(&driver_power_usage, input_dens[reverse_idx],
                             input_prob[reverse_idx], 1.0, period);
        power_add_usage(power_usage, &driver_power_usage);

        power_usage_inverter(&driver_power_usage, input_dens[reverse_idx],
                             input_prob[reverse_idx], 2.0, period);
        power_add_usage(power_usage, &driver_power_usage);

        power_usage_inverter(&driver_power_usage, input_dens[reverse_idx],
                             1 - input_prob[reverse_idx], 2.0, period);
        power_add_usage(power_usage, &driver_power_usage);

        /* Add level restorer after every 2 stages (level_idx %2 == 1)
         * But if there is an odd # of stages, just put one at the last
         * stage (level_idx == LUT_size - 1) and not at the stage just before
         * the last stage (level_idx != LUT_size - 2)
         */
        if (((level_idx % 2 == 1) && (level_idx != lut_size - 2))
            || (level_idx == lut_size - 1)) {
            level_restorer_this_level = true;
        } else {
            level_restorer_this_level = false;
        }

        /* Loop through the 2-muxs at each level */
        for (MUX_idx = 0; MUX_idx < MUXs_this_level; MUX_idx++) {
            t_power_usage sub_power;
            float out_prob;
            float out_dens;
            float sum_prob = 0;
            int sram_offset = MUX_idx * vtr::ipow(2, level_idx + 1);
            int sram_per_branch = vtr::ipow(2, level_idx);
            int branch_lvl_idx;
            int sram_idx;
            float v_out;

            /* Calculate output probability of multiplexer */
            out_prob = internal_prob[level_idx][MUX_idx * 2]
                           * (1 - input_prob[reverse_idx])
                       + internal_prob[level_idx][MUX_idx * 2 + 1]
                             * input_prob[reverse_idx];

            /* Calculate output density of multiplexer */
            out_dens = internal_dens[level_idx][MUX_idx * 2]
                           * (1 - input_prob[reverse_idx])
                       + internal_dens[level_idx][MUX_idx * 2 + 1]
                             * input_prob[reverse_idx];

#ifdef POWER_LUT_FAST
            out_dens += ((1 - internal_prob[level_idx][MUX_idx * 2]) * internal_prob[level_idx][MUX_idx * 2 + 1]
                         + internal_prob[level_idx][MUX_idx * 2] * (1 - internal_prob[level_idx][MUX_idx * 2 + 1]))
                        * input_dens[reverse_idx];
#elif defined(POWER_LUT_SLOW)
            for (sram_idx = sram_offset;
                 sram_idx < sram_offset + sram_per_branch; sram_idx++) {
                float branch_prob = 1.;
                if (SRAM_values[sram_idx]
                    == SRAM_values[sram_idx + sram_per_branch]) {
                    continue;
                }
                for (branch_lvl_idx = 0; branch_lvl_idx < level_idx;
                     branch_lvl_idx++) {
                    int branch_lvl_reverse_idx = lut_size - branch_lvl_idx - 1;
                    int even_odd = sram_idx / vtr::ipow(2, branch_lvl_idx);
                    if (even_odd % 2 == 0) {
                        branch_prob *= (1 - input_prob[branch_lvl_reverse_idx]);
                    } else {
                        branch_prob *= input_prob[branch_lvl_reverse_idx];
                    }
                }
                sum_prob += branch_prob;
            }
            out_dens += sum_prob * input_dens[reverse_idx];
#endif

            /* Calculate output voltage of multiplexer */
            if (level_restorer_this_level) {
                v_out = power_ctx.tech->Vdd;
            } else {
                v_out = (1 - input_prob[reverse_idx])
                            * power_calc_mux_v_out(2, 1.0,
                                                   internal_v[level_idx][MUX_idx * 2],
                                                   internal_prob[level_idx][MUX_idx * 2 + 1])
                        + input_prob[reverse_idx]
                              * power_calc_mux_v_out(2, 1.0,
                                                     internal_v[level_idx][MUX_idx * 2 + 1],
                                                     internal_prob[level_idx][MUX_idx * 2]);
            }

            /* Save internal node info */
            internal_dens[level_idx + 1][MUX_idx] = out_dens;
            internal_prob[level_idx + 1][MUX_idx] = out_prob;
            internal_v[level_idx + 1][MUX_idx] = v_out;

            /* Calculate power of the 2-mux */
            power_usage_mux_singlelevel_dynamic(&sub_power, 2,
                                                internal_dens[level_idx + 1][MUX_idx],
                                                internal_v[level_idx + 1][MUX_idx],
                                                &internal_prob[level_idx][MUX_idx * 2],
                                                &internal_dens[level_idx][MUX_idx * 2],
                                                &internal_v[level_idx][MUX_idx * 2],
                                                input_dens[reverse_idx], input_prob[reverse_idx],
                                                transistor_size, period);
            power_add_usage(power_usage, &sub_power);

            /* Add the level-restoring buffer if necessary */
            if (level_restorer_this_level) {
                /* Level restorer */
                power_usage_buffer(&sub_power, 1,
                                   internal_prob[level_idx + 1][MUX_idx],
                                   internal_dens[level_idx + 1][MUX_idx], true, period);
                power_add_usage(power_usage, &sub_power);
            }
        }
    }

    /* Free allocated memory */
    for (i = 0; i <= lut_size; i++) {
        delete[](internal_prob[i]);
        delete[](internal_dens[i]);
        delete[](internal_v[i]);
    }
    delete[](internal_prob);
    delete[](internal_dens);
    delete[](internal_v);

    /* Callibration */
    callibration = power_ctx.commonly_used->component_callibration[POWER_CALLIB_COMPONENT_LUT];
    if (callibration->is_done_callibration()) {
        scale_factor = callibration->scale_factor(lut_size, transistor_size);
        power_scale_usage(power_usage, scale_factor);
    }

    return;
}

/**
 * This function calculates power of a local interconnect structure
 * - power_usage: (Return value) Power usage of the structure
 * - pb: The physical block to which this interconnect belongs
 * - interc_pins: The interconnect input/ouput pin information
 * - interc_length: The physical length spanned by the interconnect (meters)
 */
void power_usage_local_interc_mux(t_power_usage* power_usage, t_pb* pb, t_interconnect_pins* interc_pins, ClusterBlockId iblk) {
    int pin_idx;
    int out_port_idx;
    int in_port_idx;
    float* in_dens;
    float* in_prob;
    t_power_usage MUX_power;
    t_interconnect* interc = interc_pins->interconnect;
    t_interconnect_power* interc_power = interc->interconnect_power;
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& power_ctx = g_vpr_ctx.power();

    power_zero_usage(power_usage);

    /* Ensure port/pins are structured as expected */
    switch (interc_pins->interconnect->type) {
        case DIRECT_INTERC:
            VTR_ASSERT(interc_power->num_input_ports == 1);
            VTR_ASSERT(interc_power->num_output_ports == 1);
            break;
        case MUX_INTERC:
            VTR_ASSERT(interc_power->num_output_ports == 1);
            break;
        case COMPLETE_INTERC:
            break;
        default:
            //pass
            break;
    }

    /* Power of transistors to build interconnect structure */
    switch (interc_pins->interconnect->type) {
        case DIRECT_INTERC:
            /* Direct connections require no transistors */
            break;
        case MUX_INTERC:
        case COMPLETE_INTERC:
            /* Many-to-1, or Many-to-Many
             * Implemented as a multiplexer for each output
             * */
            in_dens = new float[interc->interconnect_power->num_input_ports];
            in_prob = new float[interc->interconnect_power->num_input_ports];
            for (auto i = 0; i < interc->interconnect_power->num_input_ports; i++) {
                in_dens[i] = 0.0;
                in_prob[i] = 0.0;
            }

            for (out_port_idx = 0;
                 out_port_idx < interc->interconnect_power->num_output_ports;
                 out_port_idx++) {
                for (pin_idx = 0;
                     pin_idx < interc->interconnect_power->num_pins_per_port;
                     pin_idx++) {
                    int selected_input = OPEN;

                    /* Clear input densities */
                    for (in_port_idx = 0;
                         in_port_idx
                         < interc->interconnect_power->num_input_ports;
                         in_port_idx++) {
                        in_dens[in_port_idx] = 0.;
                        in_prob[in_port_idx] = 0.;
                    }

                    /* Get probability/density of input signals */
                    if (pb) {
                        int cluster_pin_idx = interc_pins->output_pins[out_port_idx][pin_idx]->pin_count_in_cluster;
                        if (!cluster_ctx.clb_nlist.block_pb(iblk)->pb_route.count(cluster_pin_idx)) {
                            selected_input = 0;
                        } else {
                            AtomNetId output_pin_net = cluster_ctx.clb_nlist.block_pb(iblk)->pb_route[cluster_pin_idx].atom_net_id;
                            for (in_port_idx = 0; in_port_idx < interc->interconnect_power->num_input_ports; in_port_idx++) {
                                t_pb_graph_pin* input_pin = interc_pins->input_pins[in_port_idx][pin_idx];

                                if (!cluster_ctx.clb_nlist.block_pb(iblk)->pb_route.count(input_pin->pin_count_in_cluster)) continue;

                                AtomNetId input_pin_net = cluster_ctx.clb_nlist.block_pb(iblk)->pb_route[input_pin->pin_count_in_cluster].atom_net_id;
                                /* Find input pin that connects through the mux to the output pin */
                                if (output_pin_net == input_pin_net) {
                                    selected_input = in_port_idx;
                                }

                                /* Initialize input densities */
                                if (input_pin_net) {
                                    in_dens[in_port_idx] = pin_dens(pb, input_pin, iblk);
                                    in_prob[in_port_idx] = pin_prob(pb, input_pin, iblk);
                                }
                            }

                            /* Check that the input pin was found with a matching net to the output pin */
                            VTR_ASSERT(selected_input != OPEN);
                        }
                    } else {
                        selected_input = 0;
                    }

                    /* Calculate power of the multiplexer */
                    power_usage_mux_multilevel(&MUX_power,
                                               power_get_mux_arch(interc_pins->interconnect->interconnect_power->num_input_ports,
                                                                  power_ctx.arch->mux_transistor_size),
                                               in_prob,
                                               in_dens, selected_input, true, power_ctx.solution_inf.T_crit);

                    power_add_usage(power_usage, &MUX_power);
                }
            }

            delete[](in_dens);
            delete[](in_prob);
            break;
        default:
            VTR_ASSERT(0);
    }

    power_add_usage(&interc_pins->interconnect->interconnect_power->power_usage,
                    power_usage);
}

/**
 *  This calculates the power of a multilevel multiplexer, with static inputs
 *  - power_usage: (Return value) The power usage of the multiplexer
 *  - mux_arch: The information on the multiplexer architecture
 *  - in_prob: Array of input signal probabilities
 *  - in_dens: Array of input transition densitites
 *  - selected_input: The index of the input that has been statically selected
 *  - output_level_restored: Whether the output is level restored to Vdd.
 */
void power_usage_mux_multilevel(t_power_usage* power_usage,
                                t_mux_arch* mux_arch,
                                float* in_prob,
                                float* in_dens,
                                int selected_input,
                                bool output_level_restored,
                                float period) {
    float output_density;
    float output_prob;
    float V_out;
    bool found;
    PowerSpicedComponent* callibration;
    float scale_factor;
    int* selector_values = new int[mux_arch->levels];
    for (auto i = 0; i < mux_arch->levels; i++)
        selector_values[i] = 0;

    auto& power_ctx = g_vpr_ctx.power();

    VTR_ASSERT(selected_input != OPEN);

    power_zero_usage(power_usage);

    /* Find selection index at each level */
    found = mux_find_selector_values(selector_values, mux_arch->mux_graph_head,
                                     selected_input);

    VTR_ASSERT(found);

    /* Calculate power of the multiplexor stages, from final stage, to first stages */
    power_usage_mux_rec(power_usage, &output_density, &output_prob, &V_out,
                        mux_arch->mux_graph_head, mux_arch, selector_values, in_prob,
                        in_dens, output_level_restored, period);

    delete[](selector_values);

    callibration = power_ctx.commonly_used->component_callibration[POWER_CALLIB_COMPONENT_MUX];
    if (callibration->is_done_callibration()) {
        scale_factor = callibration->scale_factor(mux_arch->num_inputs,
                                                  mux_arch->transistor_size);
        power_scale_usage(power_usage, scale_factor);
    }
}

/**
 * Internal function, used recursively by power_calc_mux
 */
static void power_usage_mux_rec(t_power_usage* power_usage, float* out_prob, float* out_dens, float* v_out, t_mux_node* mux_node, t_mux_arch* mux_arch, int* selector_values, float* primary_input_prob, float* primary_input_dens, bool v_out_restored, float period) {
    int input_idx;
    float* in_prob;
    float* in_dens;
    float* v_in;
    t_power_usage sub_power_usage;
    auto& power_ctx = g_vpr_ctx.power();

    /* Single input mux is really just a wire, and has no power.
     * Ensure that it has no children before returning. */
    if (mux_node->num_inputs == 1) {
        VTR_ASSERT(mux_node->level == 0);
        return;
    }

    v_in = new float[mux_node->num_inputs];
    for (auto i = 0; i < mux_node->num_inputs; i++)
        v_in[i] = 0.0;
    if (mux_node->level == 0) {
        /* First level of mux - inputs are primar inputs */
        in_prob = &primary_input_prob[mux_node->starting_pin_idx];
        in_dens = &primary_input_dens[mux_node->starting_pin_idx];

        for (input_idx = 0; input_idx < mux_node->num_inputs; input_idx++) {
            v_in[input_idx] = power_ctx.tech->Vdd;
        }
    } else {
        /* Higher level of mux - inputs recursive from lower levels */
        in_prob = new float[mux_node->num_inputs];
        in_dens = new float[mux_node->num_inputs];
        for (auto i = 0; i < mux_node->num_inputs; i++) {
            in_prob[i] = 0;
            in_dens[i] = 0;
        }

        for (input_idx = 0; input_idx < mux_node->num_inputs; input_idx++) {
            /* Call recursively for multiplexer driving the input */
            power_usage_mux_rec(power_usage, &in_prob[input_idx],
                                &in_dens[input_idx], &v_in[input_idx],
                                &mux_node->children[input_idx], mux_arch, selector_values,
                                primary_input_prob, primary_input_dens, false, period);
        }
    }

    power_usage_mux_singlelevel_static(&sub_power_usage, out_prob, out_dens,
                                       v_out, mux_node->num_inputs, selector_values[mux_node->level],
                                       in_prob, in_dens, v_in, mux_arch->transistor_size, v_out_restored,
                                       period);
    power_add_usage(power_usage, &sub_power_usage);

    if (mux_node->level != 0) {
        delete[](in_prob);
        delete[](in_dens);
    }

    delete[](v_in);
}

/**
 * This function calculates the power of a multistage buffer
 * - power_usage: (Return value) Power usage of buffer
 * - size: The size of the final buffer stage, relative to min-sized inverter
 * - in_prob: The signal probability of the input
 * - in_dens: The transition density of the input
 * - level_restored: Whether this buffer must level restore the input
 * - input_mux_size: If fed by a mux, the size of this mutliplexer
 */
void power_usage_buffer(t_power_usage* power_usage, float size, float in_prob, float in_dens, bool level_restorer, float period) {
    t_power_usage sub_power_usage;
    int i, num_stages;
    float stage_effort;
    float stage_inv_size;
    float stage_in_prob;
    float input_dyn_power;
    float scale_factor;
    PowerSpicedComponent* callibration;

    power_zero_usage(power_usage);

    if (size == 0.) {
        return;
    }

    auto& power_ctx = g_vpr_ctx.power();

    num_stages = power_calc_buffer_num_stages(size,
                                              power_ctx.arch->logical_effort_factor);
    stage_effort = calc_buffer_stage_effort(num_stages, size);

    stage_in_prob = in_prob;
    for (i = 0; i < num_stages; i++) {
        stage_inv_size = pow(stage_effort, i);

        if (i == 0) {
            if (level_restorer) {
                /* Sense Buffer */
                power_usage_level_restorer(&sub_power_usage, &input_dyn_power,
                                           in_dens, stage_in_prob, period);
            } else {
                power_usage_inverter(&sub_power_usage, in_dens, stage_in_prob,
                                     stage_inv_size, period);
            }
        } else {
            power_usage_inverter(&sub_power_usage, in_dens, stage_in_prob,
                                 stage_inv_size, period);
        }
        power_add_usage(power_usage, &sub_power_usage);

        stage_in_prob = 1 - stage_in_prob;
    }

    /* Callibration */
    if (level_restorer) {
        callibration = power_ctx.commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER_WITH_LEVR];
    } else {
        callibration = power_ctx.commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER];
    }

    if (callibration->is_done_callibration()) {
        scale_factor = callibration->scale_factor(1, size);
        power_scale_usage(power_usage, scale_factor);
    }

    /* Short-circuit: add a factor to dynamic power, but the factor is not in addition to the input power
     * Need to subtract input before adding factor - this matters for small buffers
     */
    /*power_usage->dynamic += (power_usage->dynamic - input_dyn_power)
     * power_calc_buffer_sc(num_stages, stage_effort, level_restorer,
     * input_mux_size); */
}
