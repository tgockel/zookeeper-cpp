/** \file
 *  Controls the import of the \c string_view type. This is probably \c std::string_view, but can be a custom type (as
 *  long as it behaves in a manner similar enough to \c std::string_view).
**/
#pragma once

#include <zk/config.hpp>

/** \addtogroup Client
 *  \{
**/

#ifndef ZKPP_STRING_VIEW_USE_STD_EXPERIMENTAL
#   define ZKPP_STRING_VIEW_USE_STD_EXPERIMENTAL 0
#endif

#ifndef ZKPP_STRING_VIEW_USE_CUSTOM
#   define ZKPP_STRING_VIEW_USE_CUSTOM 0
#endif

#ifndef ZKPP_STRING_VIEW_USE_STD
#   if ZKPP_STRING_VIEW_USE_STD_EXPERIMENTAL || ZKPP_STRING_VIEW_USE_CUSTOM
#       define ZKPP_STRING_VIEW_USE_STD 0
#   else
#       define ZKPP_STRING_VIEW_USE_STD 1
#   endif
#endif

#if ZKPP_STRING_VIEW_USE_STD
#   define ZKPP_STRING_VIEW_INCLUDE <string_view>
#   define ZKPP_STRING_VIEW_TYPE    std::string_view
#elif ZKPP_STRING_VIEW_USE_STD_EXPERIMENTAL
#   define ZKPP_STRING_VIEW_INCLUDE <experimental/string_view>
#   define ZKPP_STRING_VIEW_TYPE    std::experimental::string_view
#elif ZKPP_STRING_VIEW_USE_CUSTOM
#   if !defined ZKPP_STRING_VIEW_INCLUDE || !defined ZKPP_STRING_VIEW_TYPE
#       error "When ZKPP_STRING_VIEW_USE_CUSTOM is set, you must also define ZKPP_STRING_VIEW_INCLUDE and"
#       error "ZKPP_STRING_VIEW_TYPE."
#   endif
#else
#   error "Unknown type to use for zk::string_view"
#endif

#include ZKPP_STRING_VIEW_INCLUDE

namespace zk
{

using string_view = ZKPP_STRING_VIEW_TYPE;

}

/** \} **/
