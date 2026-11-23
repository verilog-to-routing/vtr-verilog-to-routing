#pragma once

#include "user_place_constraints.h"
#include "user_relative_macros.h"

///@brief Used to print vpr's floorplanning constraints and relative placement macros to an echo file "vpr_constraints.echo"
void echo_constraints(char* filename, const UserPlaceConstraints& constraints, const UserRelativeMacros& relative_macros);
