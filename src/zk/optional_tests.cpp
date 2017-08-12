#include <zk/tests/test.hpp>

#include "optional.hpp"

namespace zk
{

GTEST_TEST(optional_test, integer)
{
    optional<int> x = nullopt;
    CHECK_FALSE(x);
    x = 1;
    CHECK_TRUE(x);
    CHECK_EQ(1, x.value());
}

}
