#include <zk/tests/test.hpp>

#include <chrono>
#include <thread>

#include "subprocess.hpp"

namespace zk::server::detail
{

GTEST_TEST(subprocess_tests, echo)
{
    subprocess proc("echo", { "Hello, world!" });

    std::string read_str;
    while (read_str.size() < 14)
        read_str += proc.stdout().read(4096);
    CHECK_EQ("Hello, world!\n", read_str);
}

// `cat` hangs on shutdown due to never getting EOF from stdin -- however, we should still be able to kill the process
// with a signal on scope exit.
GTEST_TEST(subprocess_tests, cat_kill)
{
    std::chrono::steady_clock::time_point scope_exit_time;
    {
        subprocess proc("cat");
        scope_exit_time = std::chrono::steady_clock::now();
    }
    auto elapsed = std::chrono::steady_clock::now() - scope_exit_time;
    CHECK(elapsed < std::chrono::milliseconds(100));
}

}
