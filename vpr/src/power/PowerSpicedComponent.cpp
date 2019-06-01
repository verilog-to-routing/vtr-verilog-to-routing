/************************* INCLUDES *********************************/
#include <cstring>
#include <cfloat>
#include <limits>
#include <algorithm>
#include <string>

using namespace std;

#include "vtr_assert.h"

#include "vpr_error.h"
#include "PowerSpicedComponent.h"

bool sorter_PowerCallibSize(PowerCallibSize* a, PowerCallibSize* b);
bool sorter_PowerCallibInputs(PowerCallibInputs* a, PowerCallibInputs* b);

PowerCallibInputs::PowerCallibInputs(PowerSpicedComponent* parent_,
                                     float inputs)
    : parent(parent_)
    , num_inputs(inputs)
    , sorted(false)
    , done_callibration(false) {
    /* Add min/max bounding entries */
    add_size(0);
    add_size(std::numeric_limits<float>::max());
}

PowerCallibInputs::~PowerCallibInputs() {
    for (auto entry : entries) {
        delete entry;
    }
}

void PowerCallibInputs::add_size(float transistor_size, float power) {
    PowerCallibSize* entry = new PowerCallibSize(transistor_size, power);
    entries.push_back(entry);
    sorted = false;
}

bool sorter_PowerCallibSize(PowerCallibSize* a, PowerCallibSize* b) {
    return a->transistor_size < b->transistor_size;
}

void PowerCallibInputs::sort_me() {
    sort(entries.begin(), entries.end(), sorter_PowerCallibSize);
    sorted = true;
}

void PowerCallibInputs::callibrate() {
    VTR_ASSERT(entries.size() >= 2);

    for (vector<PowerCallibSize*>::iterator it = entries.begin() + 1;
         it != entries.end() - 1; it++) {
        float est_power = parent->component_usage(num_inputs,
                                                  (*it)->transistor_size);
        (*it)->factor = (*it)->power / est_power;
    }

    /* Set min-value placeholder */
    entries[0]->factor = entries[1]->factor;

    /* Set max-value placeholder */
    entries[entries.size() - 1]->factor = entries[entries.size() - 2]->factor;

    done_callibration = true;
}

PowerCallibSize* PowerCallibInputs::get_entry_bound(bool lower,
                                                    float transistor_size) {
    PowerCallibSize* prev = entries[0];

    VTR_ASSERT(sorted);
    for (vector<PowerCallibSize*>::iterator it = entries.begin() + 1;
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

    done_callibration = false;
    sorted = true;
}

PowerSpicedComponent::~PowerSpicedComponent() {
    for (auto entry : entries) {
        delete entry;
    }
}

PowerCallibInputs* PowerSpicedComponent::add_entry(int num_inputs) {
    PowerCallibInputs* entry = new PowerCallibInputs(this, num_inputs);
    entries.push_back(entry);
    return entry;
}

PowerCallibInputs* PowerSpicedComponent::get_entry(int num_inputs) {
    vector<PowerCallibInputs*>::iterator it;

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

PowerCallibInputs* PowerSpicedComponent::get_entry_bound(bool lower,
                                                         int num_inputs) {
    PowerCallibInputs* prev = entries[0];

    VTR_ASSERT(sorted);
    for (vector<PowerCallibInputs*>::iterator it = entries.begin() + 1;
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
    VTR_ASSERT(!done_callibration);
    PowerCallibInputs* inputs_entry = get_entry(num_inputs);
    inputs_entry->add_size(transistor_size, power);
    sorted = false;
}

float PowerSpicedComponent::scale_factor(int num_inputs,
                                         float transistor_size) {
    PowerCallibInputs* inputs_lower;
    PowerCallibInputs* inputs_upper;

    PowerCallibSize* size_lower;
    PowerCallibSize* size_upper;

    float factor_lower = 0.;
    float factor_upper = 0.;
    float factor;

    float perc_upper;

    VTR_ASSERT(done_callibration);

    inputs_lower = get_entry_bound(true, num_inputs);
    inputs_upper = get_entry_bound(false, num_inputs);

    if (inputs_lower) {
        /* Interpolation of factor between sizes for lower # inputs */
        VTR_ASSERT(inputs_lower->done_callibration);
        size_lower = inputs_lower->get_entry_bound(true, transistor_size);
        size_upper = inputs_lower->get_entry_bound(false, transistor_size);

        if (size_lower && size_upper) {
            perc_upper = (transistor_size - size_lower->transistor_size)
                         / (size_upper->transistor_size - size_lower->transistor_size);
            factor_lower = perc_upper * size_upper->factor
                           + (1 - perc_upper) * size_lower->factor;
        } else {
            VPR_THROW(VPR_ERROR_POWER, "Failed to interpolate transitor size");
        }
    }

    if (inputs_upper) {
        /* Interpolation of factor between sizes for upper # inputs */
        VTR_ASSERT(inputs_upper->done_callibration);
        size_lower = inputs_upper->get_entry_bound(true, transistor_size);
        size_upper = inputs_upper->get_entry_bound(false, transistor_size);

        if (size_lower && size_upper) {
            perc_upper = (transistor_size - size_lower->transistor_size)
                         / (size_upper->transistor_size - size_lower->transistor_size);
            factor_upper = perc_upper * size_upper->factor
                           + (1 - perc_upper) * size_lower->factor;
        } else {
            VPR_THROW(VPR_ERROR_POWER, "Failed to interpolate transitor size");
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

bool sorter_PowerCallibInputs(PowerCallibInputs* a, PowerCallibInputs* b) {
    return a->num_inputs < b->num_inputs;
}

void PowerSpicedComponent::sort_me() {
    sort(entries.begin(), entries.end(), sorter_PowerCallibInputs);

    for (vector<PowerCallibInputs*>::iterator it = entries.begin();
         it != entries.end(); it++) {
        (*it)->sort_me();
    }
    sorted = true;
}

void PowerSpicedComponent::callibrate() {
    sort_me();

    for (vector<PowerCallibInputs*>::iterator it = entries.begin();
         it != entries.end(); it++) {
        (*it)->callibrate();
    }
    done_callibration = true;
}

bool PowerSpicedComponent::is_done_callibration() {
    return done_callibration;
}

void PowerSpicedComponent::print(FILE* fp) {
    fprintf(fp, "%s\n", name.c_str());
    for (vector<PowerCallibInputs*>::iterator it = entries.begin() + 1;
         it != entries.end() - 1; it++) {
        fprintf(fp, "Num Inputs: %d\n", (*it)->num_inputs);
        for (vector<PowerCallibSize*>::iterator it2 = (*it)->entries.begin()
                                                      + 1;
             it2 != (*it)->entries.end() - 1; it2++) {
            fprintf(fp, "  Transistor Size: %6f Factor: %3f\n",
                    (*it2)->transistor_size, (*it2)->factor);
        }
    }
}
