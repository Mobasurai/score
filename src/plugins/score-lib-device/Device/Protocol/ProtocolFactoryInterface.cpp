// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ProtocolFactoryInterface.hpp"

#include <QUrl>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Device::DeviceEnumerator)

namespace Device
{
ProtocolFactory::~ProtocolFactory() = default;

ProtocolFactory::Flags ProtocolFactory::flags() const noexcept
{
  return {};
}

int ProtocolFactory::visualPriority() const noexcept
{
  return 0;
}

QUrl ProtocolFactory::manual() const noexcept
{
  return {};
}

DeviceEnumerators
ProtocolFactory::getEnumerators(const score::DocumentContext& ctx) const
{
  return {};
}

bool ProtocolFactory::checkResourcesAvailable(
    const Device::DeviceSettings& a, const DeviceResourceMap&) const noexcept
{
  return true;
}

DeviceEnumerator::~DeviceEnumerator() { }

}
