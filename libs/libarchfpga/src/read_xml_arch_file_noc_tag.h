
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

#endif //VTR_READ_XML_ARCH_FILE_NOC_TAG_H
