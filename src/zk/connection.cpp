#include "connection.hpp"
#include "connection_zk.hpp"
#include "error.hpp"

#include <zookeeper/zookeeper.h>

namespace zk
{

connection::~connection() noexcept
{ }

std::shared_ptr<connection> connection::create(string_view conn_string)
{
    return std::make_shared<connection_zk>(conn_string);
}

future<zk::state> connection::watch_state()
{
    std::unique_lock<std::mutex> ax(_state_change_promises_protect);
    _state_change_promises.emplace_back();
    return _state_change_promises.rbegin()->get_future();
}

void connection::on_session_event(zk::state new_state)
{
    std::unique_lock<std::mutex> ax(_state_change_promises_protect);
    auto l_state_change_promises = std::move(_state_change_promises);
    ax.unlock();

    auto ex = new_state == zk::state::expired_session       ? get_exception_ptr_of(error_code::session_expired)
            : new_state == zk::state::authentication_failed ? get_exception_ptr_of(error_code::authentication_failed)
            :                                             std::exception_ptr();

    for (auto& p : l_state_change_promises)
    {
        if (ex)
            p.set_exception(ex);
        else
            p.set_value(new_state);
    }
}

}
