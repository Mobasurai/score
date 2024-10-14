#pragma once
#include <Process/Style/ScenarioStyle.hpp>

#include <Vst/EffectModel.hpp>

#include <score/graphics/GraphicWidgets.hpp>
#include <score/graphics/TextItem.hpp>

#include <ossia/dataflow/safe_nodes/port.hpp>

#include <verdigris>

namespace Process
{
struct Context;
}
namespace vst
{

class EffectItem final : public score::EmptyRectItem
{
  QGraphicsItem* rootItem{};
  std::vector<std::pair<ControlInlet*, score::EmptyRectItem*>> controlItems;

public:
  EffectItem(const Model& effect, const Process::Context& doc, QGraphicsItem* root);

  void setupInlet(const Model& fx, ControlInlet& inlet, const Process::Context& doc);

private:
  void updateRect();
};

class GraphicsSlider final
    : public QObject
    , public score::QGraphicsSliderBase<GraphicsSlider>
{
  W_OBJECT(GraphicsSlider)
  Q_INTERFACES(QGraphicsItem)
  friend struct QGraphicsSliderBase<GraphicsSlider>;

  double m_value{};
  double m_execValue{};
  AEffect* fx{};
  int num{};

private:
  bool m_grab{};
  bool m_hasExec{};

public:
  static const constexpr double min = 0.;
  static const constexpr double max = 1.;
  friend struct score::DefaultControlImpl;
  friend struct score::DefaultGraphicsSliderImpl;
  GraphicsSlider(AEffect* fx, int num, QGraphicsItem* parent);

  static double from01(double v) { return v; }
  static double map(double v) { return v; }
  static double unmap(double v) { return v; }

  void setValue(double v);
  double value() const;
  void setExecutionValue(double v);
  void resetExecution();

  bool moving = false;

public:
  void valueChanged(double arg_1) W_SIGNAL(valueChanged, arg_1);
  void sliderMoved() W_SIGNAL(sliderMoved);
  void sliderReleased() W_SIGNAL(sliderReleased);

private:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
      override;
};

struct VSTFloatSlider : ossia::safe_nodes::control_in
{
  static QWidget* make_widget(
      AEffect* fx, const ControlInlet& inlet, const score::DocumentContext& ctx,
      QWidget* parent, QObject* context);
  static QGraphicsItem* make_item(
      AEffect* fx, ControlInlet& inlet, const score::DocumentContext& ctx,
      QGraphicsItem* parent, QObject* context);
};

}
