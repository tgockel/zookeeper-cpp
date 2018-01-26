#pragma once

#include <zk/config.hpp>

namespace zk
{

class acl;
class acl_rule;
struct acl_version;
struct child_version;
class client;
class connection;
class connection_params;
enum class create_mode : unsigned int;
class create_result;
class error;
class event;
class exists_result;
enum class error_code : int;
enum class event_type : int;
class get_acl_result;
class get_children_result;
class get_result;
class multi_result;
class multi_op;
class notification;
class op;
enum class op_type : int;
enum class permission : unsigned int;
class set_result;
enum class state : int;
struct transaction_id;
struct version;
class watch_children_result;
class watch_exists_result;
class watch_op;
class watch_result;

}
