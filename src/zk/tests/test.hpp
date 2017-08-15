#pragma once

#include <zk/config.hpp>

#ifdef GTEST_API_
#   error "GTest was included externally -- you MUST include <zk/tests/test.hpp> first"
#endif

// Before including GTest, tell it not to define the various generically-named macros (which clash with other common
// names).
#define GTEST_DONT_DEFINE_ASSERT_EQ 1
#define GTEST_DONT_DEFINE_ASSERT_NE 1
#define GTEST_DONT_DEFINE_ASSERT_LE 1
#define GTEST_DONT_DEFINE_ASSERT_LT 1
#define GTEST_DONT_DEFINE_ASSERT_GE 1
#define GTEST_DONT_DEFINE_ASSERT_GT 1
#define GTEST_DONT_DEFINE_TEST      1
#define GTEST_DONT_DEFINE_FAIL      1
#define GTEST_DONT_DEFINE_SUCCEED   1

#include <gtest/gtest.h>

// Maybe some day this will be defined
#ifndef GTEST_TEST_F
#   define GTEST_TEST_F TEST_F
#endif

namespace zk::test
{

/** \def CHECK
 *  Check a condition and abort the test in failure if it does not hold. When performing binary comparisons, prefer to
 *  use the check macros which accept two arguments (\c CHECK_EQ, \c CHECK_LT, etc), as these will print out the values
 *  in failure cases.
 *
 *  \example
 *  \code
 *  CHECK(something) << "Something isn't right!";
 *  \endcode
**/
#define CHECK(cond)       GTEST_TEST_BOOLEAN_(cond,     #cond, false, true,  GTEST_FATAL_FAILURE_)
#define CHECK_TRUE(cond)  GTEST_TEST_BOOLEAN_(!!(cond), #cond, false, true,  GTEST_FATAL_FAILURE_)
#define CHECK_FALSE(cond) GTEST_TEST_BOOLEAN_(!(cond),  #cond, true,  false, GTEST_FATAL_FAILURE_)
#define CHECK_EQ          GTEST_ASSERT_EQ
#define CHECK_NE          GTEST_ASSERT_NE
#define CHECK_LT          GTEST_ASSERT_LT
#define CHECK_LE          GTEST_ASSERT_LE
#define CHECK_GT          GTEST_ASSERT_GT
#define CHECK_GE          GTEST_ASSERT_GE
#define CHECK_SUCCESS     GTEST_SUCCEED
#define CHECK_FAIL        GTEST_FAIL

using test_fixture = ::testing::Test;

namespace detail
{

template <typename TException>
struct check_throws_info
{
    ptr<const char> filename;
    std::size_t     line_no;

    explicit check_throws_info(ptr<const char> filename, std::size_t line_no) :
            filename(filename),
            line_no(line_no)
    { }

    friend std::ostream& operator<<(std::ostream& os, const check_throws_info& info)
    {
        return os << info.filename << ':' << info.line_no;
    }
};

template <typename TException, typename FAction>
void operator+(const check_throws_info<TException>& info, FAction&& action)
{
    try
    {
        std::forward<FAction>(action)();
        CHECK_FAIL() << "At " << info << ": Action was supposed to throw, but it did not"; // LCOV_EXCL_LINE
    }
    catch (const TException&)
    {
        CHECK_SUCCESS() << "Successfully threw expected error";
    }
}

/** \def CHECK_THROWS
 *  Ensure that a block of code throws an exception of type \a ex.
 *
 *  \code
 *  CHECK_THROWS(std::logic_error)
 *  {
 *      throw std::logic_error("Some issue");
 *  }; // <- note the ';'
 *  \endcode
**/
#define CHECK_THROWS(ex)                                                                                               \
    ::zk::test::detail::check_throws_info<ex>(__FILE__, __LINE__) + [&] ()

}

}
