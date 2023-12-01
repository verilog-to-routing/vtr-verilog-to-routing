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
    bool apply_move(std::vector<molMoveDescription>& new_locs, t_clustering_data& clustering_data, int thread_id);
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

class quasiDirectedSameTypePackingSwap : public packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedCompatibleTypePackingSwap : public packingMoveGenerator {
  public:
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

class quasiDirectedSameSizePackingSwap : public packingMoveGenerator {
    bool propose_move(std::vector<molMoveDescription>& new_locs);
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs);
};

/************ Moves that evaluate on abosrbed Connections *********************/
class randomConnPackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedConnPackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeConnPackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeConnPackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeConnPackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

/************ Moves that evaluate on abosrbed Terminals *********************/
class randomTerminalPackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedTerminalPackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeTerminalPackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeTerminalPackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeTerminalPackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

/************ Moves that evaluate on abosrbed Terminals and nets *********************/
class randomTerminalNetPackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedTerminalNetPackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeTerminalNetPackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeTerminalNetPackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeTerminalNetPackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

/************ Moves that evaluate on abosrbed Terminals and nets new formula *********************/
class randomTerminalNetNewFormulaPackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedTerminalNetNewFormulaPackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeTerminalNetNewFormulaPackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeTerminalNetNewFormulaPackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeTerminalNetNewFormulaPackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

/************ Moves that evaluate on abosrbed Terminals and nets new formula *********************/
class randomTerminalOutsidePackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedTerminalOutsidePackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeTerminalOutsidePackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeTerminalOutsidePackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeTerminalOutsidePackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

/************ Moves that evaluate on Packing cost function *********************/
class randomCostEvaluationPackingSwap : public randomPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCostEvaluationPackingSwap : public quasiDirectedPackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameTypeCostEvaluationPackingSwap : public quasiDirectedSameTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedCompatibleTypeCostEvaluationPackingSwap : public quasiDirectedCompatibleTypePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};

class quasiDirectedSameSizeCostEvaluationPackingSwap : public quasiDirectedSameSizePackingSwap {
  public:
    bool evaluate_move(const std::vector<molMoveDescription>& new_locs) override;
};
#endif //VTR_PACKINGMOVEGENERATOR_H