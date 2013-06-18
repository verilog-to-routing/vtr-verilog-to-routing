/************************* INCLUDES *********************************/
#include <cstring>
#include <cfloat>
using namespace std;

#include <assert.h>

#include "util.h"
#include "PowerSpicedComponent.h"

PowerSpicedComponent::PowerSpicedComponent(float (*fn)(float size)) {
	component_usage = fn;

	/* Always pad with a high and low entry */
	num_entries = 2;
	size_factors = (t_power_callib_pair*) my_malloc(
			num_entries * sizeof(t_power_callib_pair));
	size_factors[0].size = 0;
	size_factors[1].size = FLT_MAX;

	done_callibration = false;
	sorted = true;
}

void PowerSpicedComponent::add_entry(float size, float power) {
	if (done_callibration) {
		assert(0);
	}

	num_entries++;

	size_factors = (t_power_callib_pair*) my_realloc(size_factors,
			(num_entries) * sizeof(t_power_callib_pair));
	assert(size_factors);

	// Thre is always an extra entry at the end for max-value placeholder
	size_factors[num_entries - 2].size = size;
	size_factors[num_entries - 2].power = power;

	sorted = false;
}

int search_callib_pair(const void * k, const void * e) {
	const t_power_callib_pair * key = (const t_power_callib_pair*) k;
	const t_power_callib_pair * elem = (const t_power_callib_pair*) e;

	if (key->size < elem->size)
		return -1;
	else if (key->size >= elem->size && key->size < (elem + 1)->size)
		return 0;
	else
		return 1;
}

float PowerSpicedComponent::scale_factor(float size) {
	t_power_callib_pair * result;
	t_power_callib_pair key;
	float perc_upper;

	if (!done_callibration)
		assert(0);

	if (num_entries <= 2)
		return 1.0;

	key.size = size;
	result = (t_power_callib_pair*) bsearch(&key, size_factors, num_entries - 1,
			sizeof(t_power_callib_pair), search_callib_pair);
	assert(result);

	perc_upper = (size - result->size) / ((result + 1)->size - result->size);

	return perc_upper * (result + 1)->factor + (1 - perc_upper) * result->factor;

}

int compare_callib_pair(const void * a, const void * b) {
	const t_power_callib_pair * a_pair = (const t_power_callib_pair*) a;
	const t_power_callib_pair * b_pair = (const t_power_callib_pair*) b;

	if (a_pair->size < b_pair->size) {
		return -1;
	} else if (a_pair->size == b_pair->size) {
		return 0;
	} else {
		return 1;
	}
}

void PowerSpicedComponent::sort_sizes(void) {
	if (num_entries > 2) {
		qsort(&size_factors[1], num_entries - 2, sizeof(t_power_callib_pair),
				compare_callib_pair);
	}

	sorted = true;
}

void PowerSpicedComponent::callibrate(void) {
	sort_sizes();

	for (int i = 1; i < num_entries - 1; i++) {
		float est_power = (*component_usage)(size_factors[i].size);
		size_factors[i].factor = size_factors[i].power / est_power;
	}

	// Set min-value placeholder
	size_factors[0].size = 0.;
	size_factors[0].factor = size_factors[1].factor;

	// Set max-value placeholder
	size_factors[num_entries - 1].size = FLT_MAX;
	size_factors[num_entries - 1].factor = size_factors[num_entries - 2].factor;

	done_callibration = true;
}

bool PowerSpicedComponent::is_done_callibration(void) {
	return done_callibration;
}
