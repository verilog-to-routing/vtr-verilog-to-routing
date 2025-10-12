#pragma once
/**
 * @file
 * @author  Yulang (Robert) Luo
 * @date    October 2025
 * @brief   The decalarations of the Analytical Draw Manager class which is used
 *          to handle graphics updates during analytical placement.
 */

#include <string>

// Forward declarations
class PartialPlacement;

/**
 * @class AnalyticalDrawManager
 * @brief Manages graphics updates during analytical placement operations.
 *
 * This class provides a clean interface for updating the screen during
 * analytical placement without requiring the placement code to be littered
 * with NO_GRAPHICS conditional compilation directives.
 */
class AnalyticalDrawManager {
public:
    /**
     * @brief Constructor initializes the draw manager with a reference to the
     *        current partial placement.
     */
    explicit AnalyticalDrawManager(const PartialPlacement& p_placement);

    /**
     * @brief Destructor cleans up the reference in the draw state.
     */
    ~AnalyticalDrawManager();

    /**
     * @brief Update screen with current analytical placement state
     * @param msg A message to display with the update
     */
    void update_graphics(const std::string& msg);
};
