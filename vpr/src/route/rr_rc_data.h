#ifndef _RR_RC_DATA_H_
#define _RR_RC_DATA_H_

/*
 * Reistance/Capacitance data for an RR Nodes
 *
 * In practice many RR nodes have the same values, so they are fly-weighted
 * to keep t_rr_node small. Each RR node holds an rc_index which allows
 * retrieval of it's RC data.
 *
 * R:  Resistance to go through an RR node.  This is only metal
 *     resistance (end to end, so conservative) -- it doesn't include the
 *     switch that leads to another rr_node.
 * C:  Total capacitance of an RR node.  Includes metal capacitance, the
 *     input capacitance of all switches hanging off the node, the
 *     output capacitance of all switches to the node, and the connection
 *     box buffer capacitances hanging off it.
 */
struct t_rr_rc_data {
    t_rr_rc_data(float Rval, float Cval) noexcept;

    float R;
    float C;
};

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
