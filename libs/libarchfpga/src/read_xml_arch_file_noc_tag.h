
#ifndef VTR_READ_XML_ARCH_FILE_NOC_TAG_H
#define VTR_READ_XML_ARCH_FILE_NOC_TAG_H

#include "pugixml.hpp"
#include "pugixml_loc.hpp"
#include "physical_types.h"

/**
 * @brief Parses NoC-related information under <noc> tag.
 * @param noc_tag An XML node pointing to <noc> tag.
 * @param arch High-level architecture information. This function fills
 * arch->noc with NoC-related information.
 * @param loc_data Points to the location in the xml file where the parser is reading.
 */
void process_noc_tag(pugi::xml_node noc_tag,
                     t_arch* arch,
                     const pugiutil::loc_data& loc_data);

/**
 * @brief Describes a mesh topology as specified in the architecture file.
 *        It is assumed that NoC routers are equally distanced in each axis.
 */
struct t_mesh_region {
    /// The location the bottom left NoC router on the X-axis.
    float start_x;
    /// The location the top right NoC router on the X-axis.
    float end_x;
    /// The location the bottom left NoC router on the Y-axis.
    float start_y;
    /// The location the top right NoC router on the Y-axis.
    float end_y;
    /// The layer from which the mesh start.
    int start_layer;
    /// The layer at which the mesh ends.
    int end_layer;
    /// The number of NoC routers in each row or column.
    int mesh_size;
};

#endif //VTR_READ_XML_ARCH_FILE_NOC_TAG_H
