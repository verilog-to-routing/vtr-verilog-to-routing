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

#include "logic_types.h"
#include "clock_types.h"

//#include "physical_types.h"



/* for vib tag */
struct t_seg_group {
    std::string name; 
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
    std::vector<t_from_or_to_inf> froms;
};

struct t_second_stage_mux_inf : t_first_stage_mux_inf {
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

class VibInf {
  public:
    VibInf();

  public:
    void set_name(const std::string name);
    void set_pbtype_name(const std::string pbtype_name);
    void set_seg_group_num(const int seg_group_num);
    void set_switch_idx(const int switch_idx);
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
    std::vector<t_seg_group> get_seg_groups() const;
    std::vector<t_first_stage_mux_inf> get_first_stages() const;
    std::vector<t_second_stage_mux_inf> get_second_stages() const;


  private:
    std::string name_;           /* vib name */
    std::string pbtype_name_;    /* pbtype name of vib */
    int seg_group_num_;          /* seg group number of vib */
    int switch_idx_;             /* vib switch index */
    std::vector<t_seg_group> seg_groups_;
    std::vector<t_first_stage_mux_inf> first_stages_;
    std::vector<t_second_stage_mux_inf> second_stages_;
};

#endif
