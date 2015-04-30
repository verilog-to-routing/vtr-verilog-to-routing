#pragma once

//Use custom assert message formatter
#define BOOST_ENABLE_ASSERT_HANDLER
#include <boost/assert.hpp>

#define ASSERT(expr) BOOST_ASSERT(expr)
#define ASSERT_MSG(expr, msg) BOOST_ASSERT_MSG(expr, msg)
#define VERIFY(expr) BOOST_VERIFY(expr)
#define VERIFY_MSG(expr, msg) BOOST_VERIFY_MSG(expr, msg)

