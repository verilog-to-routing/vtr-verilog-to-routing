#ifndef _RR_RC_DATA_H_
#define _RR_RC_DATA_H_

#include "rr_node_types.h"

/*
 * Returns the index to a t_rr_rc_data matching the specified values.
 *
 * If an existing t_rr_rc_data matches the specified R/C it's index
 * is returned, otherwise the t_rr_rc_data is created.
 *
 * The returned indicies index into DeviceContext.rr_rc_data.
 */
short find_create_rr_rc_data(const float R, const float C);

#endif /* _RR_RC_DATA_H_ */
