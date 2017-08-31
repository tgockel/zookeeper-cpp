#pragma once

/** \addtogroup Client
 *  \{
**/

/** \def ZKPP_USER_CONFIG
 *  A user-defined configuration file to be included before all other ZooKeeper C++ content.
**/
#ifdef ZKPP_USER_CONFIG
#   include ZKPP_USER_CONFIG
#endif

#define ZKPP_VERSION_MAJOR 0
#define ZKPP_VERSION_MINOR 1
#define ZKPP_VERSION_PATCH 0

/** \def ZKPP_DEBUG
 *  Was ZooKeeper C++ compiled in debug mode?
 *  This value must be the same between when the SO was built and when you are compiling. In general, this is not useful
 *  outside of library maintainers.
 *
 *  \warning
 *  Keep in mind this value is \e always defined. Use `#if ZKPP_DEBUG`, \e not `#ifdef ZKPP_DEBUG`.
**/
#ifndef ZKPP_DEBUG
#   define ZKPP_DEBUG 0
#endif

/** \def ZKPP_SO
 *  Are you using shared objects (DLLs in Windows)?
**/
#ifndef ZKPP_SO
#   define ZKPP_SO 1
#endif

/** \def ZKPP_EXPORT
 *  If using shared objects, this class or function should be exported.
 *
 *  \def ZKPP_IMPORT
 *  If using shared objects, this class or function should be imported.
 *
 *  \def ZKPP_HIDDEN
 *  This symbol is only visible within the same shared object in which the translation unit will end up. Symbols which
 *  are "hidden" will \e not be put into the global offset table, which means code can be more optimal when it involves
 *  hidden symbols at the cost that nothing outside of the SO can access it.
**/
#if ZKPP_SO
#   if defined(__GNUC__)
#       define ZKPP_EXPORT __attribute__((visibility("default")))
#       define ZKPP_IMPORT
#       define ZKPP_HIDDEN __attribute__((visibility("hidden")))
#   else
#       error "Unknown shared object semantics for this compiler."
#   endif
#else
#   define ZKPP_EXPORT
#   define ZKPP_IMPORT
#   define ZKPP_HIDDEN
#endif

/** \} **/

namespace zk
{

/** \addtogroup Client
 *  \{
**/

/** A simple, unowned pointer. It operates exactly like using \c *, but removes the question of \c * associativity and
 *  is easier to read when \c const qualifiers are involved.
**/
template <typename T>
using ptr = T*;

/** \} **/

}
