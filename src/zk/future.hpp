/** \file
 *  Controls the import of \c future and \c promise types. These are probably \c std::future and \c std::promise, but
 *  can be your custom types (as long as they behave in a manner similar enough to \c std::future and \c std::promise).
**/
#pragma once

#include <zk/config.hpp>

/** \addtogroup Client
 *  \{
**/

/** \def ZKPP_FUTURE_USE_STD_EXPERIMENTAL
 *  Set this to 1 to use \c std::experimental::future and \c std::experimental::promise as the backing types for
 *  \c zk::future and \c zk::promise.
**/
#ifndef ZKPP_FUTURE_USE_STD_EXPERIMENTAL
#   define ZKPP_FUTURE_USE_STD_EXPERIMENTAL 0
#endif

/** \def ZKPP_FUTURE_USE_BOOST
 *  Set this to 1 to use \c boost::future and \c boost::promise as the backing types for
 *  \c zk::future and \c zk::promise.
**/
#ifndef ZKPP_FUTURE_USE_BOOST
#   define ZKPP_FUTURE_USE_BOOST 0
#endif

/** \def ZKPP_FUTURE_USE_CUSTOM
 *  Set this to 1 to use custom definitions of \c zk::future and \c zk::promise. If this is set, you must also set
 *  \c ZKPP_FUTURE_TEMPLATE, \c ZKPP_PROMISE_TEMPLATE, and \c ZKPP_FUTURE_INCLUDE.
 *
 *  \def ZKPP_FUTURE_TEMPLATE
 *  The template to use for \c zk::future. By default, this is \c std::future.
 *
 *  \def ZKPP_PROMISE_TEMPLATE
 *  The template to use for \c zk::promise. This should be highly related to \c ZKPP_FUTURE_TEMPLATE
 *
 *  \def ZKPP_ASYNC_TEMPLATE
 *  The template to use for \c zk::async. This should be highly related to \c ZKPP_FUTURE_TEMPLATE and usually mapped
 *  to std::async or boost::async.
 *
 *  \def ZKPP_LAUNCH_ENUM
 *  The enum to use for \c zk::async launch policy. This should be highly related to \c ZKPP_FUTURE_TEMPLATE and usually mapped
 *  to std::launch::async or boost::launch::async.
 *
 *  \def ZKPP_FUTURE_INCLUDE
 *  The file to include to get the implementation for \c future and \c promise. If you define \c ZKPP_FUTURE_TEMPLATE
 *  and \c ZKPP_PROMISE_TEMPLATE, you must also define this.
**/
#ifndef ZKPP_FUTURE_USE_CUSTOM
#   define ZKPP_FUTURE_USE_CUSTOM 0
#endif

/** \def ZKPP_FUTURE_USE_STD
 *  Set this to 1 to use \c std::future and \c std::promise as the backing types for \c zk::future and \c zk::promise.
 *  This is the default behavior.
**/
#ifndef ZKPP_FUTURE_USE_STD
#   if ZKPP_FUTURE_USE_BOOST || ZKPP_FUTURE_USE_STD_EXPERIMENTAL || ZKPP_FUTURE_USE_CUSTOM
#       define ZKPP_FUTURE_USE_STD 0
#   else
#       define ZKPP_FUTURE_USE_STD 1
#   endif
#endif

#if ZKPP_FUTURE_USE_STD
#   define ZKPP_FUTURE_INCLUDE   <future>
#   define ZKPP_FUTURE_TEMPLATE  std::future
#   define ZKPP_PROMISE_TEMPLATE std::promise
#   define ZKPP_ASYNC_TEMPLATE   std::async
#   define ZKPP_LAUNCH_ENUM      std::launch
#elif ZKPP_FUTURE_USE_STD_EXPERIMENTAL
#   define ZKPP_FUTURE_INCLUDE   <experimental/future>
#   define ZKPP_FUTURE_TEMPLATE  std::experimental::future
#   define ZKPP_PROMISE_TEMPLATE std::experimental::promise
#   define ZKPP_ASYNC_TEMPLATE   std::async
#   define ZKPP_LAUNCH_ENUM      std::launch
#elif ZKPP_FUTURE_USE_BOOST
#   define BOOST_THREAD_PROVIDES_FUTURE
#   define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#   define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#   define ZKPP_FUTURE_INCLUDE   <boost/thread/future.hpp>
#   define ZKPP_FUTURE_TEMPLATE  boost::future
#   define ZKPP_PROMISE_TEMPLATE boost::promise
#   define ZKPP_ASYNC_TEMPLATE   boost::async
#   define ZKPP_LAUNCH_ENUM      boost::launch
#elif ZKPP_FUTURE_USE_CUSTOM
#   if !defined ZKPP_FUTURE_TEMPLATE || !defined ZKPP_PROMISE_TEMPLATE || !defined ZKPP_FUTURE_INCLUDE
#       error "When ZKPP_FUTURE_USE_CUSTOM is set, you must also define ZKPP_FUTURE_TEMPLATE, ZKPP_PROMISE_TEMPLATE,"
#       error "and ZKPP_FUTURE_INCLUDE."
#   endif
#else
#   error "Unknown type to use for zk::future and zk::promise"
#endif

#include ZKPP_FUTURE_INCLUDE

/** \} **/

namespace zk
{

/** \addtogroup Client
 *  \{
**/

template <typename T>
using future = ZKPP_FUTURE_TEMPLATE<T>;

template <typename T>
using promise = ZKPP_PROMISE_TEMPLATE<T>;

using ZKPP_LAUNCH_ENUM;
using ZKPP_ASYNC_TEMPLATE;

/** \} **/

}
