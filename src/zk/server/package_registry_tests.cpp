#include <zk/tests/test.hpp>

#include "classpath.hpp"
#include "package_registry.hpp"
#include "package_registry_tests.hpp"

#include <memory>
#include <stdexcept>

namespace zk::server
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// test_package_registry                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

package_registry& test_package_registry::instance()
{
    static auto instance_ptr = std::make_shared<package_registry>();
    return *instance_ptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unit Tests                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(package_registry_tests, test_package_registry_global_instance)
{
    CHECK_LT(0U, test_package_registry::instance().size());
}

GTEST_TEST(package_registry_tests, registration)
{
    package_registry registry;
    CHECK_TRUE(registry.empty());

    auto registration1 = registry.register_classpath_server("1.0", classpath({ "RANDOM" }));
    CHECK_EQ(1U, registry.size());
    CHECK_THROWS(std::invalid_argument) { registry.register_classpath_server("1.0", classpath({ "RANDOM" })); };

    auto registration2 = registry.register_classpath_server("2.0", classpath({ "RANDOM" }));
    CHECK_EQ(2U, registry.size());
    registration2.reset();
    CHECK_EQ(1U, registry.size());
    CHECK_FALSE(registry.unregister_server(registration2));

    CHECK_TRUE(registry.unregister_server(std::move(registration1)));
    CHECK_TRUE(registry.empty());
}

}
