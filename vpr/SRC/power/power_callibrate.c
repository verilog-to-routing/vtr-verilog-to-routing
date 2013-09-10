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

/* This file provides functions used to verify the power estimations
 * againt SPICE.
 */

/************************* INCLUDES *********************************/
#include <assert.h>
#include <iostream>

#include "power_callibrate.h"
#include "power_components.h"
#include "power_lowlevel.h"
#include "power_util.h"
#include "power_cmos_tech.h"
#include "globals.h"

/************************* FUNCTION DECLARATIONS ********************/
static char binary_not(char c);

/************************* FUNCTION DEFINITIONS *********************/

/* This function prints high-activitiy and zero-activity single-cycle
 * energy estimations for a variety of components and sizes.
 */
void power_print_spice_comparison(void) {
//
	t_power_usage sub_power_usage;
//
//	float inv_sizes[5] = { 1, 8, 16, 32, 64 };
//
//	float buffer_sizes[3] = { 16, 25, 64 };
//
	unsigned int LUT_sizes[3] = { 6 };
//
//	float sb_buffer_sizes[6] = { 9, 9, 16, 16, 25, 25 };
//	unsigned int sb_mux_sizes[6] = { 4, 8, 12, 16, 20, 25 };
//
//	unsigned int mux_sizes[5] = { 4, 8, 12, 16, 20 };
//
	unsigned int i, j;
	float * dens = NULL;
	float * prob = NULL;
	char * SRAM_bits = NULL;
	int sram_idx;
//
	g_solution_inf.T_crit = 1.0e-8;
//
//
//	 fprintf(g_power_output->out, "Energy of INV (High Activity)\n");
//	 for (i = 0; i < (sizeof(inv_sizes) / sizeof(float)); i++) {
//	 power_usage_inverter(&sub_power_usage, 2, 0.5, inv_sizes[i],
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%g\t%g\n", inv_sizes[i],
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
//	 fprintf(g_power_output->out, "Energy of INV (No Activity)\n");
//	 for (i = 0; i < (sizeof(inv_sizes) / sizeof(float)); i++) {
//	 power_usage_inverter(&sub_power_usage, 0, 1, inv_sizes[i],
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%g\t%g\n", inv_sizes[i],
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//	 }
//
//	 fprintf(g_power_output->out, "Energy of Mux (High Activity)\n");
//	 for (i = 0; i < (sizeof(mux_sizes) / sizeof(int)); i++) {
//	 t_power_usage mux_power_usage;
//
//	 power_zero_usage(&mux_power_usage);
//
//	 dens = (float*) my_realloc(dens, mux_sizes[i] * sizeof(float));
//	 prob = (float*) my_realloc(prob, mux_sizes[i] * sizeof(float));
//	 for (j = 0; j < mux_sizes[i]; j++) {
//	 dens[j] = 2;
//	 prob[j] = 0.5;
//	 }
//	 power_usage_mux_multilevel(&mux_power_usage,
//	 power_get_mux_arch(mux_sizes[i]), prob, dens, 0, FALSE,
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%d\t%g\n", mux_sizes[i],
//	 (mux_power_usage.dynamic + mux_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
//	 fprintf(g_power_output->out, "Energy of Mux (No Activity)\n");
//	 for (i = 0; i < (sizeof(mux_sizes) / sizeof(int)); i++) {
//	 t_power_usage mux_power_usage;
//
//	 power_zero_usage(&mux_power_usage);
//
//	dens = (float*) my_realloc(dens, mux_sizes[i] * sizeof(float));
//	prob = (float*) my_realloc(prob, mux_sizes[i] * sizeof(float));
//	for (j = 0; j < mux_sizes[i]; j++) {
//		if (j == 0) {
//			dens[j] = 0;
//			prob[j] = 1;
//		} else {
//			dens[j] = 0;
//			prob[j] = 0;
//		}
//	}
//	 power_usage_mux_multilevel(&mux_power_usage,
//	 power_get_mux_arch(mux_sizes[i]), prob, dens, 0, FALSE,
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%d\t%g\n", mux_sizes[i],
//	 (mux_power_usage.dynamic + mux_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
//	 fprintf(g_power_output->out, "Energy of Buffer (High Activity)\n");
//	 for (i = 0; i < (sizeof(buffer_sizes) / sizeof(float)); i++) {
//	 power_usage_buffer(&sub_power_usage, buffer_sizes[i], 0.5, 2, FALSE,
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%g\t%g\n", buffer_sizes[i],
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
//	 fprintf(g_power_output->out, "Energy of Buffer (No Activity)\n");
//	 for (i = 0; i < (sizeof(buffer_sizes) / sizeof(float)); i++) {
//	 power_usage_buffer(&sub_power_usage, buffer_sizes[i], 1, 0, FALSE,
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%g\t%g\n", buffer_sizes[i],
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
	fprintf(g_power_output->out, "Energy of LUT (High Activity)\n");
	for (i = 0; i < (sizeof(LUT_sizes) / sizeof(int)); i++) {
		for (j = 1; j <= LUT_sizes[i]; j++) {
			SRAM_bits = (char*) my_realloc(SRAM_bits,
					((1 << j) + 1) * sizeof(char));
			if (j == 1) {
				SRAM_bits[0] = '1';
				SRAM_bits[1] = '0';
			} else {
				for (sram_idx = 0; sram_idx < (1 << (j - 1)); sram_idx++) {
					SRAM_bits[sram_idx + (1 << (j - 1))] = binary_not(
							SRAM_bits[sram_idx]);
				}
			}
			SRAM_bits[1 << j] = '\0';
		}

		dens = (float*) my_realloc(dens, LUT_sizes[i] * sizeof(float));
		prob = (float*) my_realloc(prob, LUT_sizes[i] * sizeof(float));
		for (j = 0; j < LUT_sizes[i]; j++) {
			dens[j] = 1.0 / (float) LUT_sizes[i];
			prob[j] = 0.5;
		}
		power_usage_lut(&sub_power_usage, LUT_sizes[i], 1.0, SRAM_bits, prob,
				dens, power_callib_period);

		t_power_usage power_usage_mux;

		float p[6] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };
		float d[6] = { 1, 1, 1, 1, 1, 1 };
		power_usage_mux_multilevel(&power_usage_mux, power_get_mux_arch(6, 1.0),
				p, d, 0, TRUE, g_solution_inf.T_crit);

		power_add_usage(&sub_power_usage, &power_usage_mux);

		fprintf(g_power_output->out, "%d\t%g\n", LUT_sizes[i],
				power_sum_usage(&sub_power_usage));
	}
//
//	 fprintf(g_power_output->out, "Energy of LUT (No Activity)\n");
//	 for (i = 0; i < (sizeof(LUT_sizes) / sizeof(int)); i++) {
//	 for (j = 1; j <= LUT_sizes[i]; j++) {
//	 SRAM_bits = (char*) my_realloc(SRAM_bits,
//	 ((1 << j) + 1) * sizeof(char));
//	 if (j == 1) {
//	 SRAM_bits[0] = '1';
//	 SRAM_bits[1] = '0';
//	 } else {
//	 for (sram_idx = 0; sram_idx < (1 << (j - 1)); sram_idx++) {
//	 SRAM_bits[sram_idx + (1 << (j - 1))] = binary_not(
//	 SRAM_bits[sram_idx]);
//	 }
//	 }
//	 SRAM_bits[1 << j] = '\0';
//	 }
//
//	 dens = (float*) my_realloc(dens, LUT_sizes[i] * sizeof(float));
//	 prob = (float*) my_realloc(prob, LUT_sizes[i] * sizeof(float));
//	 for (j = 0; j < LUT_sizes[i]; j++) {
//	 dens[j] = 0;
//	 prob[j] = 1;
//	 }
//	 power_usage_lut(&sub_power_usage, LUT_sizes[i], SRAM_bits, prob, dens,
//	 power_callib_period);
//	 fprintf(g_power_output->out, "%d\t%g\n", LUT_sizes[i],
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit * 2);
//	 }
//
	fprintf(g_power_output->out, "Energy of FF (High Activity)\n");
	power_usage_ff(&sub_power_usage, 1.0, 0.5, 3, 0.5, 1, 0.5, 2,
			power_callib_period);
	fprintf(g_power_output->out, "%g\n",
			(sub_power_usage.dynamic + sub_power_usage.leakage));
//
//	 fprintf(g_power_output->out, "Energy of FF (No Activity)\n");
//	 power_usage_ff(&sub_power_usage, 1, 0, 1, 0, 1, 0, power_callib_period);
//	 fprintf(g_power_output->out, "%g\n",
//	 (sub_power_usage.dynamic + sub_power_usage.leakage)
//	 * g_solution_inf.T_crit * 2);
//
//	 fprintf(g_power_output->out, "Energy of SB (High Activity)\n");
//	 for (i = 0; i < (sizeof(sb_buffer_sizes) / sizeof(float)); i++) {
//	 t_power_usage sb_power_usage;
//
//	 power_zero_usage(&sb_power_usage);
//
//	 dens = (float*) my_realloc(dens, sb_mux_sizes[i] * sizeof(float));
//	 prob = (float*) my_realloc(prob, sb_mux_sizes[i] * sizeof(float));
//	 for (j = 0; j < sb_mux_sizes[i]; j++) {
//	 dens[j] = 2;
//	 prob[j] = 0.5;
//	 }
//
//	 power_usage_mux_multilevel(&sub_power_usage,
//	 power_get_mux_arch(sb_mux_sizes[i]), prob, dens, 0, TRUE,
//	 power_callib_period);
//	 power_add_usage(&sb_power_usage, &sub_power_usage);
//
//	 power_usage_buffer(&sub_power_usage, sb_buffer_sizes[i], 0.5, 2, TRUE,
//	 power_callib_period);
//	 power_add_usage(&sb_power_usage, &sub_power_usage);
//
//	 fprintf(g_power_output->out, "%d\t%.0f\t%g\n", sb_mux_sizes[i],
//	 sb_buffer_sizes[i],
//	 (sb_power_usage.dynamic + sb_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//	 }
//
//	 fprintf(g_power_output->out, "Energy of SB (No Activity)\n");
//	 for (i = 0; i < (sizeof(sb_buffer_sizes) / sizeof(float)); i++) {
//	 t_power_usage sb_power_usage;
//
//	 power_zero_usage(&sb_power_usage);
//
//	 dens = (float*) my_realloc(dens, sb_mux_sizes[i] * sizeof(float));
//	 prob = (float*) my_realloc(prob, sb_mux_sizes[i] * sizeof(float));
//	 for (j = 0; j < sb_mux_sizes[i]; j++) {
//	 if (j == 0) {
//	 dens[j] = 0;
//	 prob[j] = 1;
//	 } else {
//	 dens[j] = 0;
//	 prob[j] = 0;
//	 }
//	 }
//
//	 power_usage_mux_multilevel(&sub_power_usage,
//	 power_get_mux_arch(sb_mux_sizes[i]), prob, dens, 0, TRUE,
//	 power_callib_period);
//	 power_add_usage(&sb_power_usage, &sub_power_usage);
//
//	 power_usage_buffer(&sub_power_usage, sb_buffer_sizes[i], 1, 0, TRUE,
//	 power_callib_period);
//	 power_add_usage(&sb_power_usage, &sub_power_usage);
//
//	 fprintf(g_power_output->out, "%d\t%.0f\t%g\n", sb_mux_sizes[i],
//	 sb_buffer_sizes[i],
//	 (sb_power_usage.dynamic + sb_power_usage.leakage)
//	 * g_solution_inf.T_crit);
//}

}

static char binary_not(char c) {
	if (c == '1') {
		return '0';
	} else {
		return '1';
	}
}

float power_usage_buf_for_callibration(int num_inputs, float transistor_size) {
	t_power_usage power_usage;

	assert(num_inputs == 1);

	power_usage_buffer(&power_usage, transistor_size, 0.5, 2.0, FALSE,
			power_callib_period);

	return power_sum_usage(&power_usage);
}

float power_usage_buf_levr_for_callibration(int num_inputs,
		float transistor_size) {
	t_power_usage power_usage;

	assert(num_inputs == 1);

	power_usage_buffer(&power_usage, transistor_size, 0.5, 2.0, TRUE,
			power_callib_period);

	return power_sum_usage(&power_usage);
}

float power_usage_mux_for_callibration(int num_inputs, float transistor_size) {
	t_power_usage power_usage;
	float * dens;
	float * prob;

	dens = (float*) my_malloc(num_inputs * sizeof(float));
	prob = (float*) my_malloc(num_inputs * sizeof(float));
	for (int i = 0; i < num_inputs; i++) {
		dens[i] = 2;
		prob[i] = 0.5;
	}

	power_usage_mux_multilevel(&power_usage,
			power_get_mux_arch(num_inputs, transistor_size), prob, dens, 0,
			FALSE, power_callib_period);

	free(dens);
	free(prob);

	return power_sum_usage(&power_usage);
}

float power_usage_lut_for_callibration(int num_inputs, float transistor_size) {
	t_power_usage power_usage;
	char * SRAM_bits;
	float * dens;
	float * prob;
	int lut_size = num_inputs;

	/* Initialize an SRAM pattern that guarantees the outputs toggle with
	 * every input toggle.
	 */
	SRAM_bits = (char*) my_malloc(((1 << lut_size) + 1) * sizeof(char));
	for (int i = 1; i <= lut_size; i++) {
		if (i == 1) {
			SRAM_bits[0] = '1';
			SRAM_bits[1] = '0';
		} else {
			for (int sram_idx = 0; sram_idx < (1 << (i - 1)); sram_idx++) {
				SRAM_bits[sram_idx + (1 << (i - 1))] = binary_not(
						SRAM_bits[sram_idx]);
			}
		}
		SRAM_bits[1 << i] = '\0';
	}

	dens = (float*) my_malloc(lut_size * sizeof(float));
	prob = (float*) my_malloc(lut_size * sizeof(float));
	for (int i = 0; i < lut_size; i++) {
		dens[i] = 1;
		prob[i] = 0.5;
	}
	power_usage_lut(&power_usage, lut_size, transistor_size, SRAM_bits, prob,
			dens, power_callib_period);

	free(SRAM_bits);
	free(dens);
	free(prob);

	return power_sum_usage(&power_usage);
}

float power_usage_ff_for_callibration(int num_inputs, float transistor_size) {
	t_power_usage power_usage;

	assert(num_inputs == 1);

	power_usage_ff(&power_usage, transistor_size, 0.5, 3, 0.5, 1, 0.5, 2,
			power_callib_period);

	return power_sum_usage(&power_usage);
}

void power_callibrate(void) {
	/* Buffers and Mux must be done before LUT/FF */

	g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER]->callibrate();
	g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_BUFFER_WITH_LEVR]->callibrate();
	g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_MUX]->callibrate();
	g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_LUT]->callibrate();
	g_power_commonly_used->component_callibration[POWER_CALLIB_COMPONENT_FF]->callibrate();
}

void power_print_callibration(void) {
	power_print_title(g_power_output->out, "Callibration Data");
	for (int i = 0; i < POWER_CALLIB_COMPONENT_MAX; i++) {
		g_power_commonly_used->component_callibration[i]->print(g_power_output->out);
	}
}
