#include <zk/tests/test.hpp>

#include "error.hpp"

namespace zk
{

GTEST_TEST(error_code_tests, throwing)
{
    for (error_code code : all_error_codes())
    {
        try
        {
            throw_error(code);
        }
        catch (const system_error& ex)
        {
            CHECK_EQ(code, ex.code());
            CHECK_TRUE(is_system_error(code));
        }
        catch (const api_error& ex)
        {
            CHECK_EQ(code, ex.code());
            CHECK_TRUE(is_api_error(code));
        }
        catch (const unknown_error& ex)
        {
            CHECK_EQ(code, ex.code());
            bool is_expected_unknown = code == error_code::ok
                                    || code == error_code::system_error
                                    || code == error_code::api_error;
            CHECK_TRUE(is_expected_unknown) << code;
        }
    }
}

GTEST_TEST(error_code_tests, to_string_bogus_code)
{
    CHECK_EQ("error_code(19)", to_string(static_cast<error_code>(19)));
}

}
