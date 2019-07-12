/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com),
 *            Alexandrea Demmings (alexandrea.demmings@unb.ca, lxdemmings@gmail.com) and
 *             Dr. Kenneth B. Kent (ken@unb.ca)
 *             for the Reconfigurable Computing Research Lab at the
 *              Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include <string>

#include "internal_bits.hpp"
#include "rtl_int.hpp"
#include "rtl_utils.hpp"

using namespace BitSpace;

class compare_bit
{
private:
	uint8_t result = 0x0;

public:
	compare_bit(uint8_t set_to){result = set_to;}

	bool is_unk(){	return (!result); }

	bool is_gt(){	return (result&(0x1)); }
	bool is_eq(){	return (result&(0x2)); }
	bool is_lt(){	return (result&(0x4)); }

	bool is_ne(){	return (!is_eq()); }
	bool is_ge(){	return (result&(0x3)); }
	bool is_le(){	return (result&(0x6)); }

};

#define UNK_EVAL compare_bit(0x0)
#define GT_EVAL compare_bit(0x1)
#define EQ_EVAL compare_bit(0x2)
#define LT_EVAL compare_bit(0x4)

static compare_bit eval_op(VNumber& a_in, VNumber& b_in)
{

	assert_Werr( a_in.size() ,
		"empty 1st bit string" 
	);

	assert_Werr( b_in.size() ,
		"empty 2nd bit string" 
	);

	bool neg_a = (a_in.is_negative());
	bool neg_b = (b_in.is_negative());


	if(neg_a && !neg_b)
	{
		return LT_EVAL;
	}
	else if(!neg_a && neg_b)
	{
		return GT_EVAL;
	}

	VNumber a;
	VNumber b;
	bool invert_result = (neg_a && neg_b);


	if(invert_result)
	{
		a = a_in.twos_complement();
		b = b_in.twos_complement();
	}
	else
	{
		a = a_in;
		b = b_in;
	}


	size_t std_length = std::max(a.size(), b.size());
	bit_value_t pad_a = a.get_padding_bit();
	bit_value_t pad_b = b.get_padding_bit();


	for(size_t i=std_length-1; i < std_length ; i--)
	{
		bit_value_t bit_a = pad_a;
		if(i < a.size())
		{
			bit_a = a.get_bit_from_lsb(i);
		}

		bit_value_t bit_b = pad_b;
		if(i < b.size())
		{
			bit_b = b.get_bit_from_lsb(i);
		}
		
		if(BitSpace::l_lt[bit_a][bit_b] == BitSpace::_1)
		{
			return (!invert_result)? LT_EVAL: GT_EVAL;
		}
		else if(BitSpace::l_gt[bit_a][bit_b] == BitSpace::_1)
		{
			return (!invert_result)? GT_EVAL: LT_EVAL;
		}
		else if(BitSpace::l_eq[bit_a][bit_b] == BitSpace::_1)
		{
			continue;
		}
		else
		{
			return UNK_EVAL;
		}
	}

	return EQ_EVAL;
}

static compare_bit eval_op(VNumber a,int64_t b)
{
	VNumber bits_value =  VNumber(std::to_string(std::abs(b)));
	if(b < 0)
		bits_value = bits_value.twos_complement();

	return eval_op(a, bits_value);
}

/**
 * Check if the Operation Should be Signed by Checking if Both Operands Are Signed:
 */
static bool is_signed_operation(VNumber& a, VNumber& b)
{
	bool is_signed_operation = false;

	if((true == a.is_signed()) && (true == b.is_signed()))
	{
		is_signed_operation = true;
	}

	return is_signed_operation;
}

/**
 * Addition operations
 */
static VNumber sum_op(VNumber& a, VNumber& b, const bit_value_t& initial_carry, bool is_twos_complement_subtraction)
{

	assert_Werr( a.size() ,
		"empty 1st bit string" 
	);

	assert_Werr( b.size() ,
		"empty 2nd bit string" 
	);

	size_t std_length = std::max(a.size(), b.size());
	size_t new_length = ((true == is_twos_complement_subtraction) ? (std_length) : (std_length + 1));
	const bit_value_t pad_a = a.get_padding_bit();
	const bit_value_t pad_b = b.get_padding_bit();
	bool is_addition_signed_operation = is_signed_operation(a, b);

	//("pad_b: '" << (unsigned(pad_b)) << "'");

	bit_value_t previous_carry = initial_carry;
	VNumber result(new_length, _0, is_addition_signed_operation, a.is_defined_size() && b.is_defined_size()); 


	for(size_t i = 0; i < new_length; i++)
	{
		bit_value_t bit_a = pad_a;
		if(i < a.size())
		{
			bit_a = a.get_bit_from_lsb(i);
		}

		bit_value_t bit_b = pad_b;
		if(i < b.size())
		{
			bit_b = b.get_bit_from_lsb(i);
		}


		result.set_bit_from_lsb(i, l_sum[previous_carry][bit_a][bit_b]);
		previous_carry = l_carry[previous_carry][bit_a][bit_b];

	}

	return result;
}

static VNumber shift_op(VNumber& a, int64_t b, bool sign_shift)
{
	VNumber to_return;

	if(b==0)
	{
		to_return = a;
	}
	//if b is negative then shift right
	else if(b < 0)
	{
		size_t u_b = static_cast<size_t>(-b);
		bit_value_t pad = ( sign_shift ) ? a.get_padding_bit(): BitSpace::_0;
		to_return = VNumber(a.size(), pad, sign_shift, a.is_defined_size());
		for(size_t i=0; i < (a.size() - u_b); i++)
		{
			to_return.set_bit_from_lsb(i, a.get_bit_from_lsb(i+u_b));
		}
	}
	else
	{
		size_t u_b = static_cast<size_t>(b);
		bit_value_t pad = BitSpace::_0;
		to_return =VNumber((a.size() + u_b), pad, sign_shift, a.is_defined_size());
		for(size_t i=0; i < a.size(); i++)
		{
			to_return.set_bit_from_lsb(i+u_b, a.get_bit_from_lsb(i));
		}
	}
	return to_return;
}

bool V_TRUE(VNumber& a)
{
	return a.is_true();
}

bool V_FALSE(VNumber& a)
{
	return a.is_false();
}

bool V_UNK(VNumber& a)
{
	return a.is_dont_care_string();
}

bool V_IS_X(VNumber& a)
{
	return a.is_x();
}

bool V_IS_Z(VNumber& a)
{
	return a.is_z();
}

bool V_IS_SIGNED(VNumber& a)
{
	return a.is_signed();
}

bool V_IS_UNSIGNED(VNumber& a)
{
	return !a.is_signed();
}

std::string V_STRING(VNumber& a)
{
	return a.to_printable();
}

/***
 *                    __          __   __   ___  __       ___    __       
 *    |  | |\ |  /\  |__) \ /    /  \ |__) |__  |__)  /\   |  | /  \ |\ | 
 *    \__/ | \| /~~\ |  \  |     \__/ |    |___ |  \ /~~\  |  | \__/ | \| 
 *                                                                        
 */

VNumber V_BITWISE_NOT(VNumber& a)
{
	return a.invert();
}

VNumber V_LOGICAL_NOT(VNumber& a)
{
	VNumber ored = a.bitwise_reduce(l_or);
	VNumber noted = ored.invert();
	return noted;
}

VNumber V_ADD(VNumber& a)
{
	VNumber result(a);
	return result;
}

VNumber V_MINUS(VNumber& a)
{
	return a.twos_complement();
}

VNumber V_UNSIGNED(VNumber& a)
{
	return a.to_unsigned();
}

VNumber V_SIGNED(VNumber& a)
{
	return a.to_signed();
}

VNumber V_BITWISE_AND(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_and);
	return to_return;
}

VNumber V_BITWISE_OR(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_or);
	return to_return;
}

VNumber V_BITWISE_XOR(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_xor);
	return to_return;
}

VNumber V_BITWISE_NAND(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_nand);
	return to_return;
}

VNumber V_BITWISE_NOR(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_nor);
	return to_return;
}

VNumber V_BITWISE_XNOR(VNumber& a)
{
	VNumber to_return = a.bitwise_reduce(l_xnor);
	return to_return;
}

/***
 *     __               __          __   __   ___  __       ___    __       
 *    |__) | |\ |  /\  |__) \ /    /  \ |__) |__  |__)  /\   |  | /  \ |\ | 
 *    |__) | | \| /~~\ |  \  |     \__/ |    |___ |  \ /~~\  |  | \__/ | \| 
 *                                                                          
 */

VNumber V_REPLICATE(VNumber& a, VNumber& n_times)
{
	assert_Werr(! n_times.is_dont_care_string(),
        "Cannot use undefined number for the replication count");

	return a.replicate(n_times.get_value());
}

VNumber V_CONCAT(std::vector<VNumber> concat_list)
{
	assert_Werr(!concat_list.empty(),
        "Concat List cannot be empty");

	VNumber init = concat_list[0];
	for(size_t i=1; i<concat_list.size(); i++)
	{
		init = init.insert_at_lsb(concat_list[i]);
	}
	return init;
}

VNumber V_BITWISE_AND(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_and);
}

VNumber V_BITWISE_OR(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_or);
}

VNumber V_BITWISE_XOR(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_xor);
}

VNumber V_BITWISE_NAND(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_nand);
}

VNumber V_BITWISE_NOR(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_nor);
}

VNumber V_BITWISE_XNOR(VNumber& a, VNumber& b)
{
	return a.bitwise(b,l_xnor);
}

/**
 * Logical Operations
 */

VNumber V_CASE_EQUAL(VNumber& a, VNumber& b)
{
	VNumber longEval = a.bitwise(b, l_case_eq);
	VNumber eq = V_BITWISE_AND(longEval);
	return eq;
}

VNumber V_CASE_NOT_EQUAL(VNumber& a, VNumber& b)
{
	VNumber eq = V_CASE_EQUAL(a,b);
	VNumber neq = V_LOGICAL_NOT(eq);
	return neq;
}

VNumber V_LOGICAL_AND(VNumber& a, VNumber& b)
{
	VNumber reduxA = a.bitwise_reduce(l_or);
	VNumber reduxB = b.bitwise_reduce(l_or);

	VNumber to_return = reduxA.bitwise(reduxB, l_and);

	return to_return;
}

VNumber V_LOGICAL_OR(VNumber& a, VNumber& b)
{
	VNumber reduxA = a.bitwise_reduce(l_or);
	VNumber reduxB = b.bitwise_reduce(l_or);

	VNumber to_return = reduxA.bitwise(reduxB, l_or);

	return to_return;
}

VNumber V_LT(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_lt()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_GT(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_gt()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_EQUAL(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_eq()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_GE(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_ge()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_LE(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_le()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_NOT_EQUAL(VNumber& a, VNumber& b)
{
	compare_bit cmp = eval_op(a,b);
	BitSpace::bit_value_t result = cmp.is_unk()? BitSpace::_x: cmp.is_ne()? BitSpace::_1: BitSpace::_0;
	VNumber to_return(1, result, false, true);
	return to_return;
}

VNumber V_SIGNED_SHIFT_LEFT(VNumber& a, VNumber& b)
{
	if(b.is_dont_care_string())	
		return VNumber("2'sbxx");
	
	return shift_op(a, b.get_value(), a.is_signed());
}

VNumber V_SHIFT_LEFT(VNumber& a, VNumber& b)
{
	if(b.is_dont_care_string())	
		return VNumber("2'sbxx");
	
	return shift_op(a, b.get_value(), false);
}

VNumber V_SIGNED_SHIFT_RIGHT(VNumber& a, VNumber& b)
{
	if(b.is_dont_care_string())	
		return VNumber("2'sbxx");
	
	return shift_op(a, -1* b.get_value(), a.is_signed());
}

VNumber V_SHIFT_RIGHT(VNumber& a, VNumber& b)
{
	if(b.is_dont_care_string())	
		return VNumber("2'sbxx");
	
	return shift_op(a, -1* b.get_value(), false);
}

VNumber V_ADD(VNumber& a, VNumber& b)
{
	return sum_op(a, b, _0, /* is_twos_complement_subtraction */ false);
}

VNumber V_MINUS(VNumber& a, VNumber& b)
{
	size_t std_length = std::max(a.size(), b.size());
	VNumber padded_a(a, std_length);
	VNumber padded_b(b, std_length);

	VNumber complement = V_MINUS(padded_b);
	if (padded_b.is_negative() && complement.is_negative())
	{
		/* special case: 2's comp is identical to original, must pad */
		complement = VNumber(padded_b, padded_b.size()+1);
		complement = V_MINUS(complement);
	}

	return sum_op(padded_a, complement, _0, /* is_twos_complement_subtraction */ true);
}

VNumber V_MULTIPLY(VNumber& a_in, VNumber& b_in)
{

	if(a_in.is_dont_care_string() || b_in.is_dont_care_string())
	{
		return VNumber("2'sbxx");
	}	

	VNumber a;
	VNumber b;

	bool is_multiply_signed_operation = is_signed_operation(a_in, b_in);
	bool neg_a = a_in.is_negative();
	bool neg_b = b_in.is_negative();

	
	if(neg_a)
	{
		a = V_MINUS(a_in);

		if (a.is_negative())
		{
			/* special case: 2's comp is identical to original, must pad */
			a = VNumber(a_in, a_in.size()+1);
			a = V_MINUS(a);
		}
	}
	else
	{
		a = a_in;
	}

	if(neg_b)
	{
		b = V_MINUS(b_in);

		if (b.is_negative())
		{
			/* special case: 2's comp is identical to original, must pad */
			b = VNumber(b_in, b_in.size()+1);
			b = V_MINUS(b);
		}
	}
	else
	{
		b = b_in;
	}

		
	bool invert_result = ((!neg_a && neg_b) || (neg_a && !neg_b));


	VNumber result("0");
	VNumber b_copy = b;

	for(size_t i = 0; i < a.size(); i++)
	{
		bit_value_t bit_a = a.get_bit_from_lsb(i);


		if(bit_a == _1)
		{
			result = V_ADD(result, b_copy);
		}


		b_copy = shift_op(b_copy, 1, is_multiply_signed_operation);

	}


	if(invert_result)
	{
		result = V_MINUS(result);
	}


	return result;
}

/*
 * From Table 5-6 "Power operator rules" of IEEE Standard 1364-2005:
 *  "Verilog Hardware Description Language"; on Page 46 (PDF Page 76):
 *
 * Table 5-6 — Power operator rules:
 *
 * |-----------------------------------------------------------------------------|
 * | |  \ op1 is -> |               |                  |      |   |              |
 * | \/   \         | negative < –1 | –1               | zero | 1 | positive > 1 |
 * | op2 is \       |               |                  |      |   |              |
 * |-----------------------------------------------------------------------------|
 * |                |               |                  |      |   |              |
 * | Positive       | op1 ** op2    | op2 is odd -> –1 | 0    | 1 | op1 ** op2   |
 * |                |               | op2 is even -> 1 |      |   |              |
 * |                |               |                  |      |   |              |
 * |-----------------------------------------------------------------------------|
 * |                |               |                  |      |   |              |
 * | Zero           | 1             | 1                | 1    | 1 | 1            |
 * |                |               |                  |      |   |              |
 * |-----------------------------------------------------------------------------|
 * |                |               |                  |      |   |              |
 * | Negative       | 0             | op2 is odd -> –1 | 'bx  | 1 | 0            |
 * |                |               | op2 is even -> 1 |      |   |              |
 * |                |               |                  |      |   |              |
 * |-----------------------------------------------------------------------------|
 */
VNumber V_POWER(VNumber& a, VNumber& b)
{
	if(a.is_dont_care_string() || b.is_dont_care_string())
	{
		return VNumber("2'sbxx");
	}

	compare_bit res_a = eval_op(a, 0);
	short val_a = 	(res_a.is_eq()) 		? 	0: 
					(res_a.is_lt()) 		? 	(eval_op(a,-1).is_lt())	?	-2: -1:
					/* GREATHER_THAN */  		(eval_op(a,1).is_gt())	?	2: 1;

	compare_bit res_b = eval_op(b, 0);
	short val_b = 	(res_b.is_eq()) 			? 	0: 
					(res_b.is_lt()) 		? 	-1:
					/* GREATHER_THAN */				1;


	// Compute: Case Where 'val_a <= -2' or 'val_a >= 2'; As-Per the Spec:
	if(val_b > 0 && (val_a < -1 || val_a > 1 ))
	{
		VNumber result("2'sb01");
		VNumber one = VNumber("2'sb01");
		VNumber tmp_b = b;


		while(eval_op(tmp_b, 0).is_gt())
		{

			VNumber tmp_b_comp = V_MINUS(tmp_b, one);
			if (tmp_b_comp.is_negative() && tmp_b.is_negative())
			{
				/* special case: 2's comp is identical to original, must pad */
				tmp_b_comp = VNumber(tmp_b, tmp_b.size()+1);
				tmp_b_comp = V_MINUS(tmp_b_comp);
			}
			tmp_b = tmp_b_comp;

			result = V_MULTIPLY(result, a);

		}


		return result;
	}
	else if (val_b == 0 || val_a == 1)	
	{
		return VNumber("2'sb01");
	}
	else if(val_b == -1 && val_a == 0)
	{
		return VNumber("2'sbxx");
	}
	else if(val_a == -1)
	{
		// Even:
		if(BitSpace::_0 == b.get_bit_from_lsb(0))
		{
			return VNumber("2'sb01");
		}
		// Odd:
		else
		{
			return VNumber("2'sb11");
		}
	}
	else	
	{
		return VNumber("2'sb00");
	}
}

/////////////////////////////
VNumber V_DIV(VNumber& a_in, VNumber& b_in)
{
	if(a_in.is_dont_care_string() || b_in.is_dont_care_string() || eval_op(b_in,0).is_eq())
		return VNumber("2'sbxx");

	VNumber result("0");

	bool is_division_signed_operation = is_signed_operation(a_in, b_in);

	bool neg_a = a_in.is_negative();
	bool neg_b = b_in.is_negative();

	VNumber a = neg_a ? V_MINUS(a_in) : a_in;
	VNumber b = neg_b ? V_MINUS(b_in) : b_in;

	if (neg_a && a.is_negative())
	{
		/* special case: 2's comp is identical to original, must pad */
		a = VNumber(a_in, a_in.size()+1);
		a = V_MINUS(a);
	}

	if (neg_b && b.is_negative())
	{
		/* special case: 2's comp is identical to original, must pad */
		b = VNumber(b_in, b_in.size()+1);
		b = V_MINUS(b);
	}


	while(eval_op(a, b).is_ge() )
	{
		VNumber  count("1");
		VNumber  tmp = b;

		// initialize our variables
		VNumber  sub_with = tmp;
		VNumber	 count_sub_with = count;
		while(eval_op(tmp, a).is_le())
		{
			sub_with = tmp;
			count_sub_with = count;
			count = shift_op(count, 1, is_division_signed_operation);
			tmp = shift_op(tmp, 1, is_division_signed_operation);
		}
		a = V_MINUS(a, sub_with);
		result = V_ADD(result, count_sub_with);
	}

	return (neg_a != neg_b) ? V_MINUS(result) : result;
}

VNumber V_MOD(VNumber& a_in, VNumber& b_in)
{
	if(a_in.is_dont_care_string() || b_in.is_dont_care_string() || eval_op(b_in, 0).is_eq())
		return VNumber("2'sbxx");

	bool neg_a = a_in.is_negative();
	bool neg_b = b_in.is_negative();

	VNumber a = neg_a ? V_MINUS(a_in) : a_in;
	VNumber b = neg_b ? V_MINUS(b_in) : b_in;

	if (neg_a && a.is_negative())
	{
		/* special case: 2's comp is identical to original, must pad */
		a = VNumber(a_in, a_in.size()+1);
		a = V_MINUS(a);
	}

	if (neg_b && b.is_negative())
	{
		/* special case: 2's comp is identical to original, must pad */
		b = VNumber(b_in, b_in.size()+1);
		b = V_MINUS(b);
	}

	bool is_modulo_signed_operation = is_signed_operation(a, b);

	while(eval_op(a, b).is_ge())
	{
		VNumber  tmp = b;
		VNumber  sub_with = tmp;

		while( eval_op(tmp, a).is_le() )
		{
			sub_with = tmp;
			tmp = shift_op(tmp, 1, is_modulo_signed_operation);
		}
		a = V_MINUS(a, sub_with);
	}

	return (neg_a) ? V_MINUS(a) : a;
}

/***
 *    ___  ___  __             __          __   __   ___  __       ___    __       
 *     |  |__  |__) |\ |  /\  |__) \ /    /  \ |__) |__  |__)  /\   |  | /  \ |\ | 
 *     |  |___ |  \ | \| /~~\ |  \  |     \__/ |    |___ |  \ /~~\  |  | \__/ | \| 
 *                                                                                 
*/
VNumber V_TERNARY(VNumber& a_in, VNumber& b_in, VNumber& c_in)
{
	/*	if a evaluates properly	*/
	compare_bit eval = eval_op(V_LOGICAL_NOT(a_in),0);

	return (eval.is_unk())?	b_in.bitwise(c_in, l_ternary):
			(eval.is_eq())?	VNumber(b_in):
							VNumber(c_in);
}
