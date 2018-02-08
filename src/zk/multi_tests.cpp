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

    auto node1 = c.create("/test-", buffer_from("Going to delete"), create_mode::sequential).get().name();
    auto node2 = c.create("/test-", buffer_from("First data"), create_mode::sequential).get().name();

    multi_op txn =
    {
        op::check("/"),
        op::create("/test-", buffer_from("1"), create_mode::sequential),
        op::erase(node1),
        op::set(node2, buffer_from("Second data")),
    };
    multi_result res = c.commit(txn).get();
    CHECK_EQ(4U, res.size());

    // node1 should be erased
    CHECK_FALSE(c.exists(node1).get());

    auto node2_contents = c.get(node2).get().data();
    CHECK_TRUE(node2_contents == buffer_from("Second data"));

    // Create a child of the node created in the multi
    auto name_to_make = res[1].as_create().name() + "/some-subnode";
    c.create(name_to_make, buffer_from("sub")).get();
}

GTEST_TEST_F(multi_tests, commit_fail_check)
{
    client c = get_connected_client();

    auto base = c.create("/test-", buffer_from("Base"), create_mode::sequential).get().name();
    multi_op txn =
    {
        op::create(base + "/a", buffer_from("a")),
        op::check(base + "/does-not-exist"),
        op::create(base + "/b", buffer_from("b")),
    };

    try
    {
        c.commit(txn).get();
    }
    catch (const transaction_failed& ex)
    {
        CHECK_EQ(error_code::no_entry, ex.underlying_cause());
        CHECK_EQ(1U, ex.failed_op_index());
    }
}

}
