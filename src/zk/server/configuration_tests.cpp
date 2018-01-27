#include <zk/string_view.hpp>
#include <zk/tests/test.hpp>

#include "configuration.hpp"

namespace zk::server
{

static string_view configuration_source_file_example =
R"(# http://hadoop.apache.org/zookeeper/docs/current/zookeeperAdmin.html

tickTime=2500
initLimit=10
syncLimit=5
dataDir=/var/lib/zookeeper

# the port at which the clients will connect
clientPort=2181

# specify all zookeeper servers
server.1=zookeeper1:2888:3888
server.2=zookeeper2:2888:3888
server.3=zookeeper3:2888:3888

#preAllocSize=65536
#snapCount=1000

leaderServes=yes

# A server key that we do not understand
randomExtra=Value
)";

GTEST_TEST(configuration_tests, from_example)
{
    auto parsed = configuration::from_string(configuration_source_file_example);
    CHECK_EQ(2500U, parsed.tick_time().count());
    CHECK_EQ(10U,   parsed.init_limit().value());
    CHECK_EQ(5U,    parsed.sync_limit().value());
    CHECK_EQ(2181U, parsed.client_port());
    CHECK_TRUE(parsed.leader_serves().value());

    auto servers = parsed.servers();
    CHECK_EQ(3U, servers.size());
    CHECK_EQ("zookeeper1:2888:3888", servers.at("1"));
    CHECK_EQ("zookeeper2:2888:3888", servers.at("2"));
    CHECK_EQ("zookeeper3:2888:3888", servers.at("3"));

    auto unrecognized = parsed.unknown_settings();
    CHECK_EQ(1U, unrecognized.size());
    CHECK_EQ("Value", unrecognized.at("randomExtra"));
}
}
