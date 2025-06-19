#include "global.h"

#include "internal.hpp"

ABC_NAMESPACE_IMPL_START

namespace CaDiCaL {

/*------------------------------------------------------------------------*/

bool parse_int_str (const char *val_str, int &val) {
  if (!strcmp (val_str, "true"))
    val = 1;
  else if (!strcmp (val_str, "false"))
    val = 0;
  else {
    const char *p = val_str;
    int sign;

    if (*p == '-')
      sign = -1, p++;
    else
      sign = 1;

    int ch;
    if (!isdigit ((ch = *p++)))
      return false;

    const int64_t bound = -(int64_t) INT_MIN;
    int64_t mantissa = ch - '0';

    while (isdigit (ch = *p++)) {
      if (bound / 10 < mantissa)
        mantissa = bound;
      else
        mantissa *= 10;
      const int digit = ch - '0';
      if (bound - digit < mantissa)
        mantissa = bound;
      else
        mantissa += digit;
    }

    int exponent = 0;
    if (ch == 'e') {
      while (isdigit ((ch = *p++)))
        exponent = exponent ? 10 : ch - '0';
      if (ch)
        return false;
    } else if (ch)
      return false;

    CADICAL_assert (exponent <= 10);
    int64_t val64 = mantissa;
    for (int i = 0; i < exponent; i++)
      val64 *= 10;

    if (sign < 0) {
      val64 = -val64;
      if (val64 < INT_MIN)
        val64 = INT_MIN;
    } else {
      if (val64 > INT_MAX)
        val64 = INT_MAX;
    }

    CADICAL_assert (INT_MIN <= val64);
    CADICAL_assert (val64 <= INT_MAX);

    val = val64;
  }
  return true;
}

/*------------------------------------------------------------------------*/

bool has_suffix (const char *str, const char *suffix) {
  size_t k = strlen (str), l = strlen (suffix);
  return k > l && !strcmp (str + k - l, suffix);
}

bool has_prefix (const char *str, const char *prefix) {
  for (const char *p = str, *q = prefix; *q; q++, p++)
    if (*q != *p)
      return false;
  return true;
}

/*------------------------------------------------------------------------*/

bool is_color_option (const char *arg) {
  return !strcmp (arg, "--color") || !strcmp (arg, "--colors") ||
         !strcmp (arg, "--colour") || !strcmp (arg, "--colours") ||
         !strcmp (arg, "--color=1") || !strcmp (arg, "--colors=1") ||
         !strcmp (arg, "--colour=1") || !strcmp (arg, "--colours=1") ||
         !strcmp (arg, "--color=true") || !strcmp (arg, "--colors=true") ||
         !strcmp (arg, "--colour=true") || !strcmp (arg, "--colours=true");
}

bool is_no_color_option (const char *arg) {
  return !strcmp (arg, "--no-color") || !strcmp (arg, "--no-colors") ||
         !strcmp (arg, "--no-colour") || !strcmp (arg, "--no-colours") ||
         !strcmp (arg, "--color=0") || !strcmp (arg, "--colors=0") ||
         !strcmp (arg, "--colour=0") || !strcmp (arg, "--colours=0") ||
         !strcmp (arg, "--color=false") ||
         !strcmp (arg, "--colors=false") ||
         !strcmp (arg, "--colour=false") ||
         !strcmp (arg, "--colours=false");
}

/*------------------------------------------------------------------------*/

static uint64_t primes[] = {
    1111111111111111111lu, 2222222222222222249lu, 3333333333333333347lu,
    4444444444444444537lu, 5555555555555555621lu, 6666666666666666677lu,
    7777777777777777793lu, 8888888888888888923lu, 9999999999999999961lu,
};

uint64_t hash_string (const char *str) {
  const unsigned size = sizeof primes / sizeof *primes;
  uint64_t res = 0;
  unsigned char ch;
  unsigned i = 0;
  for (const char *p = str; (ch = *p); p++) {
    res += ch;
    res *= primes[i++];
    if (i == size)
      i = 0;
  }
  return res;
}

} // namespace CaDiCaL

ABC_NAMESPACE_IMPL_END
