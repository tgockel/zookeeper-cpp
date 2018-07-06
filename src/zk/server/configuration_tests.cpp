#include <zk/string_view.hpp>
#include <zk/tests/test.hpp>

#include <iostream>
#include <sstream>

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
    CHECK_EQ(10U,   parsed.init_limit());
    CHECK_EQ(5U,    parsed.sync_limit());
    CHECK_EQ(2181U, parsed.client_port());
    CHECK_TRUE(parsed.leader_serves());

    auto servers = parsed.servers();
    CHECK_EQ(3U, servers.size());
    CHECK_EQ("zookeeper1:2888:3888", servers.at(server_id(1)));
    CHECK_EQ("zookeeper2:2888:3888", servers.at(server_id(2)));
    CHECK_EQ("zookeeper3:2888:3888", servers.at(server_id(3)));

    auto unrecognized = parsed.unknown_settings();
    CHECK_EQ(1U, unrecognized.size());
    CHECK_EQ("Value", unrecognized.at("randomExtra"));

    auto configured = configuration::make_minimal("/var/lib/zookeeper")
                      .tick_time(std::chrono::milliseconds(2500))
                      .init_limit(10)
                      .sync_limit(5)
                      .client_port(2181)
                      .leader_serves(true)
                      .add_server(server_id(1), "zookeeper1")
                      .add_server(server_id(2), "zookeeper2")
                      .add_server(server_id(3), "zookeeper3")
                      .add_setting("randomExtra", "Value")
                      ;
    CHECK_EQ(configured, parsed);

    std::ostringstream os;
    parsed.save(os);
    auto reloaded = configuration::from_string(os.str());
    CHECK_EQ(parsed, reloaded);
}

GTEST_TEST(configuration_tests, minimal)
{
    auto minimal = configuration::make_minimal("/some/path", 2345);
    CHECK_EQ("/some/path", minimal.data_directory().value());
    CHECK_EQ(2345,         minimal.client_port());
    CHECK_TRUE(minimal.is_minimal());
}

static string_view configuration_source_with_four_letter_words_example =
R"(# http://hadoop.apache.org/zookeeper/docs/current/zookeeperAdmin.html

tickTime=2500
initLimit=10
syncLimit=5
dataDir=/var/lib/zookeeper
4lw.commands.whitelist=stat,mntr,srvr,ruok
)";

GTEST_TEST(configuration_tests, four_letter_words)
{
    auto parsed = configuration::from_string(configuration_source_with_four_letter_words_example);

    CHECK_EQ(4U, parsed.four_letter_word_whitelist().size());
    std::set<std::string> expected = { "stat", "mntr", "srvr", "ruok" };
    CHECK_TRUE(expected == parsed.four_letter_word_whitelist());
}

}
