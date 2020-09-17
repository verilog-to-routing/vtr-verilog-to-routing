#ifndef REGION_H
#define REGION_H

#include "vtr_strong_id.h"
#include "globals.h"

/**
 * This class stores the data for each constraint region - the region bounds and the subtile bounds.
 */

//Type tag for RegionId
struct region_id_tag;

//A unique identifier for a region
typedef vtr::StrongId<region_id_tag> RegionId;

class Region {
  public: //accessors
  private:
    //may need to include zmin, zmax for future use in 3D FPGA designs
    vtr::Rect<int> region_bounds; //xmin, xmax, ymin, ymax
    int subTilemin, subTilemax;
};

#endif /* REGION_H */
