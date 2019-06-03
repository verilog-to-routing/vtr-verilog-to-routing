#ifndef VRP_DOMAIN_PAIR_H
#define VRP_DOMAIN_PAIR_H

#include "tatum/TimingGraphFwd.hpp"

//A pair of clock domains
struct DomainPair {
    DomainPair(tatum::DomainId l, tatum::DomainId c)
        : launch(l)
        , capture(c) {}

    tatum::DomainId launch;
    tatum::DomainId capture;

    friend bool operator<(const DomainPair& lhs, const DomainPair& rhs) {
        return std::tie(lhs.launch, lhs.capture) < std::tie(rhs.launch, rhs.capture);
    }
};

#endif
