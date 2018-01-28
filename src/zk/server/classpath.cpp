#include "classpath.hpp"

#include <zk/string_view.hpp>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <ostream>
#include <sstream>
#include <system_error>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace zk::server
{

template <typename TContainer, typename TSep>
void join(std::ostream& os, const TContainer& src, TSep sep)
{
    bool first = true;
    for (const auto& x : src)
    {
        if (!std::exchange(first, false))
            os << sep;
        os << x;
    }
}

static bool file_exists(ptr<const char> path)
{
    struct ::stat s;
    if (::stat(path, &s))
    {
        if (errno == ENOENT)
            return false;
        else
            throw std::system_error(errno, std::system_category());
    }
    else
    {
        return true;
    }
}

static classpath find_system_default()
{
    string_view locations[] =
        {
            "/usr/share/java",
            "/usr/local/share/java",
        };
    string_view requirements[] =
        {
            "zookeeper.jar",
            "slf4j-api.jar",
            "slf4j-simple.jar",
        };

    std::vector<std::string> components;
    std::vector<string_view> unfound;
    for (auto jar : requirements)
    {
        bool found = false;
        for (auto base_loc : locations)
        {
            auto potential_sz = base_loc.size() + jar.size() + 4;
            char potential[potential_sz];
            std::snprintf(potential, potential_sz, "%s/%s", base_loc.data(), jar.data());

            if (file_exists(potential))
            {
                found = true;
                components.emplace_back(std::string(potential));
                break;
            }
        }

        if (!found)
            unfound.emplace_back(jar);
    }

    if (unfound.empty())
    {
        return classpath(std::move(components));
    }
    else
    {
        std::ostringstream os;
        os << "Could not find requirement" << ((unfound.size() == 1U) ? "" : "s") << ": ";
        join(os, unfound, ", ");
        os << ". Searched paths: ";
        join(os, locations, ", ");
        throw std::runtime_error(os.str());
    }

}

classpath classpath::system_default()
{
    static const auto instance = find_system_default();
    return instance;
}

classpath::classpath(std::vector<std::string> components) noexcept :
        _components(std::move(components))
{ }

std::string classpath::command_line() const
{
    std::ostringstream os;
    os << *this;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const classpath& self)
{
    join(os, self._components, ':');
    return os;
}

}
