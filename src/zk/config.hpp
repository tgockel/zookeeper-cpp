#pragma once

/// \addtogroup Client
/// \{

/// \def ZKPP_USER_CONFIG
/// A user-defined configuration file to be included before all other ZooKeeper C++ content.
#ifdef ZKPP_USER_CONFIG
#   include ZKPP_USER_CONFIG
#endif

#define ZKPP_VERSION_MAJOR 0
#define ZKPP_VERSION_MINOR 2
#define ZKPP_VERSION_PATCH 3

/// \def ZKPP_DEBUG
/// Was ZooKeeper C++ compiled in debug mode? This value must be the same between when the SO was built and when you are
/// compiling. In general, this is not useful outside of library maintainers.
///
/// \warning
/// Keep in mind this value is \e always defined. Use `#if ZKPP_DEBUG`, \e not `#ifdef ZKPP_DEBUG`.
#ifndef ZKPP_DEBUG
#   define ZKPP_DEBUG 0
#endif

/// \}

namespace zk
{

/// \addtogroup Client
/// \{

/// A simple, unowned pointer. It operates exactly like using \c *, but removes the question of \c * associativity and
/// is easier to read when \c const qualifiers are involved.
template <typename T>
using ptr = T*;

/// \}

}
