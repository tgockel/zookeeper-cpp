#pragma once

#include <zk/config.hpp>

#include <functional>
#include <iosfwd>
#include <string>

#include "forwards.hpp"

namespace zk
{

/** Enumeration of types of events that may occur. **/
enum class event_type : int
{
    error           =  0, //!< Invalid event (this should never be issued).
    created         =  1, //!< Issued when a znode at a given path is created.
    erased          =  2, //!< Issued when a znode at a given path is erased.
    changed         =  3, //!< Issued when the data of a watched znode are altered. This event value is issued whenever
                          //!< a \e set operation occurs without an actual contents check, so there is no guarantee the
                          //!< data actually changed.
    child           =  4, //!< Issued when the children of a watched znode are created or deleted. This event is not
                          //!< issued when the data within children is altered.
    session         = -1, //!< This value is issued as part of an event when the \c state changes.
    not_watching    = -2, //!< Watch has been forcefully removed. This is generated when the server for some reason
                          //!< (probably a resource constraint), will no longer watch a node for a client.
};

std::ostream& operator<<(std::ostream&, const event_type&);

std::string to_string(const event_type&);

/** Enumeration of states the client may be at in a \c watch_callback. It represents the state of the connection at the
 *  time the event was generated.
**/
enum class state : int
{
    closed                =    0, //!< The client is not connected to any server in the ensemble.
    connecting            =    1, //!< The client is connecting.
    associating           =    2, //!< Client is attempting to associate a session.
    connected             =    3, //!< The client is in the connected state -- it is connected to a server in the
                                  //!< ensemble (one of the servers specified in the host connection parameter during
                                  //!< ZooKeeper client creation).
    read_only             =    5, //!< The client is connected to a read-only server, that is the server which is not
                                  //!< currently connected to the majority. The only operations allowed after receiving
                                  //!< this state is read operations. This state is generated for read-only clients only
                                  //!< since read/write clients aren't allowed to connect to read-only servers.
    not_connected         =  999,
    expired_session       = -112, //!< The serving cluster has expired this session. The ZooKeeper client connection
                                  //!< (the session) is no longer valid. You must create a new client \c connection if
                                  //!< you with to access the ensemble.
    authentication_failed = -113, //!< Authentication has failed -- connection requires a new \c connection instance
                                  //!< with different credentials.
};

std::ostream& operator<<(std::ostream&, const state&);

std::string to_string(const state&);

/** Function to be called when an event happens.
 *
 *  \note
 *  If you are familiar with the ZooKeeper C API, this simple callback function might seem very simplistic. Since this
 *  API only supports non-global watches, the extra parameters are not helpful and generally unsafe. As an example, the
 *  \c path parameter is not included. It is not helpful to include, since you already know the path you specified when
 *  you set the watch in the first place. Furthermore, it is unsafe, as the contents addressed by the pointer are only
 *  safe in the callback thread. While we could copy the path into an \c std::string, this would require an allocation
 *  on every delivery, which is very intrusive.
**/
using watch_callback = std::function<void (event_type, state)>;

class watch_handle final
{
};

}
