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

typedef uint64_t veri_internal_bits_t;
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
        switch(c)
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

        T bits = _All_x;

        size_t get_bit_location(size_t address)
        {
            return ((address%this->size())<<1);
        }

        T make_insert(size_t loc, bit_value_t value)
        {
            return (static_cast<T>(value) << (loc));
        }

        T make_mask(size_t loc)
        {
            return (~make_insert(loc,_0));
        }

    public :

        BitFields(bit_value_t init_v)
        {
            this->bits = 
                (_0 == init_v)? _All_0:
                (_1 == init_v)? _All_1:
                (_z == init_v)? _All_z:
                                _All_x;
        } 
        
        bit_value_t get_bit(size_t address)
        {
            return ((this->bits >> (this->get_bit_location(address))) & 0x3UL);
        }

        void set_bit(size_t address, bit_value_t value)
        {	
            size_t real_address = this->get_bit_location(address);

            T long_value = static_cast<T>(value);

            T set_value = long_value << real_address;
            T zero_out_location = ~(0x3UL << real_address);

            this->bits &= zero_out_location;
            this->bits |= set_value;
        }

        static size_t size()
        {
            return (sizeof(T)<<2); // 8 bit in a byte, 2 bits for a verilog bits = 4 bits in a byte, << 2 = sizeof x 4
        }
    };

    #define DEBUG_V_BITS

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
            return (address / BitFields<veri_internal_bits_t>::size() );
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
                to_return.push_back(BitSpace::bit_to_c(this->get_bit(address)));
            }

            if(!big_endian)
                return std::string(to_return.crbegin(), to_return.crend());
            else
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
    }; 
}

//template<size_t bit_size>
class VNumber 
{
private:
    bool sign = false;
    bool defined_size = false;
    BitSpace::VerilogBits bitstring = BitSpace::VerilogBits(1, BitSpace::_x);

    VNumber(BitSpace::VerilogBits other_bitstring, bool other_sign)
    {
        bitstring = BitSpace::VerilogBits(other_bitstring);
        sign = other_sign;
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
    }

    VNumber(const std::string& verilog_string)
    {
        set_value(verilog_string);
    }

    VNumber(int64_t numeric_value)
    {
        set_value(numeric_value);
    }

    VNumber(size_t len, BitSpace::bit_value_t initial_bits, bool input_sign)
    {
        this->bitstring = BitSpace::VerilogBits(len, initial_bits);
        this->sign = input_sign;
    }
    
    /***
     * getters
     */
    int64_t get_value()
    {
        assert_Werr( (! this->bitstring.has_unknowns() ) ,
                    "Invalid Number contains dont care values. number: " + this->bitstring.to_string(false)
        );   

        int64_t result = 0;
        BitSpace::bit_value_t pad = this->get_padding_bit(); // = this->is_negative();
        for(size_t bit_index = 0; bit_index < this->size(); bit_index++)
        {
            int64_t current_bit = static_cast<int64_t>(pad);
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

    /***
     * setters
     */
    void set_value(const std::string& input)
    {
        std::string verilog_string(input);

        if(!verilog_string.size())
        {
            return;
        }

        size_t loc = verilog_string.find("\'");


        if(loc == std::string::npos)
        {
            verilog_string.insert(0, "\'sd");
            loc = 0;
        }

        size_t bitsize = 0;
        if(loc != 0)
        {
            std::string bit_length_char = verilog_string.substr(0, loc);
            bitsize = strtoul(bit_length_char.c_str(), nullptr, 10);
            this->defined_size = true;
        }
        else
        {
            bitsize = 32;
            this->defined_size = false;
        }


        this->sign = false;
        if(std::tolower(verilog_string[loc+1]) == 's')
        {
            this->sign = true;
        }


        char base = static_cast<char>(std::tolower(verilog_string[loc+1+sign]));


        uint8_t radix = 0;
        switch(base){
            case 'b':   radix = 2;  break;
            case 'o':   radix = 8;  break;
            case 'd':   radix = 10; break;
            case 'h':   radix = 16; break;
            default:    
                assert_Werr( false,
                        "Invalid radix base for number: " + std::string(1,base)
                ); 
                break;
        }


        //remove underscores
        std::string v_value_str = verilog_string.substr(loc+2+sign);
        v_value_str.erase(std::remove(v_value_str.begin(), v_value_str.end(), '_'), v_value_str.end());


        //little endian bitstring string
        std::string temp_bitstring = string_of_radix_to_bitstring(v_value_str, radix);


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

    VNumber twos_complement()
    {
        return VNumber(this->bitstring.twos_complement(),this->sign);
    }

    VNumber invert()
    {
        return VNumber(this->bitstring.invert(),this->sign);
    }

    VNumber bitwise_reduce(const BitSpace::bit_value_t lut[4][4])
    {
        return VNumber(this->bitstring.bitwise_reduce(lut),false);
    }

    /**
     * Binary Reduction operations
     */
    VNumber bitwise(VNumber& b, const BitSpace::bit_value_t lut[4][4])
    {
        size_t std_length = std::max(this->size(), b.size());
        const BitSpace::bit_value_t pad_a = this->get_padding_bit();
        const BitSpace::bit_value_t pad_b = b.get_padding_bit();

        VNumber result(std_length, BitSpace::_x, false);

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

};

#endif
