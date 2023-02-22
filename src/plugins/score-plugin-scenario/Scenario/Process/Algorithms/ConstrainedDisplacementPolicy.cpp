#include "ConstrainedDisplacementPolicy.hpp"

namespace Scenario
{

void ConstrainedDisplacementPolicy::init(
    ProcessModel& scenario, const QVector<Id<TimeSyncModel>>& draggedElements)
{
}

void ConstrainedDisplacementPolicy::computeDisplacement(
    ProcessModel& scenario, const QVector<Id<TimeSyncModel>>& draggedElements,
    const TimeVal& deltaTime, ElementsProperties& elementsProperties)
{
  // Scale all the intervals before and after.
  if(draggedElements.empty())
    return;
  auto tn_id = draggedElements[0];
  auto& tn = scenario.timeSyncs.at(tn_id);
  const auto& intervalsBefore = Scenario::previousNonGraphIntervals(tn, scenario);
  const auto& intervalsAfter = Scenario::nextNonGraphIntervals(tn, scenario);
  QObjectList processesToSave;

  // 1. Find the delta bounds.
  // We have to stop as soon as a interval would become too small.
  TimeVal min{TimeVal::infinity};
  TimeVal max{TimeVal::infinity};
  for(const auto& id : intervalsBefore)
  {
    auto it = elementsProperties.intervals.find(id);
    if(it == elementsProperties.intervals.end())
    {
      auto& c = scenario.intervals.at(id);
      for(auto& proc : c.processes)
        processesToSave.append(&proc);
      if(c.duration.defaultDuration() < min)
        min = c.duration.defaultDuration();
    }
    else
    {
      const IntervalProperties& c = it->second;
      if(c.oldDefault < min)
        min = c.oldDefault;
    }
  }

  for(const auto& id : intervalsAfter)
  {
    auto it = elementsProperties.intervals.find(id);
    if(it == elementsProperties.intervals.end())
    {
      auto& c = scenario.intervals.at(id);
      for(auto& proc : c.processes)
        processesToSave.append(&proc);
      if(c.duration.defaultDuration() < max)
        max = c.duration.defaultDuration();
    }
    else
    {
      const IntervalProperties& c = it->second;
      if(c.oldDefault < max)
        max = c.oldDefault;
    }
  }

  // Save cables

  if(!processesToSave.empty())
  {
    elementsProperties.cables = Dataflow::saveCables(
        processesToSave, score::IDocument::documentContext(scenario));
  }

  // 2. Rescale deltaTime
  auto dt = deltaTime;
  if(min != TimeVal{TimeVal::infinity} && dt < TimeVal::zero() && dt < -min)
  {
    dt = -min;
  }
  else if(max != TimeVal{TimeVal::infinity} && dt > TimeVal::zero() && dt > max)
  {
    dt = max;
  }

  for(auto& id : intervalsBefore)
  {
    auto it = elementsProperties.intervals.find(id);
    if(it != elementsProperties.intervals.end())
    {
      auto& c = it->second;
      c.newMin = std::max(TimeVal::zero(), c.oldMin + dt);
      c.newMax = c.oldMax + dt;

      SCORE_ASSERT(c.oldDefault + dt >= TimeVal::zero());
      SCORE_ASSERT(c.oldDefault + dt <= c.newMax);
    }
    else
    {
      auto& curInterval = scenario.intervals.at(id);
      IntervalProperties c{curInterval, false};
      c.oldDate = curInterval.date();
      c.oldDefault = curInterval.duration.defaultDuration();
      c.oldMin = curInterval.duration.minDuration();
      c.oldMax = curInterval.duration.maxDuration();

      c.newMin = std::max(TimeVal::zero(), c.oldMin + dt);
      c.newMax = c.oldMax + dt;

      SCORE_ASSERT(c.oldDefault + dt >= TimeVal::zero());
      SCORE_ASSERT(c.oldDefault + dt <= c.newMax);

      elementsProperties.intervals.insert({id, std::move(c)});
    }
  }

  for(auto& id : intervalsAfter)
  {
    auto it = elementsProperties.intervals.find(id);
    if(it != elementsProperties.intervals.end())
    {
      auto& c = it->second;
      c.newMin = std::max(TimeVal::zero(), c.oldMin - dt);
      c.newMax = c.oldMax - dt;

      SCORE_ASSERT(c.oldDefault - dt >= TimeVal::zero());
      SCORE_ASSERT(c.oldDefault - dt <= c.newMax);
    }
    else
    {
      auto& curInterval = scenario.intervals.at(id);
      IntervalProperties c{curInterval, false};
      c.oldDate = curInterval.date();
      c.oldDefault = curInterval.duration.defaultDuration();
      c.oldMin = curInterval.duration.minDuration();
      c.oldMax = curInterval.duration.maxDuration();

      c.newMin = std::max(TimeVal::zero(), c.oldMin - dt);
      c.newMax = c.oldMax - dt;

      SCORE_ASSERT(c.oldDefault - dt >= TimeVal::zero());
      SCORE_ASSERT(c.oldDefault - dt <= c.newMax);

      elementsProperties.intervals.insert({id, std::move(c)});
    }
  }

  auto it = elementsProperties.timesyncs.find(tn.id());
  if(it != elementsProperties.timesyncs.end())
  {
    it->second.newDate = it->second.oldDate + dt;
  }
  else
  {
    TimenodeProperties t;

    t.oldDate = tn.date();
    t.newDate = t.oldDate + dt;
    elementsProperties.timesyncs.insert({tn.id(), t});
  }
}

QString ConstrainedDisplacementPolicy::name()
{
  return QString{"Minimal way"};
}
}
