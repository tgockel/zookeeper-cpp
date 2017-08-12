#include "error.hpp"

#include <sstream>
#include <ostream>

#include <zookeeper/zookeeper.h>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// error_code                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const std::array<error_code, ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_SIZE_IMPL)>& all_error_codes()
{
    #define ZKPP_ERROR_CODE_VALUE_IMPL(cxx_name, number, c_name) error_code::cxx_name,
    static std::array<error_code, ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_SIZE_IMPL)> instance =
    {
        ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_VALUE_IMPL)
    };
    return instance;
}

std::ostream& operator<<(std::ostream& os, const error_code& code)
{
    switch (code)
    {
        #define ZKPP_ERROR_CODE_OSTREAM_CASE_IMPL(cxx_name, number, c_name) \
            case error_code::cxx_name: return os << #cxx_name;
        ZKPP_ERROR_CODE_LIST(ZKPP_ERROR_CODE_OSTREAM_CASE_IMPL)
        default:
            return os << "error_code(" << static_cast<int>(code) << ')';
    }
}

std::string to_string(const error_code& code)
{
    std::ostringstream os;
    os << code;
    return os.str();
}

void throw_error(error_code code)
{
    switch (code)
    {
    case error_code::runtime_inconsistency:         throw runtime_inconsistency();
    case error_code::data_inconsistency:            throw data_inconsistency();
    case error_code::connection_loss:               throw connection_loss();
    case error_code::marshalling_error:             throw marshalling_error();
    case error_code::unimplemented:                 throw unimplemented();
    case error_code::operation_timeout:             throw operation_timeout();
    case error_code::invalid_arguments:             throw invalid_arguments();
    case error_code::invalid_handle_state:          throw invalid_handle_state();
    case error_code::unknown_session:               throw unknown_session();
    case error_code::new_configuration_no_quorum:   throw new_configuration_no_quorum();
    case error_code::reconfiguration_in_progress:   throw reconfiguration_in_progress();
    case error_code::no_node:                       throw no_node();
    case error_code::not_authenticated:             throw not_authenticated();
    case error_code::bad_version:                   throw bad_version();
    case error_code::no_children_for_ephemerals:    throw no_children_for_ephemerals();
    case error_code::node_exists:                   throw node_exists();
    case error_code::not_empty:                     throw not_empty();
    case error_code::session_expired:               throw session_expired();
    case error_code::invalid_callback:              throw invalid_callback();
    case error_code::invalid_acl:                   throw invalid_acl();
    case error_code::authentication_failed:         throw authentication_failed();
    case error_code::closing:                       throw closing();
    case error_code::no_response:                   throw no_response();
    case error_code::session_moved:                 throw session_moved();
    case error_code::server_read_only:              throw server_read_only();
    case error_code::ephemeral_on_local_session:    throw ephemeral_on_local_session();
    case error_code::no_watcher:                    throw no_watcher();
    case error_code::reconfiguration_disabled:      throw reconfiguration_disabled();
    default:                                        throw unknown_error(code);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// errors                                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

error::error(error_code code, std::string description) :
        std::runtime_error(to_string(code) + ": " + description),
        _code(code)
{ }

error::~error() noexcept = default;

system_error::system_error(error_code code, std::string description) :
        error(code, std::move(description))
{ }

system_error::~system_error() noexcept = default;

runtime_inconsistency::runtime_inconsistency() :
        system_error(error_code::runtime_inconsistency, "A runtime inconsistency was found")
{ }

runtime_inconsistency::~runtime_inconsistency() noexcept = default;

data_inconsistency::data_inconsistency() :
        system_error(error_code::data_inconsistency, "A data inconsistency was found")
{ }

data_inconsistency::~data_inconsistency() noexcept = default;

connection_loss::connection_loss() :
        system_error(error_code::connection_loss, "Connection to the server has been lost")
{ }

connection_loss::~connection_loss() noexcept = default;

marshalling_error::marshalling_error() :
        system_error(error_code::marshalling_error, "Error while marshalling or unmarshalling data")
{ }

marshalling_error::~marshalling_error() noexcept = default;

unimplemented::unimplemented() :
        system_error(error_code::unimplemented, "Operation is unimplemented")
{ }

unimplemented::~unimplemented() noexcept = default;

operation_timeout::operation_timeout() :
        system_error(error_code::operation_timeout, "Operation timeout")
{ }

operation_timeout::~operation_timeout() noexcept = default;

invalid_arguments::invalid_arguments() :
        system_error(error_code::invalid_arguments, "Invalid arguments")
{ }

invalid_arguments::~invalid_arguments() noexcept = default;

invalid_handle_state::invalid_handle_state() :
        system_error(error_code::invalid_handle_state, "Invalid state for handle")
{ }

invalid_handle_state::~invalid_handle_state() noexcept = default;

unknown_session::unknown_session() :
        system_error(error_code::unknown_session, "Unknown session")
{ }

unknown_session::~unknown_session() noexcept = default;

new_configuration_no_quorum::new_configuration_no_quorum() :
        system_error(error_code::new_configuration_no_quorum,
                     "No quorum of new configuration is connected and up-to-date with the leader of last commmitted "
                     "configuration. Try invoking reconfiguration after new servers are connected and synced."
                    )
{ }

new_configuration_no_quorum::~new_configuration_no_quorum() noexcept = default;

reconfiguration_in_progress::reconfiguration_in_progress() :
        system_error(error_code::reconfiguration_in_progress,
                     "Another reconfiguration is in progress -- concurrent reconfigs not supported"
                    )
{ }

reconfiguration_in_progress::~reconfiguration_in_progress() noexcept = default;

api_error::api_error(error_code code, std::string description) :
        error(code, std::move(description))
{ }

api_error::~api_error() noexcept = default;

no_node::no_node() :
        api_error(error_code::no_node, "Node does not exist")
{ }

no_node::~no_node() noexcept = default;

not_authenticated::not_authenticated() :
        api_error(error_code::not_authenticated, "Not authenticated")
{ }

not_authenticated::~not_authenticated() noexcept = default;

bad_version::bad_version() :
        api_error(error_code::bad_version, "Version conflict")
{ }

bad_version::~bad_version() noexcept = default;

no_children_for_ephemerals::no_children_for_ephemerals() :
        api_error(error_code::no_children_for_ephemerals, "Ephemeral nodes may not have children")
{ }

no_children_for_ephemerals::~no_children_for_ephemerals() noexcept = default;

node_exists::node_exists() :
        api_error(error_code::node_exists, "Node already exists")
{ }

node_exists::~node_exists() noexcept = default;

not_empty::not_empty() :
        api_error(error_code::not_empty, "Cannot erase a node with children")
{ }

not_empty::~not_empty() noexcept = default;

session_expired::session_expired() :
        api_error(error_code::session_expired, "The session has been expired by the server")
{ }

session_expired::~session_expired() noexcept = default;

invalid_callback::invalid_callback() :
        api_error(error_code::invalid_callback, "Invalid callback specified")
{ }

invalid_callback::~invalid_callback() noexcept = default;

invalid_acl::invalid_acl() :
        api_error(error_code::invalid_acl, "Invalid ACL specified")
{ }

invalid_acl::~invalid_acl() noexcept = default;

authentication_failed::authentication_failed() :
        api_error(error_code::authentication_failed, "Client authentication failed")
{ }

authentication_failed::~authentication_failed() noexcept = default;

closing::closing() :
        api_error(error_code::closing, "Client is closing")
{ }

closing::~closing() noexcept = default;

no_response::no_response() :
        api_error(error_code::no_response, "No server responses to process")
{ }

no_response::~no_response() noexcept = default;

session_moved::session_moved() :
        api_error(error_code::session_moved, "Session moved to another server, so operation was ignored")
{ }

session_moved::~session_moved() noexcept = default;

server_read_only::server_read_only() :
        api_error(error_code::server_read_only, "State-changing request was passed to read-only server")
{ }

server_read_only::~server_read_only() noexcept = default;

ephemeral_on_local_session::ephemeral_on_local_session() :
        api_error(error_code::ephemeral_on_local_session, "Cannot create ephemeral node on a local session")
{ }

ephemeral_on_local_session::~ephemeral_on_local_session() noexcept = default;

no_watcher::no_watcher() :
        api_error(error_code::no_watcher, "Cannot remove a non-existing watcher")
{ }

no_watcher::~no_watcher() noexcept = default;

reconfiguration_disabled::reconfiguration_disabled() :
        api_error(error_code::reconfiguration_disabled, "Cluster reconfiguration feature is disabled")
{ }

reconfiguration_disabled::~reconfiguration_disabled() noexcept = default;

unknown_error::unknown_error(error_code code) :
        error(code, "Error code not recognized")
{ }

unknown_error::~unknown_error() noexcept = default;

}
