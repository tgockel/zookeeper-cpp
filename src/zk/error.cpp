#include "error.hpp"
#include "exceptions.hpp"

#include <sstream>
#include <ostream>

#include <zookeeper/zookeeper.h>

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// error_code                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::ostream& operator<<(std::ostream& os, const error_code& code)
{
    switch (code)
    {
    case error_code::ok: return os << "ok";
    default:             return os << "error_code(" << static_cast<int>(code) << ')';
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
    case error_code::connection_loss:               zk::throw_exception( connection_loss() );
    case error_code::marshalling_error:             zk::throw_exception( marshalling_error() );
    case error_code::not_implemented:               zk::throw_exception( not_implemented("unspecified") );
    case error_code::invalid_arguments:             zk::throw_exception( invalid_arguments() );
    case error_code::new_configuration_no_quorum:   zk::throw_exception( new_configuration_no_quorum() );
    case error_code::reconfiguration_in_progress:   zk::throw_exception( reconfiguration_in_progress() );
    case error_code::no_entry:                      zk::throw_exception( no_entry() );
    case error_code::not_authorized:                zk::throw_exception( not_authorized() );
    case error_code::version_mismatch:              zk::throw_exception( version_mismatch() );
    case error_code::no_children_for_ephemerals:    zk::throw_exception( no_children_for_ephemerals() );
    case error_code::entry_exists:                  zk::throw_exception( entry_exists() );
    case error_code::not_empty:                     zk::throw_exception( not_empty() );
    case error_code::session_expired:               zk::throw_exception( session_expired() );
    case error_code::authentication_failed:         zk::throw_exception( authentication_failed() );
    case error_code::closed:                        zk::throw_exception( closed() );
    case error_code::read_only_connection:          zk::throw_exception( read_only_connection() );
    case error_code::ephemeral_on_local_session:    zk::throw_exception( ephemeral_on_local_session() );
    case error_code::reconfiguration_disabled:      zk::throw_exception( reconfiguration_disabled() );
    case error_code::transaction_failed:            zk::throw_exception( transaction_failed(error_code::transaction_failed, 0U) );
    default:                                        zk::throw_exception( error(code, "unknown") );
    }
}

zk::exception_ptr get_exception_ptr_of(error_code code)
{
    try
    {
        throw_error(code);
    }
    catch (...)
    {
        return zk::current_exception();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// error_category                                                                                                     //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class error_category_impl final : public std::error_category
{
public:
    virtual ptr<const char> name() const noexcept override { return "zookeeper"; }

    virtual std::string message(int condition) const override;
};

std::string error_category_impl::message(int condition) const
{
    return to_string(static_cast<error_code>(condition));
}

const std::error_category& error_category()
{
    static error_category_impl instance;
    return instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// errors                                                                                                             //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static std::string format_error(error_code code, const std::string& description)
{
    if (description.empty())
        return to_string(code);
    else
        return to_string(code) + ": " + description;
}

error::error(error_code code, const std::string& description) :
        std::system_error(static_cast<int>(code), error_category(), format_error(code, description)),
        _code(code)
{ }

error::~error() noexcept = default;

transport_error::transport_error(error_code code, const std::string& description) :
        error(code, std::move(description))
{ }

transport_error::~transport_error() noexcept = default;

connection_loss::connection_loss() :
        transport_error(error_code::connection_loss, "")
{ }

connection_loss::~connection_loss() noexcept = default;

marshalling_error::marshalling_error() :
        transport_error(error_code::marshalling_error, "")
{ }

marshalling_error::~marshalling_error() noexcept = default;

not_implemented::not_implemented(ptr<const char> op_name) :
        error(error_code::not_implemented, std::string("Operation not implemented: ") + op_name)
{ }

not_implemented::~not_implemented() noexcept = default;

invalid_arguments::invalid_arguments(error_code code, const std::string& description) :
        error(code, description)
{ }

invalid_arguments::invalid_arguments() :
        invalid_arguments(error_code::invalid_arguments, "")
{ }

invalid_arguments::~invalid_arguments() noexcept = default;

authentication_failed::authentication_failed() :
        invalid_arguments(error_code::authentication_failed, "")
{ }

authentication_failed::~authentication_failed() noexcept = default;

invalid_ensemble_state::invalid_ensemble_state(error_code code, const std::string& description) :
        error(code, description)
{ }

invalid_ensemble_state::~invalid_ensemble_state() noexcept = default;

new_configuration_no_quorum::new_configuration_no_quorum() :
        invalid_ensemble_state(error_code::new_configuration_no_quorum, "")
{ }

new_configuration_no_quorum::~new_configuration_no_quorum() noexcept = default;

reconfiguration_in_progress::reconfiguration_in_progress() :
        invalid_ensemble_state(error_code::reconfiguration_in_progress, "")
{ }

reconfiguration_in_progress::~reconfiguration_in_progress() noexcept = default;

reconfiguration_disabled::reconfiguration_disabled() :
        invalid_ensemble_state(error_code::reconfiguration_disabled, "")
{ }

reconfiguration_disabled::~reconfiguration_disabled() noexcept = default;

invalid_connection_state::invalid_connection_state(error_code code, const std::string& description) :
        error(code, description)
{ }

invalid_connection_state::~invalid_connection_state() noexcept = default;

session_expired::session_expired() :
        invalid_connection_state(error_code::session_expired, "")
{ }

session_expired::~session_expired() noexcept = default;

not_authorized::not_authorized() :
        invalid_connection_state(error_code::not_authorized, "")
{ }

not_authorized::~not_authorized() noexcept = default;

closed::closed() :
        invalid_connection_state(error_code::closed, "")
{ }

closed::~closed() noexcept = default;

ephemeral_on_local_session::ephemeral_on_local_session() :
        invalid_connection_state(error_code::ephemeral_on_local_session, "")
{ }

ephemeral_on_local_session::~ephemeral_on_local_session() noexcept = default;

read_only_connection::read_only_connection() :
        invalid_connection_state(error_code::read_only_connection, "")
{ }

read_only_connection::~read_only_connection() noexcept = default;

check_failed::check_failed(error_code code, const std::string& description) :
        error(code, description)
{ }

check_failed::~check_failed() noexcept = default;

no_entry::no_entry() :
        check_failed(error_code::no_entry, "")
{ }

no_entry::~no_entry() noexcept = default;

entry_exists::entry_exists() :
        check_failed(error_code::entry_exists, "")
{ }

entry_exists::~entry_exists() noexcept = default;

not_empty::not_empty() :
        check_failed(error_code::not_empty, "")
{ }

not_empty::~not_empty() noexcept = default;

version_mismatch::version_mismatch() :
        check_failed(error_code::version_mismatch, "")
{ }

version_mismatch::~version_mismatch() noexcept = default;

no_children_for_ephemerals::no_children_for_ephemerals() :
        check_failed(error_code::no_children_for_ephemerals, "")
{ }

no_children_for_ephemerals::~no_children_for_ephemerals() noexcept = default;

transaction_failed::transaction_failed(error_code underlying_cause, std::size_t op_index) :
        check_failed(error_code::transaction_failed,
                     std::string("Could not commit transaction due to ") + to_string(underlying_cause)
                     + " on operation " + std::to_string(op_index)
                    ),
        _underlying_cause(underlying_cause),
        _op_index(op_index)
{ }

transaction_failed::~transaction_failed() noexcept = default;

}
