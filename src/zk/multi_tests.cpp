#include <zk/server/server_tests.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>

#include "client.hpp"
#include "error.hpp"
#include "multi.hpp"
#include "string_view.hpp"

namespace zk
{

class multi_tests :
        public server::server_fixture
{ };

static buffer buffer_from(string_view str)
{
    return buffer(str.data(), str.data() + str.size());
}

GTEST_TEST_F(multi_tests, commit_no_fail)
{
    client c = get_connected_client();

    std::string node1(c.create("/test-", buffer_from("Going to delete"), create_mode::sequential).get());
    std::string node2(c.create("/test-", buffer_from("First data"), create_mode::sequential).get());

    multi_op txn =
    {
        op::check("/"),
        op::create("/test-", buffer_from("1"), create_mode::sequential),
        op::erase(node1),
        op::set(node2, buffer_from("Second data")),
    };
    c.commit(txn).get();

    // node1 should be erased
    CHECK_FALSE(c.exists(node1).get());

    std::pair<buffer, stat> node2_contents(c.get(node2).get());
    CHECK_TRUE(node2_contents.first == buffer_from("Second data"));
}

}
