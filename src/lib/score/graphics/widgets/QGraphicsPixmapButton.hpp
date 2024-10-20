#pragma once
#include <score/graphics/widgets/Constants.hpp>
#include <score/widgets/Pixmap.hpp>

#include <QGraphicsItem>
#include <QObject>

#include <score_lib_base_export.h>

#include <verdigris>

namespace score
{
class SCORE_LIB_BASE_EXPORT QGraphicsPixmapButton final
    : public QObject
    , public QGraphicsPixmapItem
{
  W_OBJECT(QGraphicsPixmapButton)
  SCORE_GRAPHICS_ITEM_TYPE(130)
  const QPixmap m_pressed, m_released;

public:
  QGraphicsPixmapButton(QPixmap pressed, QPixmap released, QGraphicsItem* parent);

public:
  void clicked() E_SIGNAL(SCORE_LIB_BASE_EXPORT, clicked)

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
};
}
