#include "SpinBoxes.hpp"

#include "DoubleSpinBox.hpp"
#include "TimeSpinBox.hpp"

#include <score/model/Skin.hpp>
#include <score/tools/Cursor.hpp>
#include <score/tools/Debug.hpp>
#include <score/widgets/ControlWidgets.hpp>
#include <score/widgets/SignalUtils.hpp>

#include <ossia/detail/algorithms.hpp>
#include <ossia/detail/flicks.hpp>

#include <QApplication>
#include <QLineEdit>
#include <QPainter>
#include <QStyleOptionComplex>
#include <QTime>
#include <QTimer>

#include <cmath>
#include <wobjectimpl.h>
W_OBJECT_IMPL(score::TimeSpinBox)
namespace score
{
static const constexpr int centStart = 20;
static const constexpr int semiquaverStart = 40;
static const constexpr int quarterStart = 60;
static const constexpr int millisecondStart = 20;
static const constexpr int secondStart = 50;
static const constexpr int minuteStart = 70;

static constexpr const auto barDuration = 4 * ossia::quarter_duration<int64_t>;
static constexpr const auto quarterDuration = ossia::quarter_duration<int64_t>;
static constexpr const auto semiquaverDuration = ossia::quarter_duration<int64_t> / 4;
static constexpr const auto centDuration = ossia::quarter_duration<int64_t> / 400;

struct BarSpinBox
{
  TimeSpinBox& self;
  void paint(QPainter& p, QRect text_rect)
  {
    auto& m_barTime = self.m_barTime;
    const auto w = text_rect.width();
    const auto cent_start = 2 + w - centStart;
    const auto sq_start = w - semiquaverStart;
    const auto q_start = w - quarterStart;
    const auto bar_w = q_start - text_rect.x();

    /*
    p.fillRect(QRect{text_rect.x(), text_rect.y(), bar_w, text_rect.height()}, Qt::red);
    p.fillRect(QRect{q_start, text_rect.y(), 20, text_rect.height()}, Qt::green);
    p.fillRect(QRect{sq_start, text_rect.y(), 20, text_rect.height()}, Qt::blue);
    p.fillRect(QRect{cent_start, text_rect.y(), 30, text_rect.height()}, Qt::yellow);
    */
    p.drawText(
        QRect{text_rect.x(), text_rect.y(), bar_w, text_rect.height()},
        QString{"%1 ."}.arg(m_barTime.bars), QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{q_start, text_rect.y(), 20, text_rect.height()},
        QString{"%1 ."}.arg(m_barTime.quarters), QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{sq_start, text_rect.y(), 20, text_rect.height()},
        QString{"%1 ."}.arg(m_barTime.semiquavers), QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{cent_start, text_rect.y(), 30, text_rect.height()},
        QString{"%1"}.arg(m_barTime.cents, 2, 10, QChar('0')),
        QTextOption(Qt::AlignLeft));
  }

  QString text() const noexcept
  {
    auto& m_barTime = self.m_barTime;
    return QString{"%1 . %2 . %3 . %4"}
        .arg(m_barTime.bars)
        .arg(m_barTime.quarters)
        .arg(m_barTime.semiquavers)
        .arg(m_barTime.cents, 2, 10, QChar('0'));
  }

  static std::optional<int64_t> parseText(QString str) noexcept
  {
    // Check for bad characters
    auto check = str;
    check.replace(QRegularExpression("[0-9]"), "");
    check.replace(' ', "");
    check.replace('.', "");
    if(!check.isEmpty())
      return {};

    int64_t time{};
    auto splitted = str.split(".");
    if(splitted.empty())
      return {};

    int cents = 0;
    int semiquavers = 0;
    int quarters = 0;
    int bars = 0;
    // Bar cents

    auto it = splitted.rbegin();
    if(it != splitted.rend())
    {
      cents = it->toInt();
      ++it;
    }
    if(it != splitted.rend())
    {
      semiquavers = it->toInt();
      ++it;
    }
    if(it != splitted.rend())
    {
      quarters = it->toInt();
      ++it;
    }
    if(it != splitted.rend())
    {
      bars = it->toInt();
      ++it;
    }

    time += barDuration * bars;
    time += quarterDuration * quarters;
    time += semiquaverDuration * semiquavers;
    time += centDuration * cents;

    return time;
  }

  void mousePress(QRect text_rect, QMouseEvent* event)
  {
    const auto w = text_rect.width();
    const auto cent_start = w - centStart;
    const auto sq_start = w - semiquaverStart;
    const auto q_start = w - quarterStart;
    const auto bar_w = q_start - text_rect.x();

    self.m_origFlicks = self.m_flicks;
    self.m_travelledY = 0;
    self.m_prevY = event->globalPosition().y();
    auto x = event->position().x();
    if(x < bar_w)
    {
      self.m_grab = TimeSpinBox::Bar;
      event->accept();
    }
    else if(x > q_start && x < sq_start)
    {
      self.m_grab = TimeSpinBox::Quarter;
      event->accept();
    }
    else if(x > sq_start && x < cent_start)
    {
      self.m_grab = TimeSpinBox::Semiquaver;
      event->accept();
    }
    else
    {
      self.m_grab = TimeSpinBox::Cent;
      event->accept();
    }
  }

  void mouseMove(QMouseEvent* event)
  {
    int pixelsTraveled = self.m_prevY - event->globalPosition().y();
    self.m_travelledY += pixelsTraveled;
    self.m_prevY = event->globalPosition().y();

    double subdivDelta = std::floor(self.m_travelledY / 6.);

    switch(self.m_grab)
    {
      case TimeSpinBox::Bar:
        self.m_flicks
            = self.m_origFlicks + subdivDelta * 4 * ossia::quarter_duration<int64_t>;
        break;
      case TimeSpinBox::Quarter:
        self.m_flicks
            = self.m_origFlicks + subdivDelta * ossia::quarter_duration<int64_t>;
        break;
      case TimeSpinBox::Semiquaver:
        self.m_flicks
            = self.m_origFlicks + subdivDelta * ossia::quarter_duration<int64_t> / 4;
        break;
      case TimeSpinBox::Cent:
        self.m_flicks
            = self.m_origFlicks + subdivDelta * ossia::quarter_duration<int64_t> / 400;
        break;
      default:
        break;
    }

    if(self.m_flicks < 0)
      self.m_flicks = 0;

    event->accept();
  }

  void mouseRelease(QMouseEvent* event)
  {
    mouseMove(event);
    event->accept();
  }
};

struct SecondSpinBox
{
  TimeSpinBox& self;
  void paint(QPainter& p, QRect text_rect)
  {
    QTime t = QTime(0, 0, 0, 0)
                  .addMSecs(self.time().impl / ossia::flicks_per_millisecond<double>);
    const auto w = text_rect.width();
    const auto cent_start = w - millisecondStart;
    const auto sq_start = w - secondStart;
    const auto q_start = w - minuteStart;
    const auto bar_w = q_start - text_rect.x();

    p.drawText(
        QRect{text_rect.x(), text_rect.y(), bar_w, text_rect.height()},
        QString{"%1 :"}.arg(t.hour(), 2, 10, QChar('0')), QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{q_start, text_rect.y(), 20, text_rect.height()},
        QString{"%1 :"}.arg(t.minute(), 2, 10, QChar('0')), QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{sq_start, text_rect.y(), 20, text_rect.height()},
        QString{"%1  ."}.arg(t.second(), 2, 10, QChar('0')),
        QTextOption(Qt::AlignRight));
    p.drawText(
        QRect{cent_start, text_rect.y(), 30, text_rect.height()},
        QString{"%1"}.arg(t.msec(), 3, 10, QChar('0')), QTextOption(Qt::AlignLeft));
  }

  QString text() const noexcept
  {
    QTime t = QTime(0, 0, 0, 0)
                  .addMSecs(self.time().impl / ossia::flicks_per_millisecond<double>);
    return QString{"%1:%2:%3.%4"}
        .arg(t.hour(), 2, 10, QChar('0'))
        .arg(t.minute(), 2, 10, QChar('0'))
        .arg(t.second(), 2, 10, QChar('0'))
        .arg(t.msec(), 3, 10, QChar('0'));
  }

  static std::optional<int64_t> parseText(QString str) noexcept
  {
    str.remove(' ');
    auto splitted = str.split(".");
    if(splitted.size() == 0)
      return {};

    auto hms = QTime::fromString(splitted[0], "H:m:s");
    if(!hms.isValid())
      return {};

    int millisecs = splitted.size() > 1 ? splitted[1].toInt() : 0;

    int64_t flicks{};
    flicks += hms.hour() * 3600 * 1000 * ossia::flicks_per_millisecond<int64_t>;
    flicks += hms.minute() * 60 * 1000 * ossia::flicks_per_millisecond<int64_t>;
    flicks += hms.second() * 1000 * ossia::flicks_per_millisecond<int64_t>;
    flicks += millisecs * ossia::flicks_per_millisecond<int64_t>;
    return flicks;
  }

  void mousePress(QRect text_rect, QMouseEvent* event)
  {
    const auto w = text_rect.width();
    const auto cent_start = w - millisecondStart;
    const auto sq_start = w - secondStart;
    const auto q_start = w - minuteStart;
    const auto bar_w = q_start - text_rect.x();

    self.m_origFlicks = self.m_flicks;
    self.m_travelledY = 0;
    self.m_prevY = event->globalPosition().y();
    auto x = event->position().x();
    if(x < bar_w)
    {
      self.m_grab = TimeSpinBox::Bar;
      event->accept();
    }
    else if(x > q_start && x < sq_start)
    {
      self.m_grab = TimeSpinBox::Quarter;
      event->accept();
    }
    else if(x > sq_start && x < cent_start)
    {
      self.m_grab = TimeSpinBox::Semiquaver;
      event->accept();
    }
    else
    {
      self.m_grab = TimeSpinBox::Cent;
      event->accept();
    }
  }

  void mouseMove(QMouseEvent* event)
  {
    int pixelsTraveled = self.m_prevY - event->globalPosition().y();
    self.m_travelledY += pixelsTraveled;
    self.m_prevY = event->globalPosition().y();

    double subdivDelta = std::floor(self.m_travelledY / 6.);

    switch(self.m_grab)
    {
      case TimeSpinBox::Bar: // Hour
        self.m_flicks
            = self.m_origFlicks
              + subdivDelta * 3600 * 1000 * ossia::flicks_per_millisecond<int64_t>;
        break;
      case TimeSpinBox::Quarter: // Minute
        self.m_flicks
            = self.m_origFlicks
              + subdivDelta * 60 * 1000 * ossia::flicks_per_millisecond<int64_t>;
        break;
      case TimeSpinBox::Semiquaver: // Second
        self.m_flicks = self.m_origFlicks
                        + subdivDelta * 1000 * ossia::flicks_per_millisecond<int64_t>;
        break;
      case TimeSpinBox::Cent: // ms
        self.m_flicks
            = self.m_origFlicks + subdivDelta * ossia::flicks_per_millisecond<int64_t>;
        break;
      default:
        break;
    }

    if(self.m_flicks < 0)
      self.m_flicks = 0;

    event->accept();
  }

  void mouseRelease(QMouseEvent* event)
  {
    mouseMove(event);
    event->accept();
  }
};

struct FlicksSpinBox
{
  TimeSpinBox& self;
  void paint(QPainter& p, QRect text_rect) { }

  QString text() const noexcept { return QString{"%1"}.arg(self.m_flicks); }

  static std::optional<int64_t> parseText(QString str) noexcept { return {}; }

  void mousePress(QRect text_rect, QMouseEvent* event) { }

  void mouseMove(QMouseEvent* event) { }

  void mouseRelease(QMouseEvent* event) { }
};

static std::vector<TimeSpinBox*> spinBoxes;
static std::vector<SpeedSlider*> speedSliders;
TimeMode globalTimeMode = TimeMode::Bars;

TimeSpinBox::TimeSpinBox(QWidget* parent)
    : QWidget(parent)
{
  auto& skin = score::Skin::instance();
  setCursor(skin.CursorSpin);
  spinBoxes.push_back(this);
}

TimeSpinBox::~TimeSpinBox()
{
  ossia::remove_one(spinBoxes, this);
}

void TimeSpinBox::setMinimumTime(ossia::time_value t)
{
  SCORE_TODO;
}

void TimeSpinBox::setMaximumTime(ossia::time_value t)
{
  SCORE_TODO;
}

void TimeSpinBox::setTime(ossia::time_value t)
{
  if(m_flicks != t.impl)
  {
    m_flicks = t.impl;
    updateTime();
  }
}

void TimeSpinBox::updateTime()
{
  if(m_flicks > ossia::time_value::infinite_min)
    return;
  const int64_t bars = m_flicks / (barDuration);
  const int64_t quarters = (m_flicks - (bars * barDuration)) / quarterDuration;
  const int64_t semiquavers
      = (m_flicks - (bars * barDuration) - (quarters * quarterDuration))
        / semiquaverDuration;
  const int64_t cents = (m_flicks - (bars * barDuration) - (quarters * quarterDuration)
                         - (semiquavers * semiquaverDuration))
                        / centDuration;
  m_barTime.bars = bars;
  m_barTime.quarters = quarters;
  m_barTime.semiquavers = semiquavers;
  m_barTime.cents = cents;
  update();
}

ossia::time_value TimeSpinBox::time() const noexcept
{
  return {m_flicks};
}

void TimeSpinBox::wheelEvent(QWheelEvent* event)
{
  event->ignore();
}

void TimeSpinBox::mousePressEvent(QMouseEvent* event)
{
  const auto text_rect = rect().adjusted(2, 2, -4, -2);

  m_startPos = score::globalPos(event);

  switch(globalTimeMode)
  {
    case Bars:
      BarSpinBox{*this}.mousePress(text_rect, event);
      break;
    case Seconds:
      SecondSpinBox{*this}.mousePress(text_rect, event);
      break;
    case Flicks:
      FlicksSpinBox{*this}.mousePress(text_rect, event);
      break;
  }

  // #if defined(__APPLE__)
  //     score::hideCursor(true);
  // #else
  //     QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
  // #endif

  event->accept();
}

void TimeSpinBox::mouseMoveEvent(QMouseEvent* event)
{
  switch(globalTimeMode)
  {
    case Bars:
      BarSpinBox{*this}.mouseMove(event);
      break;
    case Seconds:
      SecondSpinBox{*this}.mouseMove(event);
      break;
    case Flicks:
      FlicksSpinBox{*this}.mouseMove(event);
      break;
  }

  // score::moveCursorPos(m_startPos);
  // score::hideCursor(true);
  updateTime();
  timeChanged({this->m_flicks});

  event->accept();
}

void TimeSpinBox::mouseReleaseEvent(QMouseEvent* event)
{
  switch(globalTimeMode)
  {
    case Bars:
      BarSpinBox{*this}.mouseRelease(event);
      break;
    case Seconds:
      SecondSpinBox{*this}.mouseRelease(event);
      break;
    case Flicks:
      FlicksSpinBox{*this}.mouseRelease(event);
      break;
  }

  //  score::showCursor();
  //  score::setCursorPos(m_startPos);
  updateTime();
  editingFinished();

  event->accept();
}

void TimeSpinBox::mouseDoubleClickEvent(QMouseEvent* event)
{
  auto le = new QLineEdit;
  le->setGeometry(this->rect());
  le->setParent(this);
  le->show();
  switch(globalTimeMode)
  {
    case Bars:
      le->setText(BarSpinBox{*this}.text());
      break;
    case Seconds:
      le->setText(SecondSpinBox{*this}.text());
      break;
    case Flicks:
      le->setText(FlicksSpinBox{*this}.text());
      break;
  }
  le->setAlignment(Qt::AlignRight);
  le->selectAll();

  connect(le, &QLineEdit::editingFinished, this, [this, le] {
    std::optional<int64_t> flicks;
    switch(globalTimeMode)
    {
      case Bars:
        flicks = BarSpinBox{*this}.parseText(le->text());
        break;
      case Seconds:
        flicks = SecondSpinBox{*this}.parseText(le->text());
        break;
      case Flicks:
        flicks = FlicksSpinBox{*this}.parseText(le->text());
        break;
    }

    if(flicks && *flicks != m_flicks)
    {
      m_flicks = *flicks;
      updateTime();
      timeChanged({this->m_flicks});
      editingFinished();
    }
    le->deleteLater();
  });

  event->accept();
}

void TimeSpinBox::initStyleOption(QStyleOptionFrame* option) const noexcept
{
  option->initFrom(this);
  option->rect = contentsRect();
  constexpr bool frame = true;
  option->lineWidth
      = frame ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this) : 0;
  option->midLineWidth = 0;
  option->state |= QStyle::State_Sunken;
#ifdef QT_KEYPAD_NAVIGATION
  if(hasEditFocus())
    option->state |= QStyle::State_HasEditFocus;
#endif
  option->features = QStyleOptionFrame::None;
}

QSize TimeSpinBox::sizeHint() const
{
  return {150, 20};
}

QSize TimeSpinBox::minimumSizeHint() const
{
  return {50, 20};
}

void TimeSpinBox::paintEvent(QPaintEvent* event)
{
  QPainter p(this);
  QPalette pal = palette();

  QStyleOptionFrame panel;
  initStyleOption(&panel);
  style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
  QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
  // r = r.marginsRemoved(d->effectiveTextMargins());
  p.setClipRect(r);

  QFontMetrics fm = fontMetrics();

  const auto text_rect = rect().adjusted(2, 2, -4, -2);
  switch(globalTimeMode)
  {
    case Bars:
      BarSpinBox{*this}.paint(p, text_rect);
      break;
    case Seconds:
      SecondSpinBox{*this}.paint(p, text_rect);
      break;
    case Flicks:
      FlicksSpinBox{*this}.paint(p, text_rect);
      break;
  }
}

/* Speed goes from -1 to 5 */
static constexpr double valueFromSpeed(double speed)
{
  return (speed - 0.01) / 5.;
}
static constexpr double speedFromValue(double value)
{
  return value * 5. + 0.01;
}

SpeedSlider::SpeedSlider(QWidget* parent)
    : DoubleSlider{parent}
{
  speedSliders.push_back(this);
  setSpeed(1.0);
}

SpeedSlider::~SpeedSlider()
{
  ossia::remove_one(speedSliders, this);
}

double SpeedSlider::speed() const noexcept
{
  return std::round(1000 * speedFromValue(value())) / 1000;
}

void SpeedSlider::setSpeed(double v)
{
  setValue(valueFromSpeed(v));
}

void SpeedSlider::setTempo(double t)
{
  setValue(valueFromSpeed(t / ossia::root_tempo));
}

void SpeedSlider::paintEvent(QPaintEvent*)
{
  QString text;
  text.reserve(16);
  text += (globalTimeMode == TimeMode::Bars) ? "" : (showText) ? "speed: × " : "× ";

  double v = speed();
  if(globalTimeMode == TimeMode::Bars)
  {
    v *= ossia::root_tempo;
    text += QString::number(v, 'f', 1);
  }
  else
  {
    text += QString::number(v, 'f', 2);
  }

  paintWithText(text);
}

void SpeedSlider::mousePressEvent(QMouseEvent* ev)
{
  if(ev->button() == Qt::LeftButton)
    return DoubleSlider::mousePressEvent(ev);

  if(qGuiApp->keyboardModifiers() & Qt::CTRL)
  {
    setValue(valueFromSpeed(1.));
  }
  else
  {
    QTimer::singleShot(
        0, [this, pos = ev->globalPosition().toPoint()] { createPopup(pos); });
  }
  ev->ignore();
}

void SpeedSlider::createPopup(QPoint pos)
{
  auto w = new score::DoubleSpinboxWithEnter;
  w->setWindowFlag(Qt::Tool);
  w->setWindowFlag(Qt::FramelessWindowHint);
  if(globalTimeMode == TimeMode::Bars)
  {
    w->setRange(20., 500.);
    w->setDecimals(1);
    w->setValue(speed() * ossia::root_tempo);

    QObject::connect(
        w, SignalUtils::QDoubleSpinBox_valueChanged_double(), this,
        [this](double v) { this->setValue(valueFromSpeed(v / ossia::root_tempo)); });
  }
  else
  {
    w->setRange(-1., 5.);
    w->setDecimals(2);
    w->setValue(speed());

    QObject::connect(
        w, SignalUtils::QDoubleSpinBox_valueChanged_double(), this,
        [this](double v) { this->setValue(valueFromSpeed(v)); });
  }

  w->show();
  w->move(pos.x(), pos.y());
  QTimer::singleShot(5, w, [w] { w->setFocus(); });
  QObject::connect(
      w, &DoubleSpinboxWithEnter::editingFinished, w, &QObject::deleteLater);
}

void setGlobalTimeMode(TimeMode mode)
{
  globalTimeMode = mode;
  for(auto sb : spinBoxes)
  {
    sb->update();
  }
  for(auto sl : speedSliders)
  {
    sl->update();
  }
}

}
