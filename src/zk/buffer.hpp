/// \file
/// Controls the \c buffer type.
#pragma once

#include <zk/config.hpp>

#include <type_traits>

/// \addtogroup Client
/// \{

// \def ZKPP_BUFFER_USE_STD_STRING
//  Set this to 1 to use \c std::string as the backing type for zk::buffer.
//
#ifndef ZKPP_BUFFER_USE_STD_STRING
#   define ZKPP_BUFFER_USE_STD_STRING 0
#endif

/// \def ZKPP_BUFFER_USE_CUSTOM
/// Set this to 1 to use a custom definitions for \c buffer. If this is set, you must also set
/// \ref ZKPP_BUFFER_INCLUDE and \ref ZKPP_BUFFER_TYPE.
///
/// \def ZKPP_BUFFER_INCLUDE
/// The header file to use to find the \c buffer type. This value is set automatically if using built-in configuration
/// options (such as \ref ZKPP_BUFFER_USE_STD_VECTOR) and must be set manually if using \ref ZKPP_BUFFER_USE_CUSTOM.
///
/// \def ZKPP_BUFFER_TYPE
/// The type name to use for the \c buffer type. This value is set automatically if using built-in configuration
/// options (such as \ref ZKPP_BUFFER_USE_STD_VECTOR) and must be set manually if using \ref ZKPP_BUFFER_USE_CUSTOM.
#ifndef ZKPP_BUFFER_USE_CUSTOM
#   define ZKPP_BUFFER_USE_CUSTOM 0
#endif

/// \def ZKPP_BUFFER_USE_STD_VECTOR
/// Set this to 1 to use \c std::vector<char> for the implementation of \ref zk::buffer. This is the default behavior.
#ifndef ZKPP_BUFFER_USE_STD_VECTOR
#   if ZKPP_BUFFER_USE_STD_STRING || ZKPP_BUFFER_USE_CUSTOM
#       define ZKPP_BUFFER_USE_STD_VECTOR 0
#   else
#       define ZKPP_BUFFER_USE_STD_VECTOR 1
#   endif
#endif

#if ZKPP_BUFFER_USE_STD_VECTOR
#   define ZKPP_BUFFER_INCLUDE <vector>
#   define ZKPP_BUFFER_TYPE std::vector<char>
#elif ZKPP_BUFFER_USE_STD_STRING
#   define ZKPP_BUFFER_INCLUDE <string>
#   define ZKPP_BUFFER_TYPE std::string
#elif ZKPP_BUFFER_USE_CUSTOM
#   if !defined ZKPP_BUFFER_INCLUDE || !defined ZKPP_BUFFER_TYPE
#       error "When ZKPP_BUFFER_USE_CUSTOM is set, you must also define ZKPP_BUFFER_INCLUDE and ZKPP_BUFFER_TYPE"
#   endif
#else
#   error "Unknown type to use for zk::buffer"
#endif

/// \}

#include ZKPP_BUFFER_INCLUDE

namespace zk
{

/// \addtogroup Client
/// \{

/// The \c buffer type. By default, this is an \c std::vector<char>, but this can be altered by compile-time flags such
/// as \ref ZKPP_BUFFER_USE_CUSTOM. The requirements for a custom buffer are minimal -- the type must fit this criteria:
///
/// | expression            | type                      | description                                                  |
/// |:----------------------|:--------------------------|:-------------------------------------------------------------|
/// | `buffer::value_type`  | `char`                    | Buffers must be made of single-byte elements                 |
/// | `buffer::size_type`   | `std::size_t`             |                                                              |
/// | `buffer(ib, ie)`      | `buffer`                  | Constructs a buffer with the range [`ib`, `ie`)              |
/// | `buffer(buffer&&)`    | `buffer`                  | Move constructible (must be `noexcept`).                     |
/// | `operator=(buffer&&)` | `buffer&`                 | Move assignable (must be `noexcept`).                        |
/// | `size()`              | `size_type`               | Get the length of the buffer                                 |
/// | `data()`              | `const value_type*`       | Get a pointer to the beginning of the contents               |
using buffer = ZKPP_BUFFER_TYPE;

// Check through static_assert:
static_assert(sizeof(buffer::value_type) == 1U, "buffer::value_type must be single-byte elements");
static_assert(std::is_same<std::size_t, buffer::size_type>::value, "buffer::size_type must be std::size_t");
static_assert(std::is_constructible<buffer, ptr<const buffer::value_type>, ptr<const buffer::value_type>>::value,
              "buffer must be constructible with two pointers"
             );
static_assert(std::is_move_constructible<buffer>::value, "buffer must be move-constructible");
static_assert(std::is_nothrow_move_constructible<buffer>::value, "buffer must be nothrow move-constructible");
static_assert(std::is_nothrow_move_assignable<buffer>::value, "buffer must be nothrow move-assignable");
static_assert(std::is_same<decltype(std::declval<const buffer&>().size()), buffer::size_type>::value,
              "buffer::size() must return buffer::size_type"
             );
static_assert(std::is_constructible<ptr<const buffer::value_type>,
                                    decltype(std::declval<const buffer&>().data())
                                   >::value,
              "buffer::data() must return ptr<const buffer::value_type>"
             );

/// \}

}
