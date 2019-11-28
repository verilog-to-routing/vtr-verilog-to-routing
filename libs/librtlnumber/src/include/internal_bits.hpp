/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com),
 *            Alexandrea Demmings (alexandrea.demmings@unb.ca, lxdemmings@gmail.com) and
 *             Dr. Kenneth B. Kent (ken@unb.ca)
 *             for the Reconfigurable Computing Research Lab at the
 *              Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#ifndef INTERNAL_BITS_HPP
#define INTERNAL_BITS_HPP

#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <bitset>
#include "rtl_utils.hpp"

typedef uint16_t veri_internal_bits_t;
// typedef VNumber<64> veri_64_t;
// typedef VNumber<32> veri_32_t;

template <typename T>
uint8_t get_veri_integer_limit()
{
    return (sizeof(T)*8);
}

#define _static_unused(x) namespace { constexpr auto _##x = x; }

#define unroll_1d(lut) { lut[_0], lut[_1], lut[_x], lut[_z] }
#define unroll_2d(lut) { unroll_1d(lut[_0]), unroll_1d(lut[_1]), unroll_1d(lut[_x]), unroll_1d(lut[_z]) }

#define unroll_1d_invert(lut) { l_not[lut[_0]], l_not[lut[_1]], l_not[lut[_x]], l_not[lut[_z]] }
#define unroll_2d_invert(lut) { unroll_1d_invert(lut[_0]), unroll_1d_invert(lut[_1]), unroll_1d_invert(lut[_x]), unroll_1d_invert(lut[_z]) }

namespace BitSpace {
    typedef uint8_t bit_value_t;

    constexpr veri_internal_bits_t _All_0 = static_cast<veri_internal_bits_t>(0x0000000000000000UL);
    constexpr veri_internal_bits_t _All_1 = static_cast<veri_internal_bits_t>(0x5555555555555555UL);
    constexpr veri_internal_bits_t _All_x = static_cast<veri_internal_bits_t>(0xAAAAAAAAAAAAAAAAUL);
    constexpr veri_internal_bits_t _All_z = static_cast<veri_internal_bits_t>(0xFFFFFFFFFFFFFFFFUL);

    constexpr bit_value_t _0 = 0x0;
    constexpr bit_value_t _1 = 0x1;
    constexpr bit_value_t _x = 0x2;
    constexpr bit_value_t _z = 0x3;

    /***                                                              
     * these are taken from the raw verilog truth tables so that the evaluation are correct.
     * only use this to evaluate any expression for the number_t binary digits.
     * reference: http://staff.ustc.edu.cn/~songch/download/IEEE.1364-2005.pdf
     * 
     *******************************************************/

    constexpr bit_value_t l_buf[4] = {
        /*	 0   1   x   z  <- a*/
            _0,_1,_x,_x
    };
    _static_unused(l_buf)

    constexpr bit_value_t l_not[4] = {
        /*   0   1   x   z 	<- a */
            _1,_0,_x,_x
    };
    _static_unused(l_not)

    constexpr bit_value_t is_unk[4] = {
        /*	 0   1   x   z  <- a*/
            _0,_0,_1,_1
    };
    _static_unused(is_unk)

    constexpr bit_value_t is_x_bit[4] = {
        /*	 0   1   x   z  <- a*/
            _0,_0,_1,_0
    };
    _static_unused(is_x_bit)

    constexpr bit_value_t is_z_bit[4] = {
        /*	 0   1   x   z  <- a*/
            _0,_0,_0,_1
    };
    _static_unused(is_z_bit)

    constexpr bit_value_t is_one_bit[4] = {
        /*	 0   1   x   z  <- a*/
            _0,_1,_0,_0
    };
    _static_unused(is_one_bit)

    constexpr bit_value_t is_zero_bit[4] = {
        /*	 0   1   x   z  <- a*/
            _1,_0,_0,_0
    };
    _static_unused(is_zero_bit)

    constexpr bit_value_t l_and[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_0,_0,_0,_0},	
        /* 1 */	{_0,_1,_x,_x},	
        /* x */	{_0,_x,_x,_x},	
        /* z */	{_0,_x,_x,_x}
    };
    _static_unused(l_and)

    constexpr bit_value_t l_nand[4][4] = unroll_2d_invert(l_and);
    _static_unused(l_nand)

    constexpr bit_value_t l_or[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_0,_1,_x,_x},	
        /* 1 */	{_1,_1,_1,_1},	
        /* x */	{_x,_1,_x,_x},	
        /* z */	{_x,_1,_x,_x}
    };
    _static_unused(l_or)

    constexpr bit_value_t l_nor[4][4] = unroll_2d_invert(l_or);
    _static_unused(l_nor)

    constexpr bit_value_t l_xor[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_0,_1,_x,_x},	
        /* 1 */	{_1,_0,_x,_x},	
        /* x */	{_x,_x,_x,_x},	
        /* z */	{_x,_x,_x,_x}
    };
    _static_unused(l_xor)

    constexpr bit_value_t l_xnor[4][4] = unroll_2d_invert(l_xor);
    _static_unused(l_xnor)

    /*****************************************************
     *  Tran NO SUPPORT FOR THESE YET 
     */
    constexpr bit_value_t l_notif1[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_z,_1,_x,_x},
        /* 1 */	{_z,_0,_x,_x},
        /* x */	{_z,_x,_x,_x},
        /* z */	{_z,_x,_x,_x}
    };
    _static_unused(l_notif1)

    constexpr bit_value_t l_notif0[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_1,_z,_x,_x},
        /* 1 */	{_0,_z,_x,_x},
        /* x */	{_x,_z,_x,_x},
        /* z */	{_x,_z,_x,_x}
    };
    _static_unused(l_notif0)

    constexpr bit_value_t l_bufif1[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_z,_0,_x,_x},
        /* 1 */	{_z,_1,_x,_x},
        /* x */	{_z,_x,_x,_x},
        /* z */	{_z,_x,_x,_x}
    };
    _static_unused(l_bufif1)

    constexpr bit_value_t l_bufif0[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_0,_z,_x,_x},
        /* 1 */	{_1,_z,_x,_x},
        /* x */	{_x,_z,_x,_x},
        /* z */	{_x,_z,_x,_x}
    };
    _static_unused(l_bufif0)

    /* cmos gates */
    constexpr bit_value_t l_rpmos[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_0,_z,_x,_x},
        /* 1 */	{_1,_z,_x,_x},
        /* x */	{_x,_z,_x,_x},
        /* z */	{_z,_z,_z,_z}
    };
    _static_unused(l_rpmos)

    constexpr bit_value_t l_rnmos[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_z,_0,_x,_x},
        /* 1 */	{_z,_1,_x,_x},
        /* x */	{_z,_x,_x,_x},
        /* z */	{_z,_z,_z,_z}
    };
    _static_unused(l_rnmos)

    constexpr bit_value_t l_nmos[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_z,_0,_x,_x},
        /* 1 */	{_z,_1,_x,_x},
        /* x */	{_z,_x,_x,_x},
        /* z */	{_z,_z,_z,_z}
    };
    _static_unused(l_nmos)

    // see table 5-21 p:54 IEEE 1364-2005
    constexpr bit_value_t l_ternary[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_0,_x,_x,_x},
        /* 1 */	{_x,_1,_x,_x},
        /* x */	{_x,_x,_x,_x},
        /* z */	{_x,_x,_x,_x}
    };
    _static_unused(l_ternary)

    /*****
     * these extend the library and simplify the process
     */
    /* helper */
    constexpr bit_value_t l_unk[4][4] = {
        /* in /	 0   1   x   z 	<-control */	
        /* 0 */	{_x,_x,_x,_x},
        /* 1 */	{_x,_x,_x,_x},
        /* x */	{_x,_x,_x,_x},
        /* z */	{_x,_x,_x,_x}
    };
    _static_unused(l_unk)

    constexpr bit_value_t l_case_eq[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_1,_0,_0,_0},	
        /* 1 */	{_0,_1,_0,_0},	
        /* x */	{_0,_0,_1,_0},	
        /* z */	{_0,_0,_0,_1}
    };
    _static_unused(l_case_eq)

    constexpr bit_value_t l_lt[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_0,_1,_x,_x},	
        /* 1 */	{_0,_0,_x,_x},	
        /* x */	{_x,_x,_x,_x},	
        /* z */	{_x,_x,_x,_x}
    };
    _static_unused(l_lt)

    constexpr bit_value_t l_gt[4][4] = {
        /* a  /	 0   1   x   z 	<-b */	
        /* 0 */	{_0,_0,_x,_x},	
        /* 1 */	{_1,_0,_x,_x},	
        /* x */	{_x,_x,_x,_x},	
        /* z */	{_x,_x,_x,_x}
    };
    _static_unused(l_gt)

    constexpr bit_value_t l_eq[4][4] = unroll_2d(l_xnor);
    _static_unused(l_eq)

    constexpr bit_value_t l_sum[4][4][4] = {
        /* c_in */ 
        /* 0 */ unroll_2d(l_xor),
        /* 1 */ unroll_2d(l_xnor),
        /* x */ unroll_2d(l_unk),
        /* z */ unroll_2d(l_unk)
    };
    _static_unused(l_sum)

    constexpr bit_value_t l_carry[4][4][4] = {
        /* c_in */
        /* 0 */ unroll_2d(l_and),
        /* 1 */ unroll_2d(l_or),
        /* x */ unroll_2d(l_ternary),
        /* z */ unroll_2d(l_ternary)
    };
    _static_unused(l_carry)

    constexpr bit_value_t l_half_carry[4][4] = unroll_2d(l_carry[_0]);
    _static_unused(l_half_carry)

    constexpr bit_value_t l_half_sum[4][4] = unroll_2d(l_sum[_0]);
    _static_unused(l_half_sum)

    static char bit_to_c(bit_value_t bit)
    {
        switch(bit)
        {
            case _0:    return '0';
            case _1:    return '1';
            case _z:    return 'z';
            default:    return 'x';
        }
    }

    static bit_value_t c_to_bit(char c)
    {
        switch(tolower(c))
        {
            case '0':   return _0;
            case '1':   return _1;
            case 'z':   return _z;
            default:    return _x;
        }
    }

    template<typename T>
    class BitFields
    {
    private :

        T bits = static_cast<T>(_All_x);

        template<typename Addr_t>
        size_t get_bit_location(Addr_t address)
        {
            size_t current_address = static_cast<size_t>(address);
            current_address %= this->size();
            current_address <<= 1;
            return current_address;
        }

    public :

        BitFields(bit_value_t init_v)
        {
            this->bits = static_cast<T>(
                (_0 == init_v)? _All_0:
                (_1 == init_v)? _All_1:
                (_z == init_v)? _All_z:
                                _All_x
            );
        } 
        
        template<typename Addr_t>
        bit_value_t get_bit(Addr_t address)
        {
            auto result = this->bits >> this->get_bit_location(address);
            result &= 0x3;

            return static_cast<bit_value_t>(result);
        }

        template<typename Addr_t>
        void set_bit(Addr_t address, bit_value_t value)
        {	
            size_t real_address = this->get_bit_location(address);

            T set_value = static_cast<T>(value);
            set_value = static_cast<T>(set_value << real_address);

            T mask = static_cast<T>(0x3);
            mask = static_cast<T>(mask << real_address);
            mask = static_cast<T>(~(mask));

            this->bits = static_cast<T>(this->bits & mask);
            this->bits = static_cast<T>(this->bits | set_value);
        }

        /**
         * get 16 real bit (8 verilog bits) as 8 bit (char)
         */
        template<typename Addr_t>
        char get_as_char(Addr_t address)
        {
            size_t start = (8*(address));
            size_t end = (8*(address+1))-1;
            char value = 0;
            for(size_t i = start; i <= end; i++)
            {
                value += (((this->get_bit(i))? 1: 0) << i);
            }
            
            return value;
        }

        static size_t size()
        {
            return (sizeof(T)<<2); // 8 bit in a byte, 2 bits for a verilog bits = 4 bits in a byte, << 2 = sizeof x 4
        }
    };

    // #define DEBUG_V_BITS

    /*****
     * we use large array since we process the bits in chunks
     */
    class VerilogBits
    {
    private:

        std::vector<BitFields<veri_internal_bits_t>> bits;
        size_t bit_size = 0;

        size_t to_index(size_t address)
        {
            return ( address / BitFields<veri_internal_bits_t>::size() );
        }

        size_t list_size()
        {
            return this->bits.size();
        }

    public:

        VerilogBits()
        {
            this->bit_size = 0;
            this->bits = std::vector<BitSpace::BitFields<veri_internal_bits_t>>();
        }

        VerilogBits(size_t data_size, bit_value_t value_in)
        {
            this->bit_size = data_size;
            this->bits = std::vector<BitSpace::BitFields<veri_internal_bits_t>>();

            size_t bitfield_count = (this->bit_size / BitFields<veri_internal_bits_t>::size()) +1;
            
            for(size_t i=0; i<bitfield_count; i++)
            {
                this->bits.push_back(BitSpace::BitFields<veri_internal_bits_t>(value_in));
            }
        }

        VerilogBits(VerilogBits *other)
        {
            this->bit_size = other->size();
            this->bits = other->get_internal_bitvector();
        }

        size_t size()
        {
            return this->bit_size;
        }

        std::vector<BitFields<veri_internal_bits_t>> get_internal_bitvector()
        {
            return this->bits;
        }

        BitFields<veri_internal_bits_t> *get_bitfield(size_t index)
        {

    #ifdef DEBUG_V_BITS
            if (index >= this->bits.size() )
            {
                std::cerr << "Bit array indexing out of bounds " << index << " but size is " << this->bit_size  << std::endl;
                std::abort();
            }
    #endif

            return (&this->bits[index]);
        }   

        bit_value_t get_bit(size_t address)
        {
    #ifdef DEBUG_V_BITS
            if (address >= this->bit_size )
            {
                std::cerr << "Bit index array out of bounds " << address << " but size is " << this->bit_size  << std::endl;
                std::abort();
            }
    #endif

            return (this->get_bitfield(to_index(address))->get_bit(address));
        }

        void set_bit(size_t address, bit_value_t value)
        {
    #ifdef DEBUG_V_BITS
            if (address >= this->bit_size )
            {
                std::cerr << "Bit index array out of bounds " << address << " but size is " << this->bit_size  << std::endl;
                std::abort();
            }
    #endif
            (this->get_bitfield(to_index(address))->set_bit(address, value));
        }

        std::string to_string(bool big_endian)
        {
            // make a big endian string
            std::string to_return = "";
            for(size_t address=0x0; address < this->size(); address++)
            {
                char value = BitSpace::bit_to_c(this->get_bit(address));
                if(big_endian)
                {
                    to_return.push_back(value);
                }
                else
                {
                    to_return.insert(0,1,value);
                }
            }

            return to_return;              
        }

        std::string to_printable()
        {
            std::string to_return = "";

            for(size_t i=0; i< this->size(); i += 8)
            {
                to_return.insert(0,1,this->get_bitfield(to_index(i))->get_as_char(i));
            }

            return to_return;              
        }

        bool has_unknowns()
        {
            for(size_t address=0x0; address < this->size(); address++)
            {
                if(is_unk[this->get_bit(address)])
                    return true;                
            }
            
            return false;
        }

        bool is_only_z()
        {
            for(size_t address=0x0; address < this->size(); address++)
            {
                if(! is_z_bit[this->get_bit(address)])
                    return false;                
            }
            
            return true;
        }

        bool is_only_x()
        {
            for(size_t address=0x0; address < this->size(); address++)
            {
                if(! is_x_bit[this->get_bit(address)])
                    return false;               
            }
            
            return true;
        }

        bool is_true()
        {
            for(size_t address=0x0; address < this->size(); address++)
            {
                if(is_one_bit[this->get_bit(address)])
                    return true;               
            }
            
            return false; 
        }

        bool is_false()
        {
            for(size_t address=0x0; address < this->size(); address++)
            {
                if(! is_zero_bit[this->get_bit(address)])
                    return false;               
            }
            
            return true; 
        }

        /**
         * Unary Reduction operations
         * This is Msb to Lsb on purpose, as per specs
         */
        VerilogBits bitwise_reduce(const bit_value_t lut[4][4])
        {

            bit_value_t result = this->get_bit(this->size()-1);
            for(size_t i=this->size()-2; i < this->size(); i--)
            {
                result = lut[result][this->get_bit(i)];
            }

            return VerilogBits(1, result);
        }

        VerilogBits invert()
        {
            VerilogBits other(this->bit_size, _0);

            for(size_t i=0; i<this->size(); i++)
                other.set_bit(i, BitSpace::l_not[this->get_bit(i)]);
                        
            return other;
        }

        VerilogBits twos_complement()
        {
            BitSpace::bit_value_t previous_carry = BitSpace::_1;
            VerilogBits other(this->bit_size, _0);

            for(size_t i=0; i<this->size(); i++)
            {
                BitSpace::bit_value_t not_bit_i = BitSpace::l_not[this->get_bit(i)];

                other.set_bit(i,BitSpace::l_half_sum[previous_carry][not_bit_i]);
                previous_carry = BitSpace::l_half_carry[previous_carry][not_bit_i];
            }
                        
            return other;
        }

        /**
         * size of zero compact to the least amount of bits
         */
        VerilogBits resize(BitSpace::bit_value_t pad, size_t new_size)
        {

            /**
             * find the new size
             */
            if(new_size == 0)
            {

                size_t last_bit_id = this->size() - 1;
                size_t next_bit_id = last_bit_id -1;


                while(next_bit_id < this->size()-1)
                {
                    BitSpace::bit_value_t current = this->get_bit(last_bit_id);
                    BitSpace::bit_value_t next = this->get_bit(next_bit_id);
                    
                    if(current == next && current == pad)
                    {
                        last_bit_id--;
                        next_bit_id--;
                    }
                    else
                    {
                        break; /* it down. oh. oh! */
                    }

     
                }

                new_size = last_bit_id+1;
            }

            VerilogBits other(new_size, BitSpace::_0);

            size_t i = 0;

            while(i < this->size() && i < new_size)
            {
                other.set_bit(i, this->get_bit(i));
                i++;
            }

            while(i < new_size)
            {
                other.set_bit(i, pad);/* <- ask Eve about it */
                i++;
            }

            return other;
        }

        /**
         * replicates the bitset n times
         */
        VerilogBits replicate(size_t n_times)
        {
            size_t old_size = this->size();
            size_t new_size = old_size * n_times;

            VerilogBits other(new_size, BitSpace::_0);

            for(size_t i = 0; i < new_size; i += 1)
            {
                other.set_bit(i, this->get_bit( i%old_size ));
            }

            return other;
        }
    }; 
}

//template<size_t bit_size>
class VNumber 
{
private:
    bool sign = false;
    bool defined_size = false;
    BitSpace::VerilogBits bitstring = BitSpace::VerilogBits(1, BitSpace::_x);

    VNumber(BitSpace::VerilogBits other_bitstring, bool other_defined_size, bool other_sign)
    {
        bitstring = BitSpace::VerilogBits(other_bitstring);
        sign = other_sign;
        defined_size = other_defined_size;
    }

    VNumber insert(VNumber &other, size_t index_to_insert_at, size_t insertion_size)
    {
        assert_Werr(other.is_defined_size() && this->is_defined_size()
            ,"Size must be defined on both operand for insertion");

        VNumber new_bitstring(this->size() + insertion_size, BitSpace::_0, this->is_signed() && other.is_signed(), true);

        size_t index = 0;

        for(size_t i=0; i < this->size() && i < index_to_insert_at; i += 1, index +=1)
            new_bitstring.set_bit_from_lsb(index, this->get_bit_from_lsb( i ));

        for(size_t i=0; i < insertion_size; i += 1, index +=1)
            new_bitstring.set_bit_from_lsb(index, other.get_bit_from_lsb( i ));

        for(size_t i=index_to_insert_at; i < this->size(); i += 1, index +=1)
            new_bitstring.set_bit_from_lsb(index, this->get_bit_from_lsb( i ));

        return new_bitstring; 
    }

public:

    VNumber()
    {
        this->sign = false;
        this->bitstring = BitSpace::VerilogBits(1, BitSpace::_x);
        this->defined_size = false;
    }

    VNumber(VNumber&&) = default;
    VNumber& operator=(VNumber&&) = default;
    VNumber& operator=(const VNumber& other) = default;

    VNumber(const VNumber& other)
    {
        this->sign = other.sign;
        this->bitstring = other.bitstring;
        this->defined_size = other.defined_size;
    }

    VNumber(VNumber other, size_t length)
    {
        this->sign = other.sign;
        this->bitstring = other.bitstring.resize(other.get_padding_bit(),length);
        // TODO this->defined_size = true?????;
    }

    VNumber(const std::string& verilog_string)
    {
        set_value(verilog_string);
    }

    VNumber(int64_t numeric_value)
    {
        set_value(numeric_value);
    }

    VNumber(size_t len, BitSpace::bit_value_t initial_bits, bool input_sign, bool this_defined_size)
    {
        this->bitstring = BitSpace::VerilogBits(len, initial_bits);
        this->sign = input_sign;
        this->defined_size = this_defined_size;
    }
    
    /***
     * getters to 64 bit int
     */
    int64_t get_value()
    {
        using integer_t = int64_t;
        size_t bit_size = 8*sizeof(integer_t);

        assert_Werr( (! this->bitstring.has_unknowns() ) ,
                    "Invalid Number contains dont care values. number: " + this->bitstring.to_string(false)
        );   

        size_t end = this->size();
        if(end > bit_size)
        {
            printf(" === Warning: Returning a 64 bit integer from a larger bitstring (%zu). The bitstring will be truncated\n", bit_size);
            end = bit_size;
        }

        integer_t result = 0;
        BitSpace::bit_value_t pad = this->get_padding_bit(); // = this->is_negative();

        for(size_t bit_index = 0; bit_index < end; bit_index++)
        {
            integer_t current_bit = static_cast<integer_t>(pad);
            if(bit_index < this->size())
                current_bit = this->bitstring.get_bit(bit_index);

            result |= (current_bit << bit_index);
        }

        return result;
    }

    // convert lsb_msb bitstring to verilog
    std::string to_full_string()
    {
        std::string out = this->to_bit_string();
        size_t len = this->bitstring.size();

        return std::to_string(len) + ((this->is_signed())? "\'sb": "\'b") + out;
    }

    std::string to_bit_string()
    {
        std::string out = this->bitstring.to_string(false);
        return out;
    }

    std::string to_printable()
    {
        std::string out = this->bitstring.to_printable();
        return out;
    }

    /***
     * setters
     */
    void set_value(const std::string& input)
    {
        if(!input.size())
        {
            return;
        }

        std::string verilog_string(input);

        /**
         * set defaults
         */
        size_t bitsize = 32;            // 32 bit is the fall back
        this->defined_size = false;     // the size is undefined unless otherwise specified
        size_t radix = 0;              // the radix is unknown to start with
        this->sign = false;             // we treat everything as unsigned unless specified

        // if this is a string
        if(verilog_string[0] == '\"')
        {
            assert_Werr(verilog_string.size() >= 2,
                "Malformed input String for VNumber, only open quote" + verilog_string);

            assert_Werr(verilog_string.back() == '\"',
                "Malformed input String for VNumber, expected closing quotes" + verilog_string);

            verilog_string.erase(0,1);
            verilog_string.pop_back();

            size_t string_size = verilog_string.size();
            if(string_size == 0)
                string_size = 1;

            bitsize = string_size * 8;
            this->defined_size = true;
            radix = 256;
        }
        else
        {
            size_t loc = verilog_string.find("\'");
            if(loc == std::string::npos)
            {
                verilog_string.insert(0, "\'sd");
                loc = 0;
            }

            if(loc != 0)
            {
                std::string bit_length_char = verilog_string.substr(0, loc);
                bitsize = strtoul(bit_length_char.c_str(), nullptr, 10);
                this->defined_size = true;
            }

            if(std::tolower(verilog_string[loc+1]) == 's')
            {
                this->sign = true;
            }

            char base = static_cast<char>(std::tolower(verilog_string[loc+1+sign]));
            switch(base)
            {
                case 'b':   radix = 2;  break;  // binary
                case 'o':   radix = 8;  break;  // octal
                case 'd':   radix = 10; break;  // decimal
                case 'h':   radix = 16; break;  // hexadecimal
                default:    
                    assert_Werr( false,
                            "Invalid radix base for number: " + std::string(1,base)
                    ); 
                    break;
            }

            //remove underscores
            verilog_string = verilog_string.substr(loc+2+sign);
            verilog_string.erase(std::remove(verilog_string.begin(), verilog_string.end(), '_'), verilog_string.end());

            //little endian bitstring string
        }

        std::string temp_bitstring = string_of_radix_to_bitstring(verilog_string, radix);

        char pad = temp_bitstring[0];
        if(!this->sign && pad == '1')
        {
            pad = '0';
        }

        // convert the bits to the internal data struct (bit at index 0 in string is msb since string go from msb to lsb)
        BitSpace::VerilogBits new_bitstring(temp_bitstring.size(), BitSpace::_0);
        size_t counter = temp_bitstring.size()-1;
        for(char in: temp_bitstring)
        {
            new_bitstring.set_bit(counter--,BitSpace::c_to_bit(in));
        }
        
        this->bitstring = new_bitstring.resize(BitSpace::c_to_bit(pad), bitsize);
    }

    void set_value(int64_t in)
    {
        this->set_value(std::to_string(in));
    }

    size_t msb_index()
    {
        return this->bitstring.size()-1;
    }

    /****
     * bit twiddling functions
     */
    BitSpace::bit_value_t get_bit_from_msb(size_t index)
    {
        return this->bitstring.get_bit(msb_index()-index);
    }

    BitSpace::bit_value_t get_bit_from_lsb(size_t index)
    {
        if (index < this->size())
            return this->bitstring.get_bit(index);
        else
            return this->get_padding_bit();
    }

    void set_bit_from_msb(size_t index, BitSpace::bit_value_t val)
    {
        this->bitstring.set_bit(msb_index()-index, val);
    }

    void set_bit_from_lsb(size_t index, BitSpace::bit_value_t val)
    {
        this->bitstring.set_bit(index, val);
    }

    /***
     *  other
     */
    size_t size()
    {

        return this->bitstring.size();
    }
    
    BitSpace::bit_value_t get_padding_bit()
    {
        return this->is_negative()? BitSpace::_1:BitSpace::_0;
    }

    bool is_signed() const
    {
        return this->sign;
    }

    bool is_defined_size() 
    {
        return this->defined_size;
    }

    bool is_negative()
    {
        return ( this->get_bit_from_msb(0) == BitSpace::_1 && this->sign );
    }

    bool is_dont_care_string()
    {
        return this->bitstring.has_unknowns();
    }

    bool is_z()
    {
        return this->bitstring.is_only_z();
    }

    bool is_x()
    {
        return this->bitstring.is_only_x();
    }

    bool is_true()
    {
        return this->bitstring.is_true();
    }

    bool is_false()
    {
        return this->bitstring.is_false();
    }

    VNumber twos_complement()
    {
        return VNumber(this->bitstring.twos_complement(),this->defined_size,this->sign);
    }

    VNumber to_signed()
    {
        return VNumber(this->bitstring,this->defined_size,true);
    }

    VNumber to_unsigned()
    {
        return VNumber(this->bitstring,this->defined_size,false);
    }

    VNumber invert()
    {
        return VNumber(this->bitstring.invert(),this->defined_size,this->sign);
    }

    VNumber bitwise_reduce(const BitSpace::bit_value_t lut[4][4])
    {
        return VNumber(this->bitstring.bitwise_reduce(lut),this->defined_size,false);
    }

    /**
     * Binary Reduction operations
     */
    VNumber bitwise(VNumber& b, const BitSpace::bit_value_t lut[4][4])
    {
        size_t std_length = std::max(this->size(), b.size());
        const BitSpace::bit_value_t pad_a = this->get_padding_bit();
        const BitSpace::bit_value_t pad_b = b.get_padding_bit();

        VNumber result(std_length, BitSpace::_x, false, this->is_defined_size() && b.is_defined_size() );

        for(size_t i=0; i < result.size(); i++)
        {
            BitSpace::bit_value_t bit_a = pad_a;
            if(i < this->size())
                bit_a = this->get_bit_from_lsb(i);

            BitSpace::bit_value_t bit_b = pad_b;
            if(i < b.size())
                bit_b = b.get_bit_from_lsb(i);

            result.set_bit_from_lsb(i, lut[bit_a][bit_b]);
        }

        return result;
    }

    VNumber replicate(int64_t n_times_replicate)
    {

        assert_Werr(n_times_replicate > 0,
            "Cannot replicate bitstring less than 1 times");

        size_t n_times_unsigned = static_cast<size_t>(n_times_replicate);

        return VNumber(this->bitstring.replicate(n_times_unsigned),true,this->sign);
    }

    VNumber insert_at_lsb(VNumber &other)
    {
        return this->insert(other, 0, other.size());
    }

    VNumber insert_at_msb(VNumber &other)
    {
        return this->insert(other, this->size(), other.size());
    }
};

#endif
