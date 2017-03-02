#pragma once

namespace tatum {

template<class Visitor, class DelayCalc>
class SerialWalker;

template<class Visitor, class DelayCalc>
class ParallelLevelizedCilkWalker;

///The default parallel graph walker
template<class Visitor, class DelayCalc>
using ParallelWalker = ParallelLevelizedCilkWalker<Visitor, DelayCalc>;

} //namespace

