#pragma once

#include <zk/config.hpp>
#include <zk/forwards.hpp>
#include <zk/tests/test.hpp>

#include <memory>

namespace zk::server
{

class server;

class server_fixture :
        public test::test_fixture
{
public:
    virtual void SetUp() override;

    virtual void TearDown() override;

protected:
    const std::string& get_connection_string() const;

    client get_connected_client() const;

    void stop_server(bool wait_for_stop = true);

private:
    std::shared_ptr<server> _server;
    std::string             _conn_string;
};

/// Similar to \ref server_fixture, but do not start up and tear down the server with each test. Instead, setup is run
/// once at the start of a suite and torn down at the end of it.
class single_server_fixture :
        public test::test_fixture
{
public:
    static void SetUpTestCase();

    static void TearDownTestCase();

protected:
    static const std::string& get_connection_string();

    static client get_connected_client();
};

}
