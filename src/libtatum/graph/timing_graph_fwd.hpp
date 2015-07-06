#pragma once
/*
 * Forward declarations for Timing Graph and related types
 */

//The timing graph
class TimingGraph;

//Potential node types in the timing graph
enum class TN_Type;

//Various IDs used by the timing graph
typedef int NodeId;
typedef int BlockId;
typedef int EdgeId;
typedef int DomainId;
typedef int LevelId;

#define INVALID_CLOCK_DOMAIN -1

