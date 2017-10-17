struct Abc_Obj_t_ // 6 words
{
    Abc_Obj_t *       pCopy;         // the copy of this object
    Abc_Ntk_t *       pNtk;          // the host network
    int               Id;            // the object ID
    int               TravId;        // the traversal ID 
    int               nRefs;         // the number of fanouts
    unsigned          Type    :  4;  // the object type
    unsigned          fMarkA  :  1;  // the multipurpose mark
    unsigned          fMarkB  :  1;  // the multipurpose mark
    unsigned          fPhase  :  1;  // the flag to mark the phase of equivalent node
    unsigned          fPersist:  1;  // marks the persistant AIG node
    unsigned          nFanins : 24;  // the level of the node
    Abc_Obj_t *       Fanins[0];     // the array of fanins
};

struct Abc_Pin_t_ // 4 words
{
    Abc_Pin_t *       pNext;
    Abc_Pin_t *       pPrev;
    Abc_Obj_t *       pFanin;
    Abc_Obj_t *       pFanout;
};
