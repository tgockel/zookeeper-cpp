#include <zk/tests/test.hpp>

#include "pipe.hpp"

#include <unistd.h>

namespace zk::server::detail
{

GTEST_TEST(pipe_tests, read_write)
{
    std::string buff(8000, 'a');

    pipe p;
    p.write(buff);

    std::string out;
    out.reserve(buff.size());

    while (out.size() < buff.size())
        out += p.read();

    CHECK_EQ(buff, out);
}

}

