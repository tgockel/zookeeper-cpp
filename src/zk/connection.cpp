#include "connection.hpp"
#include "connection_zk.hpp"
#include "error.hpp"
#include "types.hpp"
#include "exceptions.hpp"

#include <algorithm>
#include <regex>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <zookeeper/zookeeper.h>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// connection                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

connection::~connection() noexcept
{ }

std::shared_ptr<connection> connection::connect(const connection_params& params)
{
    return std::make_shared<connection_zk>(params);
}

std::shared_ptr<connection> connection::connect(string_view conn_string)
{
    return connect(connection_params::parse(conn_string));
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
            :                                                 zk::exception_ptr();

    for (auto& p : l_state_change_promises)
    {
        if (ex)
            p.set_exception(ex);
        else
            p.set_value(new_state);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// connection_params                                                                                                  //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

connection_params::connection_params() noexcept :
        _connection_schema("zk"),
        _hosts({}),
        _chroot("/"),
        _randomize_hosts(true),
        _read_only(false),
        _timeout(default_timeout)
{ }

connection_params::~connection_params() noexcept
{ }

template <typename TMatch>
static string_view sv_from_match(const TMatch& src)
{
    return string_view(src.first, std::distance(src.first, src.second));
}

template <typename FAction>
void split_each_substr(string_view src, char delim, const FAction& action)
{
    while (!src.empty())
    {
        // if next_div is src.end, the logic still works
        auto next_div = std::find(src.begin(), src.end(), delim);
        action(string_view(src.data(), std::distance(src.begin(), next_div)));
        src.remove_prefix(std::distance(src.begin(), next_div));
        if (!src.empty())
            src.remove_prefix(1U);
    }
}

static connection_params::host_list extract_host_list(string_view src)
{
    connection_params::host_list out;
    out.reserve(std::count(src.begin(), src.end(), ','));
    split_each_substr(src, ',', [&] (string_view sub) { out.emplace_back(std::string(sub)); });
    return out;
}

static bool extract_bool(string_view key, string_view val)
{
    if (val.empty())
        zk::throw_exception(std::invalid_argument(std::string("Key ") + std::string(key) + " has blank value"));

    switch (val[0])
    {
    case '1':
    case 't':
    case 'T':
        return true;
    case '0':
    case 'f':
    case 'F':
        return false;
    default:
        zk::throw_exception(std::invalid_argument(std::string("Invalid value for ") + std::string(key) + std::string(" \"")
                                    + std::string(val) + "\" -- expected a boolean"
                                    ));
    }
}

static std::chrono::milliseconds extract_millis(string_view key, string_view val)
{
    if (val.empty())
        zk::throw_exception(std::invalid_argument(std::string("Key ") + std::string(key) + " has blank value"));

    if (val[0] == 'P')
    {
        zk::throw_exception(std::invalid_argument("ISO 8601 duration is not supported (yet)."));
    }
    else
    {
        double seconds = std::stod(std::string(val));
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(seconds));
    }
}

static void extract_advanced_options(string_view src, connection_params& out)
{
    if (src.empty() || src.size() == 1U)
        return;
    else
        src.remove_prefix(1U);

    std::string invalid_keys_msg;
    auto invalid_key = [&] (string_view key)
                       {
                           if (invalid_keys_msg.empty())
                               invalid_keys_msg = "Invalid key in querystring: ";
                           else
                               invalid_keys_msg += ", ";

                           invalid_keys_msg += std::string(key);
                       };

    split_each_substr(src, '&', [&] (string_view qp_part)
    {
        auto eq_it = std::find(qp_part.begin(), qp_part.end(), '=');
        if (eq_it == qp_part.end())
            zk::throw_exception(std::invalid_argument("Invalid connection string -- query string must be specified as "
                                        "\"key1=value1&key2=value2...\""
                                       ));

        auto key = qp_part.substr(0, std::distance(qp_part.begin(), eq_it));
        auto val = qp_part.substr(std::distance(qp_part.begin(), eq_it) + 1);

        if (key == "randomize_hosts")
            out.randomize_hosts() = extract_bool(key, val);
        else if (key == "read_only")
            out.read_only() = extract_bool(key, val);
        else if (key == "timeout")
            out.timeout() = extract_millis(key, val);
        else
            invalid_key(key);
    });

    if (!invalid_keys_msg.empty())
        zk::throw_exception(std::invalid_argument(std::move(invalid_keys_msg)));
}

connection_params connection_params::parse(string_view conn_string)
{
    static const std::regex expr(R"(([^:]+)://([^/]+)((/[^\?]*)(\?.*)?)?)",
                                 std::regex_constants::ECMAScript | std::regex_constants::optimize
                                );
    constexpr auto schema_idx      = 1U;
    constexpr auto hostaddr_idx    = 2U;
    constexpr auto path_idx        = 4U;
    constexpr auto querystring_idx = 5U;

    std::cmatch match;
    if (!std::regex_match(conn_string.begin(), conn_string.end(), match, expr))
        zk::throw_exception(std::invalid_argument(std::string("Invalid connection string (") + std::string(conn_string)
                                    + " -- format is \"schema://[auth@]${host_addrs}/[path][?options]\""
                                    ));

    connection_params out;
    out.connection_schema() = match[schema_idx].str();
    out.hosts()             = extract_host_list(sv_from_match(match[hostaddr_idx]));
    out.chroot()            = match[path_idx].str();
    if (out.chroot().empty())
        out.chroot() = "/";

    extract_advanced_options(sv_from_match(match[querystring_idx]), out);

    return out;
}

bool operator==(const connection_params& lhs, const connection_params& rhs)
{
    return lhs.connection_schema() == rhs.connection_schema()
        && lhs.hosts()             == rhs.hosts()
        && lhs.chroot()            == rhs.chroot()
        && lhs.randomize_hosts()   == rhs.randomize_hosts()
        && lhs.read_only()         == rhs.read_only()
        && lhs.timeout()           == rhs.timeout();
}

bool operator!=(const connection_params& lhs, const connection_params& rhs)
{
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const connection_params& x)
{
    os << x.connection_schema() << "://";
    bool first = true;
    for (const auto& host : x.hosts())
    {
        if (first)
            first = false;
        else
            os << ',';

        os << host;
    }
    os << x.chroot();

    first = true;
    auto query_string = [&] (ptr<const char> key, const auto& val)
                        {
                            if (first)
                            {
                                first = false;
                                os << '?';
                            }
                            else
                            {
                                os << '&';
                            }

                            os << key << '=' << val;
                        };
    if (!x.randomize_hosts())
        query_string("randomize_hosts", "false");
    if (x.read_only())
        query_string("read_only", "true");
    if (x.timeout() != connection_params::default_timeout)
        query_string("timeout", std::chrono::duration<double>(x.timeout()).count());
    return os;
}

std::string to_string(const connection_params& x)
{
    std::ostringstream os;
    os << x;
    return os.str();
}

}
