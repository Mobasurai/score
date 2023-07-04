// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "score_plugin_automation.hpp"

#include <Process/GenericProcessFactory.hpp>
#include <Process/HeaderDelegate.hpp>
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>
#include <Process/ProcessFactory.hpp>
#include <Process/Style/ScenarioStyle.hpp>

#include <Curve/Process/CurveProcessFactory.hpp>

#include <Automation/AutomationColors.hpp>
#include <Automation/AutomationExecution.hpp>
#include <Automation/AutomationModel.hpp>
#include <Automation/AutomationPresenter.hpp>
#include <Automation/AutomationProcessMetadata.hpp>
#include <Automation/AutomationView.hpp>
#include <Automation/Commands/AutomationCommandFactory.hpp>
#include <Automation/Inspector/AutomationInspectorFactory.hpp>
#include <Automation/Inspector/AutomationStateInspectorFactory.hpp>
#include <Automation/Inspector/CurvePointInspectorFactory.hpp>
#include <Automation/LocalTree.hpp>
#include <Inspector/InspectorWidgetFactoryInterface.hpp>

#include <score/plugins/FactorySetup.hpp>
#include <score/plugins/StringFactoryKey.hpp>
#include <score/tools/std/HashMap.hpp>

#include <Color/GradientExecution.hpp>
#include <Color/GradientModel.hpp>
#include <Color/GradientPresenter.hpp>
#include <Color/GradientView.hpp>

#include <score_plugin_automation_commands_files.hpp>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Automation::LayerPresenter)
namespace Automation
{
template <typename Layer_T>
class MinMaxHeaderDelegate final : public Process::DefaultHeaderDelegate
{
public:
  using model_t = std::remove_reference_t<decltype(std::declval<Layer_T>().model())>;
  MinMaxHeaderDelegate(const Process::ProcessModel& m, const Process::Context& doc)
      : Process::DefaultHeaderDelegate{m, doc}
  {
    const auto& model = static_cast<model_t&>(m_model);

    con(model, &model_t::minChanged, this, [=] { updateText(); });
    con(model, &model_t::maxChanged, this, [=] { updateText(); });
  }

  void updateText() override
  {
    auto& style = Process::Style::instance();
    const auto& model = static_cast<model_t&>(m_model);

    const QPen& pen = m_sel ? style.IntervalHeaderTextPen() : textPen(style, model);

    QString txt = model.prettyName();
    txt += "  Min: ";
    txt += QString::number(model.min());
    txt += "  Max: ";
    txt += QString::number(model.max());
    m_line = Process::makeGlyphs(txt, pen);
    update();
    updatePorts();
  }
};
using AutomationFactory = Process::ProcessFactory_T<Automation::ProcessModel>;
using AutomationLayerFactory = Curve::CurveLayerFactory_T<
    Automation::ProcessModel, Automation::LayerPresenter, Automation::LayerView,
    Automation::Colors, Automation::MinMaxHeaderDelegate<Automation::LayerPresenter>>;
}
namespace Gradient
{
using GradientFactory = Process::ProcessFactory_T<Gradient::ProcessModel>;
using GradientLayerFactory = Process::LayerFactory_T<
    Gradient::ProcessModel, Gradient::Presenter, Gradient::View>;
}

score_plugin_automation::score_plugin_automation() = default;
score_plugin_automation::~score_plugin_automation() = default;

std::vector<score::InterfaceBase*> score_plugin_automation::factories(
    const score::ApplicationContext& ctx, const score::InterfaceKey& key) const
{
  return instantiate_factories<
      score::ApplicationContext,
      FW<Process::ProcessModelFactory, Automation::AutomationFactory,
         Gradient::GradientFactory>,
      FW<Process::LayerFactory, Automation::AutomationLayerFactory,
         Gradient::GradientLayerFactory>,
      FW<Inspector::InspectorWidgetFactory, Automation::StateInspectorFactory,
         Automation::PointInspectorFactory, Automation::InspectorFactory,
         Gradient::InspectorFactory>,

      FW<LocalTree::ProcessComponentFactory, LocalTree::AutomationComponentFactory>,

      FW<Execution::ProcessComponentFactory,
         //, Interpolation::Executor::ComponentFactory,

         Automation::RecreateOnPlay::ComponentFactory,
         Gradient::RecreateOnPlay::ComponentFactory>>(ctx, key);
}

std::pair<const CommandGroupKey, CommandGeneratorMap>
score_plugin_automation::make_commands()
{
  using namespace Automation;
  using namespace Gradient;
  std::pair<const CommandGroupKey, CommandGeneratorMap> cmds{
      CommandFactoryName(), CommandGeneratorMap{}};

  ossia::for_each_type<
#include <score_plugin_automation_commands.hpp>
      >(score::commands::FactoryInserter{cmds.second});

  return cmds;
}
#include <score/plugins/PluginInstances.hpp>
SCORE_EXPORT_PLUGIN(score_plugin_automation)
