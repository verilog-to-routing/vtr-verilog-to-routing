/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com) and
 *            Dr. Kenneth B. Kent (ken@unb.ca)
 *            for the Reconfigurable Computing Research Lab at the
 *             Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include "rtl_utils.hpp"
#include <algorithm>
#include <iostream>

inline static std::string _radix_digit_to_bits_str(const char digit, short radix,  const char *FUNCT, int LINE)
{
    switch(radix)
    {
        case 2:
        {
            switch(std::tolower(digit))
            {
                case '0': return "0";
                case '1': return "1";
                case 'x': return "x";
                case 'z': return "z";
                default:  
                    _assert_Werr( false, FUNCT, LINE,
                            "INVALID BIT INPUT: " + std::string(1,digit) 
                    );
                    break;
            }
            break;
        }
        case 8:
        {
            switch(std::tolower(digit))
            {
                case '0': return "000";
                case '1': return "001";
                case '2': return "010";
                case '3': return "011";
                case '4': return "100";
                case '5': return "101";
                case '6': return "110";
                case '7': return "111";
                case 'x': return "xxx";
                case 'z': return "zzz";
                default:  
                    _assert_Werr( false, FUNCT, LINE,
                            "INVALID BIT INPUT: " + std::string(1,digit) 
                    );
                    break;
            }
            break;
        }
        case 16:
        {
            switch(std::tolower(digit))
            {
                case '0': return "0000";
                case '1': return "0001";
                case '2': return "0010";
                case '3': return "0011";
                case '4': return "0100";
                case '5': return "0101";
                case '6': return "0110";
                case '7': return "0111";
                case '8': return "1000";
                case '9': return "1001";
                case 'a': return "1010";
                case 'b': return "1011";
                case 'c': return "1100";
                case 'd': return "1101";
                case 'e': return "1110";
                case 'f': return "1111";
                case 'x': return "xxxx";
                case 'z': return "zzzz";
                default:  
                    _assert_Werr( false, FUNCT, LINE,
                            "INVALID BIT INPUT: " + std::string(1,digit) 
                    );
                    break;
            }
            break;
        }
        default:
        {
            _assert_Werr( false, FUNCT, LINE,
                "Invalid base " + std::to_string(radix)
            );
            break;
        }
    }
    std::abort();
}

#define radix_digit_to_bits(num,radix) _radix_digit_to_bits(num,radix,__func__, __LINE__)
inline static std::string _radix_digit_to_bits(const char digit, short radix,  const char *FUNCT, int LINE)
{
    std::string result = _radix_digit_to_bits_str(digit, radix, FUNCT, LINE);
    return std::string(result.crbegin(), result.crend());
}

/**********************
 * convert from different radix to bitstring
 */
std::string string_of_radix_to_bitstring(std::string orig_string, short radix)
{
	std::string result = "";	

    if(orig_string.empty())
        return "";

    switch(radix)
	{
		case 2:    
            assert_Werr(std::string::npos == orig_string.find_first_not_of("xXzZ01"),
                    "INVALID BIT INPUT: " + orig_string + "for radix 2"
            );
            break;

		case 8:    
            assert_Werr(std::string::npos == orig_string.find_first_not_of("xXzZ01234567"),
                    "INVALID BIT INPUT: " + orig_string + "for radix 8"
            );
            break;

		case 10:    
            assert_Werr(std::string::npos == orig_string.find_first_not_of("0123456789"),
                    "INVALID BIT INPUT: " + orig_string + "for radix 10"
            );
            break;

		case 16:    
            assert_Werr(std::string::npos == orig_string.find_first_not_of("xZzZ0123456789aAbBcCdDeEfF"),
                    "INVALID BIT INPUT: " + orig_string + "for radix 16"
            );
            break;

		default:	    
            assert_Werr(false, 
                    "invalid radix: " + std::to_string(radix)
            );
            break;
	}

	while(!orig_string.empty())
	{
		switch(radix)
		{
			case 10:
			{
				std::string new_number = "";

				char rem_digit = '0';
				for(char& current_digit : orig_string)
				{
					uint8_t new_pair = (static_cast<uint8_t>(rem_digit - '0')*10) + static_cast<uint8_t>(current_digit-'0');
					new_number.push_back(static_cast<char>((new_pair/2) + '0'));
                    rem_digit =         (static_cast<char>((new_pair%2) + '0'));
				}

                result.insert(result.begin(),rem_digit);
                if(new_number == "0")
                    orig_string = "";
                else
                    orig_string = new_number;

				break;
			}
			default:
			{
                result = radix_digit_to_bits(orig_string.back(), radix) + result;
                orig_string.pop_back();
                break;
			}
		}
	}
	return result;
}
