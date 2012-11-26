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

#ifndef __POWER_POWERSPICEDCOMPONENT_H__
#define __POWER_POWERSPICEDCOMPONENT_H__

/************************* STRUCTS **********************************/
typedef struct s_power_callib_pair t_power_callib_pair;
struct s_power_callib_pair {
	float size;
	float power;
	float factor;
};

class PowerSpicedComponent {
	int num_entries;
	t_power_callib_pair * size_factors;

	/* Estimation function for this component */
	float (*component_usage)(float size);

	bool sorted;
	bool done_callibration;

	//float slope;
	//float yint;

	void sort_sizes(void);

public:
	PowerSpicedComponent(float (*fn)(float size));
	void add_entry(float size, float power);
	void update_scale_factor(float (*fn)(float size));
	float scale_factor(float size);
	void callibrate(void);
	bool is_done_callibration(void);
};

#endif
