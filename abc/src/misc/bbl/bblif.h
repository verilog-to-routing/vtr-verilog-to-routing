/**CFile****************************************************************

  FileName    [bblif.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Binary BLIF representation for logic networks.]

  Synopsis    [External declarations.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - February 28, 2009.]

  Revision    [$Id: bblif.h,v 1.00 2009/02/28 00:00:00 alanmi Exp $]

***********************************************************************/
 
#ifndef ABC__aig__bbl__bblif_h
#define ABC__aig__bbl__bblif_h


/*
    This file (taken together with "bblif.c") implements a stand-alone 
    interface between ABC and an application that uses ABC. 
    
    The interface is designed to pass a combinational logic network 
    from the calling application to ABC using a binary BLIF format (BBLIF) 
    and return the network after synthesis/mapping/verification in ABC 
    back to the caller.

    The interface can do the following:
    (1) accept a combinational logic network via a set of APIs
    (2) write the logic network into a binary BLIF file readable by ABC
    (3) read a binary BLIF file with a mapped network produced by ABC
    (4) return the mapped network to the caller through a set of APIs

    It should be noted that the BBLIF interface can be used to pass
    the network from the calling application into ABC without writing it
    into a file. In this case, ABC should be compiled as a library and
    linked to the calling application. The BBLIF manager can be given
    directly to the procedure Bbl_ManToAbc() to convert it into an AIG.
    Similarly, the resulting mapped network can be converted into
    BBLIF manager and passed back after the call to Bbl_ManFromAbc().

    Here these steps are described in more detail:

    (1) The BBLIF manager is allocated by calling Bbl_ManStart() and 
        deallocated by calling Bbl_ManStop(). 
    
        The combinational network is composed of three types of objects:
        (a) combinational inputs (CIs), (b) combinational outputs (COs), 
        (c) internal logic nodes represented using Sum-of-Products (SOPs)
        similar to the way logic nodes are represented in SIS. Sequential 
        elements (flops) are currently not supported.   A CI has no fanins. 
        A CO has exactly one fanin and no fanouts. Internal nodes can 
        have any number of fanins and fanouts. Only an internal node can 
        have a logic function.

        Before constructing the BBLIF manager, each object should be 
        assigned a unique non-negative (0-based) integer ID. The sequence
        of ID numbers may have gaps in it (for example, 0, 1, 2, 5, 6, etc)
        but care should be taken that the ID numbers do not grow too large
        because internally they are used to index the objects. So if 
        the largest given ID has value N, an array of 4*N bytes will be 
        allocated internally by the BBLIF manager. Obviously if N = 1M, 
        the array will use 4Mb, but if N = 100M, it will use 0.4Gb.

        This object ID (called also "the original ID of the object") is 
        given to Bbl_ManCreateObject(), which construct the BBLIF objects 
        and to the procedure Bbl_ManAddFanin(), which creates fanin/fanout
        relations between two objects.  The exact number of fanins of an
        object should be declared when calling Bbl_ManCreateObject(). 
        Later on, each node should be assigned as many fanins using 
        Bbl_ManAddFanin(). The order/number of fanins corresponds to the 
        order/number of variables in the SOP of the logic function of the 
        node. The declared and actual number of fanins should be the same.
        otherwise the interface will not function correctly. This is checked
        by the procedure Bbl_ManCheck(), which should be called when 
        constructing all objects and their fanins is finished.

        The SOP representation of the logic function should be given to
        every internal node. It is given as a C-string, showing the SOP
        as it would appear in a BLIF or PLA file. Each cube is composed
        of characters '0', '1', and '-', and ended by a seqence of three
        characters: space ' ', followed by '0' or '1' (depending on whether 
        on- or off-set is used), followed by the new line character '\n'. 
        For example, a two-input OR has the following SOP representation: 
        "1- 1\n-1 1\n", or equivalently, "00 0\n". The SOP for a constant 
        function with no fanins is represented as " 0\n" (constant 0) and
        " 1\n" (constant 1). SOP for a constant node with some fanins
        may also be represented. For example, constant 0 node with three 
        fanins will have SOP representation as follows: "--- 0\n".

        The objects can be added to the BBLIF manager in any order, but 
        by the time the fanin/fanout connections are created, corresponding 
        objects should be already created.

        The number of objects is limited by 2^31. The number of fanins
        of one object is restricted to 2^28. The SOP representation can 
        have arbitrary many products (cubes), as long as memory is enough
        to represent them in the C-string form, as described above.

    (2) To write the manager into a file, call procedure Bbl_ManDumpBinaryBlif().
        It is recommended to use files with extension ".bblif" because it
        will allow ABC to call the approapriate reader in command "read".

    (3) To read the network from file, call procedure Bbl_ManReadBinaryBlif().

    (4) It is assumed that ABC will return the network after mapping.
        This network will arrive in a BBLIF file, from which the BBLIF 
        manager is created by the call to Bbl_ManReadBinaryBlif(). The 
        following APIs are useful to extract the mapped network from the manager:
        
        Iterator Bbl_ManForEachObj() iterates through the pointers to the
        BBLIF objects, which are guaranteed to be in a topological order. 

        For each object, the following APIs can be used:
        Bbl_ObjIsInput() returns 1 if the object is a CI
        Bbl_ObjIsOutput() returns 1 if the object is a CO
        Bbl_ObjIsLut() returns 1 if the object is a logic node (lookup table)
        Bbl_ObjFaninNumber() returns the number of fanins of the node
        Bbl_ObjSop() returns the SOP representation of the node, as described above.

        A special attention should be given to the representation of object IDs
        after mapping. Recall that when the outgoing BBLIF network is constructed, 
        the IDs of objects are assigned by the calling application and given to 
        the BBLIF manager when procedure Bbl_ManCreateObject() is called.  
        We refer to these object IDs as "original IDs of the objects". 
        
        When the network has been given to ABC, mapped, and returned to the 
        calling application in the incoming BBLIF file, only CIs and COs are 
        guaranteed to preserve their "original IDs". Other objects may be created 
        during synthesis/mapping. The original IDs of these objects are set to -1. 

        The following two APIs are used to return the IDs of objects after mapping:
        Bbl_ObjId() returns the new ID (useful to construct network after mapping)
        Bbl_ObjIdOriginal() returns the original ID (or -1 if this is a new object).

        !!!***!!!
        Note: The original ID currently cannot be returned by Bbl_ObjIdOriginal().
        It is recommended to use the work-around described below.
        !!!***!!!

        The original ID is useful to map CIs/COs after mapping into CIs/COs before
        mapping. However, the order of CIs/COs after mapping in the incoming network 
        is the same as the order of their creation by the calling application 
        in the outgoing network. This allows for a workaround that does not have 
        the need for the original IDs.  We can simply iterate through the objects 
        after mapping, and create CIs and COs in the order of their appearance, 
        and this order is guaranteed to be the same as the order of their 
        construction by the calling application.

        It is also worth noting that currently the internal node names are not 
        preserved by ABC during synthesis. This may change in the future. and then 
        some of the internal nodes will preserve their IDs, which may allow the 
        calling application to reconstruct the names of some of the nodes after 
        synthesis/mapping in ABC using their original IDs whenever available.

        Finally, iterator Bbl_ObjForEachFanin() can be used to iterate through 
        the fanins of each mapped object. For CIs, there will be no fanins. 
        For COs, there will be exactly one fanin. For the internal nodes (LUTs)
        the number of fanins is the number of inputs of these nodes.

    A demo of using this interface is included at the bottom of file "bblif.c" in 
    procedure Bbl_ManSimpleDemo(). Additional examples can be found in the files
    "abc\src\base\io\ioReadBblif.c" and "abc\src\base\io\ioWriteBblif.c". These
    files illustrate how an ABC network is created from the BBLIF data manager 
    and how the data manager is created from the ABC network. 
    
    Note that only the files "bblif.h" and "bblif.c" are needed for interfacing
    the user's application with ABC, while other files should not be compiled 
    as part of the application code.

    Finally, a warning regarding endianness. The interface may not work 
    if the BBLIF file is produced on a machine whose engianness is different 
    from the machine, which is reading this file.
*/

////////////////////////////////////////////////////////////////////////
///                          INCLUDES                                ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                         PARAMETERS                               ///
////////////////////////////////////////////////////////////////////////



ABC_NAMESPACE_HEADER_START


#ifdef _WIN32
#define inline __inline
#endif

////////////////////////////////////////////////////////////////////////
///                         BASIC TYPES                              ///
////////////////////////////////////////////////////////////////////////

// object types
typedef enum { 
    BBL_OBJ_NONE,                // 0: non-existent object
    BBL_OBJ_CI,                  // 1: primary input
    BBL_OBJ_CO,                  // 2: primary output
    BBL_OBJ_NODE,                // 3: buffer node
    BBL_OBJ_VOID                 // 4: unused object
} Bbl_Type_t;

// data manager
typedef struct Bbl_Man_t_ Bbl_Man_t;

// data object
typedef struct Bbl_Obj_t_ Bbl_Obj_t;

////////////////////////////////////////////////////////////////////////
///                      MACRO DEFINITIONS                           ///
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
///                    FUNCTION DECLARATIONS                         ///
////////////////////////////////////////////////////////////////////////

// (1) creating the data manager in the application code
extern Bbl_Man_t * Bbl_ManStart( char * pName );
extern void        Bbl_ManCreateObject( Bbl_Man_t * p, Bbl_Type_t Type, int ObjId, int nFanins, char * pSop );
extern void        Bbl_ManAddFanin( Bbl_Man_t * p, int ObjId, int FaninId );
extern int         Bbl_ManCheck( Bbl_Man_t * p );
extern void        Bbl_ManPrintStats( Bbl_Man_t * p );
extern void        Bbl_ManStop( Bbl_Man_t * p );

// (2) writing the data manager into file
extern void        Bbl_ManDumpBinaryBlif( Bbl_Man_t * p, char * pFileName );

// (3) reading the data manager from file
extern Bbl_Man_t * Bbl_ManReadBinaryBlif( char * pFileName );

// (4) returning the mapped network after reading the data manaager from file
extern char *      Bbl_ManName( Bbl_Man_t * p );
extern int         Bbl_ObjIsInput( Bbl_Obj_t * p );
extern int         Bbl_ObjIsOutput( Bbl_Obj_t * p );
extern int         Bbl_ObjIsLut( Bbl_Obj_t * p );
extern int         Bbl_ObjId( Bbl_Obj_t * p );
extern int         Bbl_ObjIdOriginal( Bbl_Man_t * pMan, Bbl_Obj_t * p );
extern int         Bbl_ObjFaninNumber( Bbl_Obj_t * p );
extern char *      Bbl_ObjSop( Bbl_Man_t * pMan, Bbl_Obj_t * p );

// for the use in iterators only
extern Bbl_Obj_t * Bbl_ManObjFirst( Bbl_Man_t * p );
extern Bbl_Obj_t * Bbl_ManObjNext( Bbl_Man_t * p, Bbl_Obj_t * pObj );
extern Bbl_Obj_t * Bbl_ObjFaninFirst( Bbl_Obj_t * p );
extern Bbl_Obj_t * Bbl_ObjFaninNext( Bbl_Obj_t * p, Bbl_Obj_t * pPrev );

// iterator through the objects
#define Bbl_ManForEachObj( p, pObj )                \
    for ( pObj = Bbl_ManObjFirst(p); pObj; pObj = Bbl_ManObjNext(p, pObj) )
// iterator through the fanins fo the an object
#define Bbl_ObjForEachFanin( pObj, pFanin )         \
    for ( pFanin = Bbl_ObjFaninFirst(pObj); pFanin; pFanin = Bbl_ObjFaninNext(pObj, pFanin) )

// these additional procedures are provided to transform truth tables into SOPs, and vice versa
extern char *      Bbl_ManTruthToSop( unsigned * pTruth, int nVars );
extern unsigned *  Bbl_ManSopToTruth( char * pSop, int nVars );

// write text BLIF file for debugging
extern void        Bbl_ManDumpBlif( Bbl_Man_t * p, char * pFileName );

// a simple demo procedure
extern void        Bbl_ManSimpleDemo();




ABC_NAMESPACE_HEADER_END



#endif

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////

