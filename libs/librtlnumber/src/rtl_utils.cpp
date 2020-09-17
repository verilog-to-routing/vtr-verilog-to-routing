/* Authors: Aaron Graham (aaron.graham@unb.ca, aarongraham9@gmail.com),
 *           Jean-Philippe Legault (jlegault@unb.ca, jeanphilippe.legault@gmail.com),
 *            Alexandrea Demmings (alexandrea.demmings@unb.ca, lxdemmings@gmail.com) and
 *             Dr. Kenneth B. Kent (ken@unb.ca)
 *             for the Reconfigurable Computing Research Lab at the
 *              Univerity of New Brunswick in Fredericton, New Brunswick, Canada
 */

#include "rtl_utils.hpp"
#include <algorithm>
#include <iostream>

static const char* base_10_digits = "0123456789";

static int to_nb(char val, short base) {
    if (base == 256) {
        return (int)val;
    } else if (val >= '0' && val <= '9') {
        return val - '0';
    } else {
        return tolower(val) - 'a' + 10;
    }
}

static char to_chr(int val, short base, bool uppercase) {
    if (base == 256) {
        return (char)val;
    } else if (val >= 0 and val <= 9) {
        return val + '0';
    } else if (!uppercase) {
        return (val - 10) + 'a';
    } else {
        return (val - 10) + 'A';
    }
}

std::string convert_between_bases(std::string str, uint8_t base_from, uint8_t base_to, bool uppercase, bool big_endian) {
    std::string digits = "";
    while (str != "0") {
        int carry = 0;

        size_t start = (big_endian) ? str.size() - 1 : 0;
        size_t end = (big_endian) ? 0 : str.size() - 1;
        size_t increment = (big_endian) ? -1 : 1;

        for (size_t i = start; (big_endian) ? (i >= end && i <= start) : (i >= start && i <= end); i += increment) {
            int temp = to_nb(str[i], base_from);
            temp += base_from * carry;
            carry = temp % base_to;
            temp = temp / base_to;
            str[i] = to_chr(temp, base_from, uppercase);
        }

        if (big_endian) {
            digits.push_back(to_chr(carry, base_to, uppercase));
            while (str.size() > 1 && str.back() == '0') {
                str.pop_back();
            }
        } else {
            digits.insert(0, 1, to_chr(carry, base_to, uppercase));
            while (str.size() > 1 && str[0] == '0') {
                str.erase(0, 1);
            }
        }
    }
    return digits;
}

static uint8_t _to_decimal(char digit, const char* FUNCT, int LINE) {
    switch (std::tolower(digit)) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        default:
            _assert_Werr(false, FUNCT, LINE,
                         "INVALID BIT INPUT: " + std::string(1, digit));
            break;
    }
    return 10;
}

#define to_decimal(num) _to_decimal(num, __func__, __LINE__)

static std::string _radix_digit_to_bits_str(const char digit, size_t radix, const char* FUNCT, int LINE) {
    switch (radix) {
        case 2: {
            switch (std::tolower(digit)) {
                case '0':
                    return "0";
                case '1':
                    return "1";
                case 'x':
                    return "x";
                case 'z':
                    return "z";
                default:
                    _assert_Werr(false, FUNCT, LINE,
                                 "INVALID BIT INPUT: " + std::string(1, digit));
                    break;
            }
            break;
        }
        case 8: {
            switch (std::tolower(digit)) {
                case '0':
                    return "000";
                case '1':
                    return "001";
                case '2':
                    return "010";
                case '3':
                    return "011";
                case '4':
                    return "100";
                case '5':
                    return "101";
                case '6':
                    return "110";
                case '7':
                    return "111";
                case 'x':
                    return "xxx";
                case 'z':
                    return "zzz";
                default:
                    _assert_Werr(false, FUNCT, LINE,
                                 "INVALID BIT INPUT: " + std::string(1, digit));
                    break;
            }
            break;
        }
        case 16: {
            switch (std::tolower(digit)) {
                case '0':
                    return "0000";
                case '1':
                    return "0001";
                case '2':
                    return "0010";
                case '3':
                    return "0011";
                case '4':
                    return "0100";
                case '5':
                    return "0101";
                case '6':
                    return "0110";
                case '7':
                    return "0111";
                case '8':
                    return "1000";
                case '9':
                    return "1001";
                case 'a':
                    return "1010";
                case 'b':
                    return "1011";
                case 'c':
                    return "1100";
                case 'd':
                    return "1101";
                case 'e':
                    return "1110";
                case 'f':
                    return "1111";
                case 'x':
                    return "xxxx";
                case 'z':
                    return "zzzz";
                default:
                    _assert_Werr(false, FUNCT, LINE,
                                 "INVALID BIT INPUT: " + std::string(1, digit));
                    break;
            }
            break;
        }
        case 256: {
            std::string bitstring = "";
            char temp = digit;
            // 8 bit per char
            for (int i = 0; i < 8; i++) {
                char value = temp % 2;
                temp = temp / 2;

                bitstring.insert(bitstring.begin(), (value) ? '1' : '0');
            }
            return bitstring;
        }
        default: {
            _assert_Werr(false, FUNCT, LINE,
                         "Invalid base " + std::to_string(radix));
            break;
        }
    }
    std::abort();
}

#define radix_digit_to_bits(num, radix) _radix_digit_to_bits(num, radix, __func__, __LINE__)
static std::string _radix_digit_to_bits(const char digit, size_t radix, const char* FUNCT, int LINE) {
    std::string result = _radix_digit_to_bits_str(digit, radix, FUNCT, LINE);
    return result;
}

/**********************
 * convert from different radix to bitstring
 */
std::string string_of_radix_to_bitstring(std::string orig_string, size_t radix) {
    std::string result = "";

    switch (radix) {
        case 2:
            assert_Werr(!orig_string.empty(), "INVALID BIT INPUT: empty string");

            assert_Werr(std::string::npos == orig_string.find_first_not_of("xXzZ01"),
                        "INVALID BIT INPUT: " + orig_string + "for radix 2");
            break;

        case 8:
            assert_Werr(!orig_string.empty(), "INVALID BIT INPUT: empty string");

            assert_Werr(std::string::npos == orig_string.find_first_not_of("xXzZ01234567"),
                        "INVALID BIT INPUT: " + orig_string + "for radix 8");
            break;

        case 10:
            assert_Werr(!orig_string.empty(), "INVALID BIT INPUT: empty string");

            assert_Werr(std::string::npos == orig_string.find_first_not_of("0123456789"),
                        "INVALID BIT INPUT: " + orig_string + "for radix 10");
            break;

        case 16:
            assert_Werr(!orig_string.empty(), "INVALID BIT INPUT: empty string");

            assert_Werr(std::string::npos == orig_string.find_first_not_of("xZzZ0123456789aAbBcCdDeEfF"),
                        "INVALID BIT INPUT: " + orig_string + "for radix 16");
            break;

        case 256:
            // allow all chars
            break;

        default:
            assert_Werr(false,
                        "invalid radix: " + std::to_string(radix));
            break;
    }

    while (!orig_string.empty()) {
        switch (radix) {
            case 10: {
                std::string new_number = "";

                uint8_t rem_digit = 0;
                for (char current_digit : orig_string) {
                    uint8_t new_pair = (rem_digit * 10) + to_decimal(current_digit);

                    new_number.push_back(base_10_digits[(new_pair / 2)]);
                    rem_digit = new_pair % 2;
                }

                result.insert(result.begin(), base_10_digits[rem_digit]);
                while (new_number.size() > 1
                       && new_number[0] == '0') {
                    new_number.erase(0, 1);
                }

                if (new_number == "0") {
                    orig_string = "";
                } else {
                    orig_string = new_number;
                }

                break;
            }
            default: {
                result = radix_digit_to_bits(orig_string.back(), radix) + result;
                orig_string.pop_back();
                break;
            }
        }
    }

    result.insert(result.begin(), '0');

    return result;
}
