//
// Created by elgammal on 2022-07-28.
//

#ifndef VTR_PACKINGMOVEGENERATOR_H
#define VTR_PACKINGMOVEGENERATOR_H

#include "vpr_types.h"
#include "cluster_util.h"
#include "pack_move_utils.h"


/**
 * @brief a base class for packing move generators
 *
 * This class represents the base class for all move generators.
 */
class packingMoveGenerator {
  public:
    //Propose
    virtual ~packingMoveGenerator() = default;
    virtual bool propose_move(std::vector<molMoveDescription>& new_locs) = 0;
    virtual bool evaluate_move(const std::vector<molMoveDescription>& new_locs) = 0;
    bool apply_move(std::vector<molMoveDescription>& new_locs, t_clustering_data& clustering_data);
};

class randomPackingSwap : public packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedPackingSwap : public packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedSameTypePackingSwap : public  packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedCompatibleTypePackingSwap : public  packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedSameTypeSameSizePackingSwap : public  packingMoveGenerator {
  bool propose_move(std::vector<molMoveDescription>& new_locs);
  bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedCompatibleTypeSameSizePackingSwap : public  packingMoveGenerator {
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};
#endif //VTR_PACKINGMOVEGENERATOR_H