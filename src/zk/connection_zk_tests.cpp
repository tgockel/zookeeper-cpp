#include <zk/server/server_tests.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "client.hpp"
#include "connection_zk.hpp"
#include "error.hpp"

namespace zk
{

class connection_zk_tests :
        public server::server_fixture
{ };

static buffer buffer_from(string_view str)
{
    return buffer(str.data(), str.data() + str.size());
}

GTEST_TEST_F(connection_zk_tests, get_root)
{
    client c = get_connected_client();
    auto res = c.get("/").get();
    std::cerr << res << std::endl;
    c.close();
}

GTEST_TEST_F(connection_zk_tests, exists)
{
    client c = get_connected_client();
    CHECK_TRUE(c.exists("/").get());
    CHECK_FALSE(c.exists("/some/bogus/path").get());
}

GTEST_TEST_F(connection_zk_tests, create)
{
    client c = get_connected_client();
    const char local_buf[10] = { 0 };
    auto f_create = c.create("/test-node", buffer(local_buf, local_buf + sizeof local_buf));
    auto name = f_create.get().name();
    CHECK_EQ("/test-node", name);
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_set)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    auto name = f_create.get().name();
    auto orig_stat = c.get(name).get().stat();
    auto expected_version = orig_stat.data_version;
    ++expected_version;

    c.set(name, buffer_from("WORLD")).get();
    auto contents = c.get(name).get();
    CHECK_EQ(contents.stat().data_version, expected_version);
    CHECK_TRUE(contents.data() == buffer_from("WORLD"));
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_erase)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    auto name = f_create.get().name();
    auto orig_stat = c.get(name).get().stat();
    c.erase(name, orig_stat.data_version).get();
    CHECK_THROWS(no_node)
    {
        c.get(name).get();
    };
}

GTEST_TEST_F(connection_zk_tests, create_seq_and_get_children)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    auto name = f_create.get().name();
    auto orig_stat = c.get(name).get().stat();

    c.create(name + "/a", buffer()).get();
    c.create(name + "/b", buffer()).get();
    c.create(name + "/c", buffer()).get();

    auto result = c.get_children(name).get();
    CHECK_EQ(orig_stat.data_version,  result.parent_stat().data_version);
    CHECK_LT(orig_stat.child_version, result.parent_stat().child_version);
    std::vector<std::string> expected_children = { "a", "b", "c" };
    std::sort(result.children().begin(), result.children().end());
    CHECK_TRUE(expected_children == result.children());
}

GTEST_TEST_F(connection_zk_tests, watch_change)
{
    client c = get_connected_client();
    auto name = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential).get().name();

    auto watch = c.watch(name).get();
    CHECK_TRUE(watch.initial().data() == buffer_from("Hello!"));

    c.set(name, buffer_from("world")); // don't wait -- the watch won't trigger until the operation completes
    auto ev = watch.next().get();
    CHECK_EQ(ev.type(), event_type::changed);
    CHECK_EQ(ev.state(), state::connected);
}

GTEST_TEST_F(connection_zk_tests, watch_exists)
{
    client c = get_connected_client();
    auto root_name = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential).get().name();

    auto watch_creation = c.watch_exists(root_name + "/sub").get();
    CHECK_FALSE(watch_creation.initial());

    c.create(root_name + "/sub", buffer_from("Blah"));
    auto ev = watch_creation.next().get();
    CHECK_EQ(ev.type(), event_type::created);
    CHECK_EQ(ev.state(), state::connected);

    auto watch_erase = c.watch_exists(root_name + "/sub").get();
    CHECK_TRUE(watch_erase.initial());

    c.erase(root_name + "/sub");
    auto ev2 = watch_erase.next().get();
    CHECK_EQ(ev2.type(), event_type::erased);
    CHECK_EQ(ev2.state(), state::connected);
}

GTEST_TEST_F(connection_zk_tests, load_fence)
{
    client c = get_connected_client();
    // There does not appear to be a good way to actually test this -- so just make sure we don't segfault
    c.load_fence().get();
}

}
