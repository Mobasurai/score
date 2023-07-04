#pragma once
#include <score/application/ApplicationContext.hpp>
#include <score/command/Command.hpp>
#include <score/command/CommandGeneratorMap.hpp>
#include <score/plugins/Interface.hpp>
#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <score/plugins/qt_interfaces/CommandFactory_QtInterface.hpp>
#include <score/plugins/qt_interfaces/FactoryFamily_QtInterface.hpp>
#include <score/plugins/qt_interfaces/FactoryInterface_QtInterface.hpp>
#include <score/plugins/qt_interfaces/GUIApplicationPlugin_QtInterface.hpp>
#include <score/plugins/qt_interfaces/PluginRequirements_QtInterface.hpp>

#include <utility>
#include <vector>
#include <verdigris>
/*!
 * \namespace Scenario
 * \brief Main plug-in of score.
 *
 * This plug-in introduces the central logic of score :
 * a hierarchical model of objects in a timeline.
 *
 * It also has the core graphics logic of score based on QGraphicsScene
 * and QGraphicsView.
 *
 * As such, it is quite complex and provides a lot of classes.
 *
 * We have :
 * * The Scenario::ProcessModel process.
 * * The Scenario::PanelDelegate used to display elements.
 * * The Scenario::ScenarioDocumentModel, Scenario::ScenarioDocumentPresenter,
 * Scenario::ScenarioDocumentView classes which set-up and displays the central
 * part of an score document.
 * * The Scenario::ScenarioApplicationPlugin which handles tools, Action%s, and
 * process focus.
 * * Model-view-presenter classes for the major score concepts :
 *   * Scenario::IntervalModel,
 *   * Scenario::EventModel,
 *   * Scenario::TimeSyncModel,
 *   * Scenario::StateModel,
 *   * Scenario::RackModel,
 *   * Scenario::SlotModel
 *
 * * Scenario::BaseScenario is a minimalist, fixed scenario with a single
 * interval, a start state and an end state.
 * * Inspector elements for all these objects
 * * Scenario::Palette handles user input, movements, etc.
 *
 */
namespace score
{

class DocumentDelegateFactory;
class InterfaceListBase;
class PanelFactory;
} // namespace score

class score_plugin_scenario final
    : public score::ApplicationPlugin_QtInterface
    , public score::CommandFactory_QtInterface
    , public score::FactoryList_QtInterface
    , public score::FactoryInterface_QtInterface
    , public score::Plugin_QtInterface
{
  SCORE_PLUGIN_METADATA(1, "8439ef6c-90c3-4e08-8185-6a0f3c87f8b4")
public:
  score_plugin_scenario();
  ~score_plugin_scenario() override;

private:
  score::GUIApplicationPlugin*
  make_guiApplicationPlugin(const score::GUIApplicationContext& app) override;

  std::pair<const CommandGroupKey, CommandGeneratorMap> make_commands() override;

  std::vector<std::unique_ptr<score::InterfaceListBase>> factoryFamilies() override;

  std::vector<score::InterfaceBase*> factories(
      const score::ApplicationContext&,
      const score::InterfaceKey& factoryName) const override;
  std::vector<score::InterfaceBase*> guiFactories(
      const score::GUIApplicationContext&,
      const score::InterfaceKey& factoryName) const override;

  std::vector<score::PluginKey> required() const override;
};
