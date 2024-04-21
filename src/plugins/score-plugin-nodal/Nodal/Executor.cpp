#include "Executor.hpp"

#include <Process/Execution/ProcessComponent.hpp>
#include <Process/ExecutionContext.hpp>
#include <Process/ExecutionSetup.hpp>
#include <Process/ExecutionTransaction.hpp>

#include <Scenario/Document/Interval/IntervalExecutionHelpers.hpp>

#include <Nodal/Process.hpp>

#include <score/application/ApplicationContext.hpp>
#include <score/document/DocumentContext.hpp>
#include <score/tools/Bind.hpp>

#include <ossia/dataflow/node_chain_process.hpp>
#include <ossia/dataflow/nodes/forward_node.hpp>
#include <ossia/dataflow/port.hpp>
#include <ossia/detail/flat_set.hpp>
namespace ossia
{

struct node_graph_process final : public looping_process<node_graph_process>
{
  node_graph_process() { node = std::make_shared<ossia::nodes::forward_node>(); }

  void state_impl(const ossia::token_request& req)
  {
    for(auto& process : processes)
    {
      process->state(req);
    }
    node->requested_tokens.push_back(req);
  }

  void add_process(
      std::shared_ptr<ossia::time_process>&& p, std::shared_ptr<ossia::graph_node>&& n)
  {
    nodes.insert(std::move(n));
    processes.insert(std::move(p));
  }

  void remove_process(
      const std::shared_ptr<ossia::time_process>& p,
      const std::shared_ptr<ossia::graph_node>& n)
  {
    nodes.erase(n);
    processes.erase(p);
  }
  void start() override
  {
    for(auto& process : processes)
    {
      process->start();
    }
  }

  void stop() override
  {
    for(auto& process : processes)
    {
      process->stop();
    }
    for(auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void pause() override
  {
    for(auto& process : processes)
    {
      process->pause();
    }
  }

  void resume() override
  {
    for(auto& process : processes)
    {
      process->resume();
    }
  }

  void offset_impl(time_value date) override
  {
    for(auto& process : processes)
    {
      process->offset(date);
    }
    for(auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void transport_impl(ossia::time_value date) override
  {
    for(auto& process : processes)
    {
      process->transport(date);
    }
    for(auto& node : nodes)
    {
      node->all_notes_off();
    }
  }

  void mute_impl(bool b) override
  {
    node->set_mute(b);
    for(auto& node : nodes)
      node->set_mute(b);
  }
  ossia::flat_set<std::shared_ptr<ossia::graph_node>> nodes;
  ossia::flat_set<std::shared_ptr<ossia::time_process>> processes;
};
}
namespace Nodal
{

NodalExecutorBase::NodalExecutorBase(
    Nodal::Model& element, const Execution::Context& ctx, QObject* parent)
    : ProcessComponent_T{element, ctx, "NodalExecutorComponent", parent}
{
  // TODO load node
  m_ossia_process = std::make_shared<ossia::node_graph_process>();
  m_ossia_process->node->prepare(*ctx.execState);
  this->node = m_ossia_process->node;
}

NodalExecutorBase::~NodalExecutorBase() { }

struct AddNode
{
  std::weak_ptr<ossia::graph_node> fw_node;
  std::weak_ptr<ossia::graph_node> process_node;
  std::weak_ptr<ossia::graph_interface> g_weak;
  ossia::pod_vector<std::size_t> propagated_outlets;

  void operator()() const noexcept
  {
    auto fw = fw_node.lock();
    if(!fw)
      return;

    auto oproc = process_node.lock();
    if(!oproc)
      return;

    auto g = g_weak.lock();
    if(!g)
      return;

    Execution::connectPropagated(oproc, fw, *g, propagated_outlets);
  }
};

void NodalExecutorBase::unreg(const RegisteredNode& fx, Execution::Transaction& commands)
{
  system().setup.unregister_node_soft(
      fx.comp->process().inlets(), fx.comp->process().outlets(), fx.comp->node,
      commands);
}

void NodalExecutorBase::reg(const RegisteredNode& fx, Execution::Transaction& vec)
{
  auto& proc = fx.comp->process();
  system().setup.register_node(proc.inlets(), proc.outlets(), fx.comp->node, vec);

  auto reconnectOutlets = Execution::ReconnectOutlets<NodalExecutorBase>{
      *this, this->node, proc, fx.comp->OSSIAProcessPtr(), system().execGraph};

  connect(&proc, &Process::ProcessModel::outletsChanged, this, reconnectOutlets);
  reconnectOutlets();

  vec.push_back(AddNode{
      this->node, fx.comp->node, system().execGraph,
      Execution::propagatedOutlets(proc.outlets())});
}

Execution::ProcessComponent* NodalExecutorBase::make(
    Execution::ProcessComponentFactory& factory, Process::ProcessModel& proc)
{
  Execution::Transaction commands{system()};
  auto comp = factory.make(proc, this->system(), this);
  if(comp)
  {
    reg(m_nodes[proc.id()] = {comp}, commands);
    auto child_n = comp->node;
    auto child_p = comp->OSSIAProcessPtr();
    if(child_n && child_p)
    {
      // FIXME refactor with IntervalComponentBase to not duplicate
      auto& oproc = child_p;

      // Selection
      QObject::connect(
          &proc.selection, &Selectable::changed, comp.get(),
          [this, n = oproc->node](bool ok) {
        in_exec([=] {
          if(n)
            n->set_logging(ok);
        });
          });
      // Looping
      oproc->set_loops(proc.loops());
      con(proc, &Process::ProcessModel::loopsChanged, this,
          [this, p = oproc](bool b) { in_exec([=] { p->set_loops(b); }); });

      oproc->set_loop_duration(system().time(proc.loopDuration()));
      con(proc, &Process::ProcessModel::loopDurationChanged, this,
          [this, p = oproc](TimeVal t) {
        in_exec([p, t = system().time(t)] { p->set_loop_duration(t); });
      });

      oproc->set_start_offset(system().time(proc.startOffset()));
      con(proc, &Process::ProcessModel::startOffsetChanged, this,
          [this, p = oproc](TimeVal t) {
        in_exec([p, t = system().time(t)] { p->set_start_offset(t); });
      });

      auto p = std::dynamic_pointer_cast<ossia::node_graph_process>(m_ossia_process);
      commands.push_back([child_n = std::move(child_n), child_p = std::move(child_p),
                          p = std::move(p)]() mutable {
        p->add_process(std::move(child_p), std::move(child_n));
      });
    }

    commands.run_all();
  }

  return comp.get();
}

void NodalExecutorBase::added(::Execution::ProcessComponent& e) { }

std::function<void()> NodalExecutorBase::removing(
    const Process::ProcessModel& e, ::Execution::ProcessComponent& c)
{
  Execution::Transaction commands{system()};

  auto it = ossia::find_if(m_nodes, [&](const auto& v) { return v.first == e.id(); });
  if(it == m_nodes.end())
    return {};

  auto& this_fx = it->second;

  unreg(this_fx, commands);

  auto p = std::dynamic_pointer_cast<ossia::node_graph_process>(m_ossia_process);
  auto child_p = c.OSSIAProcessPtr();
  auto child_n = c.node;
  commands.push_back([child_n = std::move(child_n), child_p = std::move(child_p),
                      p = std::move(p)] { p->remove_process(child_p, child_n); });

  commands.run_all();

  c.node.reset();
  return {};
}

void NodalExecutor::cleanup()
{
  clear();
  ::Execution::ProcessComponent::cleanup();
}

NodalExecutor::~NodalExecutor() { }

}
