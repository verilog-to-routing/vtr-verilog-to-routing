#ifndef LOAD_FLAT_PLACE_H
#define LOAD_FLAT_PLACE_H

#include "vpr_types.h"

/**
 * @brief A function that prints a flat placement file
 */
void print_flat_placement(const char* flat_place_file);

/**
 * @brief A function that loads and legalizes a flat placement file
 */
bool load_flat_placement(t_vpr_setup& vpr_setup, const t_arch& arch);

#endif
