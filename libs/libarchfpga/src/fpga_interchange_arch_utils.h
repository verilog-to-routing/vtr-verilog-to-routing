#ifndef FPGAINTERCHANGE_ARCH_UTILS_FILE_H
#define FPGAINTERCHANGE_ARCH_UTILS_FILE_H

#include "arch_types.h"
#include "arch_error.h"
#include "vtr_error.h"

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "capnp/serialize.h"
#include "capnp/serialize-packed.h"
#include <fcntl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/****************** Utility functions ******************/

/**
 * @brief The FPGA interchange timing model includes three different corners (min, typ and max) for each of the two
 * speed_models (slow and fast).
 *
 * Timing data can be found on PIPs, nodes, site pins and bel pins.
 * This function retrieves the timing value based on the wanted speed model and the wanted corner.
 *
 * Corner model is considered valid if at least one configuration is set.
 * In that case this value shall be returned.
 *
 * More information on the FPGA Interchange timing model can be found here:
 *   - https://github.com/chipsalliance/fpga-interchange-schema/blob/main/interchange/DeviceResources.capnp
 */

float get_corner_value(DeviceResources::Device::CornerModel::Reader model, const char* speed_model, const char* value);

#ifdef __cplusplus
}
#endif

#endif
