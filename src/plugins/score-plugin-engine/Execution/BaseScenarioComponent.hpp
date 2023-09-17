#pragma once
#include <ossia/editor/scenario/clock.hpp>

#include <QObject>

#include <score_plugin_engine_export.h>

#include <memory>
#include <verdigris>
namespace ossia
{
struct time_value;
class scenario;
}

namespace Scenario
{
class IntervalModel;
class EventModel;
class StateModel;
class TimeSyncModel;
class BaseScenario;
class ScenarioInterface;
}
class DeviceList;
namespace Execution
{
class IntervalComponent;
class EventComponent;
class StateComponent;
class TimeSyncComponent;
}

// MOVEME
// Like BaseScenarioContainer but with references
// to existing elements instead.
class BaseScenarioRefContainer
{
public:
  BaseScenarioRefContainer(
      Scenario::IntervalModel& interval, Scenario::ScenarioInterface& s);

  BaseScenarioRefContainer(
      Scenario::IntervalModel& interval, Scenario::StateModel& startState,
      Scenario::StateModel& endState, Scenario::EventModel& startEvent,
      Scenario::EventModel& endEvent, Scenario::TimeSyncModel& startNode,
      Scenario::TimeSyncModel& endNode)
      : m_interval{interval}
      , m_startState{startState}
      , m_endState{endState}
      , m_startEvent{startEvent}
      , m_endEvent{endEvent}
      , m_startNode{startNode}
      , m_endNode{endNode}
  {
  }

  Scenario::IntervalModel& interval() const { return m_interval; }

  Scenario::TimeSyncModel& startTimeSync() const { return m_startNode; }
  Scenario::TimeSyncModel& endTimeSync() const { return m_endNode; }

  Scenario::EventModel& startEvent() const { return m_startEvent; }
  Scenario::EventModel& endEvent() const { return m_endEvent; }

  Scenario::StateModel& startState() const { return m_startState; }
  Scenario::StateModel& endState() const { return m_endState; }

private:
  Scenario::IntervalModel& m_interval;
  Scenario::StateModel& m_startState;
  Scenario::StateModel& m_endState;
  Scenario::EventModel& m_startEvent;
  Scenario::EventModel& m_endEvent;
  Scenario::TimeSyncModel& m_startNode;
  Scenario::TimeSyncModel& m_endNode;
};

namespace Execution
{
struct Context;
class SCORE_PLUGIN_ENGINE_EXPORT BaseScenarioElement final : public QObject
{
  W_OBJECT(BaseScenarioElement)
public:
  BaseScenarioElement(const Context&, QObject* parent);
  ~BaseScenarioElement();

  void init(bool forcePlay, BaseScenarioRefContainer);
  void cleanup();
  bool active() const { return bool(m_ossia_interval); }

  IntervalComponent& baseInterval() const;

  TimeSyncComponent& startTimeSync() const;
  TimeSyncComponent& endTimeSync() const;

  EventComponent& startEvent() const;
  EventComponent& endEvent() const;

  StateComponent& startState() const;
  StateComponent& endState() const;

  ossia::scenario& baseScenario() const;

public:
  void finished() E_SIGNAL(SCORE_PLUGIN_ENGINE_EXPORT, finished)

private:
  const Context& m_ctx;
  std::shared_ptr<ossia::scenario> m_ossia_scenario;
  std::shared_ptr<IntervalComponent> m_ossia_interval;

  std::shared_ptr<TimeSyncComponent> m_ossia_startTimeSync;
  std::shared_ptr<TimeSyncComponent> m_ossia_endTimeSync;

  std::shared_ptr<EventComponent> m_ossia_startEvent;
  std::shared_ptr<EventComponent> m_ossia_endEvent;

  std::shared_ptr<StateComponent> m_ossia_startState;
  std::shared_ptr<StateComponent> m_ossia_endState;
};
}
