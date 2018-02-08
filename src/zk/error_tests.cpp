#include <zk/tests/test.hpp>

#include "error.hpp"

namespace zk
{

static error_code all_error_codes[] =
    {
        error_code::connection_loss,
        error_code::marshalling_error,
        error_code::not_implemented,
        error_code::invalid_arguments,
        error_code::new_configuration_no_quorum,
        error_code::reconfiguration_in_progress,
        error_code::no_entry,
        error_code::not_authorized,
        error_code::version_mismatch,
        error_code::no_children_for_ephemerals,
        error_code::entry_exists,
        error_code::not_empty,
        error_code::session_expired,
        error_code::authentication_failed,
        error_code::closed,
        error_code::read_only_connection,
        error_code::ephemeral_on_local_session,
        error_code::reconfiguration_disabled,
        error_code::transaction_failed,
    };

GTEST_TEST(error_code_tests, throwing)
{
    for (error_code code : all_error_codes)
    {
        try
        {
            try
            {
                throw_error(code);
            }
            catch (const check_failed& ex)
            {
                CHECK_EQ(code, ex.code());
                CHECK_TRUE(is_check_failed(code)) << code;
                throw;
            }
            catch (const invalid_arguments& ex)
            {
                CHECK_EQ(code, ex.code());
                CHECK_TRUE(is_invalid_arguments(code)) << code;
                throw;
            }
            catch (const invalid_connection_state& ex)
            {
                CHECK_EQ(code, ex.code());
                CHECK_TRUE(is_invalid_connection_state(code)) << code;
                throw;
            }
            catch (const invalid_ensemble_state& ex)
            {
                CHECK_EQ(code, ex.code());
                CHECK_TRUE(is_invalid_ensemble_state(code)) << code;
                throw;
            }
            catch (const not_implemented& ex)
            {
                CHECK_EQ(code, ex.code());
                CHECK_EQ(error_code::not_implemented, ex.code());
                throw;
            }
            catch (const transport_error& ex)
            {
                CHECK_EQ(code, ex.code());
                throw;
            }
            catch (const error& ex)
            {
                // not a real error, so it will come across as unknown
                CHECK_EQ(error_code::ok, ex.code());
                throw;
            }
            CHECK_FAIL() << "Should not have reached this point";
        }
        catch (const error& ex)
        {
            CHECK_EQ(code, ex.code());
        }
    }
}

GTEST_TEST(error_code_tests, to_string_bogus_code)
{
    CHECK_EQ("error_code(19)", to_string(static_cast<error_code>(19)));
}

}
