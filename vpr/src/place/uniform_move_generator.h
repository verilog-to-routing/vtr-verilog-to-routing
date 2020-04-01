#ifndef VPR_UNIFORM_MOVE_GEN_H
#define VPR_UNIFORM_MOVE_GEN_H
#include "move_generator.h"

class UniformMoveGenerator : public MoveGenerator {
public:
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim,
    	std::vector<int>& , std::vector<int>& ,  std::vector<int>&, int & itype, int, const std::vector<std::vector<ClusterBlockId>>& );
    /*UniformMoveGenerator();
  private:
      std::vector<std::vector<ClusterBlockId>> blocks_by_type_;
      */
};

#endif
