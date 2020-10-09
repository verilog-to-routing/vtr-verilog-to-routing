#include "region.h"

//sentinel value for indicating that a subtile has not been specified
int NO_SUBTILE = -1;

Region::Region(){
	sub_tile = NO_SUBTILE;
}

int Region::get_xmin(){
	return region_bounds.xmin();
}

int Region::get_xmax(){
	return region_bounds.xmax();
}

int Region::get_ymin(){
	return region_bounds.ymin();
}

int Region::get_ymax(){
	return region_bounds.ymax();
}

void Region::set_region_rect(int _xmin, int _ymin, int _xmax, int _ymax){
	region_bounds.set_xmin(_xmin);
	region_bounds.set_xmax(_xmax);
	region_bounds.set_ymin(_ymin);
	region_bounds.set_ymax(_ymax);
}

int Region::get_sub_tile(){
	return sub_tile;
}

void Region::set_sub_tile(int _sub_tile){
	sub_tile = _sub_tile;
}

bool Region::do_regions_intersect(Region region){
	bool intersect = true;

	int x_min_1 = region.get_xmin();
	int x_min_2 = region_bounds.xmin();
	int int_x_min = std::max(x_min_1, x_min_2);

	int y_min_1 = region.get_ymin();
	int y_min_2 = region_bounds.ymin();
	int int_y_min = std::max(y_min_1, y_min_2);

	int x_max_1 = region.get_xmax();
	int x_max_2 = region_bounds.xmax();
	int int_x_max = std::min(x_max_1, x_max_2);

	int y_max_1 = region.get_ymax();
	int y_max_2 = region_bounds.ymax();
	int int_y_max = std::min(y_max_1, y_max_2);

	//check if rectangles dimensions are invalid
	if (int_x_min > int_x_max || int_y_min > int_y_max){
		intersect = false;
	}

	return intersect;
}

Region Region::regions_intersection(Region region){
	Region intersect;

	int x_min_1 = region.get_xmin();
	int x_min_2 = region_bounds.xmin();
	int int_x_min = std::max(x_min_1, x_min_2);

	int y_min_1 = region.get_ymin();
	int y_min_2 = region_bounds.ymin();
	int int_y_min = std::max(y_min_1, y_min_2);

	int x_max_1 = region.get_xmax();
	int x_max_2 = region_bounds.xmax();
	int int_x_max = std::min(x_max_1, x_max_2);

	int y_max_1 = region.get_ymax();
	int y_max_2 = region_bounds.ymax();
	int int_y_max = std::min(y_max_1, y_max_2);

	intersect.set_region_rect(int_x_min, int_y_min, int_x_max, int_y_max);

	return intersect;

}

bool Region::locked(){
	bool locked = false;
	bool x_matches = false;
	bool y_matches = false;

	if (region_bounds.xmin() == region_bounds.xmax()){
		x_matches = true;
	}

	if (region_bounds.ymin() == region_bounds.ymax()){
		y_matches = true;
	}


	if (x_matches && y_matches && sub_tile != NO_SUBTILE){
		locked = true;
	}

	return locked;
}
