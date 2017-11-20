#pragma once

#include <zk/config.hpp>

#include <memory>
#include <utility>

#include "buffer.hpp"
#include "forwards.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "string_view.hpp"
#include "results.hpp"
#include "types.hpp"

namespace zk
{

/** \defgroup Client
 *  Interacting with ZooKeeper as a \ref client.
 *  \{
**/

/** A ZooKeeper client connection. This is the primary class for interacting with the ZooKeeper cluster. The best way to
 *  create a client is with the static \ref connect function.
**/
class client final
{
public:
    client() noexcept;

    /** Create a client connected to the cluster specified by \c conn_string.
     *
     *  \param conn_string A ZooKeeper \ref ConnectionStrings "connection string".
    **/
    explicit client(string_view conn_string);

    /** Create a client connected with \a conn. **/
    explicit client(std::shared_ptr<connection> conn) noexcept;

    /** Create a client connected to the cluster specified by \c conn_string.
     *
     *  \param conn_string A ZooKeeper \ref ConnectionStrings "connection string".
     *  \returns A future which will be filled when the conneciton is established. The future will be filled in error if
     *   the client will never be able to connect to the cluster (for example: a bad connection string).
    **/
    static future<client> connect(string_view conn_string);

    client(const client&) noexcept = default;
    client(client&&) noexcept = default;

    client& operator=(const client&) noexcept = default;
    client& operator=(client&&) noexcept = default;

    ~client() noexcept;

    void close();

    /** Return the data and the \c stat of the node of the given \a path.
     *
     *  \throws no_node If no node with the given path exists, the future will be deliever with \c no_node.
    **/
    future<get_result> get(string_view path) const;

    /** Similar to \c get, but if the call is successful (no error is returned), a watch will be left on the node with
     *  the given \a path. The watch will be triggered by a successful operation that sets data on the node or erases
     *  the node.
     *
     *  \throws no_node If no node with the given path exists, the future will be deliever with \c no_node. To watch for
     *   the creation of a node, use \c watch_exists.
    **/
    future<watch_result> watch(string_view path) const;

    /** Return the list of the children of the node of the given \a path. The returned values are not prefixed with the
     *  provided \a path; i.e. if the database contains \c "/path/a" and \c "/path/b", the result of \c get_children for
     *  \c "/path" will be `["a", "b"]`. The list of children returned is not sorted and no guarantee is provided as to
     *  its natural or lexical order.
     *
     *  \throws no_node If no node with the given path exists, the future will be delivered with \c no_node.
    **/
    future<get_children_result> get_children(string_view path) const;

    /** Similar to \c get_children, but if the call is successful (no error is returned), a watch will be left on the
     *  node with the given \a path. The watch will be triggered by a successful operation that erases the node of the
     *  given \e path or creates or erases a child under the node.
    **/
    future<watch_children_result> watch_children(string_view path) const;

    /** Return the \c stat of the node of the given \a path or \c nullopt if no such node exists. **/
    future<exists_result> exists(string_view path) const;

    /** Similar to \c watch, but if the call is successful (no error is returned), a watch will be left on the node with
     *  the given \a path. The watch will be triggered by a successful operation that creates the node, erases the node,
     *  or sets the data on the node.
    **/
    future<watch_exists_result> watch_exists(string_view path) const;

    /** Create a node with the given \a path.
     *
     *  This operation, if successful, will trigger all the watches left on the node of the given path by \c watch API
     *  calls, and the watches left on the parent node by \c watch_children API calls.
     *
     *  \param path The path or path pattern (if using \c create_mode::sequential) to create.
     *  \param data The data to create inside the node.
     *  \param mode Specifies the behavior of the created node (see \c create_mode for more information).
     *  \param rules The ACL for the created znode. If unspecified, it is equivalent to providing \c acls::open_unsafe.
     *  \returns A future which will be filled with the name of the created znode and its \c stat.
     *
     *  \throws node_exists If a node with the same actual path already exists in the ZooKeeper, the future will be
     *   delivered with \c node_exists. Note that since a different actual path is used for each invocation of creating
     *   sequential node with the same path argument, the call should never error in this manner.
     *  \throws no_node If the parent node does not exist in the ZooKeeper, the future will be delivered with
     *   \c no_node.
     *  \throws no_children_for_ephemerals An ephemeral node cannot have children. If the parent node of the given path
     *   is ephemeral, the future will be delivered with \c no_children_for_ephemerals.
     *  \throws invalid_acl If the \a acl is invalid or empty, the future will be delivered with \c invalid_acl.
     *  \throws invalid_arguments The maximum allowable size of the data array is 1 MiB (1,048,576 bytes). If \a data
     *   is larger than this the future will be delivered with \c invalid_arguments.
    **/
    future<create_result> create(string_view   path,
                                 const buffer& data,
                                 const acl&    rules,
                                 create_mode   mode = create_mode::normal
                                );
    future<create_result> create(string_view   path,
                                 const buffer& data,
                                 create_mode   mode = create_mode::normal
                                );

    /** Set the data for the node of the given \a path if such a node exists and the given version matches the version
     *  of the node (if the given version is \c version::any, there is no version check). This operation, if successful,
     *  will trigger all the watches on the node of the given \c path left by \c watch calls.
     *
     *  \throws no_node If no node with the given \a path exists, the future will be delivered with \c no_node.
     *  \throws bad_version If the given version \a check does not match the node's version, the future will be
     *   delivered with \c bad_version.
     *  \throws invalid_arguments The maximum allowable size of the data array is 1 MiB (1,048,576 bytes). If \a data
     *   is larger than this the future will be delivered with \c invalid_arguments.
    **/
    future<set_result> set(string_view path, const buffer& data, version check = version::any());

    /** Return the ACL and \c stat of the node of the given path.
     *
     *  \throws no_node If no node with the given path exists, the future will be deliever with \c no_node.
    **/
    future<get_acl_result> get_acl(string_view path) const;

    /** Set the ACL for the node of the given \a path if such a node exists and the given version \a check matches the
     *  version of the node.
     *
     *  \param check If specified, check that the ACL matches. Keep in mind this is the \c acl_version, not the data
     *   \c version -- there is no way to have this operation fail on changes to \c stat::data_version.
     *
     *  \throws no_node If no node with the given \a path exists, the future will be delivered with \c no_node.
     *  \throws bad_version If the given version \a check does not match the node's version, the future will be
     *   delivered with \c bad_version.
    **/
    future<void> set_acl(string_view path, const acl& rules, acl_version check = acl_version::any());

    /** Erase the node with the given \a path. The call will succeed if such a node exists, and the given version
     *  \a check matches the node's version (if the given version is \c version::any, it matches any node's versions).
     *  This operation, if successful, will trigger all the watches on the node of the given path left by \c watch API
     *  calls, watches left by \c watch_exists API calls, and the watches on the parent node left by \c watch_children
     *  API calls.
     *
     *  \throws no_node If no node with the given \a path exists, the future will be delivered with \c no_node.
     *  \throws bad_version If the given version \a check does not match the node's version, the future will be
     *   delivered with \c bad_version.
     *  \throws not_empty You are only allowed to erase nodes with no children. If the node has children, the future
     *   will be delievered with \c not_empty.
    **/
    future<void> erase(string_view path, version check = version::any());

    /** Ensure that all subsequent reads observe the data at the transaction on the server at or past real-time \e now.
     *  If your application communicates only through reads and writes of ZooKeeper, this operation is never needed.
     *  However, if your application communicates a change in ZooKeeper state through means outside of ZooKeeper (called
     *  a "hidden channel" in ZooKeeper vernacular), then it is possible for a receiver to attempt to react to a change
     *  before it can observe it through ZooKeeper state.
     *
     *  \warning
     *  The internal pipeline for this operation is not the same as modifying operations (\c set, \c create, etc.). In
     *  cases of leader failure, there is a chance that the leader does not have support from the quorum, as it has
     *  switched to a new leader. While this is rare, it is still \e possible that not all updates have been processed.
     *
     *  \note
     *  Other APIs call this operation \c sync and allow you to provide a \c path parameter. There are a few issues with
     *  this. First: that name conflicts with the commonly-used POSIX \c sync command, leading to confusion that data in
     *  ZooKeeper does not have integrity. Secondly, this operation has more in common with \c std::atomic_thread_fence
     *  or the x86 \c lfence instruction than \c sync (on the server, "flush" is an appropriate term -- just like fence
     *  implementations in CPUs). Finally, the \c path parameter is ignored by the server -- all future fetches are
     *  fenced, no matter what path is specified. In the future, ZooKeeper might support partitioning, in which case the
     *  \c path parameter might become relevant.
     *
     *  It is often not necessary to wait for the fence future to be returned, as future reads will be synced without
     *  waiting. However, there is no guarantee on the ordering of the read if the future returned from \c load_fence
     *  is completed in error.
     *
     *  \code
     *  auto fence_future = client.load_fence();
     *  // data_future will be completed with the load_fence, even though we haven't waited to complete
     *  auto data_future = client.get("/some/path");
     *
     *  // Useful to use when_all to concat and error check (C++ Extensions for Concurrency, ISO/IEC TS 19571:2016)
     *  auto guaranteed_future = std::when_all(std::move(fence_future), std::move(data_future));
     *  \endcode
    **/
    future<void> load_fence() const;

    /** Commit the transaction specified by \a txn. The operations are performed atomically: They will either all
     *  succeed or all fail.
     *
     *  \throws transaction_failed If the transaction does not complete with an error in the transaction itself (any
     *   \c error_code that fits \c is_api_error), the future will be delivered with \c transaction_failed. Check the
     *   thrown \c transaction_failed::underlying_cause for more information.
     *  \throws system_error For the same reasons any other operation might fail, the future will be delivered with a
     *   specific \c system_error.
    **/
    future<multi_result> commit(multi_op txn);

private:
    std::shared_ptr<connection> _conn;
};

/** \} **/

}
