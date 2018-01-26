#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <memory>
#include <variant>

#include "results.hpp"

namespace zk
{

/** \addtogroup Client
 *  \{
**/

/// A notification that something has happened with the connection. This can be anything from the data from a \c get
/// request being returned to a notification that we have lost our connection to the server.
class notification final
{
public:
    /// The types of notifications this can represent. The \c std::monostate_t is used for default construction -- a
    /// notification holding this will never be returned by \ref connection::receive.
    using storage_type = std::variant<std::monostate_t,
                                      std::exception_ptr,
                                      event,
                                      get_result,
                                      get_children_result,
                                      get_acl_result,
                                      exists_result,
                                      create_result,
                                      set_result
                                     >;

    /// User-defined tracker object. See \ref tracker.
    using tracker_type = ptr<void>;

public:
    explicit notification() noexcept :
            _data(std::monostate),
            _tracker(nullptr)
    { }

    explicit constexpr notification(storage_type data, tracker_type track) :
            _data(std::move(data)),
            _tracker(std::move(track))
    { }

    notification(const notification&) = default;

    notification(notification&& src) noexcept :
            _data(std::exchange(src._data, std::monostate)),
            _tracker(std::exchange(src._tracker, nullptr))
    { }

    notification& operator=(const notification&) = default;

    notification& operator=(notification&& src) noexcept
    {
        _data    = std::exchange(src._data, std::monostate);
        _tracker = std::exchange(src._tracker, nullptr);
        return *this;
    }

    /// Check that this notification holds \c T.
    ///
    /// \returns \c true if this notification holds a \c T; \c false if it does not. If this returns \c true, it is safe
    ///  to call \ref as with the given \c T.
    template <typename T> bool is() const { return std::holds_alternative<T>(_data); }

    /// Get the contents of this notification as if it was \c T. This convenience mechanism is the preferred alternative
    /// to using \c std::get on \ref get.
    ///
    /// \throws std::bad_variant_access if the contained data is not of type \c T.
    template <typename T> const T& as() const & { return std::get<T>(_data); }
    template <typename T> T&       as() &       { return std::get<T>(_data); }
    template <typename T> T&&      as() &&      { return std::get<T>(std::move(_data)); }

    /// Get the storage variant that backs this notification.
    const storage_type& get() const & { return _data; }
    storage_type&       get() &       { return _data; }
    storage_type&&      get() &&      { return std::move(_data); }

    /// Get the user-defined tracker object. This value will be the same as the \c track argument given to
    /// \ref connection::submit.
    const tracker_type& tracker() const & { return _tracker; }
    tracker_type&       tracker() &       { return _tracker; }

    /// Reset this back to the base state of empty.
    void reset();

    friend std::ostream& operator<<(std::ostream&, const notification&);

private:
    storage_type _data;
    tracker_type _tracker;
};

/** \} **/
}
