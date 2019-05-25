#pragma once

struct RouterStats {
    size_t connections_routed = 0;
    size_t nets_routed = 0;
    size_t heap_pushes = 0;
    size_t heap_pops = 0;
};
