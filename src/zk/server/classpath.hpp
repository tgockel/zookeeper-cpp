#pragma once

#include <zk/config.hpp>

#include <iosfwd>
#include <string>
#include <vector>

namespace zk::server
{

/// \addtogroup Server
/// \{

/// Represents a collection of JARs or other Java entities that should be provided as the `--classpath` to the JVM. This
/// is used to fetch both the ZooKeeper package (`zookeeper.jar`), but the required SLF JAR. If you do not know or care
/// about what that second part means, just call \ref system_default.
class classpath final
{
public:
    /// Create a classpath specification from the provided \a components.
    explicit classpath(std::vector<std::string> components) noexcept;

    /// Load the system-default classpath for ZooKeeper. This searches for `zookeeper.jar` nad `slf4j-simple.jar` in
    /// various standard locations.
    static classpath system_default();

    /// Get the command-line representation of this classpath. This puts \c ':' characters between the components.
    std::string command_line() const;

    friend std::ostream& operator<<(std::ostream&, const classpath&);

private:
    std::vector<std::string> _components;
};

/// \}

}
