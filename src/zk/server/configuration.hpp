#pragma once

#include <zk/config.hpp>
#include <zk/optional.hpp>
#include <zk/string_view.hpp>

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

namespace zk::server
{

class configuration final
{
public:
    using duration_type = std::chrono::milliseconds;

public:
    static std::uint16_t default_client_port;

    static duration_type default_tick_time;

public:
    static configuration from_file(std::string filename);

    static configuration from_stream(std::istream& stream);

    static configuration from_lines(std::vector<std::string> lines);

    static configuration from_string(string_view value);

    ~configuration() noexcept;

    std::uint16_t  client_port() const;
    configuration& client_port(optional<std::uint16_t> port);

    optional<string_view> data_directory() const;
    configuration&        data_directory(optional<std::string> path);

    duration_type  tick_time() const;
    configuration& tick_time(optional<duration_type> time);

    optional<std::size_t> init_limit() const;
    configuration&        init_limit(optional<std::size_t> limit);

    optional<std::size_t> sync_limit() const;
    configuration&        sync_limit(optional<std::size_t> limit);

    optional<bool> leader_serves() const;
    configuration& leader_serves(optional<bool> serve);

    std::unordered_map<std::string, std::string> servers() const;

    std::unordered_map<std::string, std::string> unknown_settings() const;

private:
    using line_list = std::vector<std::string>;

    template <typename T>
    struct setting_data
    {
        T           value;
        std::size_t line;
    };

    template <typename T>
    using setting = optional<setting_data<T>>;

    template <typename T>
    static setting<T> add_no_line(optional<T> value);

    template <typename T>
    static T value_or(const setting<T>& value, const T& alternative);

    template <typename T>
    static optional<T> value(const setting<T>& value);

private:
    explicit configuration();

private:
    std::string                                                _source_file;
    line_list                                                  _lines;
    setting<std::uint16_t>                                     _client_port;
    setting<std::string>                                       _data_directory;
    setting<duration_type>                                     _tick_time;
    setting<std::size_t>                                       _init_limit;
    setting<std::size_t>                                       _sync_limit;
    setting<bool>                                              _leader_serves;
    std::unordered_map<std::string, setting_data<std::string>> _server_paths;
    std::unordered_map<std::string, setting_data<std::string>> _unknown_settings;
};
}
