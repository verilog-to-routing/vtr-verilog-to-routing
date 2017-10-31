#pragma once

namespace tatum {

class SerialWalker;

class ParallelLevelizedCilkWalker;

///The default parallel graph walker
using ParallelWalker = ParallelLevelizedCilkWalker;

} //namespace

