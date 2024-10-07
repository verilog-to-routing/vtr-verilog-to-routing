/********************************************************************
 * Memeber function for class SideManagerManager
 *******************************************************************/
#include "openfpga_side_manager.h"

/* Constructors */
SideManager::SideManager(enum e_side side) {
    side_ = side;
}

SideManager::SideManager() {
    side_ = NUM_2D_SIDES;
}

SideManager::SideManager(size_t side) {
    set_side(side);
}

/* Public Accessors */
enum e_side SideManager::get_side() const {
    return side_;
}

enum e_side SideManager::get_opposite() const {
    switch (side_) {
        case TOP:
            return BOTTOM;
        case RIGHT:
            return LEFT;
        case BOTTOM:
            return TOP;
        case LEFT:
            return RIGHT;
        default:
            return NUM_2D_SIDES;
    }
}

enum e_side SideManager::get_rotate_clockwise() const {
    switch (side_) {
        case TOP:
            return RIGHT;
        case RIGHT:
            return BOTTOM;
        case BOTTOM:
            return LEFT;
        case LEFT:
            return TOP;
        default:
            return NUM_2D_SIDES;
    }
}

enum e_side SideManager::get_rotate_counterclockwise() const {
    switch (side_) {
        case TOP:
            return LEFT;
        case RIGHT:
            return TOP;
        case BOTTOM:
            return RIGHT;
        case LEFT:
            return BOTTOM;
        default:
            return NUM_2D_SIDES;
    }
}

bool SideManager::validate() const {
    if (NUM_2D_SIDES == side_) {
        return false;
    }
    return true;
}

size_t SideManager::to_size_t() const {
    switch (side_) {
        case TOP:
            return 0;
        case RIGHT:
            return 1;
        case BOTTOM:
            return 2;
        case LEFT:
            return 3;
        default:
            return 4;
    }
}

/* Convert to char* */
const char* SideManager::c_str() const {
    switch (side_) {
        case TOP:
            return "top";
        case RIGHT:
            return "right";
        case BOTTOM:
            return "bottom";
        case LEFT:
            return "left";
        default:
            return "invalid_side";
    }
}

/* Convert to char* */
std::string SideManager::to_string() const {
    std::string ret;
    switch (side_) {
        case TOP:
            ret.assign("top");
            break;
        case RIGHT:
            ret.assign("right");
            break;
        case BOTTOM:
            ret.assign("bottom");
            break;
        case LEFT:
            ret.assign("left");
            break;
        default:
            ret.assign("invalid_side");
            break;
    }

    return ret;
}

/* Public Mutators */
void SideManager::set_side(size_t side) {
    switch (side) {
        case 0:
            side_ = TOP;
            return;
        case 1:
            side_ = RIGHT;
            return;
        case 2:
            side_ = BOTTOM;
            return;
        case 3:
            side_ = LEFT;
            return;
        default:
            side_ = NUM_2D_SIDES;
            return;
    }
}

void SideManager::set_side(enum e_side side) {
    side_ = side;
    return;
}

void SideManager::set_opposite() {
    side_ = get_opposite();
    return;
}

void SideManager::rotate_clockwise() {
    side_ = get_rotate_clockwise();
    return;
}

void SideManager::rotate_counterclockwise() {
    side_ = get_rotate_counterclockwise();
    return;
}
