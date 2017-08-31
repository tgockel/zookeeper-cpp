/** \file
 *  Controls the import of the \c optional and \c nullopt_t types, as well as the \c nullopt \c constexpr. These are
 *  probably \c std::optional, \c std::nullopt_t, and \c std::nullopt, respectively, but can be your custom types (as
 *  long as they behave in a manner similar enough to \c std::optional, \c std::nullopt_t, and \c std::nullopt).
**/
#pragma once

#include <zk/config.hpp>

/** \addtogroup Client
 *  \{
**/

/** \def ZKPP_OPTIONAL_USE_STD_EXPERIMENTAL
 *  Set this to 1 to use \c std::experimental::optional, \c std::experimental::nullopt_t, and
 *  \c std::experimental::nullopt as the backing types for \c zk::optional, \c zk::nullopt_t, and \c zk::nullopt.
**/
#ifndef ZKPP_OPTIONAL_USE_STD_EXPERIMENTAL
#   define ZKPP_OPTIONAL_USE_STD_EXPERIMENTAL 0
#endif

/** \def ZKPP_OPTIONAL_USE_CUSTOM
 *  Set this to 1 to use custom definitions for \c zk::optional, \c zk::nullopt_t, and \c zk::nullopt. If this is set,
 *  you must also set \c ZKPP_OPTIONAL_INCLUDE, \c ZKPP_NULLOPT_TYPE, and \c ZKPP_NULLOPT_VALUE.
 *
 *  \def ZKPP_OPTIONAL_INCLUDE
 *  The file to include to get the implementation for \c optional, \c nullopt_t, and \c nullopt. By default, this pulls
 *  from the standard library.
 *
 *  \def ZKPP_OPTIONAL_TEMPLATE
 *  The template to use for \c zk::optional. By default, this is \c std::optional.
 *
 *  \def ZKPP_NULLOPT_TYPE
 *  The type to use for \c zk::nullopt_t. By default, this is \c std::nullopt_t.
 *
 *  \def ZKPP_NULLOPT_VALUE
 *  The value to use for \c zk::nullopt. By default, this is \c std::nullopt.
**/
#ifndef ZKPP_OPTIONAL_USE_CUSTOM
#   define ZKPP_OPTIONAL_USE_CUSTOM 0
#endif

/** \def ZKPP_OPTIONAL_USE_STD
 *  Set this to 1 to use \c std::optional, \c std::nullopt_t, and \c std::nullopt as the backing types for
 *  \c zk::optional, \c zk::nullopt_t, and \c zk::nullopt. This is the default behavior.
**/
#ifndef ZKPP_OPTIONAL_USE_STD
#   if ZKPP_OPTIONAL_USE_STD_EXPERIMENTAL || ZKPP_OPTIONAL_USE_CUSTOM
#       define ZKPP_OPTIONAL_USE_STD 0
#   else
#       define ZKPP_OPTIONAL_USE_STD 1
#   endif
#endif

#if ZKPP_OPTIONAL_USE_STD
#   define ZKPP_OPTIONAL_INCLUDE  <optional>
#   define ZKPP_OPTIONAL_TEMPLATE std::optional
#   define ZKPP_NULLOPT_TYPE      std::nullopt_t
#   define ZKPP_NULLOPT_VALUE     std::nullopt
#elif ZKPP_OPTIONAL_USE_STD_EXPERIMENTAL
#   define ZKPP_OPTIONAL_INCLUDE  <experimental/optional>
#   define ZKPP_OPTIONAL_TEMPLATE std::experimental::optional
#   define ZKPP_NULLOPT_TYPE      std::experimental::nullopt_t
#   define ZKPP_NULLOPT_VALUE     std::experimental::nullopt
#elif ZKPP_OPTIONAL_USE_CUSTOM
#   if !defined ZKPP_OPTIONAL_INCLUDE || !defined ZKPP_NULLOPT_TYPE || !defined ZKPP_NULLOPT_VALUE
#       error "When ZKPP_OPTIONAL_USE_CUSTOM is set, you must also define ZKPP_OPTIONAL_INCLUDE, ZKPP_NULLOPT_TYPE, and"
#       error "ZKPP_NULLOPT_VALUE."
#   endif
#else
#   error "Unknown type to use for zk::optional, zk::nullopt_t, and zk::nullopt"
#endif

/** \} **/

#include ZKPP_OPTIONAL_INCLUDE

namespace zk
{

/** \addtogroup Client
 *  \{
**/

template <typename T>
using optional = ZKPP_OPTIONAL_TEMPLATE<T>;

using nullopt_t = ZKPP_NULLOPT_TYPE;

using ZKPP_NULLOPT_VALUE;

/** \} **/

}
