/** \file
 *  Controls the throwing of exceptions and import of \c exception_ptr types and \c current_exception implementation.
 *  These are probably \c std::exception_ptr and \c std::current_exception(), but if boost future is used it will be
 *  boost's implementation.
**/
#pragma once

#include <zk/future.hpp>

#if ZKPP_FUTURE_USE_BOOST
#include <boost/exception_ptr.hpp>
#else
#include <exception>
#endif

namespace zk
{

/// \addtogroup Client
/// \{


#if ZKPP_FUTURE_USE_BOOST

using exception_ptr = boost::exception_ptr;
exception_ptr current_exception() noexcept;

template<typename T>
[[noreturn]] inline void throw_exception(T const & e)
{
	throw boost::enable_current_exception(e);
}


#else

using exception_ptr = std::exception_ptr;
exception_ptr current_exception() noexcept;

template<typename T>
[[noreturn]] inline void throw_exception(T const & e)
{
	throw e;
}

#endif

/// \}

}

