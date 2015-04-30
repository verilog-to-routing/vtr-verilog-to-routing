#pragma once

#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>
#include <iostream>

namespace boost {
    void assertion_failed_msg(char const * expr, char const * msg, char const * function, char const * file, long line) {
        std::cout << "***Internal Assertion Failue***"<< std::endl;
        std::cout << "   Condition: " << expr << std::endl;
        std::cout << "   Message  : " << msg << std::endl;
        std::cout << "   Location : " << file << ":" << line << " in " << function << std::endl;
        abort();
    }
}

#define ASSERT(expr) BOOST_ASSERT(expr)
#define ASSERT_MSG(expr, msg) BOOST_ASSERT_MSG(expr, msg)
