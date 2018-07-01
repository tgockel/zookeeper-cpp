#pragma once

#include <zk/config.hpp>
#include <zk/optional.hpp>
#include <zk/string_view.hpp>
#include <zk/types.hpp>

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <map>
#include <set>
#include <vector>

namespace zk::server
{

/// \addtogroup Server
/// \{

/// Represents the ID of a server in the ensemble.
///
/// \note
/// The backing type for this ID is a \c std::size_t, when a \c std::uint8_t would cover all valid values of the ID.
/// However, printing `unsigned char` types is somewhat odd, as the character value is usually printed. Beyond that,
/// using a \c std::uint8_t requires explicit casting when converting from a \c std::size_t when \c -Wconversion is
/// enabled.
struct server_id :
        strong_id<server_id, std::size_t>
{
    /// Create an instance from the given \a value.
    ///
    /// \throws std::out_of_range if \a value is not in 1-255.
    server_id(std::size_t value) :
            strong_id<server_id, std::size_t>(value)
    {
        ensure_valid();
    }

    /// Check that this ID is a valid one.
    ///
    /// \throws std::out_of_range if \a value is not in 1-255.
    void ensure_valid() const;

    /// Debug print for this instance.
    friend std::ostream& operator<<(std::ostream&, const server_id&);
};

/// Represents a configuration which should be run by \ref server instance. This can also be used to modify an existing
/// ZooKeeper server configuration file in a safer manner than the unfortunate operating practice of \c sed, \c awk, and
/// \c perl.
///
/// This can be used to quickly create a quorum peer, ready to connect to 3 servers.
/// \code
/// auto config = server::configuration::make_minimal("zk-data", 2181)
///               .add_server(1, "192.168.1.101")
///               .add_server(2, "192.168.1.102")
///               .add_server(3, "192.168.1.103");
/// config.save_file("settings.cfg");
/// {
///     std::ofstream of("zk-data/myid");
///     // Assuming this is server 1
///     of << 1 << std::endl;
/// }
/// server::server svr(config);
/// \endcode
///
/// \see server For where this is used.
/// \see server_group An example of quickly creating multiple ZooKeeper servers on a single machine (for testing).
class configuration final
{
public:
    using duration_type = std::chrono::milliseconds;

public:
    /// The default value for \ref client_port.
    static const std::uint16_t default_client_port;

    /// The default value for \ref peer_port.
    static const std::uint16_t default_peer_port;

    /// The default value for \ref leader_port.
    static const std::uint16_t default_leader_port;

    /// The default value for \ref tick_time.
    static const duration_type default_tick_time;

    /// The default value for \ref init_limit.
    static const std::size_t default_init_limit;

    /// The default value for \ref sync_limit.
    static const std::size_t default_sync_limit;

    /// The default value for \ref four_letter_word_whitelist.
    static const std::set<std::string> default_four_letter_word_whitelist;

    /// A value for \ref four_letter_word_whitelist that enables all commands. Note that this is not a list of all
    /// allowed words, but simply the string \c "*".
    static const std::set<std::string> all_four_letter_word_whitelist;

    /// All known values allowed in \ref four_letter_word_whitelist. This set comes from what ZooKeeper server 3.5.3
    /// supported, so it is possible the version of ZooKeeper you are running supports a different set.
    static const std::set<std::string> known_four_letter_word_whitelist;

public:
    /// Creates a minimal configuration, setting the four needed values. The resulting \ref configuration can be run
    /// through a file with \c save or it can run directly from the command line.
    static configuration make_minimal(std::string data_directory, std::uint16_t client_port = default_client_port);

    /// Load the configuration from a file.
    static configuration from_file(std::string filename);

    /// Load configuration from the provided \a stream.
    static configuration from_stream(std::istream& stream);

    /// Load configuration from the provided \a lines.
    static configuration from_lines(std::vector<std::string> lines);

    /// Load configuration directly from the in-memory \a value.
    static configuration from_string(string_view value);

    ~configuration() noexcept;

    /// Get the source file. This will only have a value if this was created by \ref from_file.
    const optional<std::string>& source_file() const { return _source_file; }

    /// Check if this is a "minimal" configuration -- meaning it only has a \c data_directory and \c client_port set.
    /// Configurations which are minimal can be started directly from the command line.
    bool is_minimal() const;

    /// \{
    /// The port a client should use to connect to this server.
    std::uint16_t  client_port() const;
    configuration& client_port(optional<std::uint16_t> port);
    /// \}

    /// \{
    /// The directory for "myid" file and "version-2" directory (containing the log, snapshot, and epoch files).
    const optional<std::string>& data_directory() const;
    configuration&               data_directory(optional<std::string> path);
    /// \}

    /// \{
    /// The time between server "ticks." This value is used to translate \ref init_limit and \ref sync_limit into clock
    /// times.
    duration_type  tick_time() const;
    configuration& tick_time(optional<duration_type> time);
    /// \}

    /// \{
    /// The number of ticks that the initial synchronization phase can take. This limits the length of time the
    /// ZooKeeper servers in quorum have to connect to a leader.
    std::size_t    init_limit() const;
    configuration& init_limit(optional<std::size_t> limit);
    /// \}

    /// \{
    /// Limits how far out of date a server can be from a leader.
    std::size_t    sync_limit() const;
    configuration& sync_limit(optional<std::size_t> limit);
    /// \}

    /// \{
    /// Should an elected leader accepts client connections? For higher update throughput at the slight expense of read
    /// latency, the leader can be configured to not accept clients and focus on coordination. The default to this value
    /// is \c true, which means that a leader will accept client connections.
    bool           leader_serves() const;
    configuration& leader_serves(optional<bool> serve);
    /// \}

    /// \{
    /// A list of comma separated four letter words commands that user wants to use. A valid four letter words command
    /// must be put in this list or the ZooKeeper server will not enable the command. If unspecified, the whitelist only
    /// contains "srvr" command (\ref default_four_letter_word_whitelist).
    ///
    /// \note
    /// It is planned that the ZooKeeper server will deprecate this whitelist in preference of using a JSON REST API for
    /// health checks. It is unlikely to be deprecated any time in the near future and will likely remain in the product
    /// for a very long time.
    ///
    /// \param words is the list of four letter words to allow or \c nullopt to clear the setting. If specified as an
    ///  empty set, this explicitly disables all words, which is \e different than setting this value to \c nullopt.
    ///
    /// \throws std::invalid_argument if \a words contains the all value (\c "*") but it is not the only value in the
    ///  set.
    ///
    /// \ref default_four_letter_word_whitelist
    /// \ref all_four_letter_word_whitelist
    /// \ref known_four_letter_word_whitelist
    const std::set<std::string>& four_letter_word_whitelist() const;
    configuration&               four_letter_word_whitelist(optional<std::set<std::string>> words);
    /// \}

    /// Get the servers which are part of the ZooKeeper ensemble.
    std::map<server_id, std::string> servers() const;

    /// Add a new server to the configuration.
    ///
    /// \param id The cluster unique ID of this server.
    /// \param hostname The address of the server to connect to.
    /// \param peer_port The port used to move ZooKeeper data on.
    /// \param leader_port The port used for leader election.
    /// \throws std::out_of_range if the \a id is not in the valid range of IDs (see \ref server_id).
    /// \throws std::runtime_error if there is already a server with the given \a id.
    configuration& add_server(server_id     id,
                              std::string   hostname,
                              std::uint16_t peer_port   = default_peer_port,
                              std::uint16_t leader_port = default_leader_port
                             );

    /// Get settings that were in the configuration file (or manually added with \ref add_setting) but unknown to this
    /// library.
    std::map<std::string, std::string> unknown_settings() const;

    /// Add an arbitrary setting with the \a key and \a value.
    ///
    /// \note
    /// You should not use this frequently -- prefer the named settings.
    configuration& add_setting(std::string key, std::string value);

    /// Write this configuration to the provided \a stream.
    ///
    /// \see save_file
    void save(std::ostream& stream) const;

    /// Save this configuration to \a filename. On successful save, \c source_file will but updated to reflect the new
    /// file.
    void save_file(std::string filename);

    /// \{
    /// Check for equality of configuration. This does not check the specification in lines, but the values of the
    /// settings. In other words, two configurations with \c tick_time set to 1 second are equal, even if the source
    /// files are different and \c "tickTime=1000" was set on different lines.
    friend bool operator==(const configuration& lhs, const configuration& rhs);
    friend bool operator!=(const configuration& lhs, const configuration& rhs);
    /// \}

private:
    using line_list = std::vector<std::string>;

    template <typename T>
    struct setting
    {
        setting() noexcept;
        setting(T value, std::size_t line) noexcept;

        optional<T> value;
        std::size_t line;
    };

    template <typename T, typename FEncode>
    void set(setting<T>& target, optional<T> value, string_view key, const FEncode& encode);

    template <typename T>
    void set(setting<T>& target, optional<T> value, string_view key);

private:
    explicit configuration();

private:
    optional<std::string>                       _source_file;
    line_list                                   _lines;
    setting<std::uint16_t>                      _client_port;
    setting<std::string>                        _data_directory;
    setting<duration_type>                      _tick_time;
    setting<std::size_t>                        _init_limit;
    setting<std::size_t>                        _sync_limit;
    setting<bool>                               _leader_serves;
    setting<std::set<std::string>>              _four_letter_word_whitelist;
    std::map<server_id, setting<std::string>>   _server_paths;
    std::map<std::string, setting<std::string>> _unknown_settings;
};

/// \}

}

namespace std
{

template <>
struct hash<zk::server::server_id>
{
    using argument_type = zk::server::server_id;
    using result_type   = std::size_t;

    result_type operator()(const argument_type& x) const
    {
        return zk::hash(x);
    }
};

}
