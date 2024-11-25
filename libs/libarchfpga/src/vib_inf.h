#ifndef VIB_INF_H
#define VIB_INF_H

#include <functional>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include <map>
#include <unordered_map>
#include <limits>
#include <numeric>
#include <set>
#include <unordered_set>

#include "vtr_ndmatrix.h"
#include "vtr_hash.h"
#include "vtr_bimap.h"
#include "vtr_string_interning.h"
#include "vtr_log.h"

#include "logic_types.h"
#include "clock_types.h"

//#include "physical_types.h"



/* for vib tag */
enum e_parallel_axis_vib {
    X,
    Y,
    BOTH_DIR
};

struct t_seg_group {
    std::string name; 
    e_parallel_axis_vib axis;
    int seg_index;
    int track_num;
};

enum e_multistage_mux_from_or_to_type {
    PB = 0,
    SEGMENT,
    MUX
};

struct t_from_or_to_inf {
    std::string type_name;
    e_multistage_mux_from_or_to_type from_type;  //from_or_to_type
    int type_index = -1;
    int phy_pin_index = -1;
    char seg_dir = ' ';
    int seg_index = -1;
};

struct t_first_stage_mux_inf {
    std::string mux_name; 
    std::vector<std::vector<std::string>> from_tokens;
    std::vector<t_from_or_to_inf> froms;
};

struct t_second_stage_mux_inf : t_first_stage_mux_inf {
    std::vector<std::string> to_tokens;
    std::vector<t_from_or_to_inf> to;    // for io type, port[pin] may map to several sinks
};

// struct t_vib_inf {
//     std::string name;           /* vib name */
//     std::string pbtype_name;    /* pbtype name of vib */
//     int seg_group_num;          /* seg group number of vib */
//     int switch_idx;             /* vib switch index */
//     std::vector<t_seg_group> seg_groups;
//     std::vector<t_first_stage_mux_inf> first_stages;
//     std::vector<t_second_stage_mux_inf> second_stages;
// };

/* VibInf is used to reserve the VIB information.                     *
 * For example, a VIB is described:                                   *
 *  <vib name="vib0" pbtype_name="clb" vib_seg_group="4" arch_vib_switch="mux0">
      <seg_group name="l1" track_nums="20"/>
      <seg_group name="l2" track_nums="20"/>
      <seg_group name="l4" track_nums="16"/>
      <seg_group name="l8" track_nums="16"/>
      <multistage_muxs>
        <first_stage>
          <mux name="MUX0">  <from>L1.E0 L1.E1</from>     </mux>
          <mux name="MUX1">  <from>clb.O[0] L1.E2</from>  </mux>
        </first_stage>
        <second_stage>
          <mux name="MUX-0">  <to>clb.I[0]</to> <from>MUX0 MUX1</from>   </mux>
          <mux name="MUX-1">  <to>L1.N0</to>    <from>MUX0 MUX1</from>   </mux>
        </second_stage>
      </multistage_muxs>
    </vib>
    
   Its corresponding figure is shown:
   
 *                                                 | L1.N0
 *                               +-----------------|-------+
 *         L1.E0-----------------|>|\      MUX-1  _|   vib0|----------\
 *         L1.E1-----------------|>| |----|      /__\      |...        } l1: 20 tracks
 *                               | |/     |       | |      |          /
 *                               | MUX0   |-------| |      |----------\
 *         L1.E2-----------------|>|\     |         |      |...        } l2: 20 tracks
 *                               | | |--| |         |      |          /
 *            |------------------|>|/   | |         |      |----------\
 *            |                  | MUX1 |-|---------|      |...        } l4: 16 tracks
 *            |                  |      | |                |          /
 *            |      ...         |      | |                |----------\
 *            |                  |     _|_|                |...        } l8: 16 tracks
 *            |                  |     \__/ MUX-0          |          /
 *            |                  +-------------------------+
 *        O[0]|                          |       
 *  +-------------+                      |
 *  |             |<----------------------               
 *  |    clb      | I[0]             
 *  +-------------+                                                          
 * 
 * Attributes:  
 * name_: The name of the VIB type, "vib0" in the example. 
 * pbtype_name_: The pbtype of the VIB, "clb" in the example. 
 * seg_group_num_: The number of segment groups. 
 * seg_groups_: The segments applied in the VIB. Their names correspond to segment names in <segmentlist>.
 * switch_name_:The name of the switch type used in the VIB, "mux0" in the example.
 * switch_idx_: The index of corresponding switch in <switchlist>.
 * first_stages_: The info of first stage MUXes, including the names of the MUXes and their from info. 
 * second_stages_: The info of second stage MUXes, including the names of the MUXes and their from/to info.*/

class VibInf {
  public:
    VibInf();

  public:
    void set_name(const std::string name);
    void set_pbtype_name(const std::string pbtype_name);
    void set_seg_group_num(const int seg_group_num);
    void set_switch_idx(const int switch_idx);
    void set_switch_name(const std::string switch_name);
    void set_seg_groups(const std::vector<t_seg_group> seg_groups);
    void push_seg_group(const t_seg_group seg_group);
    void set_first_stages(const std::vector<t_first_stage_mux_inf> first_stages);
    void push_first_stage(const t_first_stage_mux_inf first_stage);
    void set_second_stages(const std::vector<t_second_stage_mux_inf> second_stages);
    void push_second_stage(const t_second_stage_mux_inf second_stage);

    std::string get_name() const;
    std::string get_pbtype_name() const;
    int get_seg_group_num() const;
    int get_switch_idx() const;
    std::string get_switch_name() const;
    std::vector<t_seg_group> get_seg_groups() const;
    std::vector<t_first_stage_mux_inf> get_first_stages() const;
    std::vector<t_second_stage_mux_inf> get_second_stages() const;
    size_t medium_mux_index_by_name(const std::string& name) const;


  private:
    std::string name_;           /* vib name */
    std::string pbtype_name_;    /* pbtype name of vib */
    int seg_group_num_;          /* seg group number of vib */
    int switch_idx_;             /* vib switch index */
    std::string switch_name_;    /* vib switch name */
    std::vector<t_seg_group> seg_groups_;
    std::vector<t_first_stage_mux_inf> first_stages_;
    std::vector<t_second_stage_mux_inf> second_stages_;
};

/************************* VIB_GRID ***********************************/
/* Describe different VIB type on different locations by immitating t_grid_loc_def. */

struct t_vib_grid_loc_spec {
    t_vib_grid_loc_spec(std::string start, std::string end, std::string repeat, std::string incr)
        : start_expr(std::move(start))
        , end_expr(std::move(end))
        , repeat_expr(std::move(repeat))
        , incr_expr(std::move(incr)) {}

    std::string start_expr; //Starting position (inclusive)
    std::string end_expr;   //Ending position (inclusive)

    std::string repeat_expr; //Distance between repeated
                             // region instances

    std::string incr_expr; //Distance between block instantiations
                           // with the region
};

enum VibGridDefType {
    VIB_AUTO,
    VIB_FIXED
};

struct t_vib_grid_loc_def {
    t_vib_grid_loc_def(std::string block_type_val, int priority_val)
        : block_type(block_type_val)
        , priority(priority_val)
        , x("0", "W-1", "max(w+1,W)", "w") //Fill in x direction, no repeat, incr by block width
        , y("0", "H-1", "max(h+1,H)", "h") //Fill in y direction, no repeat, incr by block height
    {}

    std::string block_type; //The block type name

    int priority = 0; //Priority of the specification.
                      // In case of conflicting specifications
                      // the largest priority wins.

    t_vib_grid_loc_spec x; //Horizontal location specification
    t_vib_grid_loc_spec y; //Veritcal location specification
    
};

struct t_vib_layer_def {
    std::vector<t_vib_grid_loc_def> loc_defs; //The list of block location definitions for this layer specification
};

struct t_vib_grid_def {
    VibGridDefType grid_type = VibGridDefType::VIB_AUTO; //The type of this grid specification

    std::string name = ""; //The name of this device

    int width = -1;  //Fixed device width (only valid for grid_type == FIXED)
    int height = -1; //Fixed device height (only valid for grid_type == FIXED)

    float aspect_ratio = 1.; //Aspect ratio for auto-sized devices (only valid for
                             //grid_type == AUTO)
    std::vector<t_vib_layer_def> layers;
};

///@brief DeviceGrid represents the FPGA fabric. It is used to get information about different layers and tiles.
// TODO: All of the function that use helper functions of this class should pass the layer_num to the functions, and the default value of layer_num should be deleted eventually.
class VibDeviceGrid {
  public:
    VibDeviceGrid() = default;
    VibDeviceGrid(std::string grid_name, vtr::NdMatrix<const VibInf*, 3> vib_grid);

    const std::string& name() const { return name_; }

    ///@brief Return the number of layers(number of dies)
    inline int get_num_layers() const {
        return (int)vib_grid_.dim_size(0);
    }

    ///@brief Return the width of the grid at the specified layer
    size_t width() const { return vib_grid_.dim_size(1); }
    ///@brief Return the height of the grid at the specified layer
    size_t height() const { return vib_grid_.dim_size(2); }

    ///@brief Return the size of the flattened grid on the given layer
    inline size_t grid_size() const {
        return vib_grid_.size();
    }

    const VibInf* get_vib(size_t layer, size_t x, size_t y) const {
      return vib_grid_[layer][x][y];
    }

    size_t num_medium_nodes(size_t layer, size_t x, size_t y) const {
      return vib_grid_[layer][x][y]->get_first_stages().size();
    }

    std::string medium_node_name(size_t layer, size_t x, size_t y, size_t medium_index) const {
      return vib_grid_[layer][x][y]->get_first_stages()[medium_index].mux_name;
    }

    std::string vib_pbtype_name(size_t layer, size_t x, size_t y) const {
      return vib_grid_[layer][x][y]->get_pbtype_name();
    }

    bool is_empty() const {
      return vib_grid_.empty();
    }

  private:

    std::string name_;

    /**
     * @brief grid_ is a 3D matrix that represents the grid of the FPGA chip.
     * @note The first dimension is the layer number (grid_[0] corresponds to the bottom layer), the second dimension is the x coordinate, and the third dimension is the y coordinate.
     * @note Note that vtr::Matrix operator[] returns and intermediate type
     * @note which can be used for indexing in the second dimension, allowing
     * @note traditional 2-d indexing to be used
     */
    vtr::NdMatrix<const VibInf*, 3> vib_grid_; //This stores the grid of complex blocks. It is a 3D matrix: [0..num_layers-1][0..grid.width()-1][0..grid_height()-1]

};

#endif
