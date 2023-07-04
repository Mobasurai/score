// This is an open source non-commercial project. Dear PVS-Studio, please check
// it. PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "TemporalIntervalView.hpp"

#include "TemporalIntervalPresenter.hpp"

#include <Process/Style/ScenarioStyle.hpp>

#include <Scenario/Document/Event/EventModel.hpp>
#include <Scenario/Document/Interval/IntervalHeader.hpp>
#include <Scenario/Document/Interval/IntervalModel.hpp>
#include <Scenario/Document/Interval/IntervalPixmaps.hpp>
#include <Scenario/Document/Interval/IntervalPresenter.hpp>
#include <Scenario/Document/Interval/IntervalView.hpp>

#include <score/graphics/GraphicsItem.hpp>
#include <score/graphics/PainterPath.hpp>
#include <score/model/Skin.hpp>

#include <QGraphicsItem>
#include <QGraphicsView>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOption>
#include <qnamespace.h>

#include <wobjectimpl.h>
W_OBJECT_IMPL(Scenario::TemporalIntervalView)
class QGraphicsSceneHoverEvent;
class QStyleOptionGraphicsItem;
class QWidget;

namespace Scenario
{
TemporalIntervalView::TemporalIntervalView(
    TemporalIntervalPresenter& presenter, QGraphicsItem* parent)
    : IntervalView{presenter, parent}
{
  this->setCacheMode(QGraphicsItem::NoCache);
  this->setParentItem(parent);

  this->setZValue(ZPos::Interval);
  //  this->setFlag(ItemClipsChildrenToShape);
}

TemporalIntervalView::~TemporalIntervalView() { }

QRectF TemporalIntervalView::boundingRect() const
{
  qreal x = std::min(0., minWidth());
  qreal rectW = infinite() ? defaultWidth() : maxWidth();
  rectW -= x;
  return {x, -3., rectW, qreal(intervalAndRackHeight()) + 3.};
}

const TemporalIntervalPresenter& TemporalIntervalView::presenter() const
{
  return static_cast<const TemporalIntervalPresenter&>(m_presenter);
}

void TemporalIntervalView::updatePaths()
{
  solidPath.clear();
  playedSolidPath.clear();

  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = playWidth();

  // Paths
  if(play_w <= 0.)
  {
    if(infinite())
    {
      if(min_w != 0.)
      {
        solidPath.lineTo(min_w, 0.);
      }

      // TODO end state should be hidden
      // - dashedPath.moveTo(min_w, 0.);
      // - dashedPath.lineTo(def_w, 0.);
    }
    else if(min_w == max_w) // TODO rigid()
    {
      solidPath.lineTo(def_w, 0.);
    }
    else
    {
      if(min_w != 0.)
      {
        solidPath.lineTo(min_w, 0.);
      }
    }
  }
  else
  {
    if(infinite())
    {
      if(min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
        // if(play_w < min_w)
        {
          solidPath.lineTo(min_w, 0.);
        }
      }
    }
    else if(min_w == max_w) // TODO rigid()
    {
      playedSolidPath.lineTo(std::min(play_w, def_w), 0.);
      solidPath.lineTo(def_w, 0.);
    }
    else
    {
      if(min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
        solidPath.lineTo(min_w, 0.);
      }
    }
  }
}

void TemporalIntervalView::drawDashedPath(
    QPainter& p, QRectF visibleRect, const Process::Style& skin)
{
  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = playWidth();

  auto& dash_pixmap = intervalDashedPixmap(skin);

  // Paths
  if(play_w <= min_w)
  {
    if(infinite())
    {
      IntervalPixmaps::drawDashes(min_w, def_w, p, visibleRect, dash_pixmap);
    }
    else if(min_w != max_w)
    {
      IntervalPixmaps::drawDashes(min_w, max_w, p, visibleRect, dash_pixmap);
    }
  }
}

void TemporalIntervalView::drawPlayDashedPath(
    QPainter& p, QRectF visibleRect, const Process::Style& skin)
{
  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = playWidth();

  // Paths
  if(play_w <= min_w)
    return;
  if(presenter().model().duration.isRigid())
    return;

  double actual_min = std::max(min_w, visibleRect.left());
  double actual_max = std::min(infinite() ? def_w : max_w, visibleRect.right());

  auto& pixmaps = intervalPixmaps(skin);

  // waiting
  const int idx = m_waiting ? skin.skin.PulseIndex : 0;
  IntervalPixmaps::drawDashes(
      actual_min, actual_max, p, visibleRect, pixmaps.playDashed[idx]);

  // played
  IntervalPixmaps::drawDashes(
      actual_min, std::min(actual_max, play_w), p, visibleRect,
      pixmaps.playDashed.back());

  p.setPen(skin.IntervalPlayLinePen(skin.IntervalPlayFill()));

  p.drawLine(QPointF{actual_min, -0.5}, QPointF{std::min(actual_max, play_w), -0.5});
}

void TemporalIntervalView::updatePlayPaths()
{
  playedSolidPath.clear();

  const qreal min_w = minWidth();
  const qreal max_w = maxWidth();
  const qreal def_w = defaultWidth();
  const qreal play_w = playWidth();

  // Paths
  if(play_w <= 0.)
  {
    return;
  }
  else
  {
    if(infinite())
    {
      if(min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
      }
    }
    else if(min_w == max_w) // TODO rigid()
    {
      playedSolidPath.lineTo(std::min(play_w, def_w), 0.);
    }
    else
    {
      if(min_w != 0.)
      {
        playedSolidPath.lineTo(std::min(play_w, min_w), 0.);
      }
    }
  }
}

void TemporalIntervalView::paint(
    QPainter* p, const QStyleOptionGraphicsItem* so, QWidget*)
{
  auto view = ::getView(*this);
  if(!view)
    return;
  auto& painter = *p;
  const auto rect = boundingRect();

  QPointF sceneDrawableTopLeft = view->mapToScene(-10, 0);
  QPointF sceneDrawableBottomRight
      = view->mapToScene(view->width() + 10, view->height() + 10);
  QPointF itemDrawableTopLeft = this->mapFromScene(sceneDrawableTopLeft);
  QPointF itemDrawableBottomRight = this->mapFromScene(sceneDrawableBottomRight);

  itemDrawableTopLeft.rx() = std::max(itemDrawableTopLeft.x(), 0.);
  itemDrawableTopLeft.ry() = std::max(itemDrawableTopLeft.y(), 0.);

  itemDrawableBottomRight.rx() = std::min(itemDrawableBottomRight.x(), rect.width());
  itemDrawableBottomRight.ry() = std::min(itemDrawableBottomRight.y(), rect.height());
  if(itemDrawableTopLeft.x() > rect.width())
  {
    return;
  }
  if(itemDrawableBottomRight.y() > rect.height())
  {
    return;
  }

  painter.setRenderHint(QPainter::Antialiasing, false);
  auto& skin = Process::Style::instance();

  const QRectF visibleRect = QRectF{itemDrawableTopLeft, itemDrawableBottomRight};
  auto& c = presenter().model();
  if(c.smallViewVisible())
  {
    // Background
    itemDrawableBottomRight.rx() = std::min(itemDrawableBottomRight.x(), m_defaultWidth);
    const auto backgroundRect = QRectF{
        itemDrawableTopLeft + QPointF{0.5, 2.},
        itemDrawableBottomRight + QPointF{-0.5, -2.}};

    auto brush = m_presenter.model().metadata().getColor().getBrush().main.brush;
    auto col = brush.color();
    col.setAlphaF(0.6f);
    brush.setColor(col);
    painter.fillRect(backgroundRect, brush);
  }

  // Colors
  const auto& defaultColor = this->intervalColor(skin);

  // Drawing
  if(!solidPath.isEmpty())
  {
    painter.setPen(skin.IntervalSolidPen(defaultColor));
    painter.drawPath(solidPath);
  }

  drawDashedPath(painter, visibleRect, skin);

  if(!playedSolidPath.isEmpty())
  {
    if(m_execPing.running())
    {
      const auto& nextPen = m_execPing.getNextPen(
          defaultColor.color(), skin.IntervalPlayFill().color(),
          skin.IntervalSolidPen(skin.IntervalPlayFill()));
      painter.setPen(nextPen);
      update();
      if(!m_execPing.running())
      {
        m_playWidth = 0.;
        updatePlayPaths();
      }
    }
    else
    {
      painter.setPen(skin.IntervalSolidPen(skin.IntervalPlayFill()));
    }

    painter.drawPath(playedSolidPath);
  }

  drawPlayDashedPath(painter, visibleRect, skin);
#if defined(SCORE_SCENARIO_DEBUG_RECTS)
  painter.setPen(Qt::darkRed);
  painter.setBrush(Qt::NoBrush);
  painter.drawRect(boundingRect());
#endif
}

void TemporalIntervalView::hoverEnterEvent(QGraphicsSceneHoverEvent* h)
{
  QGraphicsItem::hoverEnterEvent(h);
  if(h->pos().y() < 4)
    setUngripCursor();
  else
    unsetCursor();

  intervalHoverEnter();
}

void TemporalIntervalView::hoverLeaveEvent(QGraphicsSceneHoverEvent* h)
{
  QGraphicsItem::hoverLeaveEvent(h);
  unsetCursor();
  intervalHoverLeave();
}

void TemporalIntervalView::setSelected(bool selected)
{
  m_selected = selected;
  update();
}

void TemporalIntervalView::setExecutionDuration(const TimeVal& progress)
{
  // FIXME this should be merged with the slot in IntervalPresenter!!!
  // Also make a setting to disable it since it may take a lot of time
  if(!qFuzzyCompare(progress.msec(), 0))
  {
    if(!m_counterItem.isVisible())
      m_counterItem.setVisible(true);
    updateCounterPos();

    m_counterItem.setText(progress.toString());
  }
  else
  {
    m_counterItem.setVisible(false);
  }
  update();
}

}
