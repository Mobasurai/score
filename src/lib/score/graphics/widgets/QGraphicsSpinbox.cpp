#include <score/graphics/DefaultGraphicsSpinboxImpl.hpp>
#include <score/graphics/InfiniteScroller.hpp>
#include <score/graphics/widgets/QGraphicsSpinbox.hpp>
#include <score/model/Skin.hpp>
#include <score/serialization/StringConstants.hpp>
#include <score/tools/Debug.hpp>

#include <ossia/detail/math.hpp>

#include <wobjectimpl.h>
W_OBJECT_IMPL(score::QGraphicsSpinbox);
W_OBJECT_IMPL(score::QGraphicsIntSpinbox);

namespace score
{

QGraphicsSpinbox::QGraphicsSpinbox(QGraphicsItem* parent)
    : QGraphicsItem{parent}
{
  auto& skin = score::Skin::instance();
  setCursor(skin.CursorPointingHand);
  min = -100;
  max = 100;
}

QGraphicsSpinbox::~QGraphicsSpinbox() = default;

void QGraphicsSpinbox::setValue(float v)
{
  m_value = ossia::clamp(v, 0., 1.);
  update();
}

void QGraphicsSpinbox::setExecutionValue(float v)
{
  m_execValue = ossia::clamp(v, 0., 1.);
  m_hasExec = true;
  update();
}

void QGraphicsSpinbox::resetExecution()
{
  m_hasExec = false;
  update();
}

void QGraphicsSpinbox::setRange(float min, float max)
{
  this->min = min;
  this->max = max;
  update();
}

float QGraphicsSpinbox::value() const
{
  return m_value;
}

void QGraphicsSpinbox::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  m_value = 0.5;
  sliderMoved();
  sliderReleased();
  update();
}

void QGraphicsSpinbox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mousePressEvent(*this, event);
}

void QGraphicsSpinbox::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mouseMoveEvent(*this, event);
}

void QGraphicsSpinbox::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mouseReleaseEvent(*this, event);
}

QRectF QGraphicsSpinbox::boundingRect() const
{
  return m_rect;
}

void QGraphicsSpinbox::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  const double val = map(m_value);

  DefaultGraphicsSpinboxImpl::paint(
      *this, score::Skin::instance(), score::toNumber(val), painter, widget);
}

QGraphicsIntSpinbox::QGraphicsIntSpinbox(QGraphicsItem* parent)
    : QGraphicsItem{parent}
{
  auto& skin = score::Skin::instance();
  setCursor(skin.CursorPointingHand);
  min = -100;
  max = 100;
}

QGraphicsIntSpinbox::~QGraphicsIntSpinbox() = default;

void QGraphicsIntSpinbox::setValue(double v)
{
  m_value = unmap(v);
  update();
}

void QGraphicsIntSpinbox::setExecutionValue(double v)
{
  m_execValue = ossia::clamp(v, min, max);
  m_hasExec = true;
  update();
}

void QGraphicsIntSpinbox::resetExecution()
{
  m_hasExec = false;
  update();
}

void QGraphicsIntSpinbox::setRange(double min, double max)
{
  this->min = min;
  this->max = max;
  update();
}

int QGraphicsIntSpinbox::value() const
{
  return map(m_value);
}

void QGraphicsIntSpinbox::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  m_value = 0.5;
  sliderMoved();
  sliderReleased();
  update();
}

void QGraphicsIntSpinbox::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mousePressEvent(*this, event);
}

void QGraphicsIntSpinbox::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mouseMoveEvent(*this, event);
}

void QGraphicsIntSpinbox::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  DefaultGraphicsSpinboxImpl::mouseReleaseEvent(*this, event);
}

QRectF QGraphicsIntSpinbox::boundingRect() const
{
  return m_rect;
}

void QGraphicsIntSpinbox::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto& skin = score::Skin::instance();
  const int val = map(m_value);

  DefaultGraphicsSpinboxImpl::paint(
      *this, score::Skin::instance(), score::toNumber(val), painter, widget);
}

}
