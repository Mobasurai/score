#pragma once
#include <Device/Address/AddressSettings.hpp>

#include <QWidget>

#include <score_lib_process_export.h>

#include <verdigris>

class QLineEdit;
namespace score
{
struct DocumentContext;
}
namespace State
{
class DestinationQualifierWidget;
}
namespace Device
{
class NodeBasedItemModel;
}
namespace Process
{
template <typename Parent_T>
class AddressAccessorLineEdit;
class SCORE_LIB_PROCESS_EXPORT AddressAccessorEditWidget final : public QWidget
{
  W_OBJECT(AddressAccessorEditWidget)
public:
  AddressAccessorEditWidget(const score::DocumentContext& ctx, QWidget* parent);

  void setAddress(const State::AddressAccessor& addr);
  void setFullAddress(Device::FullAddressAccessorSettings&& addr);

  const Device::FullAddressAccessorSettings& address() const;

  QString addressString() const;

  void dragEnterEvent(QDragEnterEvent* event) override;
  void dropEvent(QDropEvent*) override;

public:
  void addressChanged(const Device::FullAddressAccessorSettings& arg_1)
      E_SIGNAL(SCORE_LIB_PROCESS_EXPORT, addressChanged, arg_1)

private:
  void customContextMenuEvent(const QPoint& p);
  void startLearn();

  AddressAccessorLineEdit<AddressAccessorEditWidget>* m_lineEdit{};
  Device::FullAddressAccessorSettings m_address;
  Device::NodeBasedItemModel* m_model{};
  State::DestinationQualifierWidget* m_qualifiers{};
};
}
