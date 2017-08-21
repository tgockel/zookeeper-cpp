#pragma once

#include <zk/config.hpp>

#include <array>
#include <iosfwd>
#include <stdexcept>
#include <string>

namespace zk
{

/** \addtogroup Errors
 *  \{
**/

#define ZKPP_ERROR_CODE_LIST(item)                                          \
    item(ok,                               0,   ZOK)                        \
    item(system_error,                    -1,   ZSYSTEMERROR)               \
    item(runtime_inconsistency,           -2,   ZRUNTIMEINCONSISTENCY)      \
    item(data_inconsistency,              -3,   ZDATAINCONSISTENCY)         \
    item(connection_loss,                 -4,   ZCONNECTIONLOSS)            \
    item(marshalling_error,               -5,   ZMARSHALLINGERROR)          \
    item(unimplemented,                   -6,   ZUNIMPLEMENTED)             \
    item(operation_timeout,               -7,   ZOPERATIONTIMEOUT)          \
    item(invalid_arguments,               -8,   ZBADARGUMENTS)              \
    item(invalid_handle_state,            -9,   ZINVALIDSTATE)              \
    item(unknown_session,                -12,   ZUNKNOWNSESSION)            \
    item(new_configuration_no_quorum,    -13,   ZNEWCONFIGNOQUORUM)         \
    item(reconfiguration_in_progress,    -14,   ZRECONFIGINPROGRESS)        \
    item(api_error,                     -100,   ZAPIERROR)                  \
    item(no_node,                       -101,   ZNONODE)                    \
    item(not_authenticated,             -102,   ZNOAUTH)                    \
    item(bad_version,                   -103,   ZBADVERSION)                \
    item(no_children_for_ephemerals,    -108,   ZNOCHILDRENFOREPHEMERALS)   \
    item(node_exists,                   -110,   ZNODEEXISTS)                \
    item(not_empty,                     -111,   ZNOTEMPTY)                  \
    item(session_expired,               -112,   ZSESSIONEXPIRED)            \
    item(invalid_callback,              -113,   ZINVALIDCALLBACK)           \
    item(invalid_acl,                   -114,   ZINVALIDACL)                \
    item(authentication_failed,         -115,   ZAUTHFAILED)                \
    item(closing,                       -116,   ZCLOSING)                   \
    item(no_response,                   -117,   ZNOTHING)                   \
    item(session_moved,                 -118,   ZSESSIONMOVED)              \
    item(server_read_only,              -119,   ZNOTREADONLY)               \
    item(ephemeral_on_local_session,    -120,   ZEPHEMERALONLOCALSESSION)   \
    item(no_watcher,                    -121,   ZNOWATCHER)                 \
    item(reconfiguration_disabled,      -123,   ZRECONFIGDISABLED)          \

enum class error_code : int
{
    #define ZKPP_ERROR_CODE_ENUM_DEF_IMPL(cxx_name, number, c_name) \
        cxx_name = number,

    ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_ENUM_DEF_IMPL)
};

std::ostream& operator<<(std::ostream&, const error_code&);

std::string to_string(const error_code&);

#define ZKPP_ERROR_CODE_SIZE_IMPL(cxx_name, number, c_name) +1

const std::array<error_code, ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_SIZE_IMPL)>& all_error_codes();

constexpr bool is_system_error(error_code code)
{
    return static_cast<int>(code) <= static_cast<int>(error_code::system_error)
        && static_cast<int>(code) >  static_cast<int>(error_code::api_error);
}

constexpr bool is_api_error(error_code code)
{
    return static_cast<int>(code) <= static_cast<int>(error_code::api_error);
}

[[noreturn]]
void throw_error(error_code code);

std::exception_ptr get_exception_ptr_of(error_code code);

class error :
        public std::runtime_error
{
public:
    explicit error(error_code code, std::string description);

    virtual ~error() noexcept;

    error_code code() const { return _code; }

private:
    error_code _code;
};

class system_error :
        public error
{
public:
    explicit system_error(error_code code, std::string description);

    virtual ~system_error() noexcept;
};

class runtime_inconsistency final :
        public system_error
{
public:
    explicit runtime_inconsistency();

    virtual ~runtime_inconsistency() noexcept;
};

class data_inconsistency final :
        public system_error
{
public:
    explicit data_inconsistency();

    virtual ~data_inconsistency() noexcept;
};

class connection_loss final :
        public system_error
{
public:
    explicit connection_loss();

    virtual ~connection_loss() noexcept;
};

class marshalling_error final :
        public system_error
{
public:
    explicit marshalling_error();

    virtual ~marshalling_error() noexcept;
};

class unimplemented final :
        public system_error
{
public:
    explicit unimplemented();

    virtual ~unimplemented() noexcept;
};

class operation_timeout final :
        public system_error
{
public:
    explicit operation_timeout();

    virtual ~operation_timeout() noexcept;
};

class invalid_arguments final :
        public system_error
{
public:
    explicit invalid_arguments();

    virtual ~invalid_arguments() noexcept;
};

class invalid_handle_state final :
        public system_error
{
public:
    explicit invalid_handle_state();

    virtual ~invalid_handle_state() noexcept;
};

class unknown_session final :
        public system_error
{
public:
    explicit unknown_session();

    virtual ~unknown_session() noexcept;
};

class new_configuration_no_quorum final :
        public system_error
{
public:
    explicit new_configuration_no_quorum();

    virtual ~new_configuration_no_quorum() noexcept;
};

class reconfiguration_in_progress final :
        public system_error
{
public:
    explicit reconfiguration_in_progress();

    virtual ~reconfiguration_in_progress() noexcept;
};

class api_error :
        public error
{
public:
    explicit api_error(error_code code, std::string description);

    virtual ~api_error() noexcept;
};

class no_node final :
        public api_error
{
public:
    explicit no_node();

    virtual ~no_node() noexcept;
};

class not_authenticated final :
        public api_error
{
public:
    explicit not_authenticated();

    virtual ~not_authenticated() noexcept;
};

class bad_version final :
        public api_error
{
public:
    explicit bad_version();

    virtual ~bad_version() noexcept;
};

class no_children_for_ephemerals final :
        public api_error
{
public:
    explicit no_children_for_ephemerals();

    virtual ~no_children_for_ephemerals() noexcept;
};

class node_exists final :
        public api_error
{
public:
    explicit node_exists();

    virtual ~node_exists() noexcept;
};

class not_empty final :
        public api_error
{
public:
    explicit not_empty();

    virtual ~not_empty() noexcept;
};

class session_expired final :
        public api_error
{
public:
    explicit session_expired();

    virtual ~session_expired() noexcept;
};

class invalid_callback final :
        public api_error
{
public:
    explicit invalid_callback();

    virtual ~invalid_callback() noexcept;
};

class invalid_acl final :
        public api_error
{
public:
    explicit invalid_acl();

    virtual ~invalid_acl() noexcept;
};

class authentication_failed final :
        public api_error
{
public:
    explicit authentication_failed();

    virtual ~authentication_failed() noexcept;
};

class closing final :
        public api_error
{
public:
    explicit closing();

    virtual ~closing() noexcept;
};

class no_response final :
        public api_error
{
public:
    explicit no_response();

    virtual ~no_response() noexcept;
};

class session_moved final :
        public api_error
{
public:
    explicit session_moved();

    virtual ~session_moved() noexcept;
};

class server_read_only final :
        public api_error
{
public:
    explicit server_read_only();

    virtual ~server_read_only() noexcept;
};

class ephemeral_on_local_session final :
        public api_error
{
public:
    explicit ephemeral_on_local_session();

    virtual ~ephemeral_on_local_session() noexcept;
};

class no_watcher final :
        public api_error
{
public:
    explicit no_watcher();

    virtual ~no_watcher() noexcept;
};

class reconfiguration_disabled final :
        public api_error
{
public:
    explicit reconfiguration_disabled();

    virtual ~reconfiguration_disabled() noexcept;
};

class unknown_error final :
        public error
{
public:
    explicit unknown_error(error_code code);

    virtual ~unknown_error();
};

//! \}

}
