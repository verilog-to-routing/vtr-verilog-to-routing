#ifndef OPENFPGA_SIDE_MANAGER_H
#define OPENFPGA_SIDE_MANAGER_H

/********************************************************************
 * Include header files that are required by function declaration
 *******************************************************************/
#include <cstddef>
#include <string>

/* Header files form archfpga library */
#include "physical_types.h"

/* namespace openfpga begins */
namespace openfpga {

/********************************************************************
 * Define a class for the sides of a physical block in FPGA architecture
 * Basically, each block has four sides :
 * TOP, RIGHT, BOTTOM, LEFT  
 * This class aims to provide a easy proctol for manipulating a side 
 ********************************************************************/

class SideManager {
  public: /* Constructor */
    SideManager(enum e_side side);
    SideManager();
    SideManager(size_t side);
  public: /* Accessors */
    enum e_side get_side() const;
    enum e_side get_opposite() const;
    enum e_side get_rotate_clockwise() const;
    enum e_side get_rotate_counterclockwise() const;
    bool validate() const;
    size_t to_size_t() const;
    const char* c_str() const;
    std::string to_string() const;
  public: /* Mutators */
    void set_side(size_t side);
    void set_side(enum e_side side);
    void set_opposite();
    void rotate_clockwise();
    void rotate_counterclockwise();
  private: /* internal data */
    enum e_side side_;  
};

} /* namespace openfpga ends */

#endif
