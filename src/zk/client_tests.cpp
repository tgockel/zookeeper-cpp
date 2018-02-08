#include <zk/server/server_tests.hpp>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "client.hpp"
#include "error.hpp"
#include "multi.hpp"
#include "string_view.hpp"

namespace zk
{

static buffer buffer_from(string_view str)
{
    return buffer(str.data(), str.data() + str.size());
}

class client_tests :
        public server::single_server_fixture
{ };

GTEST_TEST_F(client_tests, get_root)
{
    client c = get_connected_client();
    auto res = c.get("/").get();
    std::cerr << res << std::endl;
    c.close();
}

GTEST_TEST_F(client_tests, exists)
{
    client c = get_connected_client();
    CHECK_TRUE(c.exists("/").get());
    CHECK_FALSE(c.exists("/some/bogus/path").get());
}

GTEST_TEST_F(client_tests, create)
{
    client c = get_connected_client();
    const char local_buf[10] = { 0 };
    auto f_create = c.create("/test-node", buffer(local_buf, local_buf + sizeof local_buf));
    auto name = f_create.get().name();
    CHECK_EQ("/test-node", name);
}

GTEST_TEST_F(client_tests, create_seq_and_set)
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

GTEST_TEST_F(client_tests, create_seq_and_erase)
{
    client c = get_connected_client();
    auto f_create = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential);
    auto name = f_create.get().name();
    auto orig_stat = c.get(name).get().stat();
    c.erase(name, orig_stat.data_version).get();
    CHECK_THROWS(no_entry)
    {
        c.get(name).get();
    };
}

GTEST_TEST_F(client_tests, create_seq_and_get_children)
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

GTEST_TEST_F(client_tests, acl)
{
    client c = get_connected_client();
    auto name = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential).get().name();

    // set the data of the node a few times to make sure the data_version is different value from acl_version
    for (std::size_t changes = 0; changes < 5; ++changes)
        c.set(name, buffer_from("data change")).get();

    auto orig_result = c.get_acl(name).get();
    CHECK_EQ(acls::open_unsafe(), orig_result.acl());
    std::cerr << "HEY: " << orig_result << std::endl;

    c.set_acl(name, acls::read_unsafe(), orig_result.stat().acl_version).get();
    auto new_result = c.get_acl(name).get();
    CHECK_EQ(acls::read_unsafe(), new_result.acl());
}

GTEST_TEST_F(client_tests, watch_change)
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

GTEST_TEST_F(client_tests, watch_children)
{
    client c = get_connected_client();
    auto root_name = c.create("/test-node-", buffer_from("Hello!"), create_mode::sequential).get().name();

    c.commit({
        op::create(root_name + "/a", buffer_from("a")),
        op::create(root_name + "/b", buffer_from("b")),
        op::create(root_name + "/c", buffer_from("c")),
        op::create(root_name + "/d", buffer_from("d")),
    }).get();

    auto watch_child_creation = c.watch_children(root_name).get();
    CHECK_EQ(4U, watch_child_creation.initial().children().size());

    c.create(root_name + "/e", buffer_from("e"));
    auto ev = watch_child_creation.next().get();
    CHECK_EQ(ev.type(), event_type::child);
    CHECK_EQ(ev.state(), state::connected);

    auto watch_child_erase = c.watch_children(root_name).get();
    CHECK_EQ(5U, watch_child_erase.initial().children().size());

    c.erase(root_name + "/a");
    auto ev2 = watch_child_erase.next().get();
    CHECK_EQ(ev2.type(), event_type::child);
    CHECK_EQ(ev2.state(), state::connected);
}

GTEST_TEST_F(client_tests, watch_exists)
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

GTEST_TEST_F(client_tests, load_fence)
{
    client c = get_connected_client();
    // There does not appear to be a good way to actually test this -- so just make sure we don't segfault
    c.load_fence().get();
}

GTEST_TEST_F(client_tests, watch_close)
{
    client c   = get_connected_client();
    auto watch = c.watch("/").get();

    c.close();

    // watch should be triggered with session closed
    auto ev = watch.next().get();
    CHECK_EQ(ev.type(), event_type::session);
    CHECK_EQ(ev.state(), state::closed);
}

class stopping_client_tests :
        public server::server_fixture
{ };

GTEST_TEST_F(stopping_client_tests, watch_server_stop)
{
    client c     = get_connected_client();
    auto   watch = c.watch("/").get();

    this->stop_server(true);

    auto ev = watch.next().get();
    CHECK_EQ(ev.type(), event_type::session);
}

}
