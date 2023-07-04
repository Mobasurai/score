// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "score_plugin_engine.hpp"

#include <Device/Protocol/ProtocolFactoryInterface.hpp>

#include <Process/Execution/ProcessComponent.hpp>

#include <Engine/ApplicationPlugin.hpp>
#include <Engine/Listening/PlayListeningHandlerFactory.hpp>
#include <Execution/Clock/ClockFactory.hpp>
#include <Execution/Clock/DataflowClock.hpp>
#include <Execution/Clock/DefaultClock.hpp>
#include <Execution/Clock/ManualClock.hpp>
#include <Execution/DocumentPlugin.hpp>
#include <Execution/Settings/ExecutorFactory.hpp>
#include <Execution/Transport/JackTransport.hpp>
#include <LocalTree/Device/LocalProtocolFactory.hpp>

#include <score/plugins/FactorySetup.hpp>
#include <score/plugins/InterfaceList.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/plugins/UuidKeySerialization.hpp>

#include <ossia/editor/scenario/time_event.hpp>
#include <ossia/network/base/device.hpp>

#include <Transport/TransportInterface.hpp>

#include <score_plugin_deviceexplorer.hpp>
#include <score_plugin_scenario.hpp>
#include <wobjectimpl.h>

#include <ossia-config.hpp>
W_OBJECT_IMPL(Execution::ManualClock::TimeWidget)

score_plugin_engine::score_plugin_engine()
{
  qRegisterMetaType<ossia::bench_map>("BenchMap");
  qRegisterMetaType<Execution::ClockFactory::ConcreteKey>(
      "Execution::ClockFactory::ConcreteKey");
}

score_plugin_engine::~score_plugin_engine() { }

score::GUIApplicationPlugin*
score_plugin_engine::make_guiApplicationPlugin(const score::GUIApplicationContext& app)
{
  return new Engine::ApplicationPlugin{app};
}

std::vector<std::unique_ptr<score::InterfaceListBase>>
score_plugin_engine::factoryFamilies()
{
  return make_ptr_vector<
      score::InterfaceListBase, Execution::ProcessComponentFactoryList,
      Execution::ClockFactoryList>();
}

std::vector<std::unique_ptr<score::InterfaceBase>> score_plugin_engine::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  using namespace Scenario;
  using namespace Engine;
  using namespace Execution;
  return instantiate_factories<
      score::ApplicationContext,
      FW<Device::ProtocolFactory, Protocols::LocalProtocolFactory>,
      FW<Explorer::ListeningHandlerFactory, Execution::PlayListeningHandlerFactory>,
      FW<score::SettingsDelegateFactory, Execution::Settings::Factory>,
#if defined(OSSIA_AUDIO_JACK)
      FW<Execution::TransportInterface, Execution::JackTransport>,
#endif
      FW<Execution::ClockFactory
         // , Execution::ControlClockFactory
         ,
         Dataflow::ClockFactory
         //, ManualClock::ClockFactory
         >>(ctx, key);
}

auto score_plugin_engine::required() const -> std::vector<score::PluginKey>
{
  return {
      score_plugin_scenario::static_key(), score_plugin_deviceexplorer::static_key()};
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_plugin_engine)
