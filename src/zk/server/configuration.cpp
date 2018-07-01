#include "configuration.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <istream>
#include <ostream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace zk::server
{

void server_id::ensure_valid() const
{
    if (0U < value && value < 256U)
        return;

    std::ostringstream os;
    os << "Server ID value " << value << " is not in the valid range [1 .. 255]";
    throw std::out_of_range(os.str());
}

std::ostream& operator<<(std::ostream& os, const server_id& self)
{
    return os << self.value;
}

namespace
{

constexpr std::size_t not_a_line = ~0UL;

class zero_copy_streambuf final :
        public std::streambuf
{
public:
    zero_copy_streambuf(string_view input)
    {
        // We are just going to read from it, so this const_cast is okay
        ptr<char> p = const_cast<ptr<char>>(input.data());
        setg(p, p, p + input.size());
    }
};

}

const std::uint16_t configuration::default_client_port = std::uint16_t(2181);
const std::uint16_t configuration::default_peer_port   = std::uint16_t(2888);
const std::uint16_t configuration::default_leader_port = std::uint16_t(3888);

const configuration::duration_type configuration::default_tick_time = std::chrono::milliseconds(2000);

const std::size_t configuration::default_init_limit = 10U;
const std::size_t configuration::default_sync_limit =  5U;

const std::set<std::string> configuration::default_four_letter_word_whitelist = { "srvr" };
const std::set<std::string> configuration::all_four_letter_word_whitelist     = { "*" };

template <typename T>
configuration::setting<T>::setting() noexcept :
        value(nullopt),
        line(not_a_line)
{ }

template <typename T>
configuration::setting<T>::setting(T value, std::size_t line) noexcept :
        value(std::move(value)),
        line(line)
{ }

configuration::configuration() = default;

configuration::~configuration() noexcept = default;

configuration configuration::make_minimal(std::string data_directory, std::uint16_t client_port)
{
    configuration out;
    out.data_directory(std::move(data_directory))
       .client_port(client_port)
       .init_limit(default_init_limit)
       .sync_limit(default_sync_limit)
       ;
    return out;
}

static std::set<std::string> parse_whitelist(string_view source)
{
    std::set<std::string> out;

    while (!source.empty())
    {
        auto idx = source.find_first_of(',');
        if (idx == string_view::npos)
            idx = source.size();

        if (idx == 0)
        {
            source.remove_prefix(1);
            continue;
        }

        auto sub = source.substr(0, idx);
        source.remove_prefix(idx);
        while (!sub.empty() && std::isspace(sub.front()))
            sub.remove_prefix(1);

        while (!sub.empty() && std::isspace(sub.back()))
            sub.remove_suffix(1);

        out.insert(std::string(sub));
    }

    return out;
}

configuration configuration::from_lines(std::vector<std::string> lines)
{
    static const std::regex line_expr(R"(^([^=]+)=([^ #]+)[ #]*$)",
                                      std::regex_constants::ECMAScript | std::regex_constants::optimize
                                     );
    constexpr auto name_idx = 1U;
    constexpr auto data_idx = 2U;

    configuration out;

    for (std::size_t line_no = 0U; line_no < lines.size(); ++line_no)
    {
        const auto& line = lines[line_no];

        if (line.empty() || line[0] == '#')
            continue;

        std::smatch match;
        if (std::regex_match(line, match, line_expr))
        {
            auto name = match[name_idx].str();
            auto data = match[data_idx].str();

            if (name == "clientPort")
            {
                out._client_port = { std::uint16_t(std::atoi(data.c_str())), line_no };
            }
            else if (name == "dataDir")
            {
                out._data_directory = { std::move(data), line_no };
            }
            else if (name == "tickTime")
            {
                out._tick_time = { std::chrono::milliseconds(std::atol(data.c_str())), line_no };
            }
            else if (name == "initLimit")
            {
                out._init_limit = { std::size_t(std::atol(data.c_str())), line_no };
            }
            else if (name == "syncLimit")
            {
                out._sync_limit = { std::size_t(std::atol(data.c_str())), line_no };
            }
            else if (name == "leaderServes")
            {
                out._leader_serves = { (data == "yes"), line_no };
            }
            else if (name == "4lw.commands.whitelist")
            {
                out._four_letter_word_whitelist = { parse_whitelist(data), line_no };
            }
            else if (name.find("server.") == 0U)
            {
                auto id = std::size_t(std::atol(name.c_str() + 7));
                out._server_paths.insert({ server_id(id), { std::move(data), line_no } });
            }
            else
            {
                out._unknown_settings.insert({ std::move(name), { std::move(data), line_no } });
            }
        }
    }

    out._lines = std::move(lines);
    return out;
}

configuration configuration::from_stream(std::istream& stream)
{
    std::vector<std::string> lines;
    std::string              line;

    while(std::getline(stream, line))
    {
        lines.emplace_back(std::move(line));
    }

    if (stream.eof())
        return from_lines(std::move(lines));
    else
        throw std::runtime_error("Loading configuration did not reach EOF");
}

configuration configuration::from_file(std::string filename)
{
    std::ifstream inf(filename.c_str());
    auto out = from_stream(inf);
    out._source_file = std::move(filename);
    return out;
}

configuration configuration::from_string(string_view value)
{
    zero_copy_streambuf buff(value);
    std::istream        stream(&buff);
    return from_stream(stream);
}

bool configuration::is_minimal() const
{
    return _data_directory.value
        && _client_port.value
        && _init_limit.value     && init_limit() == default_init_limit
        && _sync_limit.value     && sync_limit() == default_sync_limit
        && _lines.size() == 4U;
}

template <typename T, typename FEncode>
void configuration::set(setting<T>& target, optional<T> value, string_view key, const FEncode& encode)
{
    std::string target_line;
    if (value)
    {
        std::ostringstream os;
        os << key << '=' << encode(*value);
        target_line = os.str();
    }

    if (target.line == not_a_line && value)
    {
        target.value = std::move(value);
        target.line  = _lines.size();
        _lines.emplace_back(std::move(target_line));
    }
    else if (target.line == not_a_line && !value)
    {
        // do nothing -- no line means no value
    }
    else
    {
        target.value        = std::move(value);
        _lines[target.line] = std::move(target_line);
    }
}

template <typename T>
void configuration::set(setting<T>& target, optional<T> value, string_view key)
{
    return set(target, std::move(value), key, [] (const T& x) -> const T& { return x; });
}

std::uint16_t configuration::client_port() const
{
    return _client_port.value.value_or(default_client_port);
}

configuration& configuration::client_port(optional<std::uint16_t> port)
{
    set(_client_port, port, "clientPort");
    return *this;
}

const optional<std::string>& configuration::data_directory() const
{
    return _data_directory.value;
}

configuration& configuration::data_directory(optional<std::string> path)
{
    set(_data_directory, std::move(path), "dataDir");
    return *this;
}

configuration::duration_type configuration::tick_time() const
{
    return _tick_time.value.value_or(default_tick_time);
}

configuration& configuration::tick_time(optional<duration_type> tick_time)
{
    set(_tick_time, tick_time, "tickTime", [] (duration_type x) { return x.count(); });
    return *this;
}

std::size_t configuration::init_limit() const
{
    return _init_limit.value.value_or(default_init_limit);
}

configuration& configuration::init_limit(optional<std::size_t> limit)
{
    set(_init_limit, limit, "initLimit");
    return *this;
}

std::size_t configuration::sync_limit() const
{
    return _sync_limit.value.value_or(default_sync_limit);
}

configuration& configuration::sync_limit(optional<std::size_t> limit)
{
    set(_sync_limit, limit, "syncLimit");
    return *this;
}

bool configuration::leader_serves() const
{
    return _leader_serves.value.value_or(true);
}

configuration& configuration::leader_serves(optional<bool> serve)
{
    set(_leader_serves, serve, "leaderServes", [] (bool x) { return x ? "yes" : "no"; });
    return *this;
}

const std::set<std::string>& configuration::four_letter_word_whitelist() const
{
    if (_four_letter_word_whitelist.value)
        return *_four_letter_word_whitelist.value;
    else
        return default_four_letter_word_whitelist;
}

configuration& configuration::four_letter_word_whitelist(optional<std::set<std::string>> words)
{
    if (words && words->size() > 1U && words->count("*"))
        throw std::invalid_argument("");

    set(_four_letter_word_whitelist,
        std::move(words),
        "4lw.commands.whitelist",
        [] (const std::set<std::string>& words)
        {
            bool               first = true;
            std::ostringstream os;

            for (const auto& word : words)
            {
                if (!std::exchange(first, false))
                    os << ',';
                os << word;
            }

            return os.str();
        }
       );
    return *this;
}

std::map<server_id, std::string> configuration::servers() const
{
    std::map<server_id, std::string> out;
    for (const auto& entry : _server_paths)
        out.insert({ entry.first, *entry.second.value });

    return out;
}

configuration& configuration::add_server(server_id     id,
                                         std::string   hostname,
                                         std::uint16_t peer_port,
                                         std::uint16_t leader_port
                                        )
{
    id.ensure_valid();

    if (_server_paths.count(id))
        throw std::runtime_error(std::string("Already a server with ID ") + std::to_string(id.value));

    hostname += ":";
    hostname += std::to_string(peer_port);
    hostname += ":";
    hostname += std::to_string(leader_port);

    auto iter = _server_paths.emplace(id, setting<std::string>()).first;
    set(iter->second, some(std::move(hostname)), std::string("server.") + std::to_string(iter->first.value));
    return *this;
}

std::map<std::string, std::string> configuration::unknown_settings() const
{
    std::map<std::string, std::string> out;
    for (const auto& entry : _unknown_settings)
        out.insert({ entry.first, *entry.second.value });

    return out;
}

configuration& configuration::add_setting(std::string key, std::string value)
{
    // This process is really inefficient, but people should not be using this very often. This is done this way because
    // it is possible to specify a key that has a known setting (such as "dataDir"), which needs to be picked up
    // correctly. `from_lines` has this logic, so just use it. Taking that matching logic out would be the best approach
    // to take, but since this shouldn't be used, I haven't bothered.
    auto source_file = _source_file;
    auto lines       = _lines;
    lines.emplace_back(key + "=" + value);

    *this        = configuration::from_lines(std::move(lines));
    _source_file = std::move(source_file);
    return *this;
}

void configuration::save(std::ostream& os) const
{
    for (const auto& line : _lines)
        os << line << std::endl;

    os.flush();
}

void configuration::save_file(std::string filename)
{
    std::ofstream ofs(filename.c_str());
    save(ofs);
    if (ofs)
        _source_file = std::move(filename);
    else
        throw std::runtime_error("Error saving file");
}

bool operator==(const configuration& lhs, const configuration& rhs)
{
    if (&lhs == &rhs)
        return true;

    auto same_items = [] (const auto& a, const auto& b)
                      {
                          return a.first        == b.first
                              && a.second.value == b.second.value;
                      };

    return lhs.client_port()                == rhs.client_port()
        && lhs.data_directory()             == rhs.data_directory()
        && lhs.tick_time()                  == rhs.tick_time()
        && lhs.init_limit()                 == rhs.init_limit()
        && lhs.sync_limit()                 == rhs.sync_limit()
        && lhs.leader_serves()              == rhs.leader_serves()
        && lhs.four_letter_word_whitelist() == rhs.four_letter_word_whitelist()
        && lhs._server_paths.size()         == rhs._server_paths.size()
        && lhs._server_paths.end()          == std::mismatch(lhs._server_paths.begin(), lhs._server_paths.end(),
                                                             rhs._server_paths.begin(), rhs._server_paths.end(),
                                                             same_items
                                                            ).first
        && lhs._unknown_settings.size()     == rhs._unknown_settings.size()
        && lhs._unknown_settings.end()      == std::mismatch(lhs._unknown_settings.begin(), lhs._unknown_settings.end(),
                                                             rhs._unknown_settings.begin(), rhs._unknown_settings.end(),
                                                             same_items
                                                            ).first
        ;
}

bool operator!=(const configuration& lhs, const configuration& rhs)
{
    return !(lhs == rhs);
}

}
