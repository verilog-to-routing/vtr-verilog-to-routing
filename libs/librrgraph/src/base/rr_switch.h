
#pragma once

#include <string>

#include "physical_types.h"

/**
 * @brief Lists all the important information about an RR switch type.
 *
 * The t_rr_switch_inf describes a switch derived from a switch described
 * by t_arch_switch_inf. This indirection allows us to vary properties of a
 * given switch, such as varying delay with switch fan-in.
  */
struct t_rr_switch_inf {
    /// Equivalent resistance of the buffer/switch.
    float R = 0.;
    /// Input capacitance.
    float Cin = 0.;
    /// Output capacitance.
    float Cout = 0.;
    /// Internal capacitance.
    float Cinternal = 0.;
    /// Intrinsic delay.  The delay through an unloaded switch is Tdel + R * Cout.
    float Tdel = 0.;
    /// The area of each transistor in the segment's driving mux measured in minimum width transistor units
    float mux_trans_size = 0.;
    /// The area of the buffer. If set to zero, area should be calculated from R
    float buf_size = 0.;
    std::string name;
    e_power_buffer_type power_buffer_type = POWER_BUFFER_TYPE_UNDEFINED;
    float power_buffer_size = 0.;

    /// Indicate whether this rr_switch is a switch type used inside clusters.
    /// These switch types are not specified in the architecture description file
    /// and are added when flat router is enabled.
    bool intra_tile = false;

  public: // Getters
    /// Returns the type of switch
    e_switch_type type() const;

    /// Returns true if this switch type isolates its input and output into
    /// separate DC-connected subcircuits
    bool buffered() const;

    /// Returns true if this switch type is configurable (i.e. can the switch can be turned on or off)
    /// This allows modelling of non-optional switches (e.g. fixed buffers, or shorted connections)
    /// which must be used (e.g. expanded by the router) if a connected segment is used.
    bool configurable() const;

    bool operator==(const t_rr_switch_inf& other) const;

    /**
     * @brief Functor for computing a hash value for t_rr_switch_inf.
     *
     * This custom hasher enables the use of t_rr_switch_inf objects as keys
     * in unordered containers such as std::unordered_map or std::unordered_set.
     */
    struct Hasher {
        std::size_t operator()(const t_rr_switch_inf& s) const;
    };

  public: // Setters
    void set_type(e_switch_type type_val);

  private:
    e_switch_type type_ = e_switch_type::INVALID;
};