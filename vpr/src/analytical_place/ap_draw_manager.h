#pragma once
/**
 * @file
 * @author  Yulang (Robert) Luo
 * @date    October 2025
 * @brief   The declarations of the AP Draw Manager class which is used
 *          to handle graphics updates during analytical placement.
 */

#include <string>
#include <memory>
#include "ap_netlist_utils.h"

// Forward declarations
class Prepacker;
class PreClusterTimingManager;
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
     * @brief Constructor initializes a owned lookup from atom block to AP block and sets references to it
     * and the current partial placement.
     */
    explicit APDrawManager(const AtomNetlist& atom_netlist, const APNetlist& ap_netlist, const Prepacker& prepacker, const PartialPlacement& p_placement);

    /**
     * @brief Destructor cleans up the reference in the draw state.
     */
    ~APDrawManager();

    /**
     * @brief Update screen with current analytical placement state (non-blocking)
     */
    void update_graphics(unsigned int iteration, enum APDrawType draw_type);

    /**
     * @brief Pause and wait at the initial setup scene before any solving begins.
     * 
     * Inside the function, the shared timing info pointer is passed to draw state
     * to prepare for critical path drawing.
     */
    void pause_at_initial_scene(const std::string& msg, PreClusterTimingManager& timing_manager);

    /**
     * @brief Pause and wait at the final global placement scene.
     */
    void pause_at_final_scene(const std::string& msg);

  private:
#ifndef NO_GRAPHICS
    /**
     * @brief Lookup used to map an atom block id to an AP block id.
     */
    AtomBlockAPBlockLookup atom_block_ap_block_lookup_;
#endif
};
