#pragma once

#ifndef NO_GRAPHICS

#include "tatum/report/TimingPathCollector.hpp"
#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

void draw_crit_path(ezgl::renderer* g);
void draw_timing_edges(ezgl::renderer* g);
void draw_routed_timing_connections(ezgl::renderer* g);


class DelayLabelDrawer {
    public:
    DelayLabelDrawer(const tatum::TimingPath& path, const int num_timing_edges, ezgl::renderer* g);
    draw_delay_labels(ezgl::renderer* g);

    private:
    struct t_delay_label_info {
        double edge_length = 0.0;
        bool skip_delay_text = false;
        ezgl::rectangle virtual_centered_text_bbox;
        ezgl::rectangle delay_text_bbox;
        double text_angle = 0.0;
    };

    struct t_overlap_info {
        int edge_index = 0;
        int num_overlaps = 0;
    }

    enum class e_label_relative_pos {
        CENTER_ABOVE,
        CENTER_BELOW,
        LEFT_ABOVE,
        LEFT_BELOW,
        RIGHT_ABOVE,
        RIGHT_BELOW,
        FAR_LEFT_ABOVE,
        FAR_LEFT_BELOW,
        FAR_RIGHT_ABOVE,
        FAR_RIGHT_BELOW,
    };

    std::vector<t_delay_label_info> delay_label_info;
    std::vector<t_overlap_info> init_overlap_info;
    
    void update_label_drawing_info();
    void update_label_bbox_from_relative_pos(e_label_relative_pos label_relative_pos);
    bool check_if_bboxes_overlap(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);
    void update_init_num_overlaps();
    void update_least_cluttering_label_locs();
};

#endif /* NO_GRAPHICS */