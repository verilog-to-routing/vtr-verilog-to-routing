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

#ifndef __POWER_POWERSPICEDCOMPONENT_NMOS_H__
#define __POWER_POWERSPICEDCOMPONENT_NMOS_H__

#include <vector>
#include <string>

/************************* STRUCTS **********************************/
class PowerSpicedComponent;

class PowerCallibSize {
  public:
    float transistor_size;
    float power;
    float factor;

    PowerCallibSize(float size, float power_)
        : transistor_size(size)
        , power(power_)
        , factor(0.) {
    }
    bool operator<(const PowerCallibSize& rhs) {
        return transistor_size < rhs.transistor_size;
    }
};

class PowerCallibInputs {
  public:
    PowerSpicedComponent* parent;
    int num_inputs;
    std::vector<PowerCallibSize*> entries;
    bool sorted;

    PowerCallibInputs(PowerSpicedComponent* parent, float num_inputs);
    ~PowerCallibInputs();

    void add_size(float transistor_size, float power = 0.);
    PowerCallibSize* get_entry_bound(bool lower, float transistor_size);
    void sort_me();
    bool done_callibration;
    void callibrate();
};

class PowerSpicedComponent {
  public:
    std::string name;
    std::vector<PowerCallibInputs*> entries;

    /* Estimation function for this component */
    float (*component_usage)(int num_inputs, float transistor_size);

    bool sorted;
    bool done_callibration;

    PowerCallibInputs* add_entry(int num_inputs);
    PowerCallibInputs* get_entry(int num_inputs);
    PowerCallibInputs* get_entry_bound(bool lower, int num_inputs);

    PowerSpicedComponent(std::string component_name,
                         float (*usage_fn)(int num_inputs, float transistor_size));
    ~PowerSpicedComponent();

    void add_data_point(int num_inputs, float transistor_size, float power);
    float scale_factor(int num_inputs, float transistor_size);
    void sort_me();

    //	void update_scale_factor(float (*fn)(float size));
    void callibrate();
    bool is_done_callibration();
    void print(FILE* fp);
};

#endif
