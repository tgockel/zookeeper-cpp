#pragma once

#include <zk/config.hpp>

#include <memory>
#include <utility>

#include "buffer.hpp"
#include "forwards.hpp"
#include "future.hpp"
#include "optional.hpp"
#include "stat.hpp"
#include "string_view.hpp"

namespace zk
{

/** When used in \c client::set, this value determines how the znode is created on the server. These values can be ORed
 *  together to create combinations.
**/
enum class create_mode : unsigned int
{
    /** Standard behavior of a znode -- the opposite of doing any of the options. **/
    normal = 0b0000,
    /** The znode will be deleted upon the client's disconnect. **/
    ephemeral = 0b0001,
    /** The name of the znode will be appended with a monotonically increasing number. The actual path name of a
     *  sequential node will be the given path plus a suffix \c "i" where \c i is the current sequential number of the
     *  node. The sequence number is always fixed length of 10 digits, 0 padded. Once such a node is created, the
     *  sequential number will be incremented by one.
    **/
    sequential = 0b0010,
    /** Container nodes are special purpose nodes useful for recipes such as leader, lock, etc. When the last child of a
     *  container is deleted, the container becomes a candidate to be deleted by the server at some point in the future.
     *  Given this property, you should be prepared to get \c no_node when creating children inside of this container
     *  node.
    **/
    container = 0b0100,
};

constexpr create_mode operator|(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) | static_cast<unsigned int>(b));
}

constexpr create_mode operator&(create_mode a, create_mode b)
{
    return create_mode(static_cast<unsigned int>(a) & static_cast<unsigned int>(b));
}

constexpr create_mode operator~(create_mode a)
{
    return create_mode(~static_cast<unsigned int>(a));
}

/** Check that \a self has \a flags set. **/
constexpr bool is_set(create_mode self, create_mode flags)
{
    return (self & flags) == flags;
}

std::ostream& operator<<(std::ostream&, const create_mode&);

std::string to_string(const create_mode&);

/** A ZooKeeper client connection. This is the primary class for interacting with the ZooKeeper cluster. **/
class client final
{
public:
    client() noexcept;

    /** Create a client connected to the cluster specified by \c conn_string. See \c connection::create for
     *  documentation on connection strings.
    **/
    explicit client(string_view conn_string);

    /** Create a client connected with \a conn. **/
    explicit client(std::shared_ptr<connection> conn) noexcept;

    /** Create a client connected to the cluster specified by \c conn_string. See \c connection::create for
     *  documentation on connection strings.
     *
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
    future<std::pair<buffer, stat>> get(string_view path) const;

    /** Create a node with the given \a path.
     *
     *  This operation, if successful, will trigger all the watches left on the node of the given path by \c watch API
     *  calls, and the watches left on the parent node by \c watch_children API calls.
     *
     *  \param path The path or path pattern (if using \c create_mode::sequential) to create.
     *  \param data The data to create inside the node.
     *  \param mode Specifies the behavior of the created node (see \c create_mode for more information).
     *  \param acls The ACL for the created znode. If unspecified, it is equivalent to providing \c acls::open_unsafe.
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
    future<std::string> create(string_view     path,
                               const buffer&   data,
                               const acl_list& acls,
                               create_mode     mode = create_mode::normal
                              );
    future<std::string> create(string_view     path,
                               const buffer&   data,
                               create_mode     mode = create_mode::normal
                              );

private:
    std::shared_ptr<connection> _conn;
};

}
