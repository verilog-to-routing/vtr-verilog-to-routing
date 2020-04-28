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

	std::unique_ptr<MoveGenerator> move_generator3;
	move_generator3 = std::make_unique<WeightedMedianMoveGenerator>();
	avail_moves.push_back(std::move(move_generator3));

	std::unique_ptr<MoveGenerator> move_generator4;
	move_generator4 = std::make_unique<WeightedCentroidMoveGenerator>();
	avail_moves.push_back(std::move(move_generator4));

	std::unique_ptr<MoveGenerator> move_generator5;
	move_generator5 = std::make_unique<FeasibleRegionMoveGenerator>();
	avail_moves.push_back(std::move(move_generator5));

	std::unique_ptr<MoveGenerator> move_generator6;
	move_generator6 = std::make_unique<CriticalUniformMoveGenerator>();
	avail_moves.push_back(std::move(move_generator6));

	
    moves_prob.push_back(prob[0]);
	for(size_t i =1; i < prob.size(); i++){
		moves_prob.push_back(prob[i]+moves_prob[i-1]);
	}
}


e_create_move StaticMoveGenerator::propose_move(t_pl_blocks_to_be_moved& blocks_affected, float rlim
	, std::vector<int>& X_coord, std::vector<int>& Y_coord, std::vector<int>& num_moves, int& type, int high_fanout_net) {

	float rand_num = vtr::irand(std::accumulate(moves_prob.begin(),moves_prob.end(), 0.0));
	for(size_t i =0; i < moves_prob.size(); i++){
		if(rand_num <= moves_prob[i]){
			++num_moves[i];
			type =i;
			return avail_moves[i]->propose_move(blocks_affected, rlim, X_coord, Y_coord, num_moves , type, high_fanout_net);
		}
	}
	++num_moves[avail_moves.size()-1];
	type = avail_moves.size()-1;
	return avail_moves[avail_moves.size()-1]->propose_move(blocks_affected, rlim, X_coord, Y_coord, num_moves, type, high_fanout_net);
}
