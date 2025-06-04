#pragma once

#include "user_place_constraints.h"

///@brief Used to print vpr's floorplanning constraints to an echo file "vpr_constraints.echo"
void echo_constraints(char* filename, const UserPlaceConstraints& constraints);
