// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "SummaryInspectorWidget.hpp"

#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Inspector/Event/EventSummaryWidget.hpp>
#include <Scenario/Inspector/Interval/IntervalSummaryWidget.hpp>
#include <Scenario/Inspector/TimeSync/TimeSyncSummaryWidget.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>

#include <Inspector/InspectorSectionWidget.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::SummaryInspectorWidget)
namespace Scenario
{
SummaryInspectorWidget::SummaryInspectorWidget(
    const IdentifiedObjectAbstract* obj,
    const ossia::hash_set<const IntervalModel*>& intervals,
    const ossia::hash_set<const TimeSyncModel*>& timesyncs,
    const ossia::hash_set<const EventModel*>& events,
    const ossia::hash_set<const StateModel*>& states,
    const score::DocumentContext& context, QWidget* parent)
    : InspectorWidgetBase{*obj, context, parent, tr("Summary")}
{
  setObjectName("SummaryInspectorWidget");
  setParent(parent);

  m_itvSection = new Inspector::InspectorSectionWidget{tr("Intervals"), false, this};
  m_properties.push_back(m_itvSection);

  for(auto c : intervals)
  {
    auto w = new IntervalSummaryWidget{*c, context, this};
    m_itvSection->addContent(w);
    m_itvs.push_back(w);
  }

  m_syncSection = new Inspector::InspectorSectionWidget{tr("Syncs"), false, this};
  m_properties.push_back(m_syncSection);
  for(auto t : timesyncs)
  {
    auto w = new TimeSyncSummaryWidget{*t, context, this};
    m_syncSection->addContent(w);
    m_syncs.push_back(w);
  }

  m_evSection = new Inspector::InspectorSectionWidget{tr("Events"), false, this};
  m_properties.push_back(m_evSection);
  for(auto ev : events)
  {
    auto w = new EventSummaryWidget{*ev, context, this};
    m_evSection->addContent(w);
    m_evs.push_back(w);
  }

  updateAreaLayout(m_properties);
}

SummaryInspectorWidget::~SummaryInspectorWidget() { }

void SummaryInspectorWidget::update(
    const QList<const IdentifiedObjectAbstract*>& sourceElements)
{
  ossia::flat_set<const EventModel*> events;
  ossia::flat_set<const TimeSyncModel*> timesyncs;
  ossia::flat_set<const IntervalModel*> intervals;
  auto scenar = dynamic_cast<ScenarioInterface*>(sourceElements[0]->parent());
  for(auto elt : sourceElements)
  {
    if(auto st = qobject_cast<const StateModel*>(elt))
    {
      if(auto ev = scenar->findEvent(st->eventId()))
      {
        auto tn = scenar->findTimeSync(ev->timeSync());
        if(!tn)
          continue;

        events.insert(ev);
        timesyncs.insert(tn);
      }
    }
    else if(auto ev = qobject_cast<const EventModel*>(elt))
    {
      auto tn = scenar->findTimeSync(ev->timeSync());
      if(!tn)
        continue;
      events.insert(ev);
      timesyncs.insert(tn);
    }
    else if(auto tn = qobject_cast<const TimeSyncModel*>(elt))
    {
      timesyncs.insert(tn);
    }
    else if(auto cstr = qobject_cast<const IntervalModel*>(elt))
    {
      intervals.insert(cstr);
    }
  }

  auto cur_events = m_evs;
  auto cur_sync = m_syncs;
  auto cur_itv = m_itvs;
  for(auto ev : events)
  {
    if(ossia::any_of(cur_events, [=](EventSummaryWidget* w) { return &w->event == ev; }))
    {
      continue;
    }
    auto w = new EventSummaryWidget{*ev, this->context(), this};
    m_evSection->addContent(w);
    m_evs.push_back(w);
  }
  for(auto w : cur_events)
  {
    auto it
        = ossia::find_if(events, [=](const EventModel* e) { return e == &w->event; });
    if(it == events.end())
    {
      ossia::remove_one(m_evs, w);
      m_evSection->removeContent(w);
    }
  }

  for(auto sn : timesyncs)
  {
    if(ossia::any_of(cur_sync, [=](TimeSyncSummaryWidget* w) { return &w->sync == sn; }))
    {
      continue;
    }
    auto w = new TimeSyncSummaryWidget{*sn, this->context(), this};
    m_syncSection->addContent(w);
    m_syncs.push_back(w);
  }
  for(auto w : cur_sync)
  {
    auto it = ossia::find_if(
        timesyncs, [=](const TimeSyncModel* e) { return e == &w->sync; });
    if(it == timesyncs.end())
    {
      ossia::remove_one(m_syncs, w);
      m_syncSection->removeContent(w);
    }
  }

  for(auto itv : intervals)
  {
    if(ossia::any_of(
           cur_itv, [=](IntervalSummaryWidget* w) { return &w->interval == itv; }))
    {
      continue;
    }
    auto w = new IntervalSummaryWidget{*itv, this->context(), this};
    m_itvSection->addContent(w);
    m_itvs.push_back(w);
  }
  for(auto w : cur_itv)
  {
    auto it = ossia::find_if(
        intervals, [=](const IntervalModel* e) { return e == &w->interval; });
    if(it == intervals.end())
    {
      ossia::remove_one(m_itvs, w);
      m_itvSection->removeContent(w);
    }
  }
}
}
