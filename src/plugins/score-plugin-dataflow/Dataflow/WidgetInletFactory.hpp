#pragma once
#include <Process/Dataflow/PortListWidget.hpp>

#include <Dataflow/ControlInletItem.hpp>
#include <Dataflow/ControlOutletItem.hpp>
#include <Dataflow/PortItem.hpp>

namespace Dataflow
{
SCORE_PLUGIN_DATAFLOW_EXPORT
QWidget* makeGraphicsViewForInspectorItem(QGraphicsItem* item, QWidget* parent);

template <typename T, typename Widget>
struct WidgetInletFactory : public Dataflow::ControlInletFactory
{
  using Model_T = T;
  UuidKey<Process::Port> concreteKey() const noexcept override
  {
    return Model_T::static_concreteKey();
  }

  Model_T* load(const VisitorVariant& vis, QObject* parent) override
  {
    return score::deserialize_dyn(vis, [&](auto&& deserializer) {
      return new Model_T{deserializer, parent};
    });
  }

  void setupInletInspector(
      const Process::Inlet& port, const score::DocumentContext& ctx, QWidget* parent,
      Inspector::Layout& lay, QObject* context) override
  {
    auto& ctrl = const_cast<Model_T&>(static_cast<const Model_T&>(port));
    auto widg = Widget::make_widget(ctrl, ctrl, ctx, parent, parent);
    if(widg)
    {
      Process::PortWidgetSetup::setupControl(ctrl, widg, ctx, lay, parent);
    }
    else
    {
      auto item = makeControlItem(ctrl, ctx, nullptr, context);
      auto widg = makeGraphicsViewForInspectorItem(item, parent);
      Process::PortWidgetSetup::setupControl(ctrl, widg, ctx, lay, parent);
    }
  }

  QGraphicsItem* makeControlItem(
      Process::ControlInlet& port, const score::DocumentContext& ctx,
      QGraphicsItem* parent, QObject* context) override
  {
    auto& ctrl = static_cast<Model_T&>(port);
    return Widget::make_item(ctrl, ctrl, ctx, nullptr, context);
  }

  Process::PortItemLayout defaultLayout() const noexcept override
  {
    return Widget::layout();
  }
};

template <typename T, typename Widget>
struct WidgetOutletFactory : public Dataflow::ControlOutletFactory
{
  using Model_T = T;
  UuidKey<Process::Port> concreteKey() const noexcept override
  {
    return Metadata<ConcreteKey_k, Model_T>::get();
  }

  Model_T* load(const VisitorVariant& vis, QObject* parent) override
  {
    return score::deserialize_dyn(vis, [&](auto&& deserializer) {
      return new Model_T{deserializer, parent};
    });
  }

  void setupOutletInspector(
      const Process::Outlet& port, const score::DocumentContext& ctx, QWidget* parent,
      Inspector::Layout& lay, QObject* context) override
  {
    auto& ctrl = static_cast<const Model_T&>(port);
    auto widg = Widget::make_widget(ctrl, ctrl, ctx, parent, parent);
    Process::PortWidgetSetup::setupControl(ctrl, widg, ctx, lay, parent);
  }

  QGraphicsItem* makeControlItem(
      Process::ControlOutlet& port, const score::DocumentContext& ctx,
      QGraphicsItem* parent, QObject* context) override
  {
    auto& ctrl = static_cast<Model_T&>(port);
    return Widget::make_item(ctrl, ctrl, ctx, nullptr, context);
  }

  Process::PortItemLayout defaultLayout() const noexcept override
  {
    return Widget::layout();
  }
};
}
