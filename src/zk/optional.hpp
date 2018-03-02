/// \file
/// Imports of \c optional and \c nullopt_t types, as well as the \c nullopt \c constexpr. These are \c std::optional,
/// \c std::nullopt_t, and \c std::nullopt, respectively.
#pragma once

#include <zk/config.hpp>

#include <optional>

namespace zk
{

/// \addtogroup Client
/// \{

template <typename T>
using optional = std::optional<T>;

using nullopt_t = std::nullopt_t;

using std::nullopt;

/// Apply \a transform with the arguments in \a x iff all of them have a value. Otherwise, \c nullopt will be returned.
template <typename FUnary, typename... T>
auto map(FUnary&& transform, const optional<T>&... x) -> optional<decltype(transform(x.value()...))>
{
    if ((x && ...))
        return transform(x.value()...);
    else
        return nullopt;
}

template <typename T>
optional<T> some(T x)
{
    return optional<T>(std::move(x));
}

/// \}

}
