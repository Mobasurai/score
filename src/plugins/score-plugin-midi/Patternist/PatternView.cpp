#include "PatternView.hpp"

#include <Process/Style/ScenarioStyle.hpp>

#include <score/graphics/GraphicWidgets.hpp>
#include <score/graphics/widgets/QGraphicsNoteChooser.hpp>
#include <score/tools/Bind.hpp>
#include <score/tools/Cursor.hpp>

#include <ossia/detail/math.hpp>

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPainter>

#include <Patternist/PatternModel.hpp>

#include <score_plugin_midi_export.h>
#include <wobjectimpl.h>
W_OBJECT_IMPL(Patternist::View)

namespace Patternist
{

View::View(const Patternist::ProcessModel& model, QGraphicsItem* parent)
    : LayerView{parent}
    , m_model{model}
{
  setFlag(QGraphicsItem::ItemClipsToShape);
  setFlag(QGraphicsItem::ItemClipsChildrenToShape);
  con(model, &Patternist::ProcessModel::patternsChanged, this,
      [this] { updateLanes(); });
  con(model, &Patternist::ProcessModel::currentPatternChanged, this,
      [this] { updateLanes(); });
  con(model, &Patternist::ProcessModel::execPosition, this, [this](int x) {
    m_execPosition = x;
    update();
  });

  updateLanes();
}

View::~View() { }

void View::updateLanes()
{
  if(!ossia::valid_index(m_model.currentPattern(), m_model.patterns()))
    return;
  auto& cur_p = m_model.patterns()[m_model.currentPattern()];
  if(cur_p.lanes.size() != m_lanes.size())
  {
    for(auto l : m_lanes)
      delete l;
    m_lanes.clear();

    for(std::size_t lane = 0; lane < cur_p.lanes.size(); lane++)
    {
      auto sl = new score::QGraphicsNoteChooser{this};

      sl->setValue(cur_p.lanes[lane].note);
      sl->setX(10);
      sl->setY(lane * 40);

      connect(sl, &score::QGraphicsNoteChooser::sliderMoved, this, [this, sl, lane] {
        noteChanged(lane, sl->value());
      });
      connect(sl, &score::QGraphicsNoteChooser::sliderReleased, this, [this] {
        noteChangeFinished();
      });
      m_lanes.push_back(sl);
    }
  }
  else
  {
    for(std::size_t lane = 0; lane < cur_p.lanes.size(); lane++)
      m_lanes[lane]->setValue(cur_p.lanes[lane].note);
  }

  update();
}

static const constexpr double lane_height = 40;
static const constexpr double box_side = 22;
static const constexpr double box_spacing = 4;
static const constexpr double x0 = 42;
static const constexpr double y0 = 2;
void View::paint_impl(QPainter* painter) const
{
  if(!ossia::valid_index(m_model.currentPattern(), m_model.patterns()))
    return;

  auto& style = score::Skin::instance();
  painter->setPen(style.Base4.main.pen1);

  // painter->setPen(Qt::white);
  auto& cur_p = m_model.patterns()[m_model.currentPattern()];
  for(std::size_t lane = 0; lane < cur_p.lanes.size(); lane++)
  {
    auto& l = cur_p.lanes[lane];

    // Draw the filled patterns
    for(int i = 0; i < cur_p.length; i++)
    {
      const QRectF rect{
          x0 + i * (box_side + box_spacing), y0 + lane_height * lane, box_side,
          box_side};

      if(l.pattern[i])
      {
        painter->setBrush(
            (i != m_execPosition) ? style.Base4 : style.Base4.lighter.brush);
        painter->drawRect(rect);
      }
    }

    // Draw the empty patterns
    for(int i = 0; i < cur_p.length; i++)
    {
      const QRectF rect{
          x0 + i * (box_side + box_spacing), y0 + lane_height * lane, box_side,
          box_side};

      if(!l.pattern[i])
      {
        painter->setBrush(
            (i != m_execPosition) ? style.Emphasis2.main.brush
                                  : style.Emphasis2.lighter.brush);
        painter->drawRect(rect);
      }
    }
  }
}

void View::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  auto& cur_p = m_model.patterns()[m_model.currentPattern()];
  for(std::size_t lane = 0; lane < cur_p.lanes.size(); lane++)
  {
    for(int i = 0; i < cur_p.length; i++)
    {
      const QRectF rect{
          x0 + i * (box_side + box_spacing), y0 + lane_height * lane, box_side,
          box_side};

      if(rect.contains(event->pos()))
      {
        toggled(lane, i);
        event->accept();
        return;
      }
    }
  }
  event->accept();
}

void View::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}

void View::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  event->accept();
}

void View::dropEvent(QGraphicsSceneDragDropEvent* event)
{
  dropReceived(event->pos(), *event->mimeData());
  event->accept();
}

}
