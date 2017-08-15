#pragma once

#include <zk/config.hpp>

#include <memory>

#include "forwards.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "string_view.hpp"

namespace zk
{

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

    client(const client&) noexcept = default;
    client(client&&) noexcept = default;

    client& operator=(const client&) noexcept = default;
    client& operator=(client&&) noexcept = default;

    ~client() noexcept;

    void close();

private:
    std::shared_ptr<connection> _conn;
};

}
