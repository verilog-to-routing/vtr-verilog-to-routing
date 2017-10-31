#ifndef VPR_ATOM_LOOKUP_FWD_H
#define VPR_ATOM_LOOKUP_FWD_H

class AtomLookup;

enum class BlockTnode {
    INTERNAL, //tnodes corresponding to internal paths withing atom blocks
    EXTERNAL  //tnodes corresponding to exteranl interface of atom blocks
};

#endif
