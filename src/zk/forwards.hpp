#pragma once

#include <zk/config.hpp>

namespace zk
{

class acl;
class acl_list;
class client;
class connection;
enum class create_mode : unsigned int;
class create_result;
class error;
class event;
class exists_result;
enum class error_code : int;
enum class event_type : int;
class get_result;
class get_children_result;
class multi_result;
class multi_op;
class op;
enum class op_type : int;
enum class permission : unsigned int;
class set_result;
enum class state : int;
struct transaction_id;
struct version;
class watch_result;

}
