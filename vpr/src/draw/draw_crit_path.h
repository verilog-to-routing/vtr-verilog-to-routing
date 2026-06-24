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
 * 
 * @param g Pointer to the ezgl::renderer object
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
     * @param g Pointer to the ezgl::renderer object.
     */
    void calculate_and_draw_labels(const tatum::TimingPath& path, ezgl::renderer* g);

    private:

    /**
     * @brief Contains all attributes of one timing edge delay label needed for calculating the least cluttered label position.
     */
    struct t_label_drawing_info {
        /// @brief Delay time across this timing edge, in seconds.
        float delay_time = 0.0;

        /// @brief True when the label should not be drawn.
        bool hide_label = false;

        /// @brief Alpha value used when drawing the label.
        int label_transparency = 0;

        /// @brief Length of the associated timing-edge flyline in world units.
        double edge_length = 0.0;

        /// @brief Label rotation angle (the same for the corresponding flyline) in degrees.
        double rotation_angle = 0.0;

        /// @brief Label bounding box centered on the timing edge before offsets are applied.
        ezgl::rectangle virtual_centered_label_bbox;

        /// @brief Final label bounding box after position offsets are applied.
        ezgl::rectangle label_bbox;
    };

    /**
     * @brief Candidate positions for placing a delay label relative to its timing-edge flyline.
     */
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

    /// @brief Per-edge label drawing information for the current timing path.
    std::vector<t_label_drawing_info> label_drawing_info_;

    /**
     * @brief Calculates delay, visibility, rotation, and initial bounding-box information for each timing edge.
     * 
     * The calculated results are updated per-edge to label_drawing_info_.
     *
     * @param path Timing path whose consecutive node pairs define the timing edges to place delay labels.
     * @param label_drawing_info Per-edge label drawing information to update.
     * @param pixels_per_world_unit The ratio between pixels and world units spanning the screen width.
     * Used to perform screen-to-world conversions for label bounding boxes that primarily use pixels.
     */
    static void calculate_basic_label_drawing_info_(const tatum::TimingPath& path,
                                                    std::vector<t_label_drawing_info>& label_drawing_info_,
                                                    double pixels_per_world_unit);

    /**
     * @brief Chooses label positions that result in the least number of overlaps between visible labels.
     *
     * The chosen bounding box positions are updated per-edge to the variable label_bbox in label_drawing_info_.
     * 
     * @param label_drawing_info Per-edge label drawing information to update.
     * @param pixels_per_world_unit The ratio between pixels and world units spanning the screen width.
     * Passed to a helper calculate_label_bbox_from_relative_pos_() to perform screen-to-world conversions
     * for label bounding boxes that primarily use pixels.
     */
    static void calculate_least_cluttered_label_pos_(std::vector<t_label_drawing_info>& label_drawing_info_,
                                                     double pixels_per_world_unit);

    /**
     * @brief Hides labels that still overlap after calculate_least_cluttered_label_pos_() has tried all candidates.
     * 
     * Information about hidden labels are updated per-edge to to the variable hide_label in label_drawing_info_.
     *
     * @param label_drawing_info Per-edge label drawing information to update.
     */
    static void hide_still_cluttered_labels_(std::vector<t_label_drawing_info>& label_drawing_info_);

    /**
     * @brief Calculates a label bounding box using the requested position relative to its timing-edge flyline.
     *
     * @param label_to_update Single Label drawing information whose bounding box should be updated.
     * @param label_relative_pos Candidate position to apply relative to the timing-edge flyline.
     * @param pixels_per_world_unit The ratio between pixels and world units spanning the screen width.
     * Used to perform screen-to-world conversions for label bounding boxes that primarily use pixels.
     */
    static void calculate_label_bbox_from_relative_pos_(t_label_drawing_info& label_to_update, e_label_relative_pos label_relative_pos,
                                                        double pixels_per_world_unit);

    /**
     * @brief Returns true if two label bounding boxes overlap.
     *
     * @param bbox1 First bounding box to compare.
     * @param bbox2 Second bounding box to compare.
     */
    static bool check_if_bboxes_overlap_(const ezgl::rectangle& bbox1, const ezgl::rectangle& bbox2);

    /**
     * @brief Draws all non-hidden delay labels in styles specified by the per-edge label drawing information.
     *
     * @param label_drawing_info Per-edge label drawing information to query for drawing.
     * @param g Pointer to the ezgl::renderer object.
     */
    static void draw_labels_(std::vector<t_label_drawing_info>& label_drawing_info_, ezgl::renderer* g);
};

#ifndef NO_SERVER

/**
 * @brief Draw critical path elements.
 * 
 * This function draws critical path elements based on the provided timing paths
 * and indexes map. It is primarily used in server mode, where items are drawn upon request.
 *
 * @param paths The vector of TimingPath objects representing the critical paths.
 * @param indexes The map of sets, where the map keys are path indices in std::vector<tatum::TimingPath>, and each set contains the indices of the data_arrival_path elements ( @ref tatum::TimingPath ) to draw.
 * @param g Pointer to the ezgl::renderer object on which the elements will be drawn.
 */
void draw_crit_path_elements(const std::vector<tatum::TimingPath>& paths, const std::map<std::size_t, std::set<std::size_t>>& indexes, bool draw_crit_path_contour, ezgl::renderer* g);

#endif /* NO_SERVER */
#endif /* NO_GRAPHICS */