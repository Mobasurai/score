#pragma once
#include <score/graphics/widgets/Constants.hpp>

#include <QGraphicsItem>
#include <QObject>

#include <score_lib_base_export.h>

#include <verdigris>

namespace score
{
class SCORE_LIB_BASE_EXPORT QGraphicsKnob
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(QGraphicsKnob)
  SCORE_GRAPHICS_ITEM_TYPE(70)
  friend struct DefaultControlImpl;
  friend struct DefaultGraphicsKnobImpl;

protected:
  double m_value{};
  double m_execValue{};
  QRectF m_rect{defaultKnobSize};

public:
  double min{}, max{}, init{};

private:
  bool m_grab{};
  bool m_hasExec{};

public:
  explicit QGraphicsKnob(QGraphicsItem* parent);
  ~QGraphicsKnob();

  double unmap(double v) const noexcept { return (v - min) / (max - min); }
  double map(double v) const noexcept { return (v * (max - min)) + min; }

  void setRect(const QRectF& r);
  void setRange(double min, double max, double init);
  void setValue(double v);
  double value() const;
  void setExecutionValue(double v);
  void resetExecution();

  QRectF boundingRect() const override;

  bool moving = false;

public:
  void sliderMoved() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderMoved)
  void sliderReleased() E_SIGNAL(SCORE_LIB_BASE_EXPORT, sliderReleased)

private:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
      override;
};
}
