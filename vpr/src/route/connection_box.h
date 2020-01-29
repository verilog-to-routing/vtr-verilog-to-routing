#ifndef CONNECTION_BOX_H
#define CONNECTION_BOX_H
// Some routing graphs have connectivity driven by types of connection boxes.
// This class relates IPIN rr nodes with connection box type and locations, used
// for connection box driven map lookahead.

#include <map>
#include <string>
#include <tuple>

#include "vtr_flat_map.h"
#include "vtr_range.h"
#include "vtr_strong_id.h"

struct connection_box_tag {};
typedef vtr::StrongId<connection_box_tag> ConnectionBoxId;

struct ConnectionBox {
    std::string name;
};

struct ConnBoxLoc {
    ConnBoxLoc()
        : box_location(std::make_pair(-1, -1)) {}
    ConnBoxLoc(
        const std::pair<size_t, size_t>& a_box_location,
        float a_site_pin_delay,
        ConnectionBoxId a_box_id)
        : box_location(a_box_location)
        , site_pin_delay(a_site_pin_delay)
        , box_id(a_box_id) {}

    std::pair<size_t, size_t> box_location;
    float site_pin_delay;
    ConnectionBoxId box_id;
};

struct SinkToIpin {
    int ipin_nodes[4];
    int ipin_count;
};

class ConnectionBoxes {
  public:
    ConnectionBoxes();

    size_t num_connection_box_types() const;
    std::pair<size_t, size_t> connection_box_grid_size() const;
    const ConnectionBox* get_connection_box(ConnectionBoxId box) const;

    bool find_connection_box(int inode,
                             ConnectionBoxId* box_id,
                             std::pair<size_t, size_t>* box_location,
                             float* site_pin_delay) const;
    const std::pair<size_t, size_t>* find_canonical_loc(int inode) const;

    // Clear IPIN map and set connection box grid size and box ids.
    void clear();
    void reset_boxes(std::pair<size_t, size_t> size,
                     const std::vector<ConnectionBox> boxes);
    void resize_nodes(size_t rr_node_size);

    void add_connection_box(int inode, ConnectionBoxId box_id, std::pair<size_t, size_t> box_location, float site_pin_delay);
    void add_canonical_loc(int inode, std::pair<size_t, size_t> loc);

    // Create map from SINK's back to IPIN's
    //
    // This must be called after all connection boxes have been added.
    void create_sink_back_ref();
    const SinkToIpin& find_sink_connection_boxes(
        int inode) const;

  private:
    std::pair<size_t, size_t> size_;
    std::vector<ConnectionBox> boxes_;
    std::vector<ConnBoxLoc> ipin_map_;
    std::vector<SinkToIpin> sink_to_ipin_;
    std::vector<std::pair<size_t, size_t>>
        canonical_loc_map_;
};

#endif
