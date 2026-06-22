#pragma once

#ifndef NO_GRAPHICS

#include <map>
#include <set>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/report/TimingPathCollector.hpp"

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

void draw_crit_path(ezgl::renderer* g);

class DelayLabelDrawer {
    public:
    DelayLabelDrawer() = default;
    void calculate_and_draw_labels(const tatum::TimingPath& path, ezgl::renderer* g);

    private:
    struct t_label_drawing_info {
        float delay_time = 0.0;
        bool hide_label = false;
        int label_transparency = 0;
        double edge_length = 0.0;
        double rotation_angle = 0.0;
        ezgl::rectangle virtual_centered_label_bbox;
        ezgl::rectangle label_bbox;
    };

    struct t_edge_length_info {
        std::size_t edge_idx = 0;
        double edge_length = 0.0;
    };

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

    std::vector<t_label_drawing_info> label_drawing_info;
    std::vector<t_edge_length_info> edge_length_info;

    void update_basic_label_drawing_info(const tatum::TimingPath& path, ezgl::renderer* g);
    void update_least_cluttering_label_pos();
    void update_label_bbox_from_relative_pos(t_label_drawing_info& label_to_update, e_label_relative_pos label_relative_pos);
    bool check_if_bboxes_overlap(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);
    void draw_labels(ezgl::renderer* g);
};

#ifndef NO_SERVER

void draw_crit_path_elements(const std::vector<tatum::TimingPath>& paths, const std::map<std::size_t, std::set<std::size_t>>& indexes, bool draw_crit_path_contour, ezgl::renderer* g);

#endif /* NO_SERVER */
#endif /* NO_GRAPHICS */