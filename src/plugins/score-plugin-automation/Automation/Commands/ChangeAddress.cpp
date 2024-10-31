// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "ChangeAddress.hpp"

#include <State/Address.hpp>
#include <State/Domain.hpp>
#include <State/Value.hpp>
#include <State/ValueConversion.hpp>

#include <Device/Address/AddressSettings.hpp>
#include <Device/Node/DeviceNode.hpp>

#include <Curve/Point/CurvePointModel.hpp>

#include <Explorer/Explorer/DeviceExplorerModel.hpp>

#include <Automation/AutomationModel.hpp>

#include <score/model/path/Path.hpp>
#include <score/model/path/PathSerialization.hpp>
#include <score/model/tree/TreeNode.hpp>
#include <score/serialization/DataStreamVisitor.hpp>

#include <ossia/network/common/destination_qualifiers.hpp>
#include <ossia/network/domain/domain.hpp>
#include <ossia/network/value/value_conversion.hpp>

#include <Color/GradientModel.hpp>

namespace Automation
{
ChangeAddress::ChangeAddress(
    const ProcessModel& autom, const State::AddressAccessor& newval)
    : m_path{autom}
    , m_old{autom.address(), autom.min(), autom.max()}
    , m_new(Explorer::makeFullAddressAccessorSettings(
          newval, score::IDocument::documentContext(autom), 0., 1., 0.5))
{
  if(m_new.domain.get())
  {
    Curve::CurveDomain c(m_new.domain.get(), m_new.value);
    m_new.domain.get() = ossia::make_domain(c.min, c.max);
  }
}

ChangeAddress::ChangeAddress(
    const ProcessModel& autom, Device::FullAddressAccessorSettings newval)
    : m_path{autom}
    , m_old{autom.address(), autom.min(), autom.max()}
    , m_new{std::move(newval)}
{
  if(m_new.domain.get())
  {
    Curve::CurveDomain c(m_new.domain.get(), m_new.value);
    m_new.domain.get() = ossia::make_domain(c.min, c.max);
  }
}

ChangeAddress::ChangeAddress(
    const ProcessModel& autom, const Device::FullAddressSettings& newval)
    : m_path{autom}
{
  m_new.address = newval.address;
  m_new.domain = newval.domain;
  if(m_new.domain.get())
  {
    Curve::CurveDomain c(m_new.domain.get(), m_new.value);
    m_new.domain.get() = ossia::make_domain(c.min, c.max);
  }
  m_new.address.qualifiers.get().unit = newval.unit;

  m_old.address = autom.address();
  m_old.domain = ossia::make_domain(autom.min(), autom.max());
}

void ChangeAddress::undo(const score::DocumentContext& ctx) const
{
  auto& autom = m_path.find(ctx);

  {
    // QSignalBlocker blck{autom.curve()};
    auto& dom = m_old.domain.get();
    autom.setMin(dom.convert_min<double>());
    autom.setMax(dom.convert_max<double>());

    autom.setAddress(m_old.address);
  }
  // autom.curve().changed();
}

void ChangeAddress::redo(const score::DocumentContext& ctx) const
{
  auto& autom = m_path.find(ctx);

  {
    // QSignalBlocker blck{autom.curve()};
    auto& dom = m_new.domain.get();
    if(dom)
    {
      autom.setMin(dom.convert_min<double>());
      autom.setMax(dom.convert_max<double>());
    }
    autom.setAddress(m_new.address);
  }
  // autom.curve().changed();
}

void ChangeAddress::serializeImpl(DataStreamInput& s) const
{
  s << m_path << m_old << m_new;
}

void ChangeAddress::deserializeImpl(DataStreamOutput& s)
{
  s >> m_path >> m_old >> m_new;
}
}
