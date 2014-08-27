#include <map>

typedef map<coeff_t, pair<int,int> > addmap_t;
typedef addmap_t::const_iterator     amiter_t;

void compute_all_sumdiffs(addmap_t &result, coeff_t num1, coeff_t num2);

void _compute_all_sumdiffs(bool is_left, addmap_t &result, coeff_t num1, coeff_t num2) {
    int shift;
    /* we store shift+1 to make possible differentiating +/- of unshifted number.
       so shift 0 is actually 1, and shift n is actually n+1 */
    #define SUM(shift)  (is_left ? pair<int,int>( shift+1, 1)  : pair<int,int>(1,  shift+1))
    #define DIFF(shift) (is_left ? pair<int,int>(-shift-1, 1)  : pair<int,int>(1, -shift-1))
    #define RDIFF(shift) (is_left ? pair<int,int>(shift+1, -1)  : pair<int,int>(-1, shift+1))

    for(shift = MIN_SHIFT; shift <= MAX_SHIFT; ++shift) {
	coeff_t sum, diff;
	diff = num1 - (num2<<shift);
	if(diff != 0) {
	    sum = num1 + (num2<<shift);
	    if(sum < MAX_NUM) result[fundamental(sum)]   = SUM(shift);
	    if(diff > 0 && diff < MAX_NUM)
		result[fundamental(diff)] = DIFF(shift);
	    else if(diff < 0 && diff > -MAX_NUM)
		result[fundamental(-diff)] = RDIFF(shift);
	}
    }
}

void compute_all_sumdiffs(addmap_t &result, coeff_t num1, coeff_t num2) {
    _compute_all_sumdiffs(false, result, num1, num2);
    _compute_all_sumdiffs(true, result, num2, num1);
}

/****/

void _compute_fast_sumdiffs(cfset_t &result, coeff_t num1, coeff_t num2) {
    for(int shift = MIN_SHIFT; shift <= MAX_SHIFT; ++shift) {
	coeff_t sum, diff;
	diff = cfabs(num1 - (num2<<shift));
	if(diff != 0) {
	    sum = num1 + (num2<<shift);
	    if(sum < MAX_NUM) 	result.insert(fundamental(sum));
	    if(diff < MAX_NUM)	result.insert(fundamental(diff));
	}
    }
}

/* does not store shift amount pairs, so usable only for cost estimation
   [but not code generation] */
void compute_fast_sumdiffs(cfset_t &result, coeff_t num1, coeff_t num2) {
    _compute_fast_sumdiffs(result, num1, num2);
    _compute_fast_sumdiffs(result, num2, num1);
}

