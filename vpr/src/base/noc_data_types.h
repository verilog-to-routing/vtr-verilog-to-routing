#ifndef NOC_DATA_TYPES_H
#define NOC_DATA_TYPES_H

#include "vtr_strong_id.h"

// data types used to index the routers and links within the noc

struct noc_router_id_tag;
struct noc_link_id_tag;

typedef vtr::StrongId<noc_router_id_tag, int> NocRouterId;
typedef vtr::StrongId<noc_link_id_tag, int> NocLinkId;

#endif