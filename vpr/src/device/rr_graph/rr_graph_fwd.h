/**********************************************************
 * MIT License
 *
 * Copyright (c) 2018 LNIS - The University of Utah
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ***********************************************************************/

/************************************************************************
 * Filename:    rr_graph_fwd.h
 * Created by:   Xifan Tang
 * Change history:
 * +---------------------------------------------------------+
 * |  Date       |    Author     | Notes
 * +---------------------------------------------------------+
 * | 2017/11/21  |  Kevin Murray | Created as a prototype 
 * +---------------------------------------------------------+
 * | 2019/07/31  |  Xifan Tang   | Complete an alpha version
 * +---------------------------------------------------------+
 ***********************************************************************/
/************************************************************************
 * This file introduces data types and pre-decleration for the RRGraph class 
 * If you want to use RRGraph class. This is a light header file you can include in your source files. 
 ***********************************************************************/
/* IMPORTANT:
 * The following preprocessing flags are added to 
 * avoid compilation error when this headers are included in more than 1 times 
 */

#ifndef RR_GRAPH_OBJ_FWD_H
#define RR_GRAPH_OBJ_FWD_H
#include "vtr_strong_id.h"

class RRGraph;

struct rr_node_id_tag;
struct rr_edge_id_tag;
struct rr_switch_id_tag;
struct rr_segment_id_tag;

typedef vtr::StrongId<rr_node_id_tag> RRNodeId;
typedef vtr::StrongId<rr_edge_id_tag> RREdgeId;
typedef vtr::StrongId<rr_switch_id_tag, short> RRSwitchId;
typedef vtr::StrongId<rr_segment_id_tag, short> RRSegmentId;

/* Create an alias to the open NodeId
 * Useful in identifying if a node exist in a rr_graph
 */
#define RRGRAPH_OPEN_NODE_ID RRNodeId(-1)
#define RRGRAPH_OPEN_EDGE_ID RREdgeId(-1)
#define RRGRAPH_OPEN_SWITCH_ID RRSwitchId(-1)
#define RRGRAPH_OPEN_SEGMENT_ID RRSegmentId(-1)

#endif

/************************************************************
 * End Of File (EOF)
 ***********************************************************/
