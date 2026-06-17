#pragma once
/**
 * @file
 * @author  Yulang (Robert) Luo
 * @date    October 2025
 * @brief   The declarations of the AP Draw Manager class which is used
 *          to handle graphics updates during analytical placement.
 */

#include <string>

// Forward declarations
struct PartialPlacement;
struct t_arch;
struct t_vpr_setup;

// Types to indicate the type of drawing operation
enum class APDrawType {
    Solver,
    Legalizer
};

/**
 * @brief Initialize graphics state and allocate draw structures for the AP flow.
 *
 * Must be called once the device grid is valid, before any APDrawManager is created.
 * Kept here (rather than in vpr_flow) because non-AP flows must not set show_graphics
 * before vpr_create_device builds the RR graph and calls init_draw_coords, which
 * accesses tile arrays that alloc_draw_structs has not yet allocated.
 */
void init_ap_graphics(const t_vpr_setup& vpr_setup, const t_arch& arch);

/**
 * @class APDrawManager
 * @brief Manages graphics updates during analytical placement operations.
 *
 * This class provides a clean interface for updating the screen during
 * analytical placement without requiring the placement code to be littered
 * with NO_GRAPHICS conditional compilation directives.
 */
class APDrawManager {
  public:
    /**
     * @brief Constructor initializes the draw manager with a reference to the
     *        current partial placement.
     */
    explicit APDrawManager(const PartialPlacement& p_placement);

    /**
     * @brief Destructor cleans up the reference in the draw state.
     */
    ~APDrawManager();

    /**
     * @brief Update screen with current analytical placement state (non-blocking)
     */
    void update_graphics(unsigned int iteration, enum APDrawType draw_type);

    /**
     * @brief Pause and wait for user interaction before continuing
     */
    void pause(const std::string& msg);
};
