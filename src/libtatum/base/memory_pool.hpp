#pragma once

//XXX: This is a somewhat hacky trick to divorce boost
//     pool from its dependance on boost::mutex, which
//     pulls in boost system and boost date time.
//     We instead override using boost::mutex and instead
//     use C++11 mutexs
#define BOOST_THREAD_MUTEX_HPP
#include <mutex>
namespace boost {
    using std::mutex;
}

#include <boost/pool/pool.hpp>

//Isolate downstream code from boost
typedef boost::pool<> MemoryPool;
