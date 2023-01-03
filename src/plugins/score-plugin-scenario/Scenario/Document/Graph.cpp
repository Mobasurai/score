// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "Graph.hpp"

#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/TimeSync/TimeSyncModel.hpp>
#include <Scenario/Process/Algorithms/Accessors.hpp>
#include <Scenario/Process/Algorithms/ContainersAccessors.hpp>
#include <Scenario/Process/ScenarioInterface.hpp>
#include <Scenario/Process/ScenarioModel.hpp>

#include <score/model/ModelMetadata.hpp>

#include <ossia/detail/pod_vector.hpp>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/labeled_graph.hpp>
#include <boost/graph/tiernan_all_cycles.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/range/iterator_range.hpp>

#include <QTimer>

#include <tsl/hopscotch_set.h>
namespace Scenario
{
/*
struct CycleDetector : public boost::dfs_visitor<>
{
  CycleDetector( std::vector<Scenario::IntervalModel*>& intervalsInCycles)
  : intervalsInCycles{intervalsInCycles}
  {

  }

  template<typename E, typename G>
  void back_edge(const E& e, G& g)
  {
    auto itv = *(Scenario::IntervalModel**)e.get_property();
    if(itv->graphal())
      intervalsInCycles.push_back(itv);
  }
  std::vector<Scenario::IntervalModel*>& intervalsInCycles;

};
*/
struct CycleDetector
{
  const Scenario::ProcessModel& scenario;
  bool& cycles;

  ossia::small_vector<IntervalModel*, 4> this_path_itvs;
  CycleDetector(const Scenario::ProcessModel& scenar, bool& cycles)
      : scenario{scenar}
      , cycles{cycles}
  {
  }

  template <typename T>
  bool allIntersectGraphal(T& a, T& b)
  {
    ossia::small_vector<Id<IntervalModel>, 4> intersect;
    a.sort([](const auto& lhs, const auto& rhs) { return lhs.val() < rhs.val(); });
    b.sort([](const auto& lhs, const auto& rhs) { return lhs.val() < rhs.val(); });
    std::set_intersection(
        a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(intersect));

    for(const auto& itv : intersect)
    {
      auto& m = scenario.interval(itv);
      if(!m.graphal())
      {
        return false;
      }
    }

    for(const auto& itv : intersect)
    {
      auto& m = scenario.interval(itv);
      this_path_itvs.push_back(&m);
    }

    return true;
  }

  bool allIntervalsAreGraph(TimeSyncModel& a, TimeSyncModel& b)
  {
    // auto prev_a = Scenario::previousIntervals(a, scenario);
    auto next_a = Scenario::nextIntervals(a, scenario);
    auto prev_b = Scenario::previousIntervals(b, scenario);
    // auto next_b = Scenario::nextIntervals(b, scenario);

    // auto prev_av = std::vector(prev_a.begin(), prev_a.end());
    //auto next_av = std::vector(next_a.begin(), next_a.end());

    //auto prev_bv = std::vector(prev_b.begin(), prev_b.end());
    // auto next_bv = std::vector(next_b.begin(), next_b.end());
    return allIntersectGraphal(next_a, prev_b);
    // else if(allIntersectGraphal(next_bv, prev_av))
    //{
    //  cycles = true;
    //}
  }

  template <typename T>
  bool anyIntersectGraphal(T& a, T& b)
  {
    ossia::small_vector<Id<IntervalModel>, 4> intersect;
    a.sort([](const auto& lhs, const auto& rhs) { return lhs.val() < rhs.val(); });
    b.sort([](const auto& lhs, const auto& rhs) { return lhs.val() < rhs.val(); });
    std::set_intersection(
        a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(intersect));

    bool ok = false;
    for(const auto& itv : intersect)
    {
      auto& m = scenario.interval(itv);
      if(m.graphal())
      {
        ok = true;
      }
    }

    if(ok)
    {
      for(const auto& itv : intersect)
      {
        auto& m = scenario.interval(itv);
        this_path_itvs.push_back(&m);
      }
    }

    return ok;
  }
  bool anyIntervalIsGraph(TimeSyncModel& a, TimeSyncModel& b)
  {
    // auto prev_a = Scenario::previousIntervals(a, scenario);
    auto next_a = Scenario::nextIntervals(a, scenario);
    auto prev_b = Scenario::previousIntervals(b, scenario);
    // auto next_b = Scenario::nextIntervals(b, scenario);

    // auto prev_av = std::vector(prev_a.begin(), prev_a.end());
    //auto next_av = std::vector(next_a.begin(), next_a.end());

    //auto prev_bv = std::vector(prev_b.begin(), prev_b.end());
    // auto next_bv = std::vector(next_b.begin(), next_b.end());
    return anyIntersectGraphal(next_a, prev_b);
    // else if(allIntersectGraphal(next_bv, prev_av))
    //{
    //  cycles = true;
    //}
  }

  template <typename Path>
  void cycle(const Path& p, const Scenario::Graph& g)
  {
    this_path_itvs.clear();
    bool has_non_graphal = false;
    for(auto it = p.begin(); it != p.end(); ++it)
    {
      TimeSyncModel* this_ts = (TimeSyncModel*)g[*it];
      TimeSyncModel* next_ts = nullptr;
      auto next = it + 1;
      if(next != p.end())
      {
        next_ts = (TimeSyncModel*)g[*next];
      }
      else
      {
        next_ts = (TimeSyncModel*)g[*p.begin()];
      }

      if(!anyIntervalIsGraph(*this_ts, *next_ts))
        has_non_graphal = true;
      //if (!allIntervalsAreGraph(*this_ts, *next_ts))
      //  has_non_graphal = true;
    }

    if(!has_non_graphal)
    {
      cycles = true;
      for(auto itv : this_path_itvs)
        itv->consistency.setValid(false);
    }
  }
};

struct PathDetectorState
{
  tsl::hopscotch_set<Scenario::TimeSyncModel*> nodes;
};

struct PathDetector : public boost::default_dfs_visitor
{
  // because these geniuses of boost decided to pass the visitor by value...
  std::shared_ptr<PathDetectorState> state{std::make_shared<PathDetectorState>()};

  void discover_vertex(Scenario::Graph::vertex_descriptor i, const Scenario::Graph& g)
  {
    state->nodes.insert(g[i]);
  }
};

TimenodeGraph::TimenodeGraph(const Scenario::ProcessModel& scenar)
    : m_scenario{scenar}
{
  for(auto& tn : scenar.getTimeSyncs())
  {
    m_vertices[&tn] = boost::add_vertex(&tn, m_graph);
  }

  for(auto& cst : scenar.getIntervals())
  {
    m_edges[&cst] = boost::add_edge(
                        m_vertices[&Scenario::startTimeSync(cst, scenar)],
                        m_vertices[&Scenario::endTimeSync(cst, scenar)], &cst, m_graph)
                        .first;
  }

  scenar.intervals.added.connect<&TimenodeGraph::intervalsChanged>(this);
  scenar.intervals.removed.connect<&TimenodeGraph::intervalsChanged>(this);
  scenar.timeSyncs.added.connect<&TimenodeGraph::timeSyncsChanged>(this);
  scenar.timeSyncs.removed.connect<&TimenodeGraph::timeSyncsChanged>(this);
}

bool TimenodeGraph::hasPath(
    const TimeSyncModel& t1, const TimeSyncModel& t2) const noexcept
{
  // First find the vertex matching the time sync after our interval
  auto first_vertex = vertices().at(&t1);

  // Do a depth-first search from where we're starting
  PathDetector vis;
  std::vector<boost::default_color_type> color_map(boost::num_vertices(graph()));

  boost::depth_first_visit(
      graph(), first_vertex, vis,
      boost::make_iterator_property_map(
          color_map.begin(), boost::get(boost::vertex_index, graph()), color_map[0]));

  return vis.state->nodes.contains((TimeSyncModel*)&t2);
}

bool TimenodeGraph::hasCycles() const noexcept
{
  return m_cycles;
}

void TimenodeGraph::recompute()
{
  m_cycles = false;
  m_vertices.clear();
  m_edges.clear();
  m_graph.clear();

  for(auto& tn : m_scenario.getTimeSyncs())
  {
    m_vertices[&tn] = boost::add_vertex(&tn, m_graph);
  }

  for(auto& cst : m_scenario.getIntervals())
  {
    cst.consistency.setValid(true);
    m_edges[&cst]
        = boost::add_edge(
              m_vertices[&Scenario::startTimeSync(cst, m_scenario)],
              m_vertices[&Scenario::endTimeSync(cst, m_scenario)], &cst, m_graph)
              .first;
  }

  CycleDetector vis{m_scenario, m_cycles};
  tiernan_all_cycles(m_graph, vis);
}

void TimenodeGraph::writeGraphviz()
{
  #if 0
  auto get_name = [](auto* elt) { return elt->metadata().getName().toStdString(); };

  std::stringstream s;
  boost::write_graphviz(
      s, m_graph,
      [&](auto& out, const auto& v) {
    out << "[label=\"" << get_name(this->m_graph[v]) << "\"]";
      },
      [&](auto& out, const auto& v) {
    out << "[label=\"" << get_name(this->m_graph[v]) << "\"]";
  });

  std::cout << s.str() << std::endl << std::flush;

  #endif
}
/*
TimenodeGraphComponents TimenodeGraph::components()
{
  ossia::int_vector component(boost::num_vertices(m_graph));
  int num = boost::connected_components(m_graph, &component[0]);

  std::vector<TimenodeGraphConnectedComponent> comps(num);
  for (auto vtx : m_vertices)
  {
    auto& comp = comps[component[vtx.second]];
    comp.syncs.push_back(vtx.first);
    for (auto& cst : Scenario::previousIntervals(*vtx.first, m_scenario))
    {
      comp.intervals.push_back(&m_scenario.interval(cst));
    }
    for (auto& cst : Scenario::nextIntervals(*vtx.first, m_scenario))
    {
      comp.intervals.push_back(&m_scenario.interval(cst));
    }
  }
  return {m_scenario, comps};
}
*/
void TimenodeGraph::intervalsChanged(const IntervalModel&)
{
  QTimer::singleShot(8, (QObject*)&m_scenario, [self = QPointer<TimenodeGraph>(this)] {
    if(self)
      self->recompute();
  });
}

void TimenodeGraph::timeSyncsChanged(const TimeSyncModel&)
{
  QTimer::singleShot(8, (QObject*)&m_scenario, [self = QPointer<TimenodeGraph>(this)] {
    if(self)
      self->recompute();
  });
}

bool TimenodeGraphComponents::isInMain(const EventModel& c) const
{
  return isInMain(Scenario::parentTimeSync(c, parentScenario(c)));
}
bool TimenodeGraphComponents::isInMain(const StateModel& c) const
{
  return isInMain(Scenario::parentTimeSync(c, parentScenario(c)));
}
bool TimenodeGraphComponents::isInMain(const TimeSyncModel& c) const
{
  auto rs = &scenario.startTimeSync();

  auto it = ossia::find_if(comps, [&](const auto& comp) {
    return ossia::contains(comp.syncs, rs) && ossia::contains(comp.syncs, &c);
  });
  return it != comps.end();
}

bool TimenodeGraphComponents::isInMain(const IntervalModel& c) const
{
  auto rs = &scenario.startTimeSync();

  auto it = ossia::find_if(comps, [&](const auto& comp) {
    return ossia::contains(comp.syncs, rs) && ossia::contains(comp.intervals, &c);
  });
  return it != comps.end();
}

const TimenodeGraphConnectedComponent&
TimenodeGraphComponents::component(const Scenario::TimeSyncModel& c) const
{
  auto it = ossia::find_if(
      comps, [&](const auto& comp) { return ossia::contains(comp.syncs, &c); });
  SCORE_ASSERT(it != comps.end());
  return *it;
}

bool TimenodeGraphConnectedComponent::isMain(const ProcessModel& root) const
{
  auto rs = &root.startTimeSync();
  return ossia::contains(syncs, rs);
}
}
