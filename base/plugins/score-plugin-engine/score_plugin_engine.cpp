// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "score_plugin_engine.hpp"

#include <ossia/network/base/device.hpp>

#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Engine/ApplicationPlugin.hpp>
#include <Execution/Automation/Component.hpp>
#include <Execution/Automation/GradientComponent.hpp>
#include <Execution/Automation/MetronomeComponent.hpp>
#include <Execution/Automation/SplineComponent.hpp>
#include <Execution/ClockManager/ClockManagerFactory.hpp>
#include <Execution/ClockManager/DefaultClockManager.hpp>
#include <Execution/DocumentPlugin.hpp>
#include <Execution/Loop/Component.hpp>
#include <Execution/Mapping/Component.hpp>
#include <Execution/ProcessComponent.hpp>
#include <Execution/ScenarioComponent.hpp>
#include <Execution/Settings/ExecutorFactory.hpp>
#include <Engine/Listening/PlayListeningHandlerFactory.hpp>
#include <LocalTree/Scenario/AutomationComponent.hpp>
#include <LocalTree/Scenario/LoopComponent.hpp>
#include <LocalTree/Scenario/MappingComponent.hpp>
#include <LocalTree/Scenario/ScenarioComponent.hpp>
#include <Protocols/Local/LocalProtocolFactory.hpp>
#include <Audio/Settings/Factory.hpp>
#include <Audio/AudioPanel.hpp>
#include <QString>
#include <ossia-config.hpp>
#include <score/plugins/customfactory/FactoryFamily.hpp>
#include <score/plugins/customfactory/FactorySetup.hpp>
#include <score/plugins/customfactory/StringFactoryKey.hpp>
#if defined(OSSIA_PROTOCOL_MINUIT)
#  include <Protocols/Minuit/MinuitProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_OSC)
#  include <Protocols/OSC/OSCProtocolFactory.hpp>
#endif

#if defined(OSSIA_PROTOCOL_OSCQUERY)
#  include <Protocols/OSCQuery/OSCQueryProtocolFactory.hpp>
#endif

#if defined(OSSIA_PROTOCOL_MIDI)
#  include <Protocols/MIDI/MIDIProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_HTTP)
#  include <Protocols/HTTP/HTTPProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_WEBSOCKETS)
#  include <Protocols/WS/WSProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_SERIAL)
#  include <Protocols/Serial/SerialProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_PHIDGETS)
#  include <Protocols/Phidgets/PhidgetsProtocolFactory.hpp>
#endif
#if defined(OSSIA_PROTOCOL_JOYSTICK)
#  include <Protocols/Joystick/JoystickProtocolFactory.hpp>
#endif

#include <Execution/Dataflow/DataflowClock.hpp>
#include <Protocols/Audio/AudioDevice.hpp>
#include <Execution/Dataflow/ManualClock.hpp>
#include <score_plugin_scenario.hpp>
#include <score_plugin_deviceexplorer.hpp>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Execution::ManualClock::TimeWidget)

score_plugin_engine::score_plugin_engine()
{
  qRegisterMetaType<Execution::ClockManagerFactory::ConcreteKey>(
      "ClockManagerKey");
  qRegisterMetaTypeStreamOperators<
      Execution::ClockManagerFactory::ConcreteKey>("ClockManagerKey");

  qRegisterMetaType<std::shared_ptr<Execution::ProcessComponent>>();
  qRegisterMetaType<std::shared_ptr<Execution::EventComponent>>();
  qRegisterMetaType<ossia::time_event::status>();
  qRegisterMetaType<ossia::time_value>();
}

score_plugin_engine::~score_plugin_engine()
{
}

score::GUIApplicationPlugin* score_plugin_engine::make_guiApplicationPlugin(
    const score::GUIApplicationContext& app)
{
  return new Engine::ApplicationPlugin{app};
}

std::vector<std::unique_ptr<score::InterfaceListBase>>
score_plugin_engine::factoryFamilies()
{
  return make_ptr_vector<
      score::InterfaceListBase, Engine::LocalTree::ProcessComponentFactoryList,
      Execution::ProcessComponentFactoryList,
      Execution::ClockManagerFactoryList>();
}

std::vector<std::unique_ptr<score::InterfaceBase>>
score_plugin_engine::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  using namespace Scenario;
  using namespace Engine;
  using namespace Execution;
  return instantiate_factories<
      score::ApplicationContext,
      FW<Device::ProtocolFactory, Network::LocalProtocolFactory

#if defined(OSSIA_PROTOCOL_OSC)
         ,
         Network::OSCProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_MINUIT)
         ,
         Network::MinuitProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_OSCQUERY)
         ,
         Network::OSCQueryProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_MIDI)
         ,
         Network::MIDIProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_HTTP)
         ,
         Network::HTTPProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_WEBSOCKETS)
         ,
         Network::WSProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_SERIAL)
         ,
         Network::SerialProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_PHIDGETS)
         ,
         Network::PhidgetProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_AUDIO)
         ,
         Dataflow::AudioProtocolFactory
#endif
#if defined(OSSIA_PROTOCOL_JOYSTICK)
        ,
        Network::JoystickProtocolFactory
#endif
         >,
      FW<Execution::ProcessComponentFactory,
         Execution::ScenarioComponentFactory
         //, Interpolation::Executor::ComponentFactory
         ,
         Automation::RecreateOnPlay::ComponentFactory,
         Mapping::RecreateOnPlay::ComponentFactory,
         Loop::RecreateOnPlay::ComponentFactory,
         Gradient::RecreateOnPlay::ComponentFactory,
         Spline::RecreateOnPlay::ComponentFactory,
         Metronome::RecreateOnPlay::ComponentFactory>,
      FW<Explorer::ListeningHandlerFactory,
         Execution::PlayListeningHandlerFactory>,
      FW<score::SettingsDelegateFactory, Execution::Settings::Factory,
         Audio::Settings::Factory>,
      FW<Engine::LocalTree::ProcessComponentFactory,
         Engine::LocalTree::ScenarioComponentFactory,
         Engine::LocalTree::LoopComponentFactory,
         Engine::LocalTree::AutomationComponentFactory,
         Engine::LocalTree::MappingComponentFactory>,
      FW<score::PanelDelegateFactory,
         Audio::PanelDelegateFactory>,
      FW<Execution::ClockManagerFactory
         // , Execution::ControlClockFactory
         , Dataflow::ClockFactory
         // , Engine::ManualClock::ClockFactory
      >>(ctx, key);
}

auto score_plugin_engine::required() const -> std::vector<score::PluginKey>
{
  return {score_plugin_scenario::static_key(),
          score_plugin_deviceexplorer::static_key()};
}

#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_plugin_engine)
