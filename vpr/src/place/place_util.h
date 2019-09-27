#ifndef PLACE_UTIL_H
#define PLACE_UTIL_H
#include <string>

//Initialize the placement context
void init_placement_context();

//Records a reasons for an aborted move
void log_move_abort(std::string reason);

//Prints a breif report about aborted move reasons and counts
void report_aborted_moves();
#endif
