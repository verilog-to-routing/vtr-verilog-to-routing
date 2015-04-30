#include <iostream>
#include "assert.hpp"

using std::cout;
using std::endl;
namespace boost {
    void assertion_failed(char const * expr, char const * function, char const * file, long line) {
        cout << "***Internal Assertion Failue***"<< endl;
        cout << "   Condition: " << expr << endl;
        cout << "   Location : " << file << ":" << line << " in " << function << endl;
        std::abort();
    }
    void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line) {
        cout << "***Internal Assertion Failue***"<< endl;
        cout << "   Condition: " << expr << endl;
        cout << "   Message  : " << msg << endl;
        cout << "   Location : " << file << ":" << line << " in " << function << endl;
        std::abort();
    }
}
