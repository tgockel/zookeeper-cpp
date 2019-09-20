#include "exceptions.hpp"

namespace zk
{

exception_ptr current_exception() noexcept
{
#if ZKPP_FUTURE_USE_BOOST
    return boost::current_exception();
#else
    return std::current_exception();
#endif
}

}
