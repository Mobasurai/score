// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <QJsonObject>
#include <QJsonValue>
#include <algorithm>
#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#include "AutomationModel.hpp"
#include <ossia/editor/dataspace/dataspace_visitors.hpp>
#include <Curve/CurveModel.hpp>
#include <State/Address.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/VisitorCommon.hpp>

template <>
void DataStreamReader::read(
    const Automation::ProcessModel& autom)
{
  readFrom(autom.curve());

  m_stream
      << *autom.outlet
      << autom.min()
      << autom.max()
      << autom.tween();

  insertDelimiter();
}


template <>
void DataStreamWriter::write(Automation::ProcessModel& autom)
{
  autom.setCurve(new Curve::Model{*this, &autom});

  autom.outlet = std::make_unique<Process::Port>(*this, &autom);

  double min, max;
  bool tw;

  m_stream >> min >> max >> tw;

  autom.setMin(min);
  autom.setMax(max);
  autom.setTween(tw);

  checkDelimiter();
}


template <>
void JSONObjectReader::read(
    const Automation::ProcessModel& autom)
{
  obj["Curve"] = toJsonObject(autom.curve());
  obj["Outlet"] = toJsonObject(*autom.outlet);
  obj[strings.Min] = autom.min();
  obj[strings.Max] = autom.max();
  obj["Tween"] = autom.tween();
}


template <>
void JSONObjectWriter::write(Automation::ProcessModel& autom)
{
  JSONObject::Deserializer curve_deser{obj["Curve"].toObject()};
  autom.setCurve(new Curve::Model{curve_deser, &autom});

  JSONObjectWriter writer{obj["Outlet"].toObject()};
  autom.outlet = std::make_unique<Process::Port>(writer, &autom);

  autom.setMin(obj[strings.Min].toDouble());
  autom.setMax(obj[strings.Max].toDouble());
  autom.setTween(obj["Tween"].toBool());
}
