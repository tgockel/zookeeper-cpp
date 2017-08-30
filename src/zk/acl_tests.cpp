#include <zk/tests/test.hpp>

#include "acl.hpp"

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// permission                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(permission_tests, all)
{
    auto all = permission::read
             | permission::write
             | permission::create
             | permission::erase
             | permission::admin;
    CHECK_EQ(permission::all, all);
    CHECK_EQ("all", to_string(all));
}

GTEST_TEST(permission_tests, stringification)
{
    CHECK_EQ("none", to_string(permission::none));
    CHECK_EQ("read|create", to_string(permission::read | permission::create));
    CHECK_EQ("write|admin", to_string(permission::write | permission::admin));
    CHECK_EQ("read|create|erase", to_string(permission::read | permission::create | permission::erase));
    CHECK_EQ("admin", to_string(permission::admin));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl_rule                                                                                                           //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(acl_rule_tests, stringification)
{
    CHECK_EQ("(ip:80.23.0.0/16, read|write)",
             to_string(acl_rule("ip", "80.23.0.0/16", permission::read | permission::write))
            );
}

GTEST_TEST(acl_rule_tests, comparisons)
{
    acl_rule creator_all = { "auth", "", permission::all };
    acl_rule world_open  = { "world", "anyone", permission::all };

    CHECK_EQ(creator_all, creator_all);
    CHECK_NE(creator_all, world_open);
    CHECK_LT(creator_all, world_open);
    CHECK_LE(creator_all, world_open);
    CHECK_GT(world_open, creator_all);
    CHECK_GE(world_open, creator_all);
}

GTEST_TEST(acl_rule_tests, hashing)
{
    acl_rule creator_all = { "auth", "", permission::all };
    acl_rule world_open  = { "world", "anyone", permission::all };

    CHECK_EQ(hash(creator_all), hash(creator_all));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// acl                                                                                                                //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(acl_tests, stringification)
{
    CHECK_EQ("[(auth, all)]",          to_string(acls::creator_all()));
    CHECK_EQ("[(world:anyone, all)]",  to_string(acls::open_unsafe()));
    CHECK_EQ("[(world:anyone, read)]", to_string(acls::read_unsafe()));

    CHECK_EQ("[(auth, read), (ip:50.40.30.0/24, all)]",
             to_string(acl({ { "auth", "", permission::read }, { "ip", "50.40.30.0/24", permission::all } }))
            );
}

}
