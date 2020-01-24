#ifndef RR_NODE_H
#define RR_NODE_H

#include "rr_node_fwd.h"
#include "vpr_types.h"

#include "vtr_range.h"

#include <memory>
#include <cstdint>

class t_rr_node {
  public: //Types
    t_rr_node(t_rr_node_storage* storage, RRNodeId id)
        : storage_(storage)
        , id_(id) {}

    //An iterator that dereferences to an edge index
    //
    //Used inconjunction with vtr::Range to return ranges of edge indices
    class edge_idx_iterator : public std::iterator<std::bidirectional_iterator_tag, t_edge_size> {
      public:
        edge_idx_iterator(value_type init)
            : value_(init) {}
        iterator operator++() {
            value_ += 1;
            return *this;
        }
        iterator operator--() {
            value_ -= 1;
            return *this;
        }
        reference operator*() { return value_; }
        pointer operator->() { return &value_; }

        friend bool operator==(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return lhs.value_ == rhs.value_; }
        friend bool operator!=(const edge_idx_iterator lhs, const edge_idx_iterator rhs) { return !(lhs == rhs); }

      private:
        value_type value_;
    };

    typedef vtr::Range<edge_idx_iterator> edge_idx_range;

  public: //Accessors
    t_rr_type type() const;
    const char* type_string() const; /* Retrieve type as a string */

    edge_idx_range edges() const { return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges())); }
    edge_idx_range configurable_edges() const { return vtr::make_range(edge_idx_iterator(0), edge_idx_iterator(num_edges() - num_non_configurable_edges())); }
    edge_idx_range non_configurable_edges() const { return vtr::make_range(edge_idx_iterator(num_edges() - num_non_configurable_edges()), edge_idx_iterator(num_edges())); }

    t_edge_size num_edges() const;
    t_edge_size num_configurable_edges() const;
    t_edge_size num_non_configurable_edges() const;

    int edge_sink_node(t_edge_size iedge) const;
    short edge_switch(t_edge_size iedge) const;

    bool edge_is_configurable(t_edge_size iedge) const;
    t_edge_size fan_in() const;

    short xlow() const;
    short ylow() const;
    short xhigh() const;
    short yhigh() const;
    signed short length() const;

    short capacity() const;

    short ptc_num() const;
    short pin_num() const;   //Same as ptc_num() but checks that type() is consistent
    short track_num() const; //Same as ptc_num() but checks that type() is consistent
    short class_num() const; //Same as ptc_num() but checks that type() is consistent

    short cost_index() const;
    short rc_index() const;
    e_direction direction() const;
    const char* direction_string() const;

    e_side side() const;
    const char* side_string() const;

    float R() const;
    float C() const;

    bool validate() const;

  public: //Mutators
    void set_type(t_rr_type new_type);

    t_edge_size add_edge(int sink_node, int iswitch);

    void shrink_to_fit();

    //Partitions all edges so that configurable and non-configurable edges
    //are organized for efficient access.
    //
    //Must be called before configurable_edges(), non_configurable_edges(),
    //num_configurable_edges(), num_non_configurable_edges() to ensure they
    //are correct.
    void partition_edges();

    void set_num_edges(size_t); //Note will remove any previous edges
    void set_edge_sink_node(t_edge_size iedge, int sink_node);
    void set_edge_switch(t_edge_size iedge, short switch_index);

    void set_fan_in(t_edge_size);

    void set_coordinates(short x1, short y1, short x2, short y2);

    void set_capacity(short);

    void set_ptc_num(short);
    void set_pin_num(short);   //Same as set_ptc_num() by checks type() is consistent
    void set_track_num(short); //Same as set_ptc_num() by checks type() is consistent
    void set_class_num(short); //Same as set_ptc_num() by checks type() is consistent

    void set_cost_index(size_t);
    void set_rc_index(short);

    void set_direction(e_direction);
    void set_side(e_side);

    void next_node() {
        id_ = RRNodeId((size_t)(id_) + 1);
    }

    RRNodeId id() const {
        return id_;
    }

    void prev_node() {
        id_ = RRNodeId((size_t)(id_)-1);
    }

  private: //Data
    t_rr_node_storage* storage_;
    RRNodeId id_;
};

/* Data that is pointed to by the .cost_index member of t_rr_node.  It's     *
 * purpose is to store the base_cost so that it can be quickly changed       *
 * and to store fields that have only a few different values (like           *
 * seg_index) or whose values should be an average over all rr_nodes of a    *
 * certain type (like T_linear etc., which are used to predict remaining     *
 * delay in the timing_driven router).                                       *
 *                                                                           *
 * base_cost:  The basic cost of using an rr_node.                           *
 * ortho_cost_index:  The index of the type of rr_node that generally        *
 *                    connects to this type of rr_node, but runs in the      *
 *                    orthogonal direction (e.g. vertical if the direction   *
 *                    of this member is horizontal).                         *
 * seg_index:  Index into segment_inf of this segment type if this type of   *
 *             rr_node is an CHANX or CHANY; OPEN (-1) otherwise.            *
 * inv_length:  1/length of this type of segment.                            *
 * T_linear:  Delay through N segments of this type is N * T_linear + N^2 *  *
 *            T_quadratic.  For buffered segments all delay is T_linear.     *
 * T_quadratic:  Dominant delay for unbuffered segments, 0 for buffered      *
 *               segments.                                                   *
 * C_load:  Load capacitance seen by the driver for each segment added to    *
 *          the chain driven by the driver.  0 for buffered segments.        */

struct t_rr_indexed_data {
    float base_cost;
    float saved_base_cost;
    int ortho_cost_index;
    int seg_index;
    float inv_length;
    float T_linear;
    float T_quadratic;
    float C_load;
};

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

#include "rr_node_impl.h"

#endif
