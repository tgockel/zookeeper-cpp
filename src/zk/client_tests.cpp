#include <zk/server/server_tests.hpp>

#include "client.hpp"

namespace zk
{

class client_tests :
        public server::single_server_fixture
{ };

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
