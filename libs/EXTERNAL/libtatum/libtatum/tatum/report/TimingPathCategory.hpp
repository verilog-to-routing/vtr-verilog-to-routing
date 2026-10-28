#pragma once

/**
 * @file
 * @brief Classification of timing paths by the kind of their startpoint and endpoint
 *        (e.g. in2reg, reg2reg), and a filter used to select which categories of paths
 *        are collected and reported.
 */

#include <climits>
#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string>

#include "tatum/TimingGraphFwd.hpp"
#include "tatum/report/TimingPathFwd.hpp"

namespace tatum {

struct SkewPath;

/**
 * @brief The category of a timing path, based on the kind of its startpoint and endpoint.
 *
 * A 'reg' startpoint/endpoint is any sequential SOURCE/SINK (i.e. one with an incoming
 * clock launch/capture edge), which includes flip-flops and the clocked ports of hard
 * blocks (e.g. RAMs). An 'in'/'out' startpoint/endpoint is any SOURCE/SINK without a
 * clock edge, such as a primary input/output. Paths launched by constant generators or
 * clock sources (e.g. clocks used as data) are therefore also 'in' paths.
 */
enum class TimingPathCategory : unsigned char {
    IN2REG,        ///< Primary input to register
    REG2REG,       ///< Register to register
    REG2OUT,       ///< Register to primary output
    IN2OUT,        ///< Primary input to primary output (purely combinational)
    NUM_CATEGORIES ///< The number of categories (not a valid category)
};

/// @brief Writes the category's name (e.g. "reg2reg") to the stream.
std::ostream& operator<<(std::ostream& os, const TimingPathCategory category);

/// @brief Returns the category's name (e.g. "reg2reg").
std::string to_string(const TimingPathCategory category);

/**
 * @brief Returns true if the given startpoint is launched by a clock (e.g. a flip-flop output).
 *
 *   @param timing_graph    The timing graph containing the startpoint.
 *   @param startpoint      The startpoint of a timing path. Must be a SOURCE node.
 */
bool is_register_startpoint(const TimingGraph& timing_graph, const NodeId startpoint);

/**
 * @brief Returns true if the given endpoint is captured by a clock (e.g. a flip-flop input).
 *
 *   @param timing_graph    The timing graph containing the endpoint.
 *   @param endpoint        The endpoint of a timing path. Must be a SINK node.
 */
bool is_register_endpoint(const TimingGraph& timing_graph, const NodeId endpoint);

/**
 * @brief Returns the category of a path with the given kinds of startpoint and endpoint.
 *
 *   @param start_is_register   True if the path starts at a register (otherwise a primary input).
 *   @param end_is_register     True if the path ends at a register (otherwise a primary output).
 */
TimingPathCategory make_timing_path_category(bool start_is_register, bool end_is_register);

/**
 * @brief Classifies the timing path between the given startpoint and endpoint.
 *
 *   @param timing_graph    The timing graph containing the path.
 *   @param startpoint      The SOURCE node which launches the path.
 *   @param endpoint        The SINK node which captures the path.
 */
TimingPathCategory classify_timing_path(const TimingGraph& timing_graph, const NodeId startpoint, const NodeId endpoint);

/**
 * @brief Classifies a timing path using its startpoint and endpoint.
 *
 * The path's startpoint must be valid (i.e. the path must have been fully traced, as is
 * the case for paths from the TimingPathCollector). This is not the case for the paths
 * returned by find_critical_paths(), which do not record their startpoint.
 *
 *   @param timing_graph    The timing graph containing the path.
 *   @param path_info       The path to classify.
 */
TimingPathCategory classify_timing_path(const TimingGraph& timing_graph, const TimingPathInfo& path_info);

/**
 * @brief Classifies a skew path using its data launch and capture nodes.
 *
 *   @param timing_graph    The timing graph containing the path.
 *   @param skew_path       The path to classify.
 */
TimingPathCategory classify_skew_path(const TimingGraph& timing_graph, const SkewPath& skew_path);

/**
 * @brief A set of timing path categories, used to select which paths are collected and reported.
 *
 * A default-constructed filter accepts all categories. For example, to select only the
 * paths which end at registers:
 *
 *      TimingPathCategoryFilter filter({TimingPathCategory::IN2REG, TimingPathCategory::REG2REG});
 */
class TimingPathCategoryFilter {
  public:
    /// @brief Creates a filter which accepts all categories.
    TimingPathCategoryFilter();

    /// @brief Creates a filter which accepts only the given categories.
    TimingPathCategoryFilter(std::initializer_list<TimingPathCategory> categories);

    /// @brief Returns a filter which accepts all categories.
    static TimingPathCategoryFilter all();

    /// @brief Returns a filter which accepts no categories.
    static TimingPathCategoryFilter none();

  public:
    /// @brief Returns true if this filter accepts the given category.
    bool contains(const TimingPathCategory category) const;

    /// @brief Returns true if this filter accepts every category.
    bool accepts_all() const;

    /**
     * @brief Returns true if any category accepted by this filter ends at an endpoint of the given kind.
     *
     * Since an endpoint's kind is known without tracing its path, this can be used to skip
     * endpoints before (expensively) tracing their paths back to the startpoint.
     *
     *   @param end_is_register     True for an endpoint which is a register, false for a primary output.
     */
    bool accepts_endpoint(bool end_is_register) const;

    /// @brief Returns the accepted categories as a comma separated list (e.g. "in2reg, reg2reg").
    std::string to_string() const;

  public:
    /// @brief Adds the given category to the filter.
    TimingPathCategoryFilter& add(const TimingPathCategory category);

    /// @brief Removes the given category from the filter.
    TimingPathCategoryFilter& remove(const TimingPathCategory category);

  private:
    /// @brief Returns the bit used to represent the given category in categories_.
    static unsigned char category_bit_(const TimingPathCategory category);

    /// @brief Bit set of the accepted categories, indexed by category.
    unsigned char categories_ = 0;

    // Each category is stored as a single bit of categories_, so there can be at most as
    // many categories as categories_ has bits. Without this check, adding too many
    // categories would silently shift bits out of the mask, and the filter would quietly
    // drop (or wrongly accept) paths in the extra categories.
    //
    // If more categories are needed, widen the bit set: change the type of categories_
    // and the return type of category_bit_() to a larger unsigned integer type (e.g.
    // uint32_t), or switch both to a std::bitset<NUM_CATEGORIES>.
    static_assert(size_t(TimingPathCategory::NUM_CATEGORIES) <= CHAR_BIT * sizeof(categories_),
                  "Too many TimingPathCategory values to fit in TimingPathCategoryFilter's bit set; widen categories_ (see comment above)");
};

} // namespace tatum
