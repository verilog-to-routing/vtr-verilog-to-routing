#include "vib_inf.h"
// #include "vtr_math.h"
// #include "vtr_util.h"
// #include "vtr_log.h"

// #include "arch_util.h"

VibInf::VibInf() {
    name_.clear();
    pbtype_name_.clear();
    seg_group_num_ = 0;
    switch_idx_ = -1;
    seg_groups_.clear();
    first_stages_.clear();
    second_stages_.clear();
}

void VibInf::set_name(const std::string name) {
    VTR_ASSERT(!name.empty());
    name_ = name;
}

void VibInf::set_pbtype_name(const std::string pbtype_name) {
    VTR_ASSERT(!pbtype_name.empty());
    pbtype_name_ = pbtype_name;
}

void VibInf::set_seg_group_num(const int seg_group_num) {
    VTR_ASSERT(seg_group_num >= 0);
    seg_group_num_ = seg_group_num;
}

void VibInf::set_switch_idx(const int switch_idx) {
    VTR_ASSERT(switch_idx != -1);
    switch_idx_ = switch_idx;
}

void VibInf::set_seg_groups(const std::vector<t_seg_group> seg_groups) {
    VTR_ASSERT(!seg_groups.empty());
    for(auto seg_group : seg_groups) {
        seg_groups_.push_back(seg_group);
    }
}

void VibInf::push_seg_group(const t_seg_group seg_group) {
    VTR_ASSERT(!seg_group.name.empty());
    seg_groups_.push_back(seg_group);
}

void VibInf::set_first_stages(const std::vector<t_first_stage_mux_inf> first_stages) {
    VTR_ASSERT(!first_stages.empty());
    for(auto first_stage : first_stages) {
        first_stages_.push_back(first_stage);
    }
}

void VibInf::push_first_stage(const t_first_stage_mux_inf first_stage) {
    VTR_ASSERT(!first_stage.mux_name.empty());
    first_stages_.push_back(first_stage);
}

void VibInf::set_second_stages(const std::vector<t_second_stage_mux_inf> second_stages) {
    VTR_ASSERT(!second_stages.empty());
    for(auto second_stage : second_stages) {
        second_stages_.push_back(second_stage);
    }
}

void VibInf::push_second_stage(const t_second_stage_mux_inf second_stage) {
    VTR_ASSERT(!second_stage.mux_name.empty());
    second_stages_.push_back(second_stage);
}



std::string VibInf::get_name() const{
    VTR_ASSERT(!name_.empty());
    return name_;
}

std::string VibInf::get_pbtype_name() const{
    VTR_ASSERT(!pbtype_name_.empty());
    return pbtype_name_;
}

int VibInf::get_seg_group_num() const{
    VTR_ASSERT(seg_group_num_ >= 0);
    return seg_group_num_;
}

int VibInf::get_switch_idx() const{
    VTR_ASSERT(switch_idx_ != -1);
    return switch_idx_;
}

std::vector<t_seg_group> VibInf::get_seg_groups() const{
    VTR_ASSERT(!seg_groups_.empty());
    return seg_groups_;
}

std::vector<t_first_stage_mux_inf> VibInf::get_first_stages() const{
    VTR_ASSERT(!first_stages_.empty());
    return first_stages_;
}

std::vector<t_second_stage_mux_inf> VibInf::get_second_stages() const{
    VTR_ASSERT(!second_stages_.empty());
    return second_stages_;
}
