#pragma once

#ifndef NO_GRAPHICS

#include <map>
#include <set>

#include "tatum/TimingConstraintsFwd.hpp"
#include "tatum/report/TimingPathCollector.hpp"

#include "ezgl/point.hpp"
#include "ezgl/graphics.hpp"

/** 
 * @brief Draws the critical path if Crit. Path (in the GUI) is selected.
 * 
 * Each stage between primitive pins is shown in a different colour.
 * User can toggle between two different visualizations:
 * a) during placement, critical path only shown as flylines
 * b) during routing, critical path is shown by both flylines and routed net connections.
 */
void draw_crit_path(ezgl::renderer* g);

/**
 * @brief Calculates and draws delay labels for critical path flylines.
 *
 * DelayLabelDrawer places one delay label per timing edge in a timing
 * path. It computes information of each label like visibility and rotation angle, 
 * and store it in a vector which a place algorithm can later query. 
 * The algorithm then tries different label positions to reduce overlap between labels.
 */
class DelayLabelDrawer {
    public:
    DelayLabelDrawer() = default;

    /**
     * @brief Calculate label positions that give the least number of overlaps and thrn draws all visible delay labels.
     *
     * @param path Timing path whose consecutive node pairs define the timing edges to place delay labels.
     */
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

    struct t_label_overlap_info {
        std::size_t edge_idx = 0;
        int num_overlaps = 0;
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

    std::vector<t_label_drawing_info> label_drawing_info_;

    static void update_basic_label_drawing_info_(const tatum::TimingPath& path,
                                                std::vector<t_label_drawing_info>& label_drawing_info_,
                                                double pixels_per_world_unit);

    static void update_least_cluttered_label_pos_(std::vector<t_label_drawing_info>& label_drawing_info_,
                                                double pixels_per_world_unit);

    static void hide_still_cluttered_labels_(std::vector<t_label_drawing_info>& label_drawing_info_);

    static void update_more_centered_label_pos_(std::vector<t_label_drawing_info>& label_drawing_info_,
                                                double pixels_per_world_unit);

    static void update_label_bbox_from_relative_pos_(t_label_drawing_info& label_to_update, e_label_relative_pos label_relative_pos,
                                                double pixels_per_world_unit);

    static bool check_if_bboxes_overlap_(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);

    static double get_bboxes_overlap_area_(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);

    static void draw_labels_(std::vector<t_label_drawing_info>& label_drawing_info_, ezgl::renderer* g);
};

#ifndef NO_SERVER

void draw_crit_path_elements(const std::vector<tatum::TimingPath>& paths, const std::map<std::size_t, std::set<std::size_t>>& indexes, bool draw_crit_path_contour, ezgl::renderer* g);

#endif /* NO_SERVER */
#endif /* NO_GRAPHICS */