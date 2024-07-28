#include <score/graphics/DefaultGraphicsSliderImpl.hpp>
#include <score/graphics/GraphicsSliderBaseImpl.hpp>
#include <score/graphics/widgets/QGraphicsLogSlider.hpp>
#include <score/model/Skin.hpp>
#include <score/serialization/StringConstants.hpp>

#include <ossia/detail/math.hpp>

#include <QGraphicsSceneMouseEvent>

#include <wobjectimpl.h>
W_OBJECT_IMPL(score::QGraphicsLogSlider);

namespace score
{
template void QGraphicsSliderBase<QGraphicsLogSlider>::setRect(const QRectF& r);

QGraphicsLogSlider::QGraphicsLogSlider(QGraphicsItem* parent)
    : QGraphicsSliderBase{parent}
{
  auto& skin = score::Skin::instance();
  setCursor(skin.CursorPointingHand);
}

QGraphicsLogSlider::~QGraphicsLogSlider()
{
  if(m_grab)
    sliderReleased();
}

void QGraphicsLogSlider::setRange(double min, double max)
{
  this->min = min;
  this->max = max;
  update();
}

void QGraphicsLogSlider::setValue(double v)
{
  m_value = ossia::clamp(v, 0., 1.);
  update();
}

void QGraphicsLogSlider::setExecutionValue(double v)
{
  m_execValue = ossia::clamp(v, 0., 1.);
  m_hasExec = true;
  update();
}

void QGraphicsLogSlider::resetExecution()
{
  m_hasExec = false;
  update();
}

double QGraphicsLogSlider::from01(double v) const noexcept
{
  return v;
}

double QGraphicsLogSlider::value() const
{
  return m_value;
}

double QGraphicsLogSlider::map(double v) const noexcept
{
  return ossia::normalized_to_log(min, max - min, v);
}

double QGraphicsLogSlider::unmap(double v) const noexcept
{
  return ossia::log_to_normalized(min, max - min, v);
}

void QGraphicsLogSlider::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSliderImpl::mousePressEvent(*this, event);
}

void QGraphicsLogSlider::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSliderImpl::mouseMoveEvent(*this, event);
}

void QGraphicsLogSlider::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSliderImpl::mouseReleaseEvent(*this, event);
}

void QGraphicsLogSlider::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  event->accept();
}

void QGraphicsLogSlider::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSliderImpl::mouseDoubleClickEvent(*this, event);
}

void QGraphicsLogSlider::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  DefaultGraphicsSliderImpl::paint(
      *this, score::Skin::instance(),
      score::toNumber(ossia::normalized_to_log(min, max - min, value())), painter,
      widget);
}
}
