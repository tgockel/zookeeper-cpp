#pragma once

#include <zk/config.hpp>

#include <memory>

#include "string_view.hpp"

namespace zk
{

class connection
{
public:
    static std::shared_ptr<connection> create(string_view conn_string);

    virtual ~connection() noexcept;

    virtual void close() = 0;
};

}
