/****************************************************************************************
 * Y.G.THIEN
 * 29 AUG 2012
 *
 * This file contains functions related to placement macros. The term "placement macros"
 * refers to a structure that contains information on blocks that need special treatment
 * during placement and possibly routing.
 *
 * An example of placement macros is a carry chain. Blocks in a carry chain have to be
 * placed in a specific orientation or relative placement so that the carry_in's and the
 * carry_out's are properly aligned. With that, the carry chains would be able to use the
 * direct connections specified in the arch file. Direct connections with the pin's
 * fc_value 0 would be treated specially in routing where the whole carry chain would be
 * treated as a unit and regular routing would not be used to connect the carry_in's and
 * carry_out's. Floorplanning constraints may also be an example of placement macros.
 *
 * The function alloc_and_load_placement_macros allocates and loads the placement
 * macros in the following steps:
 * (1) First, go through all the block types and mark down the pins that could possibly
 * be part of a placement macros.
 * (2) Then, go through the netlist of all the pins marked in (1) to find out all the
 * heads of the placement macros using criteria depending on the type of placement
 * macros. For carry chains, the heads of the placement macros are blocks with
 * carry_in's not connected to any nets (OPEN) while the carry_out's connected to the
 * netlist with only 1 SINK.
 * (3) Traverse from the heads to the tails of the placement macros and load the
 * information in the t_pl_macro data structure. Similar to (2), tails are identified
 * with criteria depending on the type of placement macros. For carry chains, the
 * tails are blocks with carry_out's not connected to any nets (OPEN) while the
 * carry_in's is connected to the netlist which has only 1 SINK.
 *
 * The only placement macros supported at the moment are the carry chains with limited
 * functionality.
 *
 * Current support for placement macros are:
 * (1) The arch parser for direct connections is working. The specifications of the direct
 * connections are specified in sample_adder_arch.xml and also in the
 * VPR_User_Manual.doc
 * (2) The placement macros allocator and loader is working.
 * (3) The initial placement of placement macros that respects the restrictions of the
 * placement macros is working.
 * (4) The post-placement legality check for placement macros is working.
 *
 * Current limitations on placement macros are:
 * (1) One block could only be a part of a carry chain. In the future, if a block is part
 * of multiple placement macros, we should load 1 huge placement macro instead of
 * multiple placement macros that contain the same block.
 * (2) Bus direct connections (direct connections with multiple bits) are supported.
 * However, a 2-bit carry chain when loaded would become 2 1-bit carry chains.
 * And because of (1), only 1 1-bit carry chain would be loaded. In the future,
 * placement macros with multiple-bit connections or multiple 1-bit connections
 * should be allowed.
 * (3) Placement macros that span longer or wider than the chip would cause an error.
 * In the future, we *might* expand the size of the chip to accommodate such
 * placement macros that are crucial.
 *
 * In order for the carry chain support to work, two changes are required in the
 * arch file.
 * (1) For carry chain support, added in a new child in <layout> called <directlist>.
 * <directlist> specifies a list of available direct connections on the FPGA chip
 * that are necessary for direct carry chain connections. These direct connections
 * would be treated specially in routing if the fc_value for the pins is specified
 * as 0. Note that only direct connections that has fc_value 0 could be used as a
 * carry chain.
 *
 * A <directlist> may have 0 or more children called <direct>. For each <direct>,
 * there are the following fields:
 * 1) name:  This specifies the name given to this particular direct connection.
 * 2) from_pin:  This specifies the SOURCEs for this direct connection. The format
 * could be as following:
 * a) type_name.port_name, for all the pins in this port.
 * b) type_name.port_name [end_pin_index:start_pin_index], for a
 * single pin, the end_pin_index and start_pin_index could be
 * the same.
 * 3) to_pin:  This specifies the SINKs for this direct connection. The format is
 * the same as from_pin.
 * Note that the width of the from_pin and to_pin has to match.
 * 4) x_offset: This specifies the x direction that this connection is going from
 * SOURCEs to SINKs.
 * 5) y_offset: This specifies the y direction that this connection is going from
 * SOURCEs to SINKs.
 * Note that the x_offset and y_offset could not both be 0.
 * 6) z_offset: This specifies the z sublocations that all the blocks in this
 * direct connection to be at.
 *
 * The example of a direct connection specification below shows a possible carry chain
 * connection going north on the FPGA chip:
 * _______________________________________________________________________________
 * | <directlist>                                                                  |
 * |   <direct name="adder_carry" from_pin="adder.cout" to_pin="adder.cin"         |
 * |           x_offset="0" y_offset="1" z_offset="0"/>                            |
 * | </directlist>                                                                 |
 * |_______________________________________________________________________________|
 * A corresponding arch file that has this direct connection is sample_adder_arch.xml
 * A corresponding blif file that uses this direct connection is adder.blif
 *
 * (2) As mentioned in (1), carry chain connections using the directs would only be
 * recognized if the pin's fc_value is 0. In order to achieve this, pin-based fc_value
 * is required. Hence, the new <fc> tag replaces both <fc_in> and <fc_out> tags.
 *
 * A <fc> tag may have 0 or more children called <pin>. For each <fc>, there are the
 * following fields:
 * 1) in_type: This specifies the default fc_type for input pins. They could
 * be "frac", "abs" or "full".
 * 2) in_val: This specifies the default fc_value for input pins.
 * 3) out_type: This specifies the default fc_type for output pins. They could
 * be "frac", "abs" or "full".
 * 4) out_val: This specifies the default fc_value for output pins.
 *
 * As for the <pin> children, there are the following fields:
 * 1) name: This specifies the name of the port/pin that the fc_type and fc_value
 * apply to. The name have to be in the format "port_name" or
 * "port_name [end_pin_index:start_pin_index]" where port_name is the name
 * of the port it apply to while end_pin_index and start_pin_index could
 * be specified to apply the fc_type and fc_value that follows to part of
 * a bus (multi-pin) port.
 * 2) fc_type: This specifies the fc_type that would be applied to the specified pins.
 * 3) fc_val: This specifies the fc_value that would be applied to the specified pins.
 *
 * The example of a pin-based fc_value specification below shows that the fc_values for
 * the cout and the cin ports are 0:
 * _______________________________________________________________________________
 * | <fc in_type="frac" in_val="0.15" out_type="frac"      |
 * |     out_val="0.15">                                                   |
 * |    <pin name="cin" fc_type="frac" fc_val="0"/>                                |
 * |    <pin name="cout" fc_type="frac" fc_val="0"/>                               |
 * | </fc>                                                                         |
 * |_______________________________________________________________________________|
 * A corresponding arch file that has this direct connection is sample_adder_arch.xml
 * A corresponding blif file that uses this direct connection is adder.blif
 *
 ****************************************************************************************/

#ifndef PLACE_MACRO_H
#define PLACE_MACRO_H
#include <vector>

#include "physical_types.h"

/* These are the placement macro structure.
 * It is in the form of array of structs instead of
 * structs of arrays for cache efficiency.
 * Could have more data members for other macro type.
 * blk_index: The cluster_ctx.blocks index of this block.
 * x_offset: The x_offset from the first macro member to this member
 * y_offset: The y_offset from the first macro member to this member
 * z_offset: The z_offset from the first macro member to this member
 */
struct t_pl_macro_member {
    ClusterBlockId blk_index;
    t_pl_offset offset;
};

/* num_blocks: The number of blocks this macro contains.
 * members: An array of blocks in this macro [0:num_macro-1].
 * idirect: The direct index as specified in the arch file
 */
struct t_pl_macro {
    std::vector<t_pl_macro_member> members;
};

/* These are the function declarations. */
std::vector<t_pl_macro> alloc_and_load_placement_macros(t_direct_inf* directs, int num_directs);
void get_imacro_from_iblk(int* imacro, ClusterBlockId iblk, const std::vector<t_pl_macro>& macros);
void free_placement_macros_structs();

#endif
