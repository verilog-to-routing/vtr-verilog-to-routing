//
// Created by amin on 9/15/23.
//

#ifndef VTR_PLACE_RE_CLUSTER_H
#define VTR_PLACE_RE_CLUSTER_H

#include "timing_place.h"

class PlaceReCluster {
  public:
    PlaceReCluster() = default;

    void re_cluster(const t_place_algorithm& place_algorithm,
                    const PlaceDelayModel* delay_model,
                    PlacerCriticalities* criticalities);
};

#endif //VTR_PLACE_RE_CLUSTER_H
