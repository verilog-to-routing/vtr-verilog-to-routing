## Uses of xlow, xhigh, ylow, yhigh

| Function | Use Scenario | One Usage (Location) | Count |
| --       | --           | --       | -- |
| xlow | retrieve physical tile from device_ctx.grid | test_fasm.cpp:193 | 16 |
| xlow | Obtain length of wire | stats.cpp:313 | 3 |
| xlow | Determine switchpoint | draw.cpp:1475 | 1 |
| xlow | Draw from one edge to the other | draw.cpp:1934 | 1 |
| xlow | Create bounding box | draw.cpp:2083 | 1 |
| xlow | Check if nodes are adjacent | check_route.cpp:481 | 1 |
| xlow | Check if node is initialized | check_route.cpp:64 | 3 |
| xlow | Include x value in console output | check_route.cpp:258 | 5 |
| xlow | Create a serial num for the wire  x value in console output | check_route.cpp:198 | 3 |
| xlow | Count cblocks | rr_graph_area.cpp:229 | 3 |
| xlow | Return the segment number (distance along the channel) of the connection box from from_rr_type (CHANX or CHANY) to to_node (IPIN) | rr_graph_util.cpp:21 | 1 |
| xlow | Check direction going from one node to another| rr_graph_util.cpp:45 | 1 |
| xlow | Write Coordinates | VprTimingGraphResolver.cpp:345 | 1 |
<br><br><br>

## Retrieve Physical Tile from device_ctx.grid
```cpp
static std::string get_pin_feature (size_t inode) {
    auto& device_ctx = g_vpr_ctx.device();

    // Get tile physical tile and the pin number
    int ilow = device_ctx.rr_nodes[inode].xlow(); // <----------------------------- Use of xlow()
    int jlow = device_ctx.rr_nodes[inode].ylow();
    auto physical_tile = device_ctx.grid[ilow][jlow].type;
    int pin_num = device_ctx.rr_nodes[inode].ptc_num();

    // Get the sub tile (type, not instance) and index of its pin that matches
    // the node index.
    const t_sub_tile* sub_tile_type = nullptr;
    int sub_tile_pin = -1;

    for (auto& sub_tile : physical_tile->sub_tiles) {
        auto max_inst_pins = sub_tile.num_phy_pins / sub_tile.capacity.total();
        for (int pin = 0; pin < sub_tile.num_phy_pins; pin++) {
            if (sub_tile.sub_tile_to_tile_pin_indices[pin] == pin_num) {
                sub_tile_type = &sub_tile;
                sub_tile_pin  = pin % max_inst_pins;
                break;
            }
        }

        if (sub_tile_type != nullptr) {
            break;
        }
    }
```

<br><br><br>  

## Obtain length of wire
```cpp
while (tptr != nullptr) {
        inode = tptr->index;
        curr_type = device_ctx.rr_nodes[inode].type();

        if (curr_type == SINK) { /* Starting a new segment */
            tptr = tptr->next;   /* Link to existing path - don't add to len. */
            if (tptr == nullptr)
                break;

            curr_type = device_ctx.rr_nodes[tptr->index].type();
        }

        else if (curr_type == CHANX || curr_type == CHANY) {
            segments++;
            length += 1 + device_ctx.rr_nodes[inode].xhigh() - device_ctx.rr_nodes[inode].xlow() // <----------------------------- Use of xlow()
                      + device_ctx.rr_nodes[inode].yhigh() - device_ctx.rr_nodes[inode].ylow();

            if (curr_type != prev_type && (prev_type == CHANX || prev_type == CHANY))
                bends++;
        }

        prev_type = curr_type;
        tptr = tptr->next;
    }

    *bends_ptr = bends;
    *len_ptr = length;
    *segments_ptr = segments;
}
```