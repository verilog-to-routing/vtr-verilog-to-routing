

#include "aig.h"

ABC_NAMESPACE_IMPL_START


void Aig_ProcedureTest()
{
    Aig_Man_t * p;
    Aig_Obj_t * pA, * pB, * pC;
    Aig_Obj_t * pFunc;
    Aig_Obj_t * pFunc2;

    p = Aig_ManStart( 1000 );

    pA = Aig_IthVar( p, 0 );
    pB = Aig_IthVar( p, 1 );
    pC = Aig_IthVar( p, 2 );

    pFunc = Aig_Mux( p, pA, pB, pC );
    pFunc2 = Aig_And( p, pA, pB );

    Aig_ObjCreatePo( p, pFunc );
    Aig_ObjCreatePo( p, pFunc2 );

    Aig_ManSetRegNum( p, 1 );

    Aig_ManCleanup( p );

    if ( !Aig_ManCheck( p ) )
    {
        printf( "Check has failed\n" );
    }

    Aig_ManDumpBlif( p, "aig_test_file.blif", NULL, NULL );
    Aig_ManStop( p );
}ABC_NAMESPACE_IMPL_END

