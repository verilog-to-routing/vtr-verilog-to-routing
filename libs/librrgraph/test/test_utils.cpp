#include "vpr_error.h"
#include "rr_graph_reader.h"
#include "rr_graph_writer.h"
#include "arch_util.h"
#include <set>

struct t_seg_details {
    int length = 0;
    int start = 0;
    bool longline = 0;
    std::unique_ptr<bool[]> sb;
    std::unique_ptr<bool[]> cb;
    short arch_wire_switch = 0;
    short arch_opin_switch = 0;
    float Rmetal = 0;
    float Cmetal = 0;
    bool twisted = 0;
    enum Direction direction = Direction::NONE;
    int group_start = 0;
    int group_size = 0;
    int seg_start = 0;
    int seg_end = 0;
    int index = 0;
    int abs_index = 0;
    float Cmetal_per_m = 0; ///<Used for power
    std::string type_name;
};

class t_chan_seg_details {
  public:
    t_chan_seg_details() = default;
    t_chan_seg_details(const t_seg_details* init_seg_details)
        : length_(init_seg_details->length)
        , seg_detail_(init_seg_details) {}

  public:
    int length() const { return length_; }
    int seg_start() const { return seg_start_; }
    int seg_end() const { return seg_end_; }

    int start() const { return seg_detail_->start; }
    bool longline() const { return seg_detail_->longline; }

    int group_start() const { return seg_detail_->group_start; }
    int group_size() const { return seg_detail_->group_size; }

    bool cb(int pos) const { return seg_detail_->cb[pos]; }
    bool sb(int pos) const { return seg_detail_->sb[pos]; }

    float Rmetal() const { return seg_detail_->Rmetal; }
    float Cmetal() const { return seg_detail_->Cmetal; }
    float Cmetal_per_m() const { return seg_detail_->Cmetal_per_m; }

    short arch_wire_switch() const { return seg_detail_->arch_wire_switch; }
    short arch_opin_switch() const { return seg_detail_->arch_opin_switch; }

    Direction direction() const { return seg_detail_->direction; }

    int index() const { return seg_detail_->index; }
    int abs_index() const { return seg_detail_->abs_index; }

    const vtr::string_view type_name() const {
        return vtr::string_view(
            seg_detail_->type_name.data(),
            seg_detail_->type_name.size());
    }

  public: //Modifiers
    void set_length(int new_len) { length_ = new_len; }
    void set_seg_start(int new_start) { seg_start_ = new_start; }
    void set_seg_end(int new_end) { seg_end_ = new_end; }

  private:
    //The only unique information about a channel segment is it's start/end
    //and length.  All other information is shared accross segment types,
    //so we use a flyweight to the t_seg_details which defines that info.
    //
    //To preserve the illusion of uniqueness we wrap all t_seg_details members
    //so it appears transparent -- client code of this class doesn't need to
    //know about t_seg_details.
    int length_ = -1;
    int seg_start_ = -1;
    int seg_end_ = -1;
    const t_seg_details* seg_detail_ = nullptr;
};

typedef vtr::NdMatrix<t_chan_seg_details, 3> t_chan_details;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void uniquify_edges(t_rr_edge_info_set& rr_edges_to_create);

static void set_grid_block_type(int priority,
                                const t_physical_tile_type* type,
                                size_t x_root, size_t y_root,
                                vtr::Matrix<t_grid_tile>& grid,
                                vtr::Matrix<int>& grid_priorities,
                                const t_metadata_dict* meta);
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void uniquify_edges(t_rr_edge_info_set& rr_edges_to_create) {
    std::sort(rr_edges_to_create.begin(), rr_edges_to_create.end());
    rr_edges_to_create.erase(std::unique(rr_edges_to_create.begin(), rr_edges_to_create.end()), rr_edges_to_create.end());
}

static void set_grid_block_type(int priority, const t_physical_tile_type* type, size_t x_root, size_t y_root, vtr::Matrix<t_grid_tile>& grid, vtr::Matrix<int>& grid_priorities, const t_metadata_dict* meta) {
    t_physical_tile_type empty_type = get_empty_physical_type();
    t_physical_tile_type_ptr EMPTY_PHYSICAL_TILE_TYPE = &empty_type;
    
    struct TypeLocation {
        TypeLocation(size_t x_val, size_t y_val, const t_physical_tile_type* type_val, int priority_val)
            : x(x_val)
            , y(y_val)
            , type(type_val)
            , priority(priority_val) {}
        size_t x;
        size_t y;
        const t_physical_tile_type* type;
        int priority;

        bool operator<(const TypeLocation& rhs) const {
            return x < rhs.x || y < rhs.y || type < rhs.type;
        }
    };

    //Collect locations effected by this block
    std::set<TypeLocation> target_locations;
    for (size_t x = x_root; x < x_root + type->width; ++x) {
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            target_locations.insert(TypeLocation(x, y, grid[x][y].type, grid_priorities[x][y]));
        }
    }

    //Record the highest priority of all effected locations
    auto iter = target_locations.begin();
    TypeLocation max_priority_type_loc = *iter;
    for (; iter != target_locations.end(); ++iter) {
        if (iter->priority > max_priority_type_loc.priority) {
            max_priority_type_loc = *iter;
        }
    }

    if (priority < max_priority_type_loc.priority) {
        //Lower priority, do not override
#ifdef VERBOSE
        VTR_LOG("Not creating block '%s' at (%zu,%zu) since overlaps block '%s' at (%zu,%zu) with higher priority (%d > %d)\n",
                type->name, x_root, y_root, max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
                max_priority_type_loc.priority, priority);
#endif
        return;
    }

    if (priority == max_priority_type_loc.priority) {
        //Ambiguous case where current grid block and new specification have equal priority
        //
        //We arbitrarily decide to take the 'last applied' wins approach, and warn the user
        //about the potential ambiguity
        VTR_LOG_WARN(
            "Ambiguous block type specification at grid location (%zu,%zu)."
            " Existing block type '%s' at (%zu,%zu) has the same priority (%d) as new overlapping type '%s'."
            " The last specification will apply.\n",
            x_root, y_root,
            max_priority_type_loc.type->name, max_priority_type_loc.x, max_priority_type_loc.y,
            priority, type->name);
    }

    //Mark all the grid tiles 'covered' by this block with the appropriate type
    //and width/height offsets
    std::set<TypeLocation> root_blocks_to_rip_up;

    for (size_t x = x_root; x < x_root + type->width; ++x) {
        VTR_ASSERT(x < grid.end_index(0));

        size_t x_offset = x - x_root;
        for (size_t y = y_root; y < y_root + type->height; ++y) {
            VTR_ASSERT(y < grid.end_index(1));
            size_t y_offset = y - y_root;

            auto& grid_tile = grid[x][y];
            VTR_ASSERT(grid_priorities[x][y] <= priority);

            if (grid_tile.type != nullptr
                && grid_tile.type != EMPTY_PHYSICAL_TILE_TYPE) {
                //We are overriding a non-empty block, we need to be careful
                //to ensure we remove any blocks which will be invalidated when we
                //overwrite part of their locations

                size_t orig_root_x = x - grid[x][y].width_offset;
                size_t orig_root_y = y - grid[x][y].height_offset;

                root_blocks_to_rip_up.insert(TypeLocation(orig_root_x, orig_root_y, grid[x][y].type, grid_priorities[x][y]));
            }

            grid[x][y].type = type;
            grid[x][y].width_offset = x_offset;
            grid[x][y].height_offset = y_offset;
            grid[x][y].meta = meta;

            grid_priorities[x][y] = priority;
        }
    }

    //Rip-up any invalidated blocks
    for (auto invalidated_root : root_blocks_to_rip_up) {
        //Mark all the grid locations used by this root block as empty
        for (size_t x = invalidated_root.x; x < invalidated_root.x + invalidated_root.type->width; ++x) {
            int x_offset = x - invalidated_root.x;
            for (size_t y = invalidated_root.y; y < invalidated_root.y + invalidated_root.type->height; ++y) {
                int y_offset = y - invalidated_root.y;

                if (grid[x][y].type == invalidated_root.type
                    && grid[x][y].width_offset == x_offset
                    && grid[x][y].height_offset == y_offset) {
                    //This is a left-over invalidated block, mark as empty
                    // Note: that we explicitly check the type and offsets, since the original block
                    //       may have been completely overwritten, and we don't want to change anything
                    //       in that case
                    VTR_ASSERT(EMPTY_PHYSICAL_TILE_TYPE->width == 1);
                    VTR_ASSERT(EMPTY_PHYSICAL_TILE_TYPE->height == 1);

#ifdef VERBOSE
                    VTR_LOG("Ripping up block '%s' at (%d,%d) offset (%d,%d). Overlapped by '%s' at (%d,%d)\n",
                            invalidated_root.type->name, invalidated_root.x, invalidated_root.y,
                            x_offset, y_offset,
                            type->name, x_root, y_root);
#endif

                    grid[x][y].type = EMPTY_PHYSICAL_TILE_TYPE;
                    grid[x][y].width_offset = 0;
                    grid[x][y].height_offset = 0;

                    grid_priorities[x][y] = std::numeric_limits<int>::lowest();
                }
            }
        }
    }
}