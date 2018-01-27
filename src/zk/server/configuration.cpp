#include "configuration.hpp"

#include <cstdlib>
#include <fstream>
#include <istream>
#include <regex>
#include <stdexcept>

namespace zk::server
{

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

std::uint16_t configuration::default_client_port = std::uint16_t(2181);

configuration::duration_type configuration::default_tick_time = std::chrono::milliseconds(2000);

configuration::configuration() = default;

configuration::~configuration() noexcept = default;

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
            else if (name.find("server.") == 0U)
            {
                out._server_paths.insert({ name.substr(7U), { std::move(data), line_no } });
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

template <typename T>
configuration::setting<T> configuration::add_no_line(optional<T> value)
{
    return map([] (T x) -> setting_data<T> { return { std::move(x), not_a_line }; }, std::move(value));
}

template <typename T>
T configuration::value_or(const setting<T>& value, const T& alternative)
{
    return map([] (const setting_data<T>& x) { return x.value; }, value)
           .value_or(alternative);
}

template <typename T>
optional<T> configuration::value(const setting<T>& value)
{
    return map([] (const setting_data<T>& x) { return x.value; }, value);
}

std::uint16_t configuration::client_port() const
{
    return value_or(_client_port, default_client_port);
}

configuration& configuration::client_port(optional<std::uint16_t> port)
{
    _client_port = add_no_line(port);
    return *this;
}

optional<string_view> configuration::data_directory() const
{
    return map([] (const auto& x) -> string_view { return x.value; }, _data_directory);
}

configuration& configuration::data_directory(optional<std::string> path)
{
    _data_directory = add_no_line(std::move(path));
    return *this;
}

configuration::duration_type configuration::tick_time() const
{
    return value_or(_tick_time, default_tick_time);
}

configuration& configuration::tick_time(optional<duration_type> tick_time)
{
    _tick_time = add_no_line(tick_time);
    return *this;
}

optional<std::size_t> configuration::init_limit() const
{
    return value(_init_limit);
}

configuration& configuration::init_limit(optional<std::size_t> limit)
{
    _init_limit = add_no_line(limit);
    return *this;
}

optional<std::size_t> configuration::sync_limit() const
{
    return value(_sync_limit);
}

configuration& configuration::sync_limit(optional<std::size_t> limit)
{
    _sync_limit = add_no_line(limit);
    return *this;
}

optional<bool> configuration::leader_serves() const
{
    return value(_leader_serves);
}

configuration& configuration::leader_serves(optional<bool> serve)
{
    _leader_serves = add_no_line(serve);
    return *this;
}

std::unordered_map<std::string, std::string> configuration::servers() const
{
    std::unordered_map<std::string, std::string> out;
    for (const auto& entry : _server_paths)
        out.insert({ entry.first, entry.second.value });

    return out;
}

std::unordered_map<std::string, std::string> configuration::unknown_settings() const
{
    std::unordered_map<std::string, std::string> out;
    for (const auto& entry : _unknown_settings)
        out.insert({ entry.first, entry.second.value });

    return out;
}

}
