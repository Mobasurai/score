#pragma once
#include <Process/Inspector/ProcessInspectorWidgetDelegate.hpp>

#include <Automation/AutomationModel.hpp>

#include <score/command/Dispatchers/CommandDispatcher.hpp>

#include <Color/GradientModel.hpp>

class QLabel;
class QWidget;
class QCheckBox;

namespace score
{
struct DocumentContext;
}
namespace State
{
struct Address;
class UnitWidget;
}
namespace Device
{
struct FullAddressAccessorSettings;
}
namespace Process
{
class AddressAccessorEditWidget;
}
namespace Explorer
{
class DeviceExplorerModel;
}
class QDoubleSpinBox;

namespace Automation
{
class ProcessModel;
class InspectorWidget final
    : public Process::InspectorWidgetDelegate_T<Automation::ProcessModel>
{
public:
  explicit InspectorWidget(
      const ProcessModel& object, const score::DocumentContext& context,
      QWidget* parent);

private:
  void on_addressChange(const Device::FullAddressAccessorSettings& newText);
  void on_minValueChanged();
  void on_maxValueChanged();
  void on_tweenChanged();

  Process::AddressAccessorEditWidget* m_lineEdit{};
  QCheckBox* m_tween{};
  QDoubleSpinBox *m_minsb{}, *m_maxsb{};

  CommandDispatcher<> m_dispatcher;
};
}

namespace Gradient
{
class ProcessModel;
class InspectorWidget final
    : public Process::InspectorWidgetDelegate_T<Gradient::ProcessModel>
{
public:
  explicit InspectorWidget(
      const ProcessModel& object, const score::DocumentContext& context,
      QWidget* parent);

private:
  void on_tweenChanged();

  QCheckBox* m_tween{};

  CommandDispatcher<> m_dispatcher;
};
}
