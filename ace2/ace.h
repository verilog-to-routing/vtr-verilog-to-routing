#ifndef __ACE_ACE_H__
#define __ACE_ACE_H__

#include <math.h>

#include "base/abc/abc.h"

#define bool	int
#define TRUE 1
#define FALSE 0

#define EPSILON 0.00001

#ifndef MIN
#define MIN(a,b) 				(a < b ? a : b)
#endif

#ifndef MAX
#define MAX(a,b) 				(a > b ? a : b)
#endif

#define ACE_P0TO1(P1,PS)		((P1)==0.0)?0.0:(((P1)==1.0)?1.0:0.5*PS/(1.0-(P1)))
#define ACE_P1TO0(P1,PS)		((P1)==0.0)?1.0:(((P1)==0.0)?0.0:0.5*PS/(P1))

#define ACE_ERROR 1

#define ACE_OPEN				-1.0
#define ACE_PI_STATIC_PROB		0.5	/* Assumed probability of the primary inputs */
#define ACE_PI_SWITCH_PROB		0.2	/* Assumed switching probability of the PIs */
#define ACE_PI_SWITCH_ACT		0.2	/* Assumed tx activity of the primary inputs */

#define ACE_CHAR_BUFFER_SIZE 	4096
#define ACE_NUM_VECTORS			5000

typedef enum {
	ACE_VEC, ACE_ACT, ACE_PD, ACE_CODED
} ace_pi_format_t;
typedef enum {
	ACE_UNDEF, ACE_DEF, ACE_SIM, ACE_NEW, ACE_OLD
} ace_status_t;

void prob_epsilon_fix(double * d);

typedef struct {
	ace_status_t status;
	int value;
	int flag;
	int depth;

	int num_ones;
	int num_toggles;
	int num_valid;

	int * values;

	double static_prob;
	double switch_prob;
	double switch_act;
	double prob0to1;
	double prob1to0;
} Ace_Obj_Info_t; /* Activity info for each node */

extern st__table * ace_info_hash_table;

Ace_Obj_Info_t * Ace_ObjInfo(Abc_Obj_t * obj);
//static inline void 				Ace_InfoPtrSet(Abc_Obj_t * obj_ptr, Ace_Obj_Info_t* info_ptr)	{obj_ptr->pTemp = info_ptr;					}

#endif
