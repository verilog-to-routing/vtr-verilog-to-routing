#pragma once

/************** Types and defines exported by route_tree_timing.c ************/
struct t_rt_node;

/* Linked list listing the children of an rt_node.                           *
 * child:  Pointer to an rt_node (child of the current node).                *
 * iswitch:  Index of the switch type used to connect to the child node.     *
 * next:   Pointer to the next linked_rt_edge in the linked list (allows     *
 *         you to get the next child of the current rt_node).                */
struct t_linked_rt_edge {
    t_rt_node* child;
    short iswitch;
    t_linked_rt_edge* next;
};

/* Structure describing one node in a routing tree (used to get net delays   *
 * incrementally during routing, as pieces are being added).                 *
 * u.child_list:  Pointer to a linked list of linked_rt_edge.  Each one of   *
 *                the linked list entries gives a child of this node.        *
 * u.next:  Used only when this node is on the free list.  Gives the next    *
 *          node on the free list.                                           *
 * parent_node:  Pointer to the rt_node that is this node's parent (used to  *
 *               make bottom to top traversals).                             *
 * re_expand:  Should this node be put on the heap as part of the partial    *
 *             routing to act as a source for subsequent connections?        *
 * parent_switch:  Index of the switch type driving this node (by its        *
 *                 parent).                                                  *
 * inode:  index (ID) of the rr_node that corresponds to this rt_node.       *
 * net_pin_index:  Net pin index associated with the rt_node. This value
 *                 ranges from 1 to fanout [1..num_pins-1]. For cases when   *
 *                 different speed paths are taken to the same SINK for      *
 *                 different pins, inode cannot uniquely identify each SINK, *
 *                 so the net pin index guarantees an unique identification  *
 *                 for each SINK rt_node. For non-SINK nodes and for SINK    *
 *                 nodes with no associated net pin index, (i.e. special     *
 *                 SINKs like the source of a clock tree which do not        *
 *                 correspond to an actual netlist connection), the value    *
 *                 for this member should be set to OPEN (-1).               *
 * C_downstream:  Total downstream capacitance from this rt_node.  That is,  *
 *                the total C of the subtree rooted at the current node,     *
 *                including the C of the current node.                       *
 * R_upstream:  Total upstream resistance from this rt_node to the net       *
 *              source, including any device_ctx.rr_nodes[].R of this node.  *
 * Tdel:  Time delay for the signal to get from the net source to this node. *
 *        Includes the time to go through this node.                         */
struct t_rt_node {
    union {
        t_linked_rt_edge* child_list;
        t_rt_node* next;
    } u;
    t_rt_node* parent_node;
    short parent_switch;
    bool re_expand;
    int inode;
    int net_pin_index;
    float C_downstream;
    float R_upstream;
    float Tdel;
};
