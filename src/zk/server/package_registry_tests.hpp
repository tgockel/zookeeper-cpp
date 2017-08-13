#pragma once

#include <zk/config.hpp>

namespace zk::server
{

class package_registry;

/** Global package registry for server unit tests. **/
class test_package_registry final
{
public:
    static package_registry& instance();
};

}
