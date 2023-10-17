/**
 * @file placer_globals.h
 * @brief Declares the accessor variable for key global
 *        structs that are used everywhere in VPR placer.
 */

#pragma once
#include "placer_context.h"

/* This defines the error tolerance for floating points variables used in *
 * cost computation. 0.01 means that there is a 1% error tolerance.       */
#define ERROR_TOL .01

extern PlacerContext g_placer_ctx;
