#include "static_move_generator.h"
#include "globals.h"
#include <algorithm>

#include "vtr_random.h"

StaticMoveGenerator::StaticMoveGenerator(const std::vector<float> & prob){
	std::unique_ptr<MoveGenerator> move_generator;
	move_generator = std::make_unique<UniformMoveGenerator>();
	avail_moves.push_back(std::move(move_generator));

	std::unique_ptr<MoveGenerator> move_generator2;
	move_generator2 = std::make_unique<MedianMoveGenerator>();
	avail_moves.push_back(std::move(move_generator2));

	moves_prob = prob;
}


e_create_move StaticMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim) {

    std::vector<double> accumulated_prob;
	accumulated_prob.push_back(moves_prob[0]);
	for(size_t i =1; i < moves_prob.size(); i++)
		accumulated_prob.push_back(moves_prob[i]+accumulated_prob[i-1]);

	double rand_num = vtr::irand(100)/100.0;
	for(size_t i =0; i < accumulated_prob.size(); i++){
		if(rand_num <= accumulated_prob[i])
			return avail_moves[i]->propose_move(blocks_affected, rlim);
	}
	return avail_moves[avail_moves.size()-1]->propose_move(blocks_affected, rlim);
}
