#pragma once

#include <Process/TimeValue.hpp>

#include <Execution/ContextMenu/PlayContextMenu.hpp>
#include <Execution/ExecutionController.hpp>

#include <score/plugins/application/GUIApplicationPlugin.hpp>
#include <score/plugins/documentdelegate/plugin/DocumentPlugin.hpp>

#include <QTimer>

#include <score_plugin_engine_export.h>

#include <memory>
namespace Scenario
{
class SpeedWidget;
}
namespace Scenario
{
class IntervalModel;
}
namespace Execution
{
struct Context;
class Clock;
class BaseScenarioElement;
}

namespace LocalTree
{
class DocumentPlugin;
}

namespace ossia
{
class audio_engine;
}

class QLabel;
namespace Engine
{

class SCORE_PLUGIN_ENGINE_EXPORT ApplicationPlugin final
    : public QObject
    , public score::GUIApplicationPlugin
{
public:
  explicit ApplicationPlugin(const score::GUIApplicationContext& app);
  ~ApplicationPlugin() override;

  void initialize() override;

  void afterStartup() override;
  score::GUIElements makeGUIElements() override;

  void prepareNewDocument() override;
  void on_initDocument(score::Document& doc) override;
  void on_createdDocument(score::Document& doc) override;
  void on_documentChanged(score::Document* olddoc, score::Document* newdoc) override;

  QWidget* setupTimingWidget(QLabel*);
  void initLocalTreeNodes(LocalTree::DocumentPlugin&);

  Execution::ExecutionController& execution() { return m_execution; }

  QTimer execution_ui_clock_timer{};

private:
  Execution::PlayContextMenu m_playActions;
  Execution::ExecutionController m_execution;

  Scenario::SpeedWidget* m_speedSlider{};
  QAction* m_musicalAct{};
};
}
