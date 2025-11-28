/************************* INCLUDES *********************************/
#include <cstring>
#include <cfloat>
#include <limits>
#include <algorithm>
#include <string>

#include "vtr_assert.h"

#include "vpr_error.h"
#include "PowerSpicedComponent.h"

bool sorter_PowerCalibSize(PowerCalibSize* a, PowerCalibSize* b);
bool sorter_PowerCalibInputs(PowerCalibInputs* a, PowerCalibInputs* b);

PowerCalibInputs::PowerCalibInputs(PowerSpicedComponent* parent_,
                                     float inputs)
    : parent(parent_)
    , num_inputs(inputs)
    , sorted(false)
    , done_calibration(false) {
    /* Add min/max bounding entries */
    add_size(0);
    add_size(std::numeric_limits<float>::max());
}

PowerCalibInputs::~PowerCalibInputs() {
    for (auto entry : entries) {
        delete entry;
    }
}

void PowerCalibInputs::add_size(float transistor_size, float power) {
    PowerCalibSize* entry = new PowerCalibSize(transistor_size, power);
    entries.push_back(entry);
    sorted = false;
}

bool sorter_PowerCalibSize(PowerCalibSize* a, PowerCalibSize* b) {
    return a->transistor_size < b->transistor_size;
}

void PowerCalibInputs::sort_me() {
    sort(entries.begin(), entries.end(), sorter_PowerCalibSize);
    sorted = true;
}

void PowerCalibInputs::calibrate() {
    VTR_ASSERT(entries.size() >= 2);

    for (std::vector<PowerCalibSize*>::iterator it = entries.begin() + 1;
         it != entries.end() - 1; it++) {
        float est_power = parent->component_usage(num_inputs,
                                                  (*it)->transistor_size);
        (*it)->factor = (*it)->power / est_power;
    }

    /* Set min-value placeholder */
    entries[0]->factor = entries[1]->factor;

    /* Set max-value placeholder */
    entries[entries.size() - 1]->factor = entries[entries.size() - 2]->factor;

    done_calibration = true;
}

PowerCalibSize* PowerCalibInputs::get_entry_bound(bool lower,
                                                    float transistor_size) {
    PowerCalibSize* prev = entries[0];

    VTR_ASSERT(sorted);
    for (std::vector<PowerCalibSize*>::iterator it = entries.begin() + 1;
         it != entries.end(); it++) {
        if ((*it)->transistor_size > transistor_size) {
            if (lower)
                return prev;
            else
                return *it;
        }
        prev = *it;
    }
    return nullptr;
}

PowerSpicedComponent::PowerSpicedComponent(std::string component_name,
                                           float (*usage_fn)(int num_inputs, float transistor_size)) {
    name = component_name;
    component_usage = usage_fn;

    /* Always pad with a high and low entry */
    add_entry(0);
    //	add_entry(std::numeric_limits<int>::max());
    add_entry(1000000000);

    done_calibration = false;
    sorted = true;
}

PowerSpicedComponent::~PowerSpicedComponent() {
    for (auto entry : entries) {
        delete entry;
    }
}

PowerCalibInputs* PowerSpicedComponent::add_entry(int num_inputs) {
    PowerCalibInputs* entry = new PowerCalibInputs(this, num_inputs);
    entries.push_back(entry);
    return entry;
}

PowerCalibInputs* PowerSpicedComponent::get_entry(int num_inputs) {
    std::vector<PowerCalibInputs*>::iterator it;

    for (it = entries.begin(); it != entries.end(); it++) {
        if ((*it)->num_inputs == num_inputs) {
            break;
        }
    }

    if (it == entries.end()) {
        return add_entry(num_inputs);
    } else {
        return *it;
    }
}

PowerCalibInputs* PowerSpicedComponent::get_entry_bound(bool lower,
                                                         int num_inputs) {
    PowerCalibInputs* prev = entries[0];

    VTR_ASSERT(sorted);
    for (std::vector<PowerCalibInputs*>::iterator it = entries.begin() + 1;
         it != entries.end(); it++) {
        if ((*it)->num_inputs > num_inputs) {
            if (lower) {
                if (prev == entries[0])
                    return nullptr;
                else
                    return prev;
            } else {
                if (*it == entries[entries.size() - 1])
                    return nullptr;
                else
                    return *it;
            }
        }
        prev = *it;
    }
    return nullptr;
}

void PowerSpicedComponent::add_data_point(int num_inputs, float transistor_size, float power) {
    VTR_ASSERT(!done_calibration);
    PowerCalibInputs* inputs_entry = get_entry(num_inputs);
    inputs_entry->add_size(transistor_size, power);
    sorted = false;
}

float PowerSpicedComponent::scale_factor(int num_inputs,
                                         float transistor_size) {
    PowerCalibInputs* inputs_lower;
    PowerCalibInputs* inputs_upper;

    PowerCalibSize* size_lower;
    PowerCalibSize* size_upper;

    float factor_lower = 0.;
    float factor_upper = 0.;
    float factor;

    float perc_upper;

    VTR_ASSERT(done_calibration);

    inputs_lower = get_entry_bound(true, num_inputs);
    inputs_upper = get_entry_bound(false, num_inputs);

    if (inputs_lower) {
        /* Interpolation of factor between sizes for lower # inputs */
        VTR_ASSERT(inputs_lower->done_calibration);
        size_lower = inputs_lower->get_entry_bound(true, transistor_size);
        size_upper = inputs_lower->get_entry_bound(false, transistor_size);

        if (size_lower && size_upper) {
            perc_upper = (transistor_size - size_lower->transistor_size)
                         / (size_upper->transistor_size - size_lower->transistor_size);
            factor_lower = perc_upper * size_upper->factor
                           + (1 - perc_upper) * size_lower->factor;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_POWER, "Failed to interpolate transistor size");
        }
    }

    if (inputs_upper) {
        /* Interpolation of factor between sizes for upper # inputs */
        VTR_ASSERT(inputs_upper->done_calibration);
        size_lower = inputs_upper->get_entry_bound(true, transistor_size);
        size_upper = inputs_upper->get_entry_bound(false, transistor_size);

        if (size_lower && size_upper) {
            perc_upper = (transistor_size - size_lower->transistor_size)
                         / (size_upper->transistor_size - size_lower->transistor_size);
            factor_upper = perc_upper * size_upper->factor
                           + (1 - perc_upper) * size_lower->factor;
        } else {
            VPR_FATAL_ERROR(VPR_ERROR_POWER, "Failed to interpolate transistor size");
        }
    }

    if (!inputs_lower) {
        factor = factor_upper;
    } else if (!inputs_upper) {
        factor = factor_lower;
    } else {
        /* Interpolation of factor between inputs */
        perc_upper = ((float)(num_inputs - inputs_lower->num_inputs))
                     / ((float)(inputs_upper->num_inputs
                                - inputs_lower->num_inputs));
        factor = perc_upper * factor_upper + (1 - perc_upper) * factor_lower;
    }
    return factor;
}

bool sorter_PowerCalibInputs(PowerCalibInputs* a, PowerCalibInputs* b) {
    return a->num_inputs < b->num_inputs;
}

void PowerSpicedComponent::sort_me() {
    sort(entries.begin(), entries.end(), sorter_PowerCalibInputs);

    for (std::vector<PowerCalibInputs*>::iterator it = entries.begin();
         it != entries.end(); it++) {
        (*it)->sort_me();
    }
    sorted = true;
}

void PowerSpicedComponent::calibrate() {
    sort_me();

    for (std::vector<PowerCalibInputs*>::iterator it = entries.begin();
         it != entries.end(); it++) {
        (*it)->calibrate();
    }
    done_calibration = true;
}

bool PowerSpicedComponent::is_done_calibration() {
    return done_calibration;
}

void PowerSpicedComponent::print(FILE* fp) {
    fprintf(fp, "%s\n", name.c_str());
    for (std::vector<PowerCalibInputs*>::iterator it = entries.begin() + 1;
         it != entries.end() - 1; it++) {
        fprintf(fp, "Num Inputs: %d\n", (*it)->num_inputs);
        for (std::vector<PowerCalibSize*>::iterator it2 = (*it)->entries.begin()
                                                           + 1;
             it2 != (*it)->entries.end() - 1; it2++) {
            fprintf(fp, "  Transistor Size: %6f Factor: %3f\n",
                    (*it2)->transistor_size, (*it2)->factor);
        }
    }
}
