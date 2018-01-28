#include "package_registry.hpp"

#include <stdexcept>

namespace zk::server
{

struct package_registry::registration_info final
{
    std::weak_ptr<package_registry> owner;
    std::string                     name;

    registration_info(std::shared_ptr<package_registry> owner, std::string name) :
            owner(std::move(owner)),
            name(std::move(name))
    { }

    ~registration_info() noexcept
    {
        if (auto strong_owner = owner.lock())
            strong_owner->unregister_server(*this);
    }
};

package_registry::package_registry() :
        _lifetime(std::make_shared<int>())
{ }

package_registry::~package_registry() noexcept
{
    std::unique_lock<std::mutex> ax(_protect);
    _lifetime.reset();
}

package_registry::registration package_registry::register_classpath_server(std::string version, classpath packages)
{
    std::unique_lock<std::mutex> ax(_protect);

    auto ret = _registrations.insert({ std::move(version), std::move(packages) });
    if (!ret.second)
        throw std::invalid_argument(version + " is already registered");

    return std::make_shared<registration_info>(std::shared_ptr<package_registry>(_lifetime, this), ret.first->first);
}

bool package_registry::unregister_server(const registration_info& reg)
{
    std::unique_lock<std::mutex> ax(_protect);
    return _registrations.erase(reg.name) > 0U;
}

bool package_registry::unregister_server(registration reg)
{
    if (reg)
        return unregister_server(*reg);
    else
        return false;
}

package_registry::size_type package_registry::size() const
{
    std::unique_lock<std::mutex> ax(_protect);
    return _registrations.size();
}

optional<classpath> package_registry::find_newest_classpath() const
{
    std::unique_lock<std::mutex> ax(_protect);
    if (_registrations.empty())
        return nullopt;
    else
        return _registrations.rbegin()->second;
}

}
