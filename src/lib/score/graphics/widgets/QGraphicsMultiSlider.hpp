#pragma once
#include <score/graphics/widgets/Constants.hpp>

#include <ossia-qt/value_metatypes.hpp>

#include <QGraphicsItem>
#include <QObject>

#include <score_lib_base_export.h>

#include <verdigris>
namespace ossia
{
struct domain;
}
namespace score
{
template <typename T>
struct SliderWrapper;
struct RightClickImpl;
class SCORE_LIB_BASE_EXPORT QGraphicsMultiSlider final
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(QGraphicsMultiSlider)
  SCORE_GRAPHICS_ITEM_TYPE(110)
public:
  template <typename T>
  friend struct SliderWrapper;
  double min{0.}, max{1.};
  int m_grab{-1};
  ossia::value m_value{};
  ossia::value m_execValue{};
  bool m_hasExec{};
  bool moving = false;
  RightClickImpl* impl{};

  QGraphicsMultiSlider(QGraphicsItem* parent);

  void setPoint(const QPointF& r);
  void setValue(ossia::value v);
  ossia::value value() const;
  void setExecutionValue(double v);
  void resetExecution();

  void setRange(const ossia::value& min, const ossia::value& max);
  void setRange(const ossia::domain& dom);

public:
  void sliderMoved() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderMoved)
  void sliderReleased() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderReleased)

private:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  QRectF boundingRect() const override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
      override;
};
}
