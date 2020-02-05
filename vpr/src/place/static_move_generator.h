#ifndef VPR_STATIC_MOVE_GEN_H
#define VPR_STATIC_MOVE_GEN_H
#include "move_generator.h"
#include "median_move_generator.h"
#include "uniform_move_generator.h"

class StaticMoveGenerator : public MoveGenerator {
private:
	std::vector<std::unique_ptr<MoveGenerator>> avail_moves;
	std::vector<float> moves_prob ;
	//= {0.1,0.9};

public:
	StaticMoveGenerator(const std::vector<float> & prob);
    e_create_move propose_move(t_pl_blocks_to_be_moved& affected_blocks, float rlim);
};
#endif
