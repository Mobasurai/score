// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "IntervalDurations.hpp"

#include "IntervalModel.hpp"

#include <Process/TimeValue.hpp>
#include <Process/TimeValueSerialization.hpp>

#include <Scenario/Document/ModelConsistency.hpp>

#include <score/serialization/DataStreamVisitor.hpp>
#include <score/serialization/JSONValueVisitor.hpp>
#include <score/serialization/JSONVisitor.hpp>

#define TIME_TOLERANCE_MSEC 0.5
#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::IntervalDurations)
namespace Scenario
{
IntervalDurations::~IntervalDurations() { }

IntervalDurations& IntervalDurations::operator=(const IntervalDurations& other)
{
  m_defaultDuration = other.m_defaultDuration;
  m_minDuration = other.m_minDuration;
  m_maxDuration = other.m_maxDuration;
  m_guiDuration = other.m_guiDuration;

  m_playPercentage = other.m_playPercentage;
  m_speed = other.m_speed;
  m_rigidity = other.m_rigidity;

  m_isMinNull = other.m_isMinNull;
  m_isMaxInfinite = other.m_isMaxInfinite;

  return *this;
}

void IntervalDurations::checkConsistency()
{
  m_model.consistency.setWarning(
      minDuration().msec() < 0 - TIME_TOLERANCE_MSEC
      || (isRigid() && minDuration() != maxDuration())); // a voir

  m_model.consistency.setValid(
      minDuration() - TimeVal::fromMsecs(TIME_TOLERANCE_MSEC) <= m_defaultDuration
      && maxDuration() + TimeVal::fromMsecs(TIME_TOLERANCE_MSEC) >= m_defaultDuration
      && m_defaultDuration.msec() + TIME_TOLERANCE_MSEC > 0);
}

void IntervalDurations::setDefaultDuration(const TimeVal& arg)
{
  SCORE_ASSERT(arg.impl >= 0);
  if(m_defaultDuration != arg)
  {
    m_defaultDuration = arg;
    defaultDurationChanged(arg);

    if(m_guiDuration < m_defaultDuration)
      setGuiDuration(m_defaultDuration * 1.1);

    checkConsistency();
  }
}

void IntervalDurations::setMinDuration(const TimeVal& arg)
{
  if(m_minDuration != arg && !m_isMinNull)
  {
    m_minDuration = arg;
    minDurationChanged(arg);

    checkConsistency();
  }
}

void IntervalDurations::setMaxDuration(const TimeVal& arg)
{
  if(m_maxDuration != arg)
  {
    m_maxDuration = arg;
    maxDurationChanged(arg);

    if(m_guiDuration < m_maxDuration && !m_isMaxInfinite)
      setGuiDuration(m_maxDuration * 1.1);

    checkConsistency();
  }
}

void IntervalDurations::setGuiDuration(TimeVal guiDuration)
{
  SCORE_ASSERT(guiDuration.impl >= 0);
  if(m_guiDuration == guiDuration)
    return;

  m_guiDuration = guiDuration;
  guiDurationChanged(guiDuration);
}

void IntervalDurations::setPlayPercentage(double arg)
{
  if(m_playPercentage == arg)
    return;

  auto old = m_playPercentage;

  m_playPercentage = arg;

  // Only used for DurationSectionWidget
  if(m_defaultDuration * std::abs(arg - old) > TimeVal::fromMsecs(32))
  {
    playPercentageChanged(arg);
  }
}

void IntervalDurations::setRigid(bool arg)
{
  if(m_rigidity == arg)
    return;

  m_rigidity = arg;
  rigidityChanged(arg);
}

void IntervalDurations::setMinNull(bool isMinNull)
{
  if(m_isMinNull == isMinNull)
    return;

  m_isMinNull = isMinNull;
  minNullChanged(isMinNull);
  minDurationChanged(minDuration());
}

void IntervalDurations::setMaxInfinite(bool isMaxInfinite)
{
  if(m_isMaxInfinite == isMaxInfinite)
    return;

  m_isMaxInfinite = isMaxInfinite;
  maxInfiniteChanged(isMaxInfinite);
  maxDurationChanged(maxDuration());
}

SCORE_PLUGIN_SCENARIO_EXPORT void IntervalDurations::Algorithms::setDurationInBounds(
    IntervalModel& cstr, const TimeVal& time)
{
  SCORE_ASSERT(time.impl >= 0);
  if(cstr.duration.defaultDuration() != time)
  {
    // Rigid
    if(cstr.duration.isRigid())
    {
      cstr.duration.setDefaultDuration(time);

      cstr.duration.setMinDuration(time);
      cstr.duration.setMaxDuration(time);
    }
    else // TODO The checking must be done elsewhere if(arg >= m_minDuration &&
         // arg <= m_maxDuration)
         // --> it should be in a command to be undoable
    {
      cstr.duration.setDefaultDuration(time);
    }
  }
}

SCORE_PLUGIN_SCENARIO_EXPORT void
IntervalDurations::Algorithms::fixAllDurations(IntervalModel& cstr, const TimeVal& time)
{
  SCORE_ASSERT(time.impl >= 0);
  auto& dur = cstr.duration;
  if(dur.defaultDuration() != time || dur.minDuration() != time
     || dur.maxDuration() != time)
  {
    dur.m_defaultDuration = time;
    dur.m_minDuration = time;
    dur.m_maxDuration = time;
    dur.m_guiDuration = time * 1.1;

    dur.defaultDurationChanged(time);
    dur.minDurationChanged(time);
    dur.maxDurationChanged(time);
    if(!dur.m_rigidity)
    {
      dur.m_rigidity = true;
      dur.rigidityChanged(true);
    }
  }
}

SCORE_PLUGIN_SCENARIO_EXPORT void IntervalDurations::Algorithms::changeAllDurations(
    IntervalModel& cstr, const TimeVal& time)
{
  SCORE_ASSERT(time.impl >= 0);
  auto& d = cstr.duration;
  if(d.isRigid())
  {
    fixAllDurations(cstr, time);
    return;
  }

  if(d.defaultDuration() != time)
  {
    const auto delta = time - d.defaultDuration();
    d.m_defaultDuration = time;
    if(auto new_gui_dur = time * 1.1; new_gui_dur > d.m_guiDuration)
      d.m_guiDuration = new_gui_dur;

    // FIXME ! why is there m_isMaxInfinite and m_maxDuration.isInfinite() ?!
    if(!d.m_isMinNull)
      d.m_minDuration += delta;

    if(!d.m_isMaxInfinite)
      d.m_maxDuration += delta;

    if(d.m_minDuration < TimeVal::zero())
      d.m_minDuration = TimeVal::zero();

    if(d.m_maxDuration <= d.m_defaultDuration && !d.m_isMaxInfinite)
      //      d.m_maxDuration =
      //      TimeVal{std::nextafter(d.m_defaultDuration.msec(),
      //      d.m_defaultDuration.msec() * 2.)};
      d.m_maxDuration = d.m_defaultDuration
                        * 1.05; // TimeVal{std::nextafter(d.m_defaultDuration.msec(),
                                // d.m_defaultDuration.msec() * 2.)};

    if(d.m_guiDuration < d.m_defaultDuration)
      d.m_guiDuration = time * 1.1;

    if(d.m_guiDuration < d.m_maxDuration && !d.m_isMaxInfinite)
      d.m_guiDuration = d.m_maxDuration * 1.1;

    d.defaultDurationChanged(time);
    d.guiDurationChanged(d.m_guiDuration);
    d.minDurationChanged(d.m_minDuration);
    d.maxDurationChanged(d.m_maxDuration);

    d.checkConsistency();
  }
}

SCORE_PLUGIN_SCENARIO_EXPORT void IntervalDurations::Algorithms::scaleAllDurations(
    IntervalModel& cstr, const TimeVal& time)
{
  if(time.impl <= 0)
  {
    cstr.duration.setDefaultDuration(TimeVal::zero());
    cstr.duration.setMinDuration(TimeVal::zero());
    cstr.duration.setMaxDuration(TimeVal::zero());
    return;
  }

  if(cstr.duration.defaultDuration() != time)
  {
    // Note: the OSSIA implementation requires min <= dur <= max at all time
    // and will throw if not the case. Hence this order.

    auto ratio = time / cstr.duration.defaultDuration();

    const bool same_min
        = (cstr.duration.defaultDuration() == cstr.duration.minDuration());
    const bool same_max
        = (cstr.duration.defaultDuration() == cstr.duration.maxDuration());

    cstr.duration.setDefaultDuration(time);

    TimeVal newmin = same_min ? time : TimeVal{cstr.duration.minDuration() * ratio};
    if(newmin.impl > time.impl)
      newmin.impl = time.impl - 1;
    if(newmin.impl < 0)
      newmin.impl = 0;

    TimeVal newmax = same_max ? cstr.duration.defaultDuration()
                              : TimeVal{cstr.duration.maxDuration() * ratio};
    if(newmax.impl < time.impl)
      newmax.impl = time.impl + 1;

    cstr.duration.setMinDuration(newmin);
    cstr.duration.setMaxDuration(newmax);
  }
}
}

template <>
void DataStreamReader::read(const Scenario::IntervalDurations& durs)
{
  m_stream << durs.m_defaultDuration << durs.m_minDuration << durs.m_maxDuration
           << durs.m_guiDuration << durs.m_speed << durs.m_rigidity << durs.m_isMinNull
           << durs.m_isMaxInfinite;
}

template <>
void DataStreamWriter::write(Scenario::IntervalDurations& durs)
{
  m_stream >> durs.m_defaultDuration >> durs.m_minDuration >> durs.m_maxDuration
      >> durs.m_guiDuration >> durs.m_speed >> durs.m_rigidity >> durs.m_isMinNull
      >> durs.m_isMaxInfinite;
}

template <>
void JSONReader::read(const Scenario::IntervalDurations& durs)
{
  obj[strings.DefaultDuration] = durs.m_defaultDuration;
  obj[strings.MinDuration] = durs.m_minDuration;
  obj[strings.MaxDuration] = durs.m_maxDuration;
  obj[strings.GuiDuration] = durs.m_guiDuration;
  obj["Speed"] = durs.m_speed;
  obj[strings.Rigidity] = durs.m_rigidity;
  obj[strings.MinNull] = durs.m_isMinNull;
  obj[strings.MaxInf] = durs.m_isMaxInfinite;
}

template <>
void JSONWriter::write(Scenario::IntervalDurations& durs)
{
  durs.m_defaultDuration <<= obj[strings.DefaultDuration];
  durs.m_minDuration <<= obj[strings.MinDuration];
  durs.m_maxDuration <<= obj[strings.MaxDuration];

  assign_with_default(durs.m_speed, obj.tryGet("Speed"), 1.0);
  durs.m_rigidity = obj[strings.Rigidity].toBool();
  durs.m_isMinNull = obj[strings.MinNull].toBool();
  durs.m_isMaxInfinite = obj[strings.MaxInf].toBool();

  using namespace std::chrono;
  if(durs.m_minDuration.msec() < 0)
    durs.m_minDuration = durs.m_defaultDuration;
  if(durs.m_maxDuration.msec() < 0)
    durs.m_maxDuration = durs.m_defaultDuration;
  if(durs.isRigid())
  {
    durs.m_minDuration = durs.m_defaultDuration;
    durs.m_maxDuration = durs.m_defaultDuration;
  }

  auto guidur = obj.constFind(strings.GuiDuration);
  if(guidur != obj.constEnd())
    durs.m_guiDuration <<= *guidur;
  else
  {
    if(durs.m_isMaxInfinite)
      durs.m_guiDuration = durs.m_defaultDuration;
    else
      durs.m_guiDuration = durs.m_maxDuration;
  }
}
