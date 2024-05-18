#pragma once

#include <Device/Address/AddressSettings.hpp>
#include <Device/Node/DeviceNode.hpp>
#include <Device/Protocol/DeviceSettings.hpp>

#include <QDialog>
#include <QWidget>

#include <score_lib_device_export.h>

#include <verdigris>

class QLineEdit;
class QComboBox;
class QSpinBox;
class QTextEdit;
class QCheckBox;
namespace Device
{
class SCORE_LIB_DEVICE_EXPORT ProtocolSettingsWidget : public QWidget
{
  W_OBJECT(ProtocolSettingsWidget)
public:
  using QWidget::QWidget;
  virtual ~ProtocolSettingsWidget();
  virtual Device::DeviceSettings getSettings() const = 0;
  virtual Device::Node getDevice() const;
  virtual void setSettings(const Device::DeviceSettings& settings) = 0;

  void changed() E_SIGNAL(SCORE_LIB_DEVICE_EXPORT, changed)

  void checkForChanges(QLineEdit*);
  void checkForChanges(QComboBox*);
  void checkForChanges(QSpinBox*);
  void checkForChanges(QTextEdit*);
  void checkForChanges(QCheckBox*);
};

class SCORE_LIB_DEVICE_EXPORT AddressDialog : public QDialog
{
public:
  using QDialog::QDialog;
  virtual ~AddressDialog();
  virtual Device::AddressSettings getSettings() const = 0;
};
}
