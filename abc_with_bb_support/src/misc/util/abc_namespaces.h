/**CFile****************************************************************

  FileName    [abc_namespaces.h]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Namespace logic.]

  Synopsis    []

  Author      [Alan Mishchenko]

  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - Nov 20, 2015.]

  Revision    []

***********************************************************************/

#ifndef ABC__misc__util__abc_namespaces_h
#define ABC__misc__util__abc_namespaces_h


////////////////////////////////////////////////////////////////////////
///                         NAMESPACES                               ///
////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
#  ifdef ABC_NAMESPACE
#    define ABC_NAMESPACE_HEADER_START namespace ABC_NAMESPACE {
#    define ABC_NAMESPACE_HEADER_END }
#    define ABC_NAMESPACE_IMPL_START namespace ABC_NAMESPACE {
#    define ABC_NAMESPACE_IMPL_END }
#    define ABC_NAMESPACE_PREFIX ABC_NAMESPACE::
#    define ABC_NAMESPACE_USING_NAMESPACE using namespace ABC_NAMESPACE;
#  else
#    define ABC_NAMESPACE_HEADER_START extern "C" {
#    define ABC_NAMESPACE_HEADER_END }
#    define ABC_NAMESPACE_IMPL_START
#    define ABC_NAMESPACE_IMPL_END
#    define ABC_NAMESPACE_PREFIX
#    define ABC_NAMESPACE_USING_NAMESPACE
#  endif // #ifdef ABC_NAMESPACE
#else
#  define ABC_NAMESPACE_HEADER_START
#  define ABC_NAMESPACE_HEADER_END
#  define ABC_NAMESPACE_IMPL_START
#  define ABC_NAMESPACE_IMPL_END
#  define ABC_NAMESPACE_PREFIX
#  define ABC_NAMESPACE_USING_NAMESPACE
#endif // #ifdef __cplusplus

#endif // #ifndef ABC__misc__util__abc_namespaces_h

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////
