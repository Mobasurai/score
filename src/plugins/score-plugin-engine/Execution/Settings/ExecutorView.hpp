#pragma once
#include <Execution/Settings/ExecutorModel.hpp>

#include <score/plugins/settingsdelegate/SettingsDelegateView.hpp>

#include <verdigris>

namespace score
{
class FormWidget;
}

namespace Execution
{
namespace Settings
{
class View : public score::GlobalSettingsView
{
  W_OBJECT(View)
public:
  View();

  SETTINGS_UI_COMBOBOX_HPP(Scheduling)
  SETTINGS_UI_COMBOBOX_HPP(Ordering)
  SETTINGS_UI_COMBOBOX_HPP(Merging)
  SETTINGS_UI_COMBOBOX_HPP(Commit)
  SETTINGS_UI_COMBOBOX_HPP(Tick)

  SETTINGS_UI_TOGGLE_HPP(Logging)
  SETTINGS_UI_TOGGLE_HPP(Bench)
  SETTINGS_UI_TOGGLE_HPP(Parallel)
  SETTINGS_UI_SPINBOX_HPP(Threads)
  SETTINGS_UI_TOGGLE_HPP(ExecutionListening)
  SETTINGS_UI_TOGGLE_HPP(ScoreOrder)
  SETTINGS_UI_TOGGLE_HPP(ValueCompilation)
  SETTINGS_UI_TOGGLE_HPP(TransportValueCompilation)

private:
  QWidget* getWidget() override;
  score::FormWidget* m_widg{};
};
}
}
