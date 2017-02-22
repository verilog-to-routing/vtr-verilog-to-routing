#pragma once

#include "ParallelLevelizedCilkWalker.hpp"

namespace tatum {

///The default parallel graph walker
template<class Visitor, class DelayCalc>
using ParallelWalker = ParallelLevelizedCilkWalker<Visitor, DelayCalc>;

} //namepsace
