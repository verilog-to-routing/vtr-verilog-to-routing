
#include "physical_types.h"



/************ Define Class Functions ************/

/* Overload < operator for Switchblock_Lookup class (used to index into a std::map) */
bool Switchblock_Lookup::operator < (const Switchblock_Lookup &obj) const {
	bool result;

	/* Implements a hierarchy of inequality. For instance, if x1 < x2 then
	   obj1 < obj2. But if x1==x2 then we have to check y1 and y2, and so on */
	if (x_coord < obj.x_coord){
		result = true;
	} else {
		if (x_coord == obj.x_coord){
			if (y_coord < obj.y_coord){
				result = true;
			} else {
				if (y_coord == obj.y_coord){
					if (from_side < obj.from_side){
						result = true;
					} else {
						if (from_side == obj.from_side){
							if (to_side < obj.to_side){
								result = true;
							} else {
								if (to_side == obj.to_side){
									if (from_track < obj.from_track){
										result = true;
									} else {
										result = false;
									}
								} else {
									result = false;
								}
							}
						} else {
							result = false;
						}
					}
				} else {
					result = false;
				}
			}
		} else {
			result = false;
		}
	}
	
	return result;
}
