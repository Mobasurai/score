#include "GraphIntervalPresenter.hpp"

#include <Process/Style/ScenarioStyle.hpp>

#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/State/StateModel.hpp>
#include <Scenario/Document/State/StatePresenter.hpp>
#include <Scenario/Document/State/StateView.hpp>

#include <score/graphics/PainterPath.hpp>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QPen>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::GraphalIntervalPresenter)

static const QPainterPathStroker& cableStroker()
{
  static const QPainterPathStroker cable_stroker{[] {
    QPen pen;
    pen.setCapStyle(Qt::PenCapStyle::RoundCap);
    pen.setJoinStyle(Qt::PenJoinStyle::RoundJoin);
    pen.setWidthF(9.);
    return pen;
  }()};
  return cable_stroker;
}
namespace Scenario
{

GraphalIntervalPresenter::GraphalIntervalPresenter(
    const IntervalModel& model, const StateView& start, const StateView& end,
    const Process::Context& ctx, QGraphicsItem* parent)
    : QGraphicsItem{parent}
    , m_model{model}
    , m_start{start}
    , m_end{end}
    , m_context{ctx}
{
  resize();
  connect(&model.selection, &Selectable::changed, this, [this] { update(); });
  connect(
      &model, &IntervalModel::executionEvent, this, [this](IntervalExecutionEvent ev) {
        if(ev == IntervalExecutionEvent::Playing)
        {
          m_execPing.start();
          update();
        }
      });
}

const Id<IntervalModel>& GraphalIntervalPresenter::id() const
{
  return m_model.id();
}

const IntervalModel& GraphalIntervalPresenter::model() const
{
  return m_model;
}

QRectF GraphalIntervalPresenter::boundingRect() const
{
  return cableStroker().createStroke(m_path).boundingRect();
}

void GraphalIntervalPresenter::resize()
{
  prepareGeometryChange();

  m_path.clear();
  {
    auto p1 = m_start.pos();
    auto p2 = m_end.pos();

    const auto rect = QRectF{p1, p2};
    auto nrect = rect.normalized();
    this->setPos(nrect.topLeft());
    nrect.translate(-nrect.topLeft().x(), -nrect.topLeft().y());

    p1 = mapFromParent(p1);
    p2 = mapFromParent(p2);

    const bool x_dir = p1.x() > p2.x();
    const auto& first = x_dir ? p1 : p2;
    const auto& last = !x_dir ? p1 : p2;
    if(x_dir)
    {
      p1.rx() -= 4.;
      p2.rx() += 4.;
    }
    else
    {
      p1.rx() += 4.;
      p2.rx() -= 4.;
    }

    const auto length = last.x() - first.x();
    if(std::abs(length) > std::numeric_limits<int>::max() * 1e-5)
      return;

    const int half_length = std::floor(0.5 * length);

    const auto y_direction = last.y() > first.y() ? 1 : -1;
    const auto offset_y = y_direction * half_length / 10.f;

    m_path.moveTo(first.x(), first.y());
    m_path.cubicTo(
        first.x() + half_length, first.y() + offset_y, last.x() - half_length,
        last.y() - offset_y, last.x(), last.y());

    const auto cur = m_path.currentPosition();
    const auto angle = m_path.angleAtPercent(0.95);
    {
      if(!x_dir)
      {
        QLineF direct{p2, p1};
        direct.setLength(5);
        direct.setAngle(angle + 45);
        m_path.moveTo(p2);
        m_path.lineTo(direct.p2());
      }
      else
      {
        QLineF direct{p2, p1};
        direct.setLength(5);
        direct.setAngle(-angle + 45);
        m_path.moveTo(p2);
        m_path.lineTo(
            {direct.p2().x() + 2 * std::abs(direct.p2().x() - p2.rx()),
             direct.p2().y()});
      }
    }

    m_path.moveTo(cur);
    {
      if(!x_dir)
      {
        QLineF direct{p2, p1};
        direct.setLength(5);
        direct.setAngle(angle - 45);
        m_path.moveTo(p2);
        m_path.lineTo(direct.p2());
      }
      else
      {
        QLineF direct{p2, p1};
        direct.setLength(5);
        direct.setAngle(-angle - 45);
        m_path.moveTo(p2);
        m_path.lineTo(
            {direct.p2().x() + 2 * std::abs(direct.p2().x() - p2.rx()),
             direct.p2().y()});
      }
    }
  }

  update();
}

const score::Brush&
GraphalIntervalPresenter::intervalColor(const Process::Style& skin) noexcept
{
  if(Q_UNLIKELY(m_model.selection.get()))
  {
    return skin.IntervalSelected();
  }
  else if(Q_UNLIKELY(!m_model.consistency.isValid()))
  {
    return skin.IntervalInvalid();
  }
  else if(Q_UNLIKELY(m_model.consistency.warning()))
  {
    return skin.IntervalWarning();
  }
  else if(Q_UNLIKELY(m_model.executionState() == IntervalExecutionState::Disabled))
  {
    return skin.IntervalInvalid();
  }
  else
  {
    return skin.IntervalBase();
  }
}

void GraphalIntervalPresenter::paint(
    QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  auto& style = Process::Style::instance();
  auto& brush = this->intervalColor(style);

  painter->setRenderHint(QPainter::Antialiasing, true);

  if(m_execPing.running())
  {
    const auto& nextPen = m_execPing.getNextPen(
        brush.color(), style.IntervalPlayFill().color(),
        brush.main.pen2_dashdot_square_miter);
    painter->setPen(nextPen);
    update();
  }
  else
  {
    painter->setPen(brush.main.pen2_dashdot_square_miter);
  }

  painter->setBrush(style.TransparentBrush());
  painter->drawPath(m_path);
  painter->setRenderHint(QPainter::Antialiasing, false);

  // TODO draw an arrow
}

QPainterPath GraphalIntervalPresenter::shape() const
{
  return cableStroker().createStroke(m_path);
}

QPainterPath GraphalIntervalPresenter::opaqueArea() const
{
  return cableStroker().createStroke(m_path);
}

bool GraphalIntervalPresenter::contains(const QPointF& point) const
{
  return cableStroker().createStroke(m_path).contains(point);
}

void GraphalIntervalPresenter::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  pressed(event->scenePos());
}

void GraphalIntervalPresenter::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  moved(event->scenePos());
}

void GraphalIntervalPresenter::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  released(event->scenePos());
}

}
