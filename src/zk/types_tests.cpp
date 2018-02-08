#include <zk/tests/test.hpp>

#include "types.hpp"

namespace zk
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// event_type                                                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(event_type_tests, stringification)
{
    CHECK_EQ("error",              to_string(event_type::error));
    CHECK_EQ("created",            to_string(event_type::created));
    CHECK_EQ("erased",             to_string(event_type::erased));
    CHECK_EQ("changed",            to_string(event_type::changed));
    CHECK_EQ("child",              to_string(event_type::child));
    CHECK_EQ("session",            to_string(event_type::session));
    CHECK_EQ("not_watching",       to_string(event_type::not_watching));
    CHECK_EQ("event_type(764532)", to_string(static_cast<event_type>(764532)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// state                                                                                                              //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTEST_TEST(state_tests, stringification)
{
    CHECK_EQ("closed",                to_string(state::closed));
    CHECK_EQ("connecting",            to_string(state::connecting));
    CHECK_EQ("connected",             to_string(state::connected));
    CHECK_EQ("read_only",             to_string(state::read_only));
    CHECK_EQ("expired_session",       to_string(state::expired_session));
    CHECK_EQ("authentication_failed", to_string(state::authentication_failed));
    CHECK_EQ("state(605983)",         to_string(static_cast<state>(605983)));
}

}
