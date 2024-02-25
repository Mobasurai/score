#pragma once
#include <Scenario/Document/Event/ExecutionStatus.hpp>
#include <Scenario/Document/ScenarioDocument/ScenarioDocumentViewConstants.hpp>

#include <score/model/ColorInterpolator.hpp>

#include <QGraphicsItem>
#include <QRect>

#include <score_plugin_scenario_export.h>

#include <verdigris>

class QGraphicsSceneDragDropEvent;
class QGraphicsSceneMouseEvent;
class QMimeData;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;

namespace Scenario
{
struct StateOverlays;
class StatePresenter;

class SCORE_PLUGIN_SCENARIO_EXPORT StateView final
    : public QObject
    , public QGraphicsItem
{
  W_OBJECT(StateView)
  Q_INTERFACES(QGraphicsItem)
public:
  static const constexpr qreal fullRadius = 6.;
  static const constexpr qreal pointRadius = 3.5;
  static const constexpr qreal notDilated = 1.;
  static const constexpr qreal dilated = 1.5;

  StateView(StatePresenter& presenter, QGraphicsItem* parent = nullptr);
  virtual ~StateView();

  static const constexpr int Type = ItemType::State;
  int type() const final override { return Type; }

  StatePresenter& presenter() const { return m_presenter; }

  QRectF boundingRect() const override;

  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
      override;

  void setContainMessage(bool);
  void setContainProcess(bool);
  void setSelected(bool arg);
  void setStatus(ExecutionStatus);
  void disableOverlay();

public:
  void dropReceived(const QMimeData& arg_1)
      E_SIGNAL(SCORE_PLUGIN_SCENARIO_EXPORT, dropReceived, arg_1)
  void startCreateMode() E_SIGNAL(SCORE_PLUGIN_SCENARIO_EXPORT, startCreateMode)
  void startCreateGraphalMode()
      E_SIGNAL(SCORE_PLUGIN_SCENARIO_EXPORT, startCreateGraphalMode)
  void startCreateSequence() E_SIGNAL(SCORE_PLUGIN_SCENARIO_EXPORT, startCreateSequence)

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
  void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
  void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
  void dragEnterEvent(QGraphicsSceneDragDropEvent* event) override;
  void dragLeaveEvent(QGraphicsSceneDragDropEvent* event) override;
  void dropEvent(QGraphicsSceneDragDropEvent* event) override;

private:
  void setDilatation(bool);
  void updateOverlay();
  StatePresenter& m_presenter;
  StateOverlays* m_overlays{};
  ExecutionStatusProperty m_status{};
  score::ColorBang m_execPing;

  bool m_dilated : 1;
  bool m_containMessage : 1;
  bool m_containProcess : 1;
  bool m_selected : 1;
  bool m_hovered : 1;
  bool m_hasOverlay : 1;
  bool m_moving : 1;
};
}
