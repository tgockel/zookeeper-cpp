#pragma once

#include <zk/config.hpp>

#include <memory>
#include <utility>

#include "buffer.hpp"
#include "forwards.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "stat.hpp"
#include "string_view.hpp"

namespace zk
{

/** A ZooKeeper client connection. This is the primary class for interacting with the ZooKeeper cluster. **/
class client final
{
public:
    client() noexcept;

    /** Create a client connected to the cluster specified by \c conn_string. See \c connection::create for
     *  documentation on connection strings.
    **/
    explicit client(string_view conn_string);

    /** Create a client connected with \a conn. **/
    explicit client(std::shared_ptr<connection> conn) noexcept;

    /** Create a client connected to the cluster specified by \c conn_string. See \c connection::create for
     *  documentation on connection strings.
     *
     *  \returns A future which will be filled when the conneciton is established. The future will be filled in error if
     *   the client will never be able to connect to the cluster (for example: a bad connection string).
    **/
    static future<client> create(string_view conn_string);

    client(const client&) noexcept = default;
    client(client&&) noexcept = default;

    client& operator=(const client&) noexcept = default;
    client& operator=(client&&) noexcept = default;

    ~client() noexcept;

    void close();

    future<std::pair<buffer, stat>> get(string_view path) const;

private:
    std::shared_ptr<connection> _conn;
};

}
